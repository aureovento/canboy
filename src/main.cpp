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

    while (gb.run()) {}

    gb.shutdown();

    return 0;   
}