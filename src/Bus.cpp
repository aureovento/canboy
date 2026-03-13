#include "Bus.h"
#include "cartridge.h"
#include "io.h"
#include <cassert>

Bus::Bus() {}

Bus::~Bus() {}

void Bus::write(uint16_t addr, uint8_t data) {
    if (addr == 0xFF50) {
        if (data != 0) bootRomEnabled = false;
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
        uint8_t bank = cart->isCGB() ? io->getVBK() : 0;
        VRAM[bank][addr - 0x8000] = data;
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        cart->write(addr, data);
    }
    else if (addr >= 0xC000 && addr <= 0xCFFF) WRAM[0][addr - 0xC000] = data;
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        WRAM[bank][addr - 0xD000] = data;
    }
    else if (addr >= 0xE000 && addr <= 0xEFFF) WRAM[0][addr - 0xE000] = data;
    else if (addr >= 0xF000 && addr <= 0xFDFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        WRAM[bank][addr - 0xF000] = data;
    }
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
    else if (addr == 0xFF51) hdmaSource = (data << 8) | (hdmaSource & 0x00FF);
    else if (addr == 0xFF52) hdmaSource = (hdmaSource & 0xFF00) | (data & 0xF0);
    else if (addr == 0xFF53) hdmaDest = ((data & 0x1F) << 8) | (hdmaDest & 0x00FF);
    else if (addr == 0xFF54) hdmaDest = (hdmaDest & 0xFF00) | (data & 0xF0);
    else if (addr == 0xFF55) startHDMA(data);
    else if (addr >= 0xFF00 && addr <= 0xFF7F) io->write(addr, data);
    else if (addr >= 0xFF80 && addr <= 0xFFFE) HRAM[addr - 0xFF80] = data;
    else if (addr == 0xFFFF) IE = data;
    else;
}

uint8_t Bus::read(uint16_t addr) {
    if (bootRomEnabled) {
        if (!cart->isCGB()) {
            if (addr < 0x100) return dmgBoot[addr];
        }
        else {
            if (addr < 0x100) return cgbBoot[addr];
            if (addr >= 0x200 && addr < 0x900) return cgbBoot[addr];
        }
    }
    if (addr >= 0x0000 && addr <= 0x7FFF) {
        assert(cart != nullptr);
        return cart->read(addr); // rom
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) {  //vram
        uint8_t stat = io->read(IO::REG::STAT);
        uint8_t lcdc = io->read(IO::REG::LCDC);
        uint8_t mode = stat & 0x03;
        uint8_t ENlcd = lcdc & 0x80;
        if (ENlcd && mode == 3) return 0xFF;
        uint8_t bank = cart->isCGB() ? io->getVBK() : 0;
        return VRAM[bank][addr - 0x8000];
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        return cart->read(addr); // ram
    }
    else if (addr >= 0xC000 && addr <= 0xCFFF) return WRAM[0][addr - 0xC000]; // wram
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        return WRAM[bank][addr - 0xD000]; // wram (banks)
    }
    else if (addr >= 0xE000 && addr <= 0xEFFF) return WRAM[0][addr - 0xE000]; // echo ram
    else if (addr >= 0xF000 && addr <= 0xFDFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        return WRAM[bank][addr - 0xF000]; // echo ram
    }
    else if (addr >= 0xFE00 && addr <= 0xFE9F) { //oam
        uint8_t stat = io->read(IO::REG::STAT);
        uint8_t lcdc = io->read(IO::REG::LCDC);
        uint8_t mode = stat & 0x03;
        uint8_t ENlcd = lcdc & 0x80;
        if (ENlcd && (mode == 2 || mode == 3)) return 0xFF;
        return OAM[addr - 0xFE00];
    }
    else if (addr >= 0xFEA0 && addr <= 0xFEFF) return 0xFF; // not usable
    else if (addr == 0xFF55) return (hdmaActive ? 0x80 : 0) | hdmaLength;
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
    if (bootRomEnabled) {
        if (!cart->isCGB()) {
            if (addr < 0x100) return dmgBoot[addr];
        }
        else {
            if (addr < 0x100) return cgbBoot[addr];
            if (addr >= 0x200 && addr < 0x900) return cgbBoot[addr];
        }
    }
    if (addr >= 0x0000 && addr <= 0x7FFF) {
        assert(cart != nullptr);
        return cart->read(addr); // rom
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) {
        uint8_t bank = cart->isCGB() ? io->getVBK() : 0;
        return VRAM[bank][addr - 0x8000]; // vram
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        return cart->read(addr); // ram
    }
    else if (addr >= 0xC000 && addr <= 0xCFFF) return WRAM[0][addr - 0xC000]; // wram
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        return WRAM[bank][addr - 0xD000]; // wram (banks)
    }
    else if (addr >= 0xE000 && addr <= 0xEFFF) return WRAM[0][addr - 0xE000]; // echo ram
    else if (addr >= 0xF000 && addr <= 0xFDFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        return WRAM[bank][addr - 0xF000]; // echo ram
    }
    else if (addr >= 0xFE00 && addr <= 0xFE9F) return OAM[addr - 0xFE00]; // oam
    else if (addr >= 0xFEA0 && addr <= 0xFEFF) return 0xFF; // not usable
    else if (addr >= 0xFF00 && addr <= 0xFF7F) return io->read(addr); // io
    else if (addr >= 0xFF80 && addr <= 0xFFFE) return HRAM[addr - 0xFF80]; // hram
    else if (addr == 0xFFFF) return IE;
    else return 0xFF;
}

void Bus::rawWrite(uint16_t addr, uint8_t data) {
    if (addr == 0xFF50) {
        if (data != 0) bootRomEnabled = false;
    }
    else if (addr >= 0x0000 && addr <= 0x7FFF) {
        assert(cart != nullptr);
        cart->write(addr, data);
    }
    else if (addr >= 0x8000 && addr <= 0x9FFF) { // vram
        uint8_t bank = cart->isCGB() ? io->getVBK() : 0;
        VRAM[bank][addr - 0x8000] = data;
    }
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        assert(cart != nullptr);
        cart->write(addr, data);
    }
    else if (addr >= 0xC000 && addr <= 0xCFFF) WRAM[0][addr - 0xC000] = data;
    else if (addr >= 0xD000 && addr <= 0xDFFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        WRAM[bank][addr - 0xD000] = data;
    }
    else if (addr >= 0xE000 && addr <= 0xEFFF) WRAM[0][addr - 0xE000] = data;
    else if (addr >= 0xF000 && addr <= 0xFDFF) {
        uint8_t bank = cart->isCGB() ? io->getSVBK() : 1;
        WRAM[bank][addr - 0xF000] = data;
    }
    else if (addr >= 0xFE00 && addr <= 0xFE9F) { // oam
        OAM[addr - 0xFE00] = data;
    }
    else if (addr >= 0xFEA0 && addr <= 0xFEFF); // not usable
    else if (addr == 0xFF46) startDMA(data);
    else if (addr >= 0xFF00 && addr <= 0xFF7F) io->write(addr, data);
    else if (addr >= 0xFF80 && addr <= 0xFFFE) HRAM[addr - 0xFF80] = data;
    else if (addr == 0xFFFF) IE = data;
    else;
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
    if (cart && cart->isCGB()) {
        file.read(reinterpret_cast<char*>(cgbBoot.data()), 0x900);
        return file.gcount() == 0x900;
    }
    file.read(reinterpret_cast<char*>(dmgBoot.data()), 0x100);
    return file.gcount() == 0x100;
}

bool Bus::isCGB() {
    return cart->isCGB();
}

void Bus::reset() {
    VRAM[0].fill(0x00);
    VRAM[1].fill(0x00);
    for (auto& bank : WRAM) bank.fill(0x00);
    OAM.fill(0x00);
    HRAM.fill(0x00);
    bootRomEnabled = true;
    IE = 0x00;
}

void Bus::startHDMA(uint8_t val) {
    hdmaLength = val & 0x7F;
    hdmaSource &= 0xFFF0;
    hdmaDest &= 0x1FF0;
    if (val & 0x80) {
        hdmaActive = true;
        hdmaHBlank = true;
    }
    else {
        hdmaHBlank = false;
        GDMA();
    }
}

void Bus::GDMA() {
    int blocks = hdmaLength + 1;
    for (int b = 0; b < blocks; b++) {
        for (int i = 0; i < 0x10; i++) {
            uint8_t data = rawRead(hdmaSource++);
            rawWrite(0x8000 | (hdmaDest & 0x1FFF), data);
            hdmaDest++;
        }
    }
    hdmaActive = false;
}

void Bus::HDMA() {
    if (!hdmaActive || !hdmaHBlank) return;
    for (int i = 0; i < 0x10; i++) {
        uint8_t data = rawRead(hdmaSource++);
        rawWrite(0x8000 | (hdmaDest & 0x1FFF), data);
        hdmaDest++;
    }
    if (hdmaLength == 0) hdmaActive = false;
    else hdmaLength--;
}