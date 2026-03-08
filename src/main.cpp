#include "Emu.h"
#include "launcher.h"

int main() {

    Launcher launcher;

    while (true) {
        std::string romPath = launcher.run();

        if (romPath.empty()) return 0;

        Emu gb;
        if (!gb.init(romPath)) return -1;

        while (gb.run()) {}
    }

    return 0;
}