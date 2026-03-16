#pragma once
#include <Windows.h>
#include <string>

class Emu;
void setOverlay(Emu* emu);

#define FILE_OPEN   1001
#define EMU_EXIT   1002
#define EMU_PAUSE   1003
#define RECENT_BASE    2001
#define RECENT_COUNT 5

static WNDPROC ogWndProc = nullptr;
void createMenu(HWND hwnd);
void addRecentROM(const std::string& path);
LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);