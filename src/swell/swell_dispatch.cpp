#include "swell_abi.hpp"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_set>

extern "C" {

void SWELL_set_app_main(SWELLAppMainProc app_main);
void SWELL_initargs(int* argc, char*** argv);
void SWELL_RunMessageLoop();
char* lstrcpyn(char* dest, const char* src, int l);
int MulDiv(int value, int mul, int div);
void Sleep(int ms);
DWORD GetTickCount();
BOOL GetFileTime(int filedes, FILETIME* creation, FILETIME* access, FILETIME* write);
BOOL WritePrivateProfileString(const char* appname, const char* keyname, const char* val, const char* fn);
DWORD GetPrivateProfileString(const char* appname, const char* keyname, const char* def, char* ret, int retsize, const char* fn);
int GetPrivateProfileInt(const char* appname, const char* keyname, int def, const char* fn);
BOOL GetPrivateProfileStruct(const char* appname, const char* keyname, void* buf, int bufsz, const char* fn);
BOOL WritePrivateProfileStruct(const char* appname, const char* keyname, const void* buf, int bufsz, const char* fn);
BOOL WritePrivateProfileSection(const char* appname, const char* strings, const char* fn);
DWORD GetPrivateProfileSection(const char* appname, char* strout, DWORD strout_len, const char* fn);
DWORD GetModuleFileName(HINSTANCE hInst, char* fn, DWORD nSize);
BOOL SWELL_PtInRect(const RECT* r, POINT p);
BOOL WinOffsetRect(RECT* r, int dx, int dy);
void WinSetRect(RECT* r, int left, int top, int right, int bottom);
BOOL WinUnionRect(RECT* out, const RECT* a, const RECT* b);
BOOL WinIntersectRect(RECT* out, const RECT* a, const RECT* b);
BOOL ShellExecute(HWND hwnd, const char* action, const char* content1, const char* content2, const char* content3, int show);
void GetTempPath(int sz, char* buf);
HANDLE GlobalAlloc(int flags, int sz);
void* GlobalLock(HANDLE h);
int GlobalSize(HANDLE h);
void GlobalUnlock(HANDLE h);
void GlobalFree(HANDLE h);
HINSTANCE LoadLibraryGlobals(const char* fileName, bool symbolsAsGlobals);
HINSTANCE LoadLibrary(const char* fileName);
void* GetProcAddress(HINSTANCE hInst, const char* procName);
BOOL FreeLibrary(HINSTANCE hInst);
void* SWELL_GetBundle(HINSTANCE hInst);
unsigned int _controlfp(unsigned int flag, unsigned int mask);
bool SWELL_GenerateGUID(void* g);

HWND CreateWindowEx(DWORD exstyle, const char* class_name, const char* window_name, DWORD style, int x, int y, int w, int h, HWND parent, HMENU menu, HINSTANCE, LPVOID param);
BOOL DestroyWindow(HWND hwnd);
BOOL ShowWindow(HWND hwnd, int cmd);
BOOL IsWindow(HWND hwnd);
BOOL IsWindowVisible(HWND hwnd);
BOOL EnableWindow(HWND hwnd, BOOL enable);
BOOL IsWindowEnabled(HWND hwnd);
HWND GetDlgItem(HWND parent, int id);
BOOL IsChild(HWND parent, HWND child);
void SetWindowText(HWND hwnd, const char* text);
int GetWindowText(HWND hwnd, char* buf, int maxlen);
int GetWindowTextLength(HWND hwnd);
void SetDlgItemText(HWND hwnd, int id, const char* text);
int GetDlgItemText(HWND hwnd, int id, char* buf, int maxlen);
BOOL GetClientRect(HWND hwnd, RECT* r);
BOOL GetWindowRect(HWND hwnd, RECT* r);
BOOL GetWindowContentViewRect(HWND hwnd, RECT* r);
BOOL SetWindowPos(HWND hwnd, HWND insert_after, int x, int y, int cx, int cy, UINT flags);
BOOL MoveWindow(HWND hwnd, int x, int y, int w, int h, BOOL repaint);
BOOL InvalidateRect(HWND hwnd, const RECT* r, BOOL erase);
BOOL UpdateWindow(HWND hwnd);
LONG_PTR GetWindowLongPtr(HWND hwnd, int index);
LONG_PTR SetWindowLongPtr(HWND hwnd, int index, LONG_PTR value);
LONG GetWindowLong(HWND hwnd, int index);
LONG SetWindowLong(HWND hwnd, int index, LONG value);
BOOL SetParent(HWND hwnd, HWND new_parent);
HWND GetWindow(HWND hwnd, UINT cmd);
HWND GetParent(HWND hwnd);
HWND GetTopWindow(HWND hwnd);
HWND GetNextWindow(HWND hwnd, UINT cmd);
BOOL ClientToScreen(HWND hwnd, POINT* p);
BOOL ScreenToClient(HWND hwnd, POINT* p);
HWND SetFocus(HWND hwnd);
HWND GetFocus();
HWND SetForegroundWindow(HWND hwnd);
HWND GetForegroundWindow();
HWND SetCapture(HWND hwnd);
HWND GetCapture();
BOOL ReleaseCapture();
BOOL EnumChildWindows(HWND parent, BOOL (*callback)(HWND, LPARAM), LPARAM lp);
BOOL EnumWindows(BOOL (*callback)(HWND, LPARAM), LPARAM lp);
HWND FindWindowEx(HWND parent, HWND child_after, const char* class_name, const char* window_name);
HWND WindowFromPoint(POINT p);
BOOL ScrollWindow(HWND hwnd, int dx, int dy, const RECT* prc_scroll, const RECT* prc_clip);
HANDLE GetProp(HWND hwnd, const char* name);
BOOL SetProp(HWND hwnd, const char* name, HANDLE value);
HANDLE RemoveProp(HWND hwnd, const char* name);
BOOL EnumPropsEx(HWND hwnd, BOOL (*callback)(HWND, const char*, HANDLE, LONG_PTR), LONG_PTR lp);
HWND SetActiveWindow(HWND hwnd);
HWND GetActiveWindow();
BOOL BringWindowToTop(HWND hwnd);
BOOL SetWindowRgn(HWND hwnd, HRGN hrgn, BOOL redraw);
int GetWindowRgn(HWND hwnd, HRGN hrgn);
BOOL IsIconic(HWND hwnd);
BOOL IsZoomed(HWND hwnd);
void SetWindowTextA(HWND hwnd, const char* text);
int GetWindowTextA(HWND hwnd, char* buf, int maxlen);
int GetWindowTextLengthA(HWND hwnd);

LRESULT DefWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT SendMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT SendMessageTimeoutA(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT flags, UINT timeout, DWORD* result);
LRESULT SendDlgItemMessageA(HWND parent, int id, UINT msg, WPARAM wp, LPARAM lp);
LRESULT SendDlgItemMessage(HWND parent, int id, UINT msg, WPARAM wp, LPARAM lp);
BOOL PostMessage(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
void SWELL_BroadcastMessage(UINT msg, WPARAM wp, LPARAM lp);
void SWELL_MessageQueue_Flush(HWND hwnd);
void SWELL_MessageQueue_Clear(HWND hwnd);
void SWELL_Internal_PostMessage_Init();
UINT_PTR SetTimer(HWND hwnd, UINT_PTR id, UINT ms, TIMERPROC proc);
BOOL KillTimer(HWND hwnd, UINT_PTR id);

HMENU CreatePopupMenu();
HMENU CreatePopupMenuEx(const char* title);
HMENU CreateMenu();
BOOL DestroyMenu(HMENU menu);
HMENU GetSubMenu(HMENU menu, int pos);
int GetMenuItemCount(HMENU menu);
UINT GetMenuItemID(HMENU menu, int pos);
BOOL SetMenuItemModifier(HMENU menu, UINT id, UINT flags, UINT modifier);
BOOL SetMenuItemText(HMENU menu, UINT id, const char* text);
BOOL EnableMenuItem(HMENU menu, UINT id, UINT flags);
BOOL DeleteMenu(HMENU menu, UINT pos, UINT flags);
BOOL CheckMenuItem(HMENU menu, UINT id, UINT flags);
BOOL InsertMenuItem(HMENU menu, UINT pos, BOOL by_position, const void* info);
BOOL SWELL_InsertMenu(HMENU menu, int pos, int flags, int id, const char* text);
BOOL AppendMenu(HMENU menu, UINT flags, UINT id, const char* text);
BOOL GetMenuItemInfo(HMENU menu, UINT item, BOOL by_position, void* info);
BOOL SetMenuItemInfo(HMENU menu, UINT item, BOOL by_position, const void* info);
BOOL DrawMenuBar(HWND hwnd);
HMENU SWELL_LoadMenu(HINSTANCE inst, const char* name);
BOOL TrackPopupMenu(HMENU menu, UINT flags, int x, int y, int reserved, HWND hwnd, const RECT* rect);
BOOL TrackPopupMenuEx(HMENU menu, UINT flags, int x, int y, HWND hwnd, void* tp);
void SWELL_SetMenuDestination(HMENU menu, HWND hwnd);
HMENU SWELL_DuplicateMenu(HMENU menu);
BOOL SetMenu(HWND hwnd, HMENU menu);
HMENU GetMenu(HWND hwnd);
HMENU SWELL_GetDefaultWindowMenu(HWND hwnd);
void SWELL_SetDefaultWindowMenu(HWND hwnd, HMENU menu);
HMENU SWELL_GetDefaultModalWindowMenu();
void SWELL_SetDefaultModalWindowMenu(HMENU menu);
HMENU SWELL_GetCurrentMenu();
void SWELL_SetCurrentMenu(HMENU menu);
void SWELL_Menu_AddMenuItem(HMENU menu, const char* text, int id);
BOOL SWELL_GenerateMenuFromList(HMENU menu, const void* list);

INT_PTR SWELL_unimplemented_stub() {
    return 0;
}

}

namespace {

struct ApiEntry {
    const char* name;
    void* func;
    bool implemented;
};

#define API_IMPL(name) {#name, reinterpret_cast<void*>(&name), true}
#define API_STUB(name) {#name, reinterpret_cast<void*>(&SWELL_unimplemented_stub), false}

ApiEntry kApiTable[] = {
    API_IMPL(SWELL_set_app_main),
    API_IMPL(lstrcpyn),
    API_IMPL(MulDiv),
    API_IMPL(Sleep),
    API_IMPL(GetTickCount),
    API_IMPL(GetFileTime),
    API_IMPL(WritePrivateProfileString),
    API_IMPL(GetPrivateProfileString),
    API_IMPL(GetPrivateProfileInt),
    API_IMPL(GetPrivateProfileStruct),
    API_IMPL(WritePrivateProfileStruct),
    API_IMPL(WritePrivateProfileSection),
    API_IMPL(GetPrivateProfileSection),
    API_IMPL(GetModuleFileName),
    API_IMPL(SWELL_PtInRect),
    API_IMPL(ShellExecute),
    API_STUB(BrowseForFiles),
    API_STUB(BrowseForSaveFile),
    API_STUB(BrowseForDirectory),
    API_STUB(BrowseFile_SetTemplate),
    API_IMPL(GetDlgItem),
    API_IMPL(ShowWindow),
    API_IMPL(DestroyWindow),
    API_STUB(SWELL_GetGestureInfo),
    API_STUB(SWELL_HideApp),
    API_IMPL(SetDlgItemText),
    API_STUB(SetDlgItemInt),
    API_STUB(GetDlgItemInt),
    API_IMPL(GetDlgItemText),
    API_IMPL(GetWindowTextLength),
    API_STUB(CheckDlgButton),
    API_STUB(IsDlgButtonChecked),
    API_IMPL(EnableWindow),
    API_IMPL(SetFocus),
    API_IMPL(GetFocus),
    API_IMPL(SetForegroundWindow),
    API_IMPL(GetForegroundWindow),
    API_IMPL(SetCapture),
    API_IMPL(GetCapture),
    API_IMPL(ReleaseCapture),
    API_IMPL(IsChild),
    API_IMPL(SetParent),
    API_IMPL(GetWindow),
    API_IMPL(EnumWindows),
    API_IMPL(FindWindowEx),
    API_IMPL(ClientToScreen),
    API_IMPL(ScreenToClient),
    API_IMPL(GetWindowRect),
    API_IMPL(GetWindowContentViewRect),
    API_IMPL(GetClientRect),
    API_IMPL(WindowFromPoint),
    API_IMPL(WinOffsetRect),
    API_IMPL(WinSetRect),
    API_IMPL(WinUnionRect),
    API_IMPL(WinIntersectRect),
    API_IMPL(SetWindowPos),
    API_STUB(SWELL_SetWindowLevel),
    API_IMPL(InvalidateRect),
    API_IMPL(UpdateWindow),
    API_IMPL(GetWindowLong),
    API_IMPL(SetWindowLong),
    API_IMPL(ScrollWindow),
    API_IMPL(EnumPropsEx),
    API_IMPL(GetProp),
    API_IMPL(SetProp),
    API_IMPL(RemoveProp),
    API_IMPL(IsWindowVisible),
    API_IMPL(IsWindow),
    API_IMPL(SetTimer),
    API_IMPL(KillTimer),
    API_STUB(ListView_InsertColumn),
    API_STUB(ListView_DeleteColumn),
    API_STUB(ListView_SetColumn),
    API_STUB(ListView_GetColumn),
    API_STUB(ListView_GetColumnWidth),
    API_STUB(ListView_InsertItem),
    API_STUB(ListView_SetItemText),
    API_STUB(ListView_SetItem),
    API_STUB(ListView_GetNextItem),
    API_STUB(ListView_GetItem),
    API_STUB(ListView_GetItemState),
    API_STUB(ListView_DeleteItem),
    API_STUB(ListView_DeleteAllItems),
    API_STUB(ListView_GetSelectedCount),
    API_STUB(ListView_GetItemCount),
    API_STUB(ListView_GetSelectionMark),
    API_STUB(ListView_SetColumnWidth),
    API_STUB(ListView_SetItemState),
    API_STUB(ListView_RedrawItems),
    API_STUB(ListView_SetItemCount),
    API_STUB(ListView_EnsureVisible),
    API_STUB(ListView_SetImageList),
    API_STUB(ListView_SubItemHitTest),
    API_STUB(ListView_GetItemText),
    API_STUB(ListView_SortItems),
    API_STUB(ListView_Scroll),
    API_STUB(ListView_GetTopIndex),
    API_STUB(ListView_GetCountPerPage),
    API_STUB(ListView_SetColumnOrderArray),
    API_STUB(ListView_GetColumnOrderArray),
    API_STUB(ListView_GetHeader),
    API_STUB(Header_GetItemCount),
    API_STUB(Header_GetItem),
    API_STUB(Header_SetItem),
    API_STUB(ListView_GetItemRect),
    API_STUB(ListView_GetSubItemRect),
    API_STUB(ListView_HitTest),
    API_STUB(SWELL_GetListViewHeaderHeight),
    API_STUB(ImageList_CreateEx),
    API_STUB(ImageList_Remove),
    API_STUB(ImageList_ReplaceIcon),
    API_STUB(ImageList_Add),
    API_STUB(ImageList_Destroy),
    API_STUB(TabCtrl_GetItemCount),
    API_STUB(TabCtrl_DeleteItem),
    API_STUB(TabCtrl_InsertItem),
    API_STUB(TabCtrl_SetCurSel),
    API_STUB(TabCtrl_GetCurSel),
    API_STUB(TabCtrl_AdjustRect),
    API_STUB(TreeView_InsertItem),
    API_STUB(TreeView_Expand),
    API_STUB(TreeView_GetSelection),
    API_STUB(TreeView_DeleteItem),
    API_STUB(TreeView_DeleteAllItems),
    API_STUB(TreeView_SelectItem),
    API_STUB(TreeView_EnsureVisible),
    API_STUB(TreeView_GetItem),
    API_STUB(TreeView_SetItem),
    API_STUB(TreeView_HitTest),
    API_STUB(TreeView_SetIndent),
    API_STUB(TreeView_GetParent),
    API_STUB(TreeView_GetChild),
    API_STUB(TreeView_GetNextSibling),
    API_STUB(TreeView_GetRoot),
    API_STUB(TreeView_SetBkColor),
    API_STUB(TreeView_SetTextColor),
    API_STUB(ListView_SetBkColor),
    API_STUB(ListView_SetTextBkColor),
    API_STUB(ListView_SetTextColor),
    API_STUB(ListView_SetGridColor),
    API_STUB(ListView_SetSelColors),
    API_STUB(SWELL_ModalWindowStart),
    API_STUB(SWELL_ModalWindowRun),
    API_STUB(SWELL_ModalWindowEnd),
    API_STUB(SWELL_CloseWindow),
    API_IMPL(CreatePopupMenu),
    API_IMPL(CreatePopupMenuEx),
    API_IMPL(DestroyMenu),
    API_IMPL(GetSubMenu),
    API_IMPL(GetMenuItemCount),
    API_IMPL(GetMenuItemID),
    API_IMPL(SetMenuItemModifier),
    API_IMPL(SetMenuItemText),
    API_IMPL(EnableMenuItem),
    API_IMPL(DeleteMenu),
    API_IMPL(CheckMenuItem),
    API_IMPL(InsertMenuItem),
    API_IMPL(SWELL_InsertMenu),
    API_IMPL(GetMenuItemInfo),
    API_IMPL(SetMenuItemInfo),
    API_IMPL(DrawMenuBar),
    API_IMPL(SWELL_LoadMenu),
    API_IMPL(TrackPopupMenu),
    API_IMPL(SWELL_SetMenuDestination),
    API_IMPL(SWELL_DuplicateMenu),
    API_IMPL(SetMenu),
    API_IMPL(GetMenu),
    API_IMPL(SWELL_GetDefaultWindowMenu),
    API_IMPL(SWELL_SetDefaultWindowMenu),
    API_IMPL(SWELL_GetDefaultModalWindowMenu),
    API_IMPL(SWELL_SetDefaultModalWindowMenu),
    API_IMPL(SWELL_GetCurrentMenu),
    API_IMPL(SWELL_SetCurrentMenu),
    API_STUB(SWELL_DialogBox),
    API_STUB(SWELL_CreateDialog),
    API_IMPL(DefWindowProc),
    API_STUB(EndDialog),
    API_STUB(SWELL_GetDefaultButtonID),
    API_IMPL(SendMessage),
    API_IMPL(SWELL_BroadcastMessage),
    API_IMPL(PostMessage),
    API_IMPL(SWELL_MessageQueue_Flush),
    API_IMPL(SWELL_MessageQueue_Clear),
    API_IMPL(SWELL_Internal_PostMessage_Init),
    API_STUB(SWELL_KeyToASCII),
    API_STUB(GetAsyncKeyState),
    API_STUB(GetCursorPos),
    API_STUB(GetMessagePos),
    API_STUB(SWELL_LoadCursor),
    API_STUB(SWELL_SetCursor),
    API_STUB(SWELL_EnableRightClickEmulate),
    API_STUB(SWELL_GetCursor),
    API_STUB(SWELL_GetLastSetCursor),
    API_STUB(SWELL_IsCursorVisible),
    API_STUB(SWELL_ShowCursor),
    API_STUB(SWELL_SetCursorPos),
    API_STUB(SWELL_GetViewPort),
    API_STUB(OpenClipboard),
    API_STUB(CloseClipboard),
    API_STUB(GetClipboardData),
    API_STUB(EmptyClipboard),
    API_STUB(SetClipboardData),
    API_STUB(RegisterClipboardFormat),
    API_STUB(EnumClipboardFormats),
    API_IMPL(GlobalAlloc),
    API_IMPL(GlobalLock),
    API_IMPL(GlobalSize),
    API_IMPL(GlobalUnlock),
    API_IMPL(GlobalFree),
    API_STUB(CreateThread),
    API_STUB(CreateEvent),
    API_STUB(CreateEventAsSocket),
    API_STUB(GetCurrentThreadId),
    API_STUB(WaitForSingleObject),
    API_STUB(WaitForAnySocketObject),
    API_STUB(CloseHandle),
    API_STUB(SetThreadPriority),
    API_STUB(SetEvent),
    API_STUB(ResetEvent),
    API_STUB(SWELL_CreateProcessFromPID),
    API_STUB(SWELL_CreateProcess),
    API_STUB(SWELL_GetProcessExitCode),
    API_IMPL(LoadLibraryGlobals),
    API_IMPL(LoadLibrary),
    API_IMPL(GetProcAddress),
    API_IMPL(FreeLibrary),
    API_IMPL(SWELL_GetBundle),
    API_STUB(SWELL_CreateMemContext),
    API_STUB(SWELL_DeleteGfxContext),
    API_STUB(SWELL_GetCtxGC),
    API_STUB(SWELL_GetCtxFrameBuffer),
    API_STUB(SWELL_PushClipRegion),
    API_STUB(SWELL_SetClipRegion),
    API_STUB(SWELL_PopClipRegion),
    API_STUB(CreateFontIndirect),
    API_STUB(CreatePen),
    API_STUB(CreateSolidBrush),
    API_STUB(CreatePenAlpha),
    API_STUB(CreateSolidBrushAlpha),
    API_STUB(SelectObject),
    API_STUB(GetStockObject),
    API_STUB(DeleteObject),
    API_STUB(SWELL_FillRect),
    API_STUB(Rectangle),
    API_STUB(Ellipse),
    API_STUB(SWELL_Polygon),
    API_STUB(MoveToEx),
    API_STUB(LineTo),
    API_STUB(SetPixel),
    API_STUB(PolyBezierTo),
    API_STUB(SWELL_DrawText),
    API_STUB(GetTextColor),
    API_STUB(SetBkMode),
    API_STUB(GetGlyphIndicesW),
    API_STUB(PolyPolyline),
    API_STUB(GetTextMetrics),
    API_STUB(GetTextFace),
    API_STUB(GetObject),
    API_STUB(CreateIconIndirect),
    API_STUB(LoadNamedImage),
    API_STUB(DrawImageInRect),
    API_STUB(BitBlt),
    API_STUB(StretchBlt),
    API_STUB(StretchBltFromMem),
    API_STUB(SWELL_GetScaling256),
    API_STUB(SWELL_ExtendedAPI),
    API_STUB(GetSysColor),
    API_STUB(SetOpaque),
    API_STUB(SetAllowNoMiddleManRendering),
    API_STUB(SWELL_SetViewGL),
    API_STUB(SWELL_GetViewGL),
    API_STUB(SWELL_SetGLContextToView),
    API_STUB(BeginPaint),
    API_STUB(EndPaint),
    API_STUB(GetWindowDC),
    API_STUB(ReleaseDC),
    API_STUB(SWELL_FillDialogBackground),
    API_STUB(SWELL_CloneGDIObject),
    API_STUB(GetSystemMetrics),
    API_STUB(DragQueryPoint),
    API_STUB(DragFinish),
    API_STUB(DragQueryFile),
    API_STUB(SWELL_InitiateDragDrop),
    API_STUB(SWELL_FinishDragDrop),
    API_STUB(SWELL_DrawFocusRect),
    API_STUB(SWELL_MakeSetCurParms),
    API_STUB(SWELL_MakeButton),
    API_STUB(SWELL_MakeEditField),
    API_STUB(SWELL_MakeLabel),
    API_STUB(SWELL_MakeControl),
    API_STUB(SWELL_MakeCombo),
    API_STUB(SWELL_MakeGroupBox),
    API_STUB(SWELL_MakeCheckBox),
    API_STUB(SWELL_MakeListBox),
    API_STUB(SWELL_Menu_AddMenuItem),
    API_STUB(SWELL_GenerateMenuFromList),
    API_STUB(SWELL_GenerateDialogFromList),
    API_IMPL(_controlfp),
    API_STUB(SWELL_LoadCursorFromFile),
    API_STUB(SWELL_SetWindowWantRaiseAmt),
    API_STUB(SWELL_GetWindowWantRaiseAmt),
    API_IMPL(GetTempPath),
    API_IMPL(SWELL_initargs),
    API_IMPL(SWELL_RunMessageLoop),
    API_STUB(SWELL_CreateXBridgeWindow),
    API_STUB(SWELL_GetOSWindow),
    API_STUB(SWELL_GetOSEvent),
    API_IMPL(SWELL_GenerateGUID),
    API_STUB(EnumChildWindows),
    API_IMPL(SetWindowText),
    API_IMPL(GetWindowText),
    API_IMPL(SetWindowTextA),
    API_IMPL(GetWindowTextA),
    API_IMPL(GetWindowTextLengthA),
    API_IMPL(GetParent),
    API_IMPL(GetTopWindow),
    API_IMPL(GetNextWindow),
    API_IMPL(SetActiveWindow),
    API_IMPL(GetActiveWindow),
    API_IMPL(BringWindowToTop),
    API_IMPL(SetWindowRgn),
    API_IMPL(GetWindowRgn),
    API_IMPL(IsIconic),
    API_IMPL(IsZoomed),
    API_IMPL(SendMessageTimeoutA),
    API_IMPL(SendDlgItemMessageA),
    API_IMPL(SendDlgItemMessage),
    API_IMPL(AppendMenu),
    API_IMPL(TrackPopupMenuEx),
    API_IMPL(SWELL_Menu_AddMenuItem),
    API_STUB(SWELL_IsGroupBox),
    API_STUB(SWELL_IsButton),
    API_STUB(SWELL_IsStaticText),
    API_STUB(SWELL_GetDesiredControlSize),
    API_STUB(AddFontResourceEx),
    API_STUB(SWELL_ChooseColor),
    API_STUB(SWELL_ChooseFont),
    API_IMPL(IsWindowEnabled),
    API_STUB(GetClassName),
    API_STUB(SWELL_SetClassName),
    API_STUB(SWELL_DisableContextMenu),
    API_STUB(EnumDisplayMonitors),
    API_STUB(GetMonitorInfo),
    API_STUB(ListView_SetExtendedListViewStyleEx),
    API_STUB(SWELL_RegisterCustomControlCreator),
    API_STUB(SWELL_UnregisterCustomControlCreator),
    API_STUB(SWELL_InitiateDragDropOfFileList),
    API_STUB(SWELL_SetListViewFastClickMask),
    API_STUB(SWELL_Register_Cursor_Resource),
};

#undef API_IMPL
#undef API_STUB

void log_unimplemented_once(const char* name) {
    static std::mutex mutex;
    static std::unordered_set<std::string> seen;
    std::lock_guard lock{mutex};
    if (seen.insert(name ? name : "").second)
        std::fprintf(stderr, "swole libSwell: unresolved SWELL API stubbed: %s\n", name);
}

} // namespace

extern "C" __attribute__((visibility("default"))) void* SWELLAPI_GetFunc(const char* name) {
    if (!name) return reinterpret_cast<void*>(0x100);
    for (const auto& entry : kApiTable) {
        if (std::strcmp(entry.name, name) == 0) {
            if (!entry.implemented) log_unimplemented_once(name);
            return entry.func;
        }
    }
    std::fprintf(stderr, "swole libSwell: SWELLAPI_GetFunc missing: %s\n", name);
    return nullptr;
}
