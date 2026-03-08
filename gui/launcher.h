#pragma once
#include <string>
#include <vector>
#include "nfd.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

enum class LauncherState {
    FolderList,
    RomList
};
struct RomEntry {
    std::string title;
    std::string path;
};

struct SDL_Renderer;
struct SDL_Window;

void drawSeparator(
    SDL_Renderer* renderer,
    SDL_Window* window,
    int startY,
    int lineHeight,
    int row,
    int scroll
);

class Launcher {
public:
    std::string run();

private:
    LauncherState state = LauncherState::FolderList;
    int currentFolder = -1;
    void scanFolders();
    void scan(const std::string& folder);
    void saveConfig();
    void loadConfig();
    void saveRecent();
    void loadRecent();
    std::string getRomTitle(const std::string& path);
    std::vector<RomEntry> roms;
    std::vector<std::string> romFolders;
    std::vector<std::string> recent;
    void openBrowser();
};