#include "Bus.h"
#include "cartridge.h"
#include "io.h"
#include <cassert>

Bus::Bus() {}

Bus::~Bus() {}

void Bus::write(uint16_t addr, uint8_t data) {
    if (addr == 0xFF50) {
        if(data != 0) bootRomEnabled = false;
    }
    else if (addr >= 0x0000 && addr <= 0x7FFF) {
        assert(cart != nullptr);
        cart->write(addr, data);
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) { // vram
        uint8_t stat = io->read(IO::REG::STAT);
        uint8_t lcdc = io->read(IO::REG::LCDC);
        uint8_t mode = stat & 0x03;
        uint8_t ENlcd = lcdc & 0x80;
        if (ENlcd && mode == 3) return;
        VRAM[addr - 0x8000] = data;
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        cart->write(addr, data);
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) WRAM[addr - 0xC000] = data;
    else if (addr >= 0xE000 && addr <= 0xFDFF) WRAM[addr - 0xE000] = data;
    else if (addr >= 0xFE00 && addr <= 0xFE9F) { // oam
        uint8_t stat = io->read(IO::REG::STAT);
        uint8_t lcdc = io->read(IO::REG::LCDC);
        uint8_t mode = stat & 0x03;
        uint8_t ENlcd = lcdc & 0x80;
        if (ENlcd && (mode == 2 || mode == 3)) return;
        OAM[addr - 0xFE00] = data;
    }
    else if (addr >= 0xFEA0 && addr <= 0xFEFF); // not usable
    else if (addr == 0xFF46) startDMA(data);
    else if (addr >= 0xFF00 && addr <= 0xFF7F) io->write(addr, data);
    else if (addr >= 0xFF80 && addr <= 0xFFFE) HRAM[addr - 0xFF80] = data;
    else if (addr == 0xFFFF) IE = data;
    else;
}

uint8_t Bus::read(uint16_t addr) {
    if (addr < 0x100 && bootRomEnabled) {
        return bootRom[addr];
    }
    else if (addr >= 0x0000 && addr <= 0x7FFF) {
        assert(cart != nullptr);
        return cart->read(addr); // rom
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) {  //vram
        uint8_t stat = io->read(IO::REG::STAT);
        uint8_t lcdc = io->read(IO::REG::LCDC);
        uint8_t mode = stat & 0x03;
        uint8_t ENlcd = lcdc & 0x80;
        if (ENlcd && mode == 3) return 0xFF;
        return VRAM[addr - 0x8000];
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        return cart->read(addr); // ram
    } 
    else if (addr >= 0xC000 && addr <= 0xDFFF) return WRAM[addr - 0xC000]; // wram
    else if (addr >= 0xE000 && addr <= 0xFDFF) return WRAM[addr - 0xE000]; // echo ram
    else if (addr >= 0xFE00 && addr <= 0xFE9F) { //oam
        uint8_t stat = io->read(IO::REG::STAT);
        uint8_t lcdc = io->read(IO::REG::LCDC);
        uint8_t mode = stat & 0x03;
        uint8_t ENlcd = lcdc & 0x80;
        if (ENlcd && (mode == 2 || mode == 3)) return 0xFF;
        return OAM[addr - 0xFE00];
    }
    else if (addr >= 0xFEA0 && addr <= 0xFEFF) return 0xFF; // not usable
    else if (addr >= 0xFF00 && addr <= 0xFF7F) return io->read(addr); // io
    else if (addr >= 0xFF80 && addr <= 0xFFFE) return HRAM[addr - 0xFF80]; // hram
    else if (addr == 0xFFFF) return IE;
    else return 0xFF;
}

void Bus::attachCart(std::unique_ptr<Cartridge> c) {
    cart = std::move(c);
}

void Bus::attachIO(IO* i) {
    io = i;
}

void Bus::startDMA(uint8_t val) {
    dmaSource = val << 8;
    dmaTicks = 0;
    dmaIndex = 0;
    dmaActive = true;
}

bool Bus::isDMAActive() {
    return dmaActive;
}

uint8_t Bus::rawRead(uint16_t addr) {
    if (addr < 0x100 && bootRomEnabled) {
        return bootRom[addr];
    }
    else if (addr >= 0x0000 && addr <= 0x7FFF) {
        assert(cart != nullptr);
        return cart->read(addr); // rom
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) return VRAM[addr - 0x8000]; // vram
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        return cart->read(addr); // ram
    }
    else if (addr >= 0xC000 && addr <= 0xDFFF) return WRAM[addr - 0xC000]; // wram
    else if (addr >= 0xE000 && addr <= 0xFDFF) return WRAM[addr - 0xE000]; // echo ram
    else if (addr >= 0xFE00 && addr <= 0xFE9F) return OAM[addr - 0xFE00]; // oam
    else if (addr >= 0xFEA0 && addr <= 0xFEFF) return 0xFF; // not usable
    else if (addr >= 0xFF00 && addr <= 0xFF7F) return io->read(addr); // io
    else if (addr >= 0xFF80 && addr <= 0xFFFE) return HRAM[addr - 0xFF80]; // hram
    else if (addr == 0xFFFF) return IE;
    else return 0xFF;
}

void Bus::tickDMA() {
    dmaTicks++;
    if (dmaTicks % 4 == 0 && dmaIndex < 160) {
        OAM[dmaIndex] = rawRead(dmaSource + dmaIndex);
        dmaIndex++;
    }
    if (dmaTicks == 640) {
        dmaActive = false;
    }
}

bool Bus::loadBootRom(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    file.read(reinterpret_cast<char*>(bootRom.data()), 0x100);
    return file.gcount() == 0x100;
}