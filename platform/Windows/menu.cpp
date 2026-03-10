#include "menu.h"
#include "Emu.h"
#include <nfd.h>

static Emu* emu = nullptr;

void setOverlay(Emu* e) {
    emu = e;
}

void createMenu(HWND hwnd)
{
    HMENU menubar = CreateMenu();
    HMENU fileMenu = CreateMenu();
    HMENU emuMenu = CreateMenu();

    AppendMenu(fileMenu, MF_STRING, FILE_OPEN, "Load ROM");
    AppendMenu(emuMenu, MF_STRING, EMU_EXIT, "Exit");
    AppendMenu(menubar, MF_POPUP, (UINT_PTR)fileMenu, "File");
    AppendMenu(menubar, MF_POPUP, (UINT_PTR)emuMenu, "Emulation");

    SetMenu(hwnd, menubar);
    DrawMenuBar(hwnd);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ogWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MenuWndProc);
}

LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case FILE_OPEN: {
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog(NULL, NULL, &outPath) == NFD_OKAY) {
                emu->loadCart(outPath);
                free(outPath);
            }
            break;
        }
        case EMU_EXIT:
            emu->reset();
            emu->romLoaded = false;
            break;
        }
        break;
    }
    }
    return CallWindowProc(ogWndProc, hwnd, msg, wParam, lParam);
}