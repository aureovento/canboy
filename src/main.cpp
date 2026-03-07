#include "Emu.h"

int main() {
    Emu gb;

    if (!gb.init()) {
        return -1;
    }

    while (gb.run()) {}

    gb.shutdown();

    return 0;   
}