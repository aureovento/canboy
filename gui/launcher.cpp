#include "launcher.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

std::string Launcher::run() {
    loadConfig();
    loadRecent();
    std::sort(romFolders.begin(), romFolders.end());

    romFolders.erase(
        std::unique(romFolders.begin(), romFolders.end()),
        romFolders.end()
    );

    if (romFolders.empty())
    {
        std::cout << "No folders configured\n";
    }

    scanFolders();

    std::sort(roms.begin(), roms.end(), [](const RomEntry& a, const RomEntry& b) {
        return a.title < b.title;
    });

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cout << "SDL init failed: " << SDL_GetError() << "\n";
        return "";
    }

    if (!TTF_Init()) {
        std::cout << "TTF init failed\n";
        return "";
    }

    SDL_Window* window = SDL_CreateWindow(
        "Launcher",
        640,
        480,
        0
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    TTF_Font* fontHeader = TTF_OpenFont("assets/kimberley bl.ttf", 24);
    TTF_Font* fontItem = TTF_OpenFont("assets/kimberley bl.ttf", 18);

    if (!fontHeader || !fontItem) {
        std::cout << "Font load failed\n";
        return "";
    }

    int index = 0;
    bool running = true;

    SDL_Event event;

    int scroll = 0;
    const int lineHeight = 35;
    const int startY = 50;
    const int visibleRows = (480 - startY) / lineHeight;

    while (running) {
        int totalItems;
        int recentHeader = -1;
        int recentStart = -1;
        int recentEnd = -1;
        int scanRow = -1;
        int folderStart = -1;

        if (state == LauncherState::FolderList) {
            scanRow = 0;
            folderStart = 1;
            int folderEnd = folderStart + romFolders.size() - 1;
            recentHeader = recent.empty() ? -1 : folderEnd + 1;
            recentStart = recent.empty() ? -1 : recentHeader + 1;
            recentEnd = recent.empty() ? -1 : recentStart + recent.size() - 1;
            totalItems = recent.empty() ? folderStart + romFolders.size() : recentEnd + 1;
        }
        else {
            totalItems = roms.size() + 1;
        }
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                case SDLK_W:
                    if (index > 0) index--;
                    if (index == recentHeader) index--;
                    if (index < scroll) scroll = index;
                    break;
                case SDLK_S:
                    if (index < totalItems - 1) index++;
                    if (index == recentHeader) index++;
                    if (index >= scroll + visibleRows) scroll = index - visibleRows + 1;
                    break;
                case SDLK_RETURN:
                    if (state == LauncherState::FolderList) {
                        if (index >= recentStart && index <= recentEnd) {
                            int recentIndex = index - recentStart;
                            if (recentIndex >= 0 && recentIndex < recent.size()) {
                                std::string path = recent[recentIndex];
                                TTF_CloseFont(fontHeader);
                                TTF_CloseFont(fontItem);
                                SDL_DestroyRenderer(renderer);
                                SDL_DestroyWindow(window);
                                TTF_Quit();
                                SDL_Quit();
                                return path;
                            }
                        }
                        else if (index == scanRow) {
                            openBrowser();
                            loadConfig();
                        }
                        else if (index >= folderStart) {
                            currentFolder = index - folderStart;
                            roms.clear();
                            scan(romFolders[currentFolder]);
                            std::sort(roms.begin(), roms.end(), [](const RomEntry& a, const RomEntry& b) {
                                return a.title < b.title;
                            });
                            state = LauncherState::RomList;
                            index = 0;
                            scroll = 0;
                        }
                    }
                    else {
                        if (index == 0) {
                            state = LauncherState::FolderList;
                            index = 0;
                            scroll = 0;
                        }
                        else {   
                            std::string path = roms[index - 1].path;
                            recent.erase(std::remove(recent.begin(), recent.end(), path), recent.end());
                            recent.insert(recent.begin(), path);
                            if (recent.size() > 2) recent.resize(2);
                            saveRecent();

                            TTF_CloseFont(fontHeader);
                            TTF_CloseFont(fontItem);
                            SDL_DestroyRenderer(renderer);
                            SDL_DestroyWindow(window);
                            TTF_Quit();
                            SDL_Quit();
                            return path;
                        }
                    }
                    break;
                case SDLK_ESCAPE:
                    running = false;
                    break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        int y = 50;
        for (int i = scroll; i < totalItems && i < scroll + visibleRows; i++)
        {
            SDL_Color color = { 200,200,200,255 };
            if ((int)i == index) color = { 255,255,100,255 };
            std::string text;
            if (state == LauncherState::FolderList) {
                if (i == recentHeader) {
                    text = "Recents";
                }
                else if (i >= recentStart && i <= recentEnd) {
                    text = getRomTitle(recent[i - recentStart]);
                }
                else if (i == scanRow){
                    text = "Open Folder";
                }
                else {
                    int folderIndex = i - folderStart;

                    if (folderIndex >= 0 && folderIndex < romFolders.size()) {
                        fs::path p(romFolders[folderIndex]);
                        text = p.filename().string();
                    }
                    else {
                        continue;
                    }
                }
            }
            else {
                if (i == 0) {
                    std::string path = romFolders[currentFolder];
                    fs::path p(path);
                    std::vector<std::string> dirs;
                    for (auto& part : p) dirs.push_back(part.string());
                    if (dirs.size() > 3) {
                        path = dirs[0] + "\\...\\" +
                            dirs[dirs.size() - 2] + "\\" +
                            dirs[dirs.size() - 1];
                    }
                    text = "Folder: " + path;
                }
                else {
                    text = roms[i - 1].title;
                }
            }
            if (i == index) text = "> " + text;
            else  text = "  " + text;
            TTF_Font* currentFont = fontItem;
            if (state == LauncherState::FolderList) {
                if (i == recentHeader || i == scanRow) currentFont = fontHeader;
            }

            SDL_Surface* surface =
                TTF_RenderText_Blended(
                    currentFont,
                    text.c_str(),
                    text.length(),
                    color
                );

            SDL_Texture* texture =
                SDL_CreateTextureFromSurface(renderer, surface);

            SDL_FRect dst;

            dst.x = 80;
            dst.y = (float)(startY + (i - scroll) * lineHeight);
            dst.w = (float)surface->w;
            dst.h = (float)surface->h;

            if (state == LauncherState::FolderList) {
                if (i == folderStart) {
                    int w;
                    SDL_GetWindowSize(window, &w, NULL);
                    float y = startY + (i - scroll) * lineHeight - 5;
                    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
                    SDL_RenderLine(renderer, 80, y, w - 40, y);
                    SDL_RenderLine(renderer, 80, y + 1, w - 40, y + 1);
                    SDL_RenderLine(renderer, 80, y + 2, w - 40, y + 2);
                }
            }

            SDL_RenderTexture(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
            SDL_DestroySurface(surface);
            y += 35;
        }
        SDL_RenderPresent(renderer);
    }

    TTF_CloseFont(fontHeader);
    TTF_CloseFont(fontItem);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return "";
}

void Launcher::scanFolders() {
    roms.clear();
    for (const auto& folder : romFolders) scan(folder);
}

void Launcher::scan(const std::string& folder) {
    if (!fs::exists(folder)) return;
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        std::string ext = entry.path().extension().string();
        if (ext == ".gb" || ext == ".gbc") {
            RomEntry rom;
            rom.path = entry.path().string();
            rom.title = getRomTitle(rom.path);
            roms.push_back(rom);
        }
    }
}

void Launcher::loadConfig() {
    fs::create_directories("save/system");
    std::ifstream file("save/system/config.json");
    if (!file.is_open()) {
        saveConfig();
        return;
    }
    json config;
    try {
        file >> config;
    }
    catch (...) {
        std::cerr << "config parse error\n";
        return;
    }

    if (config.contains("rom_folders")) romFolders = config["rom_folders"].get<std::vector<std::string>>();
}

void Launcher::saveConfig() {
    json config;
    config["rom_folders"] = romFolders;
    std::ofstream file("save/system/config.json");
    file << config.dump(4);
}

std::string Launcher::getRomTitle(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return std::filesystem::path(path).stem().string();
    file.seekg(0x134);
    char raw[16];
    file.read(raw, 16);
    std::string title;
    for (int i = 0; i < 16; i++) {
        char c = raw[i];
        if (c == 0) break;
        if (c < 32 || c > 126) break;
        title += c;
    }
    if (title.empty()) return std::filesystem::path(path).stem().string();

    return title;
}

void Launcher::openBrowser() {
    nfdchar_t* path = nullptr;
    nfdresult_t result = NFD_PickFolder(nullptr, &path);
    if (result == NFD_OKAY) {
        std::string folder = path;
        free(path);
        if (std::find(romFolders.begin(), romFolders.end(), folder) == romFolders.end()) {
            romFolders.push_back(folder);
        }
        saveConfig();
    }
}

void Launcher::loadRecent() {
    std::ifstream file("save/system/recent.json");
    if (!file.is_open()) return;
    json j;
    file >> j;
    if (j.contains("recent"))  recent = j["recent"].get<std::vector<std::string>>();
}


void Launcher::saveRecent() {
    json j;
    j["recent"] = recent;
    std::ofstream file("save/system/recent.json");
    file << j.dump(4);
}