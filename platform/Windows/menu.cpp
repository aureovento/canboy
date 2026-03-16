#include "menu.h"
#include "Emu.h"
#include <nfd.h>
#include <iostream>
#include <fstream>
#include <algorithm>

static Emu* emu = nullptr;

void setOverlay(Emu* e) {
    emu = e;
}

static HMENU RecentsMenu = nullptr;


static std::vector<std::string> loadRecents() {
    std::vector<std::string> list;
    std::ifstream f("recents.txt");
    std::string line;
    while (std::getline(f, line) && (int)list.size() < RECENT_COUNT) {
        if (!line.empty()) list.push_back(line);
    }
    return list;
}

static void saveRecents(const std::vector<std::string>& list) {
    std::ofstream f("recents.txt");
    for (auto& s : list) f << s << '\n';
}

void addRecentROM(const std::string& path) {
    auto list = loadRecents();
    list.erase(std::remove(list.begin(), list.end(), path), list.end());
    list.insert(list.begin(), path);
    if ((int)list.size() > RECENT_COUNT) list.resize(RECENT_COUNT);
    saveRecents(list);
}

static void rebuildRecentsMenu() {
    while (GetMenuItemCount(RecentsMenu) > 0)
        RemoveMenu(RecentsMenu, 0, MF_BYPOSITION);

    auto list = loadRecents();
    if (list.empty()) {
        AppendMenuA(RecentsMenu, MF_STRING | MF_GRAYED, 0, "(no recent ROMs)");
        return;
    }
    for (int i = 0; i < (int)list.size(); i++) {
        std::string label = list[i];
        size_t sep = label.find_last_of("\\/"); // to get only the file name
        if (sep != std::string::npos) label = label.substr(sep + 1);
        AppendMenuA(RecentsMenu, MF_STRING, RECENT_BASE + i, label.c_str());
    }
}


void createMenu(HWND hwnd)
{
    HMENU menubar = CreateMenu();
    HMENU fileMenu = CreateMenu();
    HMENU emuMenu = CreateMenu();
    RecentsMenu = CreatePopupMenu();

    AppendMenu(fileMenu, MF_STRING, FILE_OPEN, "Load ROM");
    AppendMenu(fileMenu, MF_POPUP, (UINT_PTR)RecentsMenu, "Recents");
    AppendMenu(emuMenu, MF_STRING, EMU_PAUSE, "Pause\tSpace");
    AppendMenu(emuMenu, MF_SEPARATOR, 0, NULL);
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
    case WM_INITMENUPOPUP: {                  
        if ((HMENU)wParam == RecentsMenu) rebuildRecentsMenu();             
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= RECENT_BASE && id < RECENT_BASE + RECENT_COUNT) {
            auto list = loadRecents();
            int idx = id - RECENT_BASE;
            if (idx < (int)list.size()) {
                std::string path = list[idx];
                if (emu->loadCart(path)) addRecentROM(path);         
            }
            break;
        }
        switch (LOWORD(wParam)) {
        case FILE_OPEN: {
            nfdchar_t* outPath = nullptr;
            if (NFD_OpenDialog(NULL, NULL, &outPath) == NFD_OKAY) {
                std::string path(outPath);
                if (emu->loadCart(path)) addRecentROM(path);
                free(outPath);
            }
            break;
        }
        case EMU_EXIT:
            emu->reset();
            emu->romLoaded = false;
            break;
        case EMU_PAUSE:
            emu->paused = !emu->paused;
            CheckMenuItem(GetMenu(hwnd), EMU_PAUSE,
                emu->paused ? MF_CHECKED : MF_UNCHECKED);
            break;
        }
        break;
    }
    }
    return CallWindowProc(ogWndProc, hwnd, msg, wParam, lParam);
}