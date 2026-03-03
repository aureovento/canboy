#include "Emu.h"
#include <iostream>

int main() {
    Emu gb;

    if (!gb.loadCart("../../../../roms/.gb")) {
        return -1;
    }

    if (!gb.init()) {
        return -1;
    }

    while (true) {
        gb.run();
    }

    return 0;   
}