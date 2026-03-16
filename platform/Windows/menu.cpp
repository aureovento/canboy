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
        AppendMenuA(RecentsMenu, MF_STRING | MF_GRAYED, 0, "no recent ROMs");
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
    HMENU settingsMenu = CreateMenu();
    RecentsMenu = CreatePopupMenu();

    AppendMenu(fileMenu, MF_STRING, FILE_OPEN, "Load ROM");
    AppendMenu(fileMenu, MF_POPUP, (UINT_PTR)RecentsMenu, "Recents");
    
    AppendMenu(emuMenu, MF_STRING, EMU_PAUSE, "Pause\tSpace");
    AppendMenu(emuMenu, MF_STRING, EMU_FASTFWD, "Fast Forward\tTab");
    AppendMenu(emuMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(emuMenu, MF_STRING, EMU_EXIT, "Exit");

    AppendMenu(settingsMenu, MF_STRING, S_CONFIG, "Configure");

    AppendMenu(menubar, MF_POPUP, (UINT_PTR)fileMenu, "File");
    AppendMenu(menubar, MF_POPUP, (UINT_PTR)emuMenu, "Emulation");
    AppendMenu(menubar, MF_POPUP, (UINT_PTR)settingsMenu, "Settings");

    SetMenu(hwnd, menubar);
    DrawMenuBar(hwnd);
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    ogWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)MenuWndProc);
}

#define CFG_LISTEN_TIMER 42
static int  listeningIdx = -1;
static HWND hDlg = nullptr;
static HWND hKeyLabels[KEY_COUNT] = {};
static HWND hKeyButtons[KEY_COUNT] = {};

static void updateKeyLabels() {
    for (int i = 0; i < KEY_COUNT; i++) SetWindowTextA(hKeyLabels[i], SDL_GetScancodeName(emu->j.cfg.keys[i]));
}

static void stopListening() {
    if (listeningIdx < 0) return;
    listeningIdx = -1;
    for (int i = 0; i < KEY_COUNT; i++) {
        EnableWindow(hKeyButtons[i], TRUE);
        SetWindowTextA(hKeyButtons[i], "Change");
    }
}

static void startListening(int idx) {
    stopListening();
    listeningIdx = idx;
    for (int i = 0; i < KEY_COUNT; i++)
        EnableWindow(hKeyButtons[i], FALSE);
    SetWindowTextA(hKeyButtons[idx], "Press key");
    SetForegroundWindow(hDlg);
    SetFocus(hDlg);
    SetTimer(hDlg, CFG_LISTEN_TIMER, 50, NULL);
}

LRESULT CALLBACK ConfigDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE: {
        hDlg = hwnd;
        const int rowH = 34, startY = 15;
        const int labelX = 20, keyX = 130, btnX = 240;
        for (int i = 0; i < KEY_COUNT; i++) {
            int y = startY + i * rowH;
            CreateWindowA("STATIC", keyActionNames[i],
                WS_CHILD | WS_VISIBLE,
                labelX, y + 4, 100, 22, hwnd, NULL, NULL, NULL);
            hKeyLabels[i] = CreateWindowA("STATIC", "",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                keyX, y + 4, 90, 22, hwnd,
                (HMENU)(UINT_PTR)(200 + i), NULL, NULL);
            hKeyButtons[i] = CreateWindowA("BUTTON", "Change",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                btnX, y, 80, 26, hwnd,
                (HMENU)(UINT_PTR)(100 + i), NULL, NULL);
        }
        int closeY = startY + KEY_COUNT * rowH + 10;
        CreateWindowA("BUTTON", "Close",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            155, closeY, 80, 28, hwnd, (HMENU)300, NULL, NULL);
        updateKeyLabels();
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 300) { DestroyWindow(hwnd); break; }
        if (id >= 100 && id < 100 + KEY_COUNT)      startListening(id - 100);
        break;
    }

    case WM_KEYDOWN: {
        if (listeningIdx < 0) break;
        if (wParam == VK_ESCAPE) { stopListening(); return 0; }
        HWND sdlHwnd = (HWND)SDL_GetPointerProperty(
            SDL_GetWindowProperties(emu->r.getWindow()),
            SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        SendMessage(sdlHwnd, WM_KEYDOWN, wParam, lParam);
        SendMessage(sdlHwnd, WM_KEYUP, wParam, lParam);
        SDL_PumpEvents();
        SDL_Event e;
        if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_EVENT_KEY_DOWN, SDL_EVENT_KEY_DOWN) > 0) {
            emu->j.cfg.keys[listeningIdx] = e.key.scancode;
            emu->j.saveConfig();
            updateKeyLabels();
            stopListening();
        }
        return 0;
    }

    case WM_CHAR:
        if (listeningIdx >= 0) return 0;
        break;

    case WM_DESTROY:
        stopListening();
        hDlg = nullptr;
        break;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}


static void showConfigDialog(HWND parent) {
    if (hDlg) { SetForegroundWindow(hDlg); return; }

    static bool registered = false;
    if (!registered) {
        WNDCLASSA wc{};
        wc.lpfnWndProc = ConfigDlgProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "KeyConfig";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&wc);
        registered = true;
    }

    int dlgW = 350, dlgH = KEY_COUNT * 34 + 90;
    RECT pr; GetWindowRect(parent, &pr);
    int x = pr.left + ((pr.right - pr.left) - dlgW) / 2;
    int y = pr.top + ((pr.bottom - pr.top) - dlgH) / 2;

    CreateWindowExA(WS_EX_DLGMODALFRAME,
        "KeyConfig", "Configure",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH,
        parent, NULL, GetModuleHandle(NULL), NULL);
}

LRESULT CALLBACK MenuWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_INITMENUPOPUP: {                  
        if ((HMENU)wParam == RecentsMenu) rebuildRecentsMenu();             
        break;
    }
    case WM_ENTERSIZEMOVE:
    case WM_ENTERMENULOOP:
        SetTimer(hwnd, 1, 16, NULL);
        break;
    case WM_EXITSIZEMOVE:
    case WM_EXITMENULOOP:
        KillTimer(hwnd, 1);
        break;
    case WM_TIMER: {
        static bool inTick = false;
        if (!inTick && emu) {
            inTick = true;
            if (wParam == 1 && GetCapture() != NULL) {
                if (emu->romLoaded) emu->r.render(emu->ppu.getFrameBuffer());
                else emu->r.idle();
            }
            else {
                emu->skipFrameCap = true;
                emu->run();
                emu->skipFrameCap = false;
            }
            inTick = false;
        }
        break;
    }
    case WM_SIZE: {
        if (emu) {
            if (emu->romLoaded) emu->r.render(emu->ppu.getFrameBuffer());
            else emu->r.idle();
        }
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
        case EMU_FASTFWD:
            emu->fastForward = !emu->fastForward;
            emu->apu.setFF(emu->fastForward);
            CheckMenuItem(GetMenu(hwnd), EMU_FASTFWD,
                emu->fastForward ? MF_CHECKED : MF_UNCHECKED);
            break;
        case S_CONFIG:
            showConfigDialog(hwnd);
            break;
        }
        break;
    }
    }
    return CallWindowProc(ogWndProc, hwnd, msg, wParam, lParam);
}