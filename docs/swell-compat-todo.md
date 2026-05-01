# SWELL Compatibility Todo

This file tracks the libSwell surface that `/usr/lib/REAPER/reaper` actually
requests on this machine. REAPER does not link `libSwell.so` through
`DT_NEEDED`; it loads `./libSwell.so` dynamically, resolves
`SWELLAPI_GetFunc`, then asks for Win32/SWELL entry points by string name.

The reference signatures live in the local WDL checkout:

- `/home/marcus/Workspace/WDL/WDL/swell/swell-functions.h`
- `/home/marcus/Workspace/WDL/WDL/swell/swell-types.h`

## Current Status

- [x] `libSwell.so` target exists in this repo.
- [x] `SWELLAPI_GetFunc`/`SWELL_set_app_main` ABI exports exist.
- [ ] The existing `swole` widget library is not yet wired to HWND/HDC/HMENU
      compatibility handles.

## Loader And Drop-In ABI

- [x] Build a shared library named `libSwell.so`.
- [x] Export `SWELLAPI_GetFunc`.
- [x] Export `SWELL_set_app_main`.
- [x] Implement a name-to-function dispatch table for every REAPER-requested API.
- [x] Add startup glue for `SWELL_initargs` and `SWELL_RunMessageLoop`.
- [x] Keep unresolved APIs visible through logging/counters during bring-up.

## Core Utilities And Files

- [x] `lstrcpyn`, `MulDiv`, `Sleep`, `GetTickCount`, `GetFileTime`
- [x] `WritePrivateProfileString`, `GetPrivateProfileString`,
      `GetPrivateProfileInt`, `GetPrivateProfileStruct`,
      `WritePrivateProfileStruct`, `WritePrivateProfileSection`,
      `GetPrivateProfileSection`
- [x] `GetModuleFileName`, `GetTempPath`, `ShellExecute`
- [ ] `BrowseForFiles`, `BrowseForSaveFile`, `BrowseForDirectory`,
      `BrowseFile_SetTemplate`
- [x] `SWELL_PtInRect`, `WinOffsetRect`, `WinSetRect`, `WinUnionRect`,
      `WinIntersectRect`
- [ ] `AddFontResourceEx`
- [x] `SWELL_GenerateGUID`, `_controlfp`

## Windows, Dialogs, And Controls

- [ ] HWND registry and lifetime model.
- [ ] `GetDlgItem`, `ShowWindow`, `DestroyWindow`, `SWELL_CloseWindow`
- [ ] Text/value APIs: `SetDlgItemText`, `SetDlgItemInt`, `GetDlgItemInt`,
      `GetDlgItemText`, `GetWindowTextLength`
- [ ] Button state APIs: `CheckDlgButton`, `IsDlgButtonChecked`
- [ ] Enable/focus/capture APIs: `EnableWindow`, `IsWindowEnabled`,
      `SetFocus`, `GetFocus`, `SetForegroundWindow`, `GetForegroundWindow`,
      `SetCapture`, `GetCapture`, `ReleaseCapture`
- [ ] Hierarchy/query APIs: `IsChild`, `SetParent`, `GetWindow`, `EnumWindows`,
      `EnumChildWindows`, `FindWindowEx`, `IsWindowVisible`, `IsWindow`,
      `WindowFromPoint`
- [ ] Geometry APIs: `ClientToScreen`, `ScreenToClient`, `GetWindowRect`,
      `GetWindowContentViewRect`, `GetClientRect`, `SetWindowPos`,
      `SWELL_SetWindowLevel`, `ScrollWindow`
- [ ] Long/property APIs: `GetWindowLong`, `SetWindowLong`, `EnumPropsEx`,
      `GetProp`, `SetProp`, `RemoveProp`
- [ ] Dialog entry points: `SWELL_DialogBox`, `SWELL_CreateDialog`,
      `DefWindowProc`, `EndDialog`, `SWELL_GetDefaultButtonID`
- [ ] Control factories: `SWELL_MakeSetCurParms`, `SWELL_MakeButton`,
      `SWELL_MakeEditField`, `SWELL_MakeLabel`, `SWELL_MakeControl`,
      `SWELL_MakeCombo`, `SWELL_MakeGroupBox`, `SWELL_MakeCheckBox`,
      `SWELL_MakeListBox`, `SWELL_GenerateDialogFromList`
- [ ] Control classification/sizing: `SWELL_IsGroupBox`, `SWELL_IsButton`,
      `SWELL_IsStaticText`, `SWELL_GetDesiredControlSize`,
      `SWELL_SetClassName`, `GetClassName`, `SWELL_DisableContextMenu`,
      `SWELL_RegisterCustomControlCreator`,
      `SWELL_UnregisterCustomControlCreator`

## Messages And Events

- [ ] `SendMessage`, `SWELL_BroadcastMessage`, `PostMessage`
- [ ] `SWELL_MessageQueue_Flush`, `SWELL_MessageQueue_Clear`,
      `SWELL_Internal_PostMessage_Init`
- [ ] `InvalidateRect`, `UpdateWindow`
- [ ] `SetTimer`, `KillTimer`
- [ ] Keyboard/mouse APIs: `SWELL_KeyToASCII`, `GetAsyncKeyState`,
      `GetCursorPos`, `GetMessagePos`, `SWELL_GetGestureInfo`
- [ ] Cursor APIs: `SWELL_LoadCursor`, `SWELL_SetCursor`,
      `SWELL_EnableRightClickEmulate`, `SWELL_GetCursor`,
      `SWELL_GetLastSetCursor`, `SWELL_IsCursorVisible`, `SWELL_ShowCursor`,
      `SWELL_SetCursorPos`, `SWELL_LoadCursorFromFile`,
      `SWELL_Register_Cursor_Resource`
- [ ] Modal loop APIs: `SWELL_ModalWindowStart`, `SWELL_ModalWindowRun`,
      `SWELL_ModalWindowEnd`

## Menus

- [ ] `CreatePopupMenu`, `CreatePopupMenuEx`, `DestroyMenu`, `GetSubMenu`,
      `GetMenuItemCount`, `GetMenuItemID`
- [ ] `SetMenuItemModifier`, `SetMenuItemText`, `EnableMenuItem`,
      `DeleteMenu`, `CheckMenuItem`, `InsertMenuItem`, `SWELL_InsertMenu`
- [ ] `GetMenuItemInfo`, `SetMenuItemInfo`, `DrawMenuBar`, `SWELL_LoadMenu`,
      `TrackPopupMenu`
- [ ] `SWELL_SetMenuDestination`, `SWELL_DuplicateMenu`, `SetMenu`, `GetMenu`
- [ ] Default/current menu APIs: `SWELL_GetDefaultWindowMenu`,
      `SWELL_SetDefaultWindowMenu`, `SWELL_GetDefaultModalWindowMenu`,
      `SWELL_SetDefaultModalWindowMenu`, `SWELL_GetCurrentMenu`,
      `SWELL_SetCurrentMenu`
- [ ] `SWELL_Menu_AddMenuItem`, `SWELL_GenerateMenuFromList`

## Common Controls

- [ ] ListView columns/items/state/sorting/scrolling/hit testing:
      `ListView_InsertColumn`, `ListView_DeleteColumn`, `ListView_SetColumn`,
      `ListView_GetColumn`, `ListView_GetColumnWidth`, `ListView_InsertItem`,
      `ListView_SetItemText`, `ListView_SetItem`, `ListView_GetNextItem`,
      `ListView_GetItem`, `ListView_GetItemState`, `ListView_DeleteItem`,
      `ListView_DeleteAllItems`, `ListView_GetSelectedCount`,
      `ListView_GetItemCount`, `ListView_GetSelectionMark`,
      `ListView_SetColumnWidth`, `ListView_SetItemState`,
      `ListView_RedrawItems`, `ListView_SetItemCount`,
      `ListView_EnsureVisible`, `ListView_SetImageList`,
      `ListView_SubItemHitTest`, `ListView_GetItemText`,
      `ListView_SortItems`, `ListView_Scroll`, `ListView_GetTopIndex`,
      `ListView_GetCountPerPage`, `ListView_SetColumnOrderArray`,
      `ListView_GetColumnOrderArray`, `ListView_GetHeader`,
      `ListView_GetItemRect`, `ListView_GetSubItemRect`, `ListView_HitTest`,
      `ListView_SetBkColor`, `ListView_SetTextBkColor`,
      `ListView_SetTextColor`, `ListView_SetGridColor`,
      `ListView_SetSelColors`, `ListView_SetExtendedListViewStyleEx`,
      `SWELL_GetListViewHeaderHeight`, `SWELL_SetListViewFastClickMask`
- [ ] Header control APIs: `Header_GetItemCount`, `Header_GetItem`,
      `Header_SetItem`
- [ ] Image lists: `ImageList_CreateEx`, `ImageList_Remove`,
      `ImageList_ReplaceIcon`, `ImageList_Add`, `ImageList_Destroy`
- [ ] Tabs: `TabCtrl_GetItemCount`, `TabCtrl_DeleteItem`,
      `TabCtrl_InsertItem`, `TabCtrl_SetCurSel`, `TabCtrl_GetCurSel`,
      `TabCtrl_AdjustRect`
- [ ] Trees: `TreeView_InsertItem`, `TreeView_Expand`,
      `TreeView_GetSelection`, `TreeView_DeleteItem`,
      `TreeView_DeleteAllItems`, `TreeView_SelectItem`,
      `TreeView_EnsureVisible`, `TreeView_GetItem`, `TreeView_SetItem`,
      `TreeView_HitTest`, `TreeView_SetIndent`, `TreeView_GetParent`,
      `TreeView_GetChild`, `TreeView_GetNextSibling`, `TreeView_GetRoot`,
      `TreeView_SetBkColor`, `TreeView_SetTextColor`

## GDI, Drawing, And GL

- [ ] HDC/GDI object model: `SWELL_CreateMemContext`,
      `SWELL_DeleteGfxContext`, `SWELL_GetCtxGC`, `SWELL_GetCtxFrameBuffer`,
      `SWELL_CloneGDIObject`, `CreateFontIndirect`, `CreatePen`,
      `CreateSolidBrush`, `CreatePenAlpha`, `CreateSolidBrushAlpha`,
      `SelectObject`, `GetStockObject`, `DeleteObject`, `GetObject`
- [ ] Clip stack: `SWELL_PushClipRegion`, `SWELL_SetClipRegion`,
      `SWELL_PopClipRegion`
- [ ] Drawing: `SWELL_FillRect`, `Rectangle`, `Ellipse`, `SWELL_Polygon`,
      `MoveToEx`, `LineTo`, `SetPixel`, `PolyBezierTo`, `SWELL_DrawText`,
      `PolyPolyline`, `SWELL_DrawFocusRect`, `SWELL_FillDialogBackground`
- [ ] Text metrics: `GetTextColor`, `SetBkMode`, `GetGlyphIndicesW`,
      `GetTextMetrics`, `GetTextFace`
- [ ] Images/blits: `CreateIconIndirect`, `LoadNamedImage`,
      `DrawImageInRect`, `BitBlt`, `StretchBlt`, `StretchBltFromMem`
- [ ] Paint/DC APIs: `BeginPaint`, `EndPaint`, `GetWindowDC`, `ReleaseDC`
- [ ] GL/view APIs: `SWELL_SetViewGL`, `SWELL_GetViewGL`,
      `SWELL_SetGLContextToView`, `SWELL_GetScaling256`,
      `SWELL_ExtendedAPI`, `SetOpaque`, `SetAllowNoMiddleManRendering`

## Clipboard, Threads, Processes, Drag/Drop, Monitors

- [ ] Clipboard: `OpenClipboard`, `CloseClipboard`,
      `GetClipboardData`, `EmptyClipboard`, `SetClipboardData`,
      `RegisterClipboardFormat`, `EnumClipboardFormats`
- [x] Global memory: `GlobalAlloc`, `GlobalLock`, `GlobalSize`,
      `GlobalUnlock`, `GlobalFree`
- [ ] Threads/events/handles: `CreateThread`, `CreateEvent`,
      `CreateEventAsSocket`, `GetCurrentThreadId`, `WaitForSingleObject`,
      `WaitForAnySocketObject`, `CloseHandle`, `SetThreadPriority`,
      `SetEvent`, `ResetEvent`
- [ ] Processes: `SWELL_CreateProcessFromPID`,
      `SWELL_CreateProcess`, `SWELL_GetProcessExitCode`
- [x] Libraries: `LoadLibraryGlobals`, `LoadLibrary`, `GetProcAddress`,
      `FreeLibrary`, `SWELL_GetBundle`
- [ ] Drag/drop: `DragQueryPoint`, `DragFinish`, `DragQueryFile`,
      `SWELL_InitiateDragDrop`, `SWELL_FinishDragDrop`,
      `SWELL_InitiateDragDropOfFileList`
- [ ] Viewport/monitor/X bridge/OS handles: `SWELL_GetViewPort`,
      `GetSystemMetrics`, `EnumDisplayMonitors`, `GetMonitorInfo`,
      `SWELL_CreateXBridgeWindow`, `SWELL_GetOSWindow`, `SWELL_GetOSEvent`
- [ ] Native dialogs: `SWELL_ChooseColor`, `SWELL_ChooseFont`

## Exact REAPER Lookup Set

REAPER requested 328 names, in this order:

```text
SWELL_set_app_main
lstrcpyn
MulDiv
Sleep
GetTickCount
GetFileTime
WritePrivateProfileString
GetPrivateProfileString
GetPrivateProfileInt
GetPrivateProfileStruct
WritePrivateProfileStruct
WritePrivateProfileSection
GetPrivateProfileSection
GetModuleFileName
SWELL_PtInRect
ShellExecute
BrowseForFiles
BrowseForSaveFile
BrowseForDirectory
BrowseFile_SetTemplate
GetDlgItem
ShowWindow
DestroyWindow
SWELL_GetGestureInfo
SWELL_HideApp
SetDlgItemText
SetDlgItemInt
GetDlgItemInt
GetDlgItemText
GetWindowTextLength
CheckDlgButton
IsDlgButtonChecked
EnableWindow
SetFocus
GetFocus
SetForegroundWindow
GetForegroundWindow
SetCapture
GetCapture
ReleaseCapture
IsChild
SetParent
GetWindow
EnumWindows
FindWindowEx
ClientToScreen
ScreenToClient
GetWindowRect
GetWindowContentViewRect
GetClientRect
WindowFromPoint
WinOffsetRect
WinSetRect
WinUnionRect
WinIntersectRect
SetWindowPos
SWELL_SetWindowLevel
InvalidateRect
UpdateWindow
GetWindowLong
SetWindowLong
ScrollWindow
EnumPropsEx
GetProp
SetProp
RemoveProp
IsWindowVisible
IsWindow
SetTimer
KillTimer
ListView_InsertColumn
ListView_DeleteColumn
ListView_SetColumn
ListView_GetColumn
ListView_GetColumnWidth
ListView_InsertItem
ListView_SetItemText
ListView_SetItem
ListView_GetNextItem
ListView_GetItem
ListView_GetItemState
ListView_DeleteItem
ListView_DeleteAllItems
ListView_GetSelectedCount
ListView_GetItemCount
ListView_GetSelectionMark
ListView_SetColumnWidth
ListView_SetItemState
ListView_RedrawItems
ListView_SetItemCount
ListView_EnsureVisible
ListView_SetImageList
ListView_SubItemHitTest
ListView_GetItemText
ListView_SortItems
ListView_Scroll
ListView_GetTopIndex
ListView_GetCountPerPage
ListView_SetColumnOrderArray
ListView_GetColumnOrderArray
ListView_GetHeader
Header_GetItemCount
Header_GetItem
Header_SetItem
ListView_GetItemRect
ListView_GetSubItemRect
ListView_HitTest
SWELL_GetListViewHeaderHeight
ImageList_CreateEx
ImageList_Remove
ImageList_ReplaceIcon
ImageList_Add
ImageList_Destroy
TabCtrl_GetItemCount
TabCtrl_DeleteItem
TabCtrl_InsertItem
TabCtrl_SetCurSel
TabCtrl_GetCurSel
TabCtrl_AdjustRect
TreeView_InsertItem
TreeView_Expand
TreeView_GetSelection
TreeView_DeleteItem
TreeView_DeleteAllItems
TreeView_SelectItem
TreeView_EnsureVisible
TreeView_GetItem
TreeView_SetItem
TreeView_HitTest
TreeView_SetIndent
TreeView_GetParent
TreeView_GetChild
TreeView_GetNextSibling
TreeView_GetRoot
TreeView_SetBkColor
TreeView_SetTextColor
ListView_SetBkColor
ListView_SetTextBkColor
ListView_SetTextColor
ListView_SetGridColor
ListView_SetSelColors
SWELL_ModalWindowStart
SWELL_ModalWindowRun
SWELL_ModalWindowEnd
SWELL_CloseWindow
CreatePopupMenu
CreatePopupMenuEx
DestroyMenu
GetSubMenu
GetMenuItemCount
GetMenuItemID
SetMenuItemModifier
SetMenuItemText
EnableMenuItem
DeleteMenu
CheckMenuItem
InsertMenuItem
SWELL_InsertMenu
GetMenuItemInfo
SetMenuItemInfo
DrawMenuBar
SWELL_LoadMenu
TrackPopupMenu
SWELL_SetMenuDestination
SWELL_DuplicateMenu
SetMenu
GetMenu
SWELL_GetDefaultWindowMenu
SWELL_SetDefaultWindowMenu
SWELL_GetCurrentMenu
SWELL_SetCurrentMenu
SWELL_DialogBox
SWELL_CreateDialog
DefWindowProc
EndDialog
SWELL_GetDefaultButtonID
SendMessage
SWELL_BroadcastMessage
PostMessage
SWELL_MessageQueue_Flush
SWELL_MessageQueue_Clear
SWELL_KeyToASCII
GetAsyncKeyState
GetCursorPos
GetMessagePos
SWELL_LoadCursor
SWELL_SetCursor
SWELL_EnableRightClickEmulate
SWELL_GetCursor
SWELL_GetLastSetCursor
SWELL_IsCursorVisible
SWELL_ShowCursor
SWELL_SetCursorPos
SWELL_GetViewPort
OpenClipboard
CloseClipboard
GetClipboardData
EmptyClipboard
SetClipboardData
RegisterClipboardFormat
EnumClipboardFormats
GlobalAlloc
GlobalLock
GlobalSize
GlobalUnlock
GlobalFree
CreateThread
CreateEvent
CreateEventAsSocket
GetCurrentThreadId
WaitForSingleObject
WaitForAnySocketObject
CloseHandle
SetThreadPriority
SetEvent
ResetEvent
SWELL_CreateProcessFromPID
SWELL_CreateProcess
SWELL_GetProcessExitCode
LoadLibraryGlobals
LoadLibrary
GetProcAddress
FreeLibrary
SWELL_GetBundle
SWELL_CreateMemContext
SWELL_DeleteGfxContext
SWELL_GetCtxGC
SWELL_GetCtxFrameBuffer
SWELL_PushClipRegion
SWELL_SetClipRegion
SWELL_PopClipRegion
CreateFontIndirect
CreatePen
CreateSolidBrush
CreatePenAlpha
CreateSolidBrushAlpha
SelectObject
GetStockObject
DeleteObject
SWELL_FillRect
Rectangle
Ellipse
SWELL_Polygon
MoveToEx
LineTo
SetPixel
PolyBezierTo
SWELL_DrawText
GetTextColor
SetBkMode
GetGlyphIndicesW
PolyPolyline
GetTextMetrics
GetTextFace
GetObject
CreateIconIndirect
LoadNamedImage
DrawImageInRect
BitBlt
StretchBlt
StretchBltFromMem
SWELL_GetScaling256
SWELL_ExtendedAPI
GetSysColor
SetOpaque
SetAllowNoMiddleManRendering
SWELL_SetViewGL
SWELL_GetViewGL
SWELL_SetGLContextToView
BeginPaint
EndPaint
GetWindowDC
ReleaseDC
SWELL_FillDialogBackground
SWELL_CloneGDIObject
GetSystemMetrics
DragQueryPoint
DragFinish
DragQueryFile
SWELL_InitiateDragDrop
SWELL_FinishDragDrop
SWELL_DrawFocusRect
SWELL_MakeSetCurParms
SWELL_MakeButton
SWELL_MakeEditField
SWELL_MakeLabel
SWELL_MakeControl
SWELL_MakeCombo
SWELL_MakeGroupBox
SWELL_MakeCheckBox
SWELL_MakeListBox
SWELL_Menu_AddMenuItem
SWELL_GenerateMenuFromList
SWELL_GenerateDialogFromList
_controlfp
SWELL_LoadCursorFromFile
SWELL_SetWindowWantRaiseAmt
SWELL_GetWindowWantRaiseAmt
GetTempPath
SWELL_initargs
SWELL_RunMessageLoop
SWELL_CreateXBridgeWindow
SWELL_GetOSWindow
SWELL_GetOSEvent
SWELL_GenerateGUID
EnumChildWindows
SWELL_IsGroupBox
SWELL_IsButton
SWELL_IsStaticText
SWELL_GetDesiredControlSize
AddFontResourceEx
SWELL_ChooseColor
SWELL_ChooseFont
IsWindowEnabled
GetClassName
SWELL_SetClassName
SWELL_DisableContextMenu
EnumDisplayMonitors
GetMonitorInfo
ListView_SetExtendedListViewStyleEx
SWELL_GetDefaultModalWindowMenu
SWELL_SetDefaultModalWindowMenu
SWELL_RegisterCustomControlCreator
SWELL_UnregisterCustomControlCreator
SWELL_InitiateDragDropOfFileList
SWELL_Internal_PostMessage_Init
SWELL_SetListViewFastClickMask
SWELL_Register_Cursor_Resource
```
