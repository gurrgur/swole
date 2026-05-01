#include "swell_abi.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

using Section = std::map<std::string, std::string>;
using IniData = std::map<std::string, Section>;

SWELLAppMainProc g_app_main{};
int g_argc{};
char** g_argv{};

std::string default_ini_path() {
    const char* home = std::getenv("HOME");
    if (!home || !*home) return ".libSwell.ini";
    return std::string{home} + "/.libSwell.ini";
}

std::string ini_path(const char* fn) {
    return (fn && *fn) ? std::string{fn} : default_ini_path();
}

std::string trim_cr(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

IniData read_ini(const char* fn) {
    IniData data;
    std::ifstream in(ini_path(fn));
    std::string section;
    std::string line;
    while (std::getline(in, line)) {
        line = trim_cr(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        if (line.front() == '[' && line.back() == ']') {
            section = line.substr(1, line.size() - 2);
            data.try_emplace(section);
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        data[section][line.substr(0, eq)] = line.substr(eq + 1);
    }
    return data;
}

bool write_ini(const char* fn, const IniData& data) {
    std::ofstream out(ini_path(fn), std::ios::trunc);
    if (!out) return false;
    for (const auto& [section, values] : data) {
        out << '[' << section << "]\n";
        for (const auto& [key, value] : values)
            out << key << '=' << value << '\n';
        out << '\n';
    }
    return true;
}

DWORD copy_string_result(const std::string& value, char* ret, int retsize) {
    if (!ret || retsize <= 0) return 0;
    const int n = std::min<int>(retsize - 1, int(value.size()));
    if (n > 0) std::memcpy(ret, value.data(), size_t(n));
    ret[n] = 0;
    return DWORD(n);
}

DWORD copy_multi_result(const std::vector<std::string>& values, char* ret, DWORD retsize) {
    if (!ret || retsize == 0) return 0;
    DWORD pos = 0;
    for (const auto& value : values) {
        if (pos + value.size() + 1 >= retsize) break;
        std::memcpy(ret + pos, value.data(), value.size());
        pos += DWORD(value.size());
        ret[pos++] = 0;
    }
    if (pos < retsize) ret[pos] = 0;
    else ret[retsize - 1] = 0;
    return pos;
}

std::string bytes_to_hex(const void* buf, int bufsz) {
    static constexpr char kHex[] = "0123456789ABCDEF";
    const auto* p = static_cast<const unsigned char*>(buf);
    std::string out;
    out.reserve(size_t(std::max(0, bufsz)) * 2);
    for (int i = 0; i < bufsz; ++i) {
        out.push_back(kHex[p[i] >> 4]);
        out.push_back(kHex[p[i] & 0x0f]);
    }
    return out;
}

int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

bool hex_to_bytes(const std::string& text, void* buf, int bufsz) {
    if (int(text.size()) < bufsz * 2) return false;
    auto* p = static_cast<unsigned char*>(buf);
    for (int i = 0; i < bufsz; ++i) {
        int hi = hex_digit(text[size_t(i) * 2]);
        int lo = hex_digit(text[size_t(i) * 2 + 1]);
        if (hi < 0 || lo < 0) return false;
        p[i] = static_cast<unsigned char>((hi << 4) | lo);
    }
    return true;
}

FILETIME to_filetime(timespec ts) {
    constexpr uint64_t kWindowsUnixEpochDelta = 11644473600ull;
    uint64_t t = (uint64_t(ts.tv_sec) + kWindowsUnixEpochDelta) * 10000000ull;
    t += uint64_t(ts.tv_nsec / 100);
    return {DWORD(t & 0xffffffffu), DWORD(t >> 32)};
}

struct GlobalBlock {
    uint32_t magic;
    int size;
};

constexpr uint32_t kGlobalBlockMagic = 0x53474c42; // SGLB

GlobalBlock* global_header(HANDLE h) {
    if (!h) return nullptr;
    auto* block = static_cast<GlobalBlock*>(h) - 1;
    return block->magic == kGlobalBlockMagic ? block : nullptr;
}

} // namespace

extern "C" {

void SWELL_set_app_main(SWELLAppMainProc app_main) {
    g_app_main = app_main;
}

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2) {
    return g_app_main ? g_app_main(msg, parm1, parm2) : 0;
}

void SWELL_initargs(int* argc, char*** argv) {
    g_argc = argc ? *argc : 0;
    g_argv = argv ? *argv : nullptr;
}

void SWELL_RunMessageLoop() {
    // The real SWELL pumps native events here. Until HWND/event dispatch exists,
    // yield briefly so callers that poll this function do not spin hot.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

char* lstrcpyn(char* dest, const char* src, int l) {
    if (!dest || l <= 0) return dest;
    if (!src) src = "";
    int n = std::min<int>(l - 1, int(std::strlen(src)));
    if (n > 0) std::memcpy(dest, src, size_t(n));
    dest[n] = 0;
    return dest;
}

int MulDiv(int value, int mul, int div) {
    if (!div) return 0;
    return int((int64_t(value) * int64_t(mul)) / int64_t(div));
}

void Sleep(int ms) {
    if (ms <= 0) ms = 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

DWORD GetTickCount() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count();
    return DWORD(ms);
}

BOOL GetFileTime(int filedes, FILETIME* creation, FILETIME* access, FILETIME* write) {
    struct stat st {};
    if (fstat(filedes, &st) != 0) return SWELL_FALSE;
    FILETIME at = to_filetime(st.st_atim);
    FILETIME mt = to_filetime(st.st_mtim);
    FILETIME ct = to_filetime(st.st_ctim);
    if (creation) *creation = ct;
    if (access) *access = at;
    if (write) *write = mt;
    return SWELL_TRUE;
}

BOOL WritePrivateProfileString(const char* appname, const char* keyname, const char* val, const char* fn) {
    if (!appname) return SWELL_FALSE;
    auto data = read_ini(fn);
    if (!keyname) data.erase(appname);
    else if (!val) data[appname].erase(keyname);
    else data[appname][keyname] = val;
    return write_ini(fn, data) ? SWELL_TRUE : SWELL_FALSE;
}

DWORD GetPrivateProfileString(const char* appname, const char* keyname, const char* def,
                              char* ret, int retsize, const char* fn) {
    auto data = read_ini(fn);
    if (!appname) {
        std::vector<std::string> sections;
        for (const auto& [section, _] : data) sections.push_back(section);
        return copy_multi_result(sections, ret, DWORD(retsize));
    }
    auto sit = data.find(appname);
    if (!keyname) {
        std::vector<std::string> keys;
        if (sit != data.end()) {
            for (const auto& [key, _] : sit->second) keys.push_back(key);
        }
        return copy_multi_result(keys, ret, DWORD(retsize));
    }
    if (sit != data.end()) {
        auto kit = sit->second.find(keyname);
        if (kit != sit->second.end()) return copy_string_result(kit->second, ret, retsize);
    }
    return copy_string_result(def ? def : "", ret, retsize);
}

int GetPrivateProfileInt(const char* appname, const char* keyname, int def, const char* fn) {
    char buf[64];
    GetPrivateProfileString(appname, keyname, "", buf, sizeof(buf), fn);
    return *buf ? std::atoi(buf) : def;
}

BOOL GetPrivateProfileStruct(const char* appname, const char* keyname, void* buf, int bufsz, const char* fn) {
    if (!buf || bufsz < 0) return SWELL_FALSE;
    std::string tmp(size_t(bufsz) * 2 + 1, '\0');
    GetPrivateProfileString(appname, keyname, "", tmp.data(), int(tmp.size()), fn);
    tmp.resize(std::strlen(tmp.c_str()));
    return hex_to_bytes(tmp, buf, bufsz) ? SWELL_TRUE : SWELL_FALSE;
}

BOOL WritePrivateProfileStruct(const char* appname, const char* keyname, const void* buf, int bufsz, const char* fn) {
    if (!buf || bufsz < 0) return SWELL_FALSE;
    auto hex = bytes_to_hex(buf, bufsz);
    return WritePrivateProfileString(appname, keyname, hex.c_str(), fn);
}

BOOL WritePrivateProfileSection(const char* appname, const char* strings, const char* fn) {
    if (!appname) return SWELL_FALSE;
    auto data = read_ini(fn);
    auto& section = data[appname];
    section.clear();
    if (strings) {
        const char* p = strings;
        while (*p) {
            std::string line = p;
            auto eq = line.find('=');
            if (eq != std::string::npos) section[line.substr(0, eq)] = line.substr(eq + 1);
            p += line.size() + 1;
        }
    }
    return write_ini(fn, data) ? SWELL_TRUE : SWELL_FALSE;
}

DWORD GetPrivateProfileSection(const char* appname, char* strout, DWORD strout_len, const char* fn) {
    std::vector<std::string> lines;
    auto data = read_ini(fn);
    auto sit = appname ? data.find(appname) : data.end();
    if (sit != data.end()) {
        for (const auto& [key, value] : sit->second)
            lines.push_back(key + "=" + value);
    }
    return copy_multi_result(lines, strout, strout_len);
}

DWORD GetModuleFileName(HINSTANCE, char* fn, DWORD nSize) {
    if (!fn || nSize == 0) return 0;
    std::vector<char> buf(std::max<DWORD>(nSize, 1));
    ssize_t n = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if (n < 0) return copy_string_result("", fn, int(nSize));
    buf[size_t(n)] = 0;
    return copy_string_result(buf.data(), fn, int(nSize));
}

BOOL SWELL_PtInRect(const RECT* r, POINT p) {
    return (r && p.x >= r->left && p.x < r->right && p.y >= r->top && p.y < r->bottom)
        ? SWELL_TRUE : SWELL_FALSE;
}

BOOL WinOffsetRect(RECT* r, int dx, int dy) {
    if (!r) return SWELL_FALSE;
    r->left += dx; r->right += dx; r->top += dy; r->bottom += dy;
    return SWELL_TRUE;
}

void WinSetRect(RECT* r, int left, int top, int right, int bottom) {
    if (!r) return;
    *r = {left, top, right, bottom};
}

BOOL WinUnionRect(RECT* out, const RECT* a, const RECT* b) {
    if (!out || !a || !b) return SWELL_FALSE;
    out->left = std::min(a->left, b->left);
    out->top = std::min(a->top, b->top);
    out->right = std::max(a->right, b->right);
    out->bottom = std::max(a->bottom, b->bottom);
    return SWELL_TRUE;
}

BOOL WinIntersectRect(RECT* out, const RECT* a, const RECT* b) {
    if (!out || !a || !b) return SWELL_FALSE;
    out->left = std::max(a->left, b->left);
    out->top = std::max(a->top, b->top);
    out->right = std::min(a->right, b->right);
    out->bottom = std::min(a->bottom, b->bottom);
    if (out->right <= out->left || out->bottom <= out->top) {
        *out = {};
        return SWELL_FALSE;
    }
    return SWELL_TRUE;
}

BOOL ShellExecute(HWND, const char*, const char* content1, const char* content2, const char* content3, int) {
    if (!content1 || !*content1) return SWELL_FALSE;
    pid_t pid = fork();
    if (pid < 0) return SWELL_FALSE;
    if (pid == 0) {
        if (content2 && *content2)
            execlp(content1, content1, content2, static_cast<char*>(nullptr));
        else if (content3 && *content3)
            execlp(content1, content1, content3, static_cast<char*>(nullptr));
        else
            execlp("xdg-open", "xdg-open", content1, static_cast<char*>(nullptr));
        _exit(127);
    }
    return SWELL_TRUE;
}

void GetTempPath(int sz, char* buf) {
    const char* tmp = std::getenv("TMPDIR");
    if (!tmp || !*tmp) tmp = "/tmp";
    std::string path = tmp;
    if (!path.empty() && path.back() != '/') path.push_back('/');
    copy_string_result(path, buf, sz);
}

HANDLE GlobalAlloc(int, int sz) {
    if (sz < 0) return nullptr;
    auto total = sizeof(GlobalBlock) + size_t(std::max(sz, 1));
    auto* block = static_cast<GlobalBlock*>(std::malloc(total));
    if (!block) return nullptr;
    block->magic = kGlobalBlockMagic;
    block->size = sz;
    return block + 1;
}

void* GlobalLock(HANDLE h) {
    return global_header(h) ? h : nullptr;
}

int GlobalSize(HANDLE h) {
    auto* block = global_header(h);
    return block ? block->size : 0;
}

void GlobalUnlock(HANDLE) {}

void GlobalFree(HANDLE h) {
    if (auto* block = global_header(h)) {
        block->magic = 0;
        std::free(block);
    }
}

HINSTANCE LoadLibraryGlobals(const char* fileName, bool symbolsAsGlobals) {
    if (!fileName) return nullptr;
    return dlopen(fileName, RTLD_NOW | (symbolsAsGlobals ? RTLD_GLOBAL : RTLD_LOCAL));
}

HINSTANCE LoadLibrary(const char* fileName) {
    return LoadLibraryGlobals(fileName, false);
}

void* GetProcAddress(HINSTANCE hInst, const char* procName) {
    return hInst && procName ? dlsym(hInst, procName) : nullptr;
}

BOOL FreeLibrary(HINSTANCE hInst) {
    return hInst && dlclose(hInst) == 0 ? SWELL_TRUE : SWELL_FALSE;
}

void* SWELL_GetBundle(HINSTANCE hInst) {
    return hInst;
}

unsigned int _controlfp(unsigned int, unsigned int) {
    return 0;
}

bool SWELL_GenerateGUID(void* g) {
    if (!g) return false;
    std::random_device rd;
    auto* guid = static_cast<GUID*>(g);
    guid->Data1 = (uint32_t(rd()) << 16) ^ uint32_t(rd());
    guid->Data2 = uint16_t(rd());
    guid->Data3 = uint16_t((rd() & 0x0fff) | 0x4000);
    for (auto& b : guid->Data4) b = uint8_t(rd());
    guid->Data4[0] = uint8_t((guid->Data4[0] & 0x3f) | 0x80);
    return true;
}

} // extern "C"
