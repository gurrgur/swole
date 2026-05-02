#include "swell_abi.hpp"

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <mutex>
#include <pthread.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

enum class HandleKind : uint32_t {
    Thread = 0x54485244,
    Event = 0x4556544E,
};

struct SwellHandle {
    uint32_t magic;
    std::atomic<bool> destroyed{false};
};

struct SwellThread : SwellHandle {
    std::thread thread;
    DWORD exit_code{0};
    bool exited{false};
    std::mutex mutex;
};

struct SwellEvent : SwellHandle {
    bool manual_reset;
    std::atomic<bool> signaled;
    std::mutex mutex;
    std::condition_variable cv;
};

struct SwellEventSocket : SwellHandle {
    int sock{-1};
    std::atomic<bool> signaled;
};

inline SwellThread* as_thread(HANDLE h) {
    if (!h) return nullptr;
    auto* p = reinterpret_cast<SwellThread*>(h);
    return p->magic == uint32_t(HandleKind::Thread) && !p->destroyed ? p : nullptr;
}

inline SwellEvent* as_event(HANDLE h) {
    if (!h) return nullptr;
    auto* p = reinterpret_cast<SwellEvent*>(h);
    return p->magic == uint32_t(HandleKind::Event) && !p->destroyed ? p : nullptr;
}

inline SwellEventSocket* as_event_socket(HANDLE h) {
    if (!h) return nullptr;
    auto* p = reinterpret_cast<SwellEventSocket*>(h);
    return p->magic == uint32_t(HandleKind::Event) && !p->destroyed ? p : nullptr;
}

void* thread_entry(void* arg) {
    auto* t = reinterpret_cast<SwellThread*>(arg);
    DWORD (*proc)(LPVOID) = nullptr;
    LPVOID param = nullptr;

    {
        std::lock_guard lock{t->mutex};
        proc = reinterpret_cast<DWORD (*)(LPVOID)>(t->thread.native_handle());
    }

    DWORD result = 0;
    if (proc) {
        result = proc(nullptr);
    }

    {
        std::lock_guard lock{t->mutex};
        t->exit_code = result;
        t->exited = true;
    }

    return nullptr;
}

struct ThreadParams {
    DWORD (*proc)(LPVOID);
    LPVOID param;
};

void* thread_entry_v2(void* arg) {
    auto* params = reinterpret_cast<ThreadParams*>(arg);
    DWORD (*proc)(LPVOID) = params->proc;
    LPVOID param = params->param;
    delete params;

    return reinterpret_cast<void*>(proc ? proc(param) : 0);
}

} // namespace

extern "C" {

HANDLE CreateThread(void*, DWORD, DWORD (*ThreadProc)(LPVOID), LPVOID parm, DWORD, DWORD* tidOut) {
    auto* t = new SwellThread{};
    t->magic = uint32_t(HandleKind::Thread);

    auto* params = new ThreadParams{ThreadProc, parm};
    t->thread = std::thread{thread_entry_v2, params};

    if (tidOut) {
        pthread_t pt = t->thread.native_handle();
        *tidOut = DWORD(reinterpret_cast<uintptr_t>(pt) & 0xFFFFFFFF);
    }

    return reinterpret_cast<HANDLE>(t);
}

HANDLE CreateEvent(void*, BOOL manualReset, BOOL initialSig, const char*) {
    auto* e = new SwellEvent{};
    e->magic = uint32_t(HandleKind::Event);
    e->manual_reset = manualReset != 0;
    e->signaled = initialSig != 0;
    return reinterpret_cast<HANDLE>(e);
}

HANDLE CreateEventAsSocket(void*, BOOL manualReset, BOOL initialSig, const char*) {
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0) return nullptr;

    auto* es = new SwellEventSocket{};
    es->magic = uint32_t(HandleKind::Event);
    es->sock = fds[0];
    es->signaled = initialSig != 0;

    if (initialSig) {
        char c = 1;
        write(fds[1], &c, 1);
    }

    close(fds[1]);
    return reinterpret_cast<HANDLE>(es);
}

DWORD GetCurrentThreadId() {
    pthread_t pt = pthread_self();
    return DWORD(reinterpret_cast<uintptr_t>(pt) & 0xFFFFFFFF);
}

DWORD WaitForSingleObject(HANDLE hand, DWORD msTO) {
    if (!hand) return WAIT_FAILED;

    if (auto* e = as_event(hand)) {
        if (e->signaled.load()) return WAIT_OBJECT_0;

        std::unique_lock lock{e->mutex};
        if (msTO == 0xffffffffu) {
            e->cv.wait(lock, [&]{ return e->signaled.load(); });
            if (!e->manual_reset) e->signaled = false;
            return WAIT_OBJECT_0;
        }

        bool result = e->cv.wait_for(lock, std::chrono::milliseconds(msTO), [&]{ return e->signaled.load(); });
        if (!result) return WAIT_TIMEOUT;
        if (!e->manual_reset) e->signaled = false;
        return WAIT_OBJECT_0;
    }

    if (auto* es = as_event_socket(hand)) {
        if (es->signaled.load()) return WAIT_OBJECT_0;

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(es->sock, &fds);

        if (msTO == 0xffffffffu) {
            int ret = select(es->sock + 1, &fds, nullptr, nullptr, nullptr);
            if (ret > 0) {
                es->signaled = true;
                return WAIT_OBJECT_0;
            }
            return WAIT_FAILED;
        }

        struct timeval tv{};
        tv.tv_sec = msTO / 1000;
        tv.tv_usec = (msTO % 1000) * 1000;

        int ret = select(es->sock + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0) {
            es->signaled = true;
            return WAIT_OBJECT_0;
        }
        return ret == 0 ? WAIT_TIMEOUT : WAIT_FAILED;
    }

    if (auto* t = as_thread(hand)) {
        if (msTO == 0) {
            std::lock_guard lock{t->mutex};
            return t->exited ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
        }

        auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(msTO);
        while (std::chrono::steady_clock::now() < deadline) {
            {
                std::lock_guard lock{t->mutex};
                if (t->exited) return WAIT_OBJECT_0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        std::lock_guard lock{t->mutex};
        return t->exited ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
    }

    return WAIT_FAILED;
}

DWORD WaitForAnySocketObject(int numObjs, HANDLE* objs, DWORD msTO) {
    if (!objs || numObjs <= 0) return WAIT_FAILED;

    std::vector<int> socks;
    socks.reserve(size_t(numObjs));

    for (int i = 0; i < numObjs; ++i) {
        if (auto* es = as_event_socket(objs[i])) {
            if (es->signaled.load()) return WAIT_OBJECT_0 + DWORD(i);
            socks.push_back(es->sock);
        } else if (auto* e = as_event(objs[i])) {
            if (e->signaled.load()) return WAIT_OBJECT_0 + DWORD(i);
        }
    }

    if (socks.empty()) {
        for (int i = 0; i < numObjs; ++i) {
            if (auto* e = as_event(objs[i])) {
                std::unique_lock lock{e->mutex};
                if (msTO == 0xffffffffu) {
                    e->cv.wait(lock, [&]{ return e->signaled.load(); });
                    if (!e->manual_reset) e->signaled = false;
                    return WAIT_OBJECT_0 + DWORD(i);
                }
                bool result = e->cv.wait_for(lock, std::chrono::milliseconds(msTO), [&]{ return e->signaled.load(); });
                if (result) {
                    if (!e->manual_reset) e->signaled = false;
                    return WAIT_OBJECT_0 + DWORD(i);
                }
            }
        }
        return WAIT_TIMEOUT;
    }

    int max_fd = 0;
    fd_set fds;
    FD_ZERO(&fds);
    for (int fd : socks) {
        FD_SET(fd, &fds);
        if (fd > max_fd) max_fd = fd;
    }

    struct timeval tv{};
    if (msTO != 0xffffffffu) {
        tv.tv_sec = msTO / 1000;
        tv.tv_usec = (msTO % 1000) * 1000;
    }

    int ret = select(max_fd + 1, &fds, nullptr, nullptr, msTO == 0xffffffffu ? nullptr : &tv);
    if (ret > 0) {
        for (int i = 0; i < numObjs; ++i) {
            if (auto* es = as_event_socket(objs[i])) {
                if (FD_ISSET(es->sock, &fds)) {
                    es->signaled = true;
                    return WAIT_OBJECT_0 + DWORD(i);
                }
            }
        }
    }
    return ret == 0 ? WAIT_TIMEOUT : WAIT_FAILED;
}

BOOL CloseHandle(HANDLE hand) {
    if (!hand) return SWELL_FALSE;

    if (auto* t = as_thread(hand)) {
        t->destroyed = true;
        if (t->thread.joinable()) t->thread.join();
        t->magic = 0;
        delete t;
        return SWELL_TRUE;
    }

    if (auto* e = as_event(hand)) {
        e->destroyed = true;
        e->magic = 0;
        delete e;
        return SWELL_TRUE;
    }

    if (auto* es = as_event_socket(hand)) {
        es->destroyed = true;
        if (es->sock >= 0) close(es->sock);
        es->magic = 0;
        delete es;
        return SWELL_TRUE;
    }

    return SWELL_FALSE;
}

BOOL SetThreadPriority(HANDLE, int) {
    return SWELL_FALSE;
}

BOOL SetEvent(HANDLE evt) {
    if (auto* e = as_event(evt)) {
        bool was_signaled = e->signaled.exchange(true);
        if (!was_signaled) {
            std::lock_guard lock{e->mutex};
            e->cv.notify_all();
        }
        return SWELL_TRUE;
    }

    if (auto* es = as_event_socket(evt)) {
        bool was_signaled = es->signaled.exchange(true);
        if (!was_signaled) {
            char c = 1;
            write(es->sock, &c, 1);
        }
        return SWELL_TRUE;
    }

    return SWELL_FALSE;
}

BOOL ResetEvent(HANDLE evt) {
    if (auto* e = as_event(evt)) {
        e->signaled = false;
        return SWELL_TRUE;
    }

    if (auto* es = as_event_socket(evt)) {
        es->signaled = false;
        char buf[256];
        while (read(es->sock, buf, sizeof(buf)) > 0) {}
        return SWELL_TRUE;
    }

    return SWELL_FALSE;
}

int SWELL_GetProcessExitCode(HANDLE hand) {
    if (!hand) return -1;

    if (auto* t = as_thread(hand)) {
        std::lock_guard lock{t->mutex};
        return t->exited ? int(t->exit_code) : 259;
    }

    return -1;
}

HANDLE SWELL_CreateProcessFromPID(int pid) {
    (void)pid;
    return nullptr;
}

HANDLE SWELL_CreateProcess(const char*, int, const char**) {
    return nullptr;
}

} // extern "C"
