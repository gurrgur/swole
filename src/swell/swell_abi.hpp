#pragma once

#include <cstdint>
#include <cstdio>

using INT_PTR = intptr_t;
using LONG_PTR = intptr_t;
using UINT_PTR = uintptr_t;
using DWORD_PTR = uintptr_t;

using BOOL = signed char;
using BYTE = unsigned char;
using WORD = unsigned short;
using DWORD = unsigned int;
using COLORREF = DWORD;
using UINT = unsigned int;
using LONG = signed int;
using ULONG = unsigned int;
using WPARAM = UINT_PTR;
using LPARAM = LONG_PTR;
using LRESULT = LONG_PTR;
using LPVOID = void*;
using HINSTANCE = void*;
using HANDLE = void*;
using HGLOBAL = void*;
using HWND = struct HWND__*;
using HMENU = struct HMENU__*;
using HDC = struct HDC__*;
using HGDIOBJ = void*;
using HCURSOR = void*;
using HDROP = void*;
using HRGN = void*;

struct POINT {
    LONG x;
    LONG y;
};

struct RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

struct GUID {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t Data4[8];
};

using TIMERPROC = void (*)(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);
using SWELLAppMainProc = INT_PTR (*)(int msg, INT_PTR parm1, INT_PTR parm2);

constexpr BOOL SWELL_FALSE = 0;
constexpr BOOL SWELL_TRUE = 1;
constexpr DWORD WAIT_OBJECT_0 = 0;
constexpr DWORD WAIT_TIMEOUT = 258;
constexpr DWORD WAIT_FAILED = 0xffffffffu;
constexpr DWORD INFINITE = 0xffffffffu;

constexpr int SWELLAPP_ONLOAD = 0x0001;
constexpr int SWELLAPP_LOADED = 0x0002;
constexpr int SWELLAPP_DESTROY = 0x0003;
constexpr int SWELLAPP_PROCESSMESSAGE = 0x0100;
