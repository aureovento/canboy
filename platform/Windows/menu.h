#pragma once
#include <Windows.h>

class Emu;
void setOverlay(Emu* emu);

#define FILE_OPEN   1001
#define EMU_EXIT   1002

static WNDPROC ogWndProc = nullptr;
void createMenu(HWND hwnd);
LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);