#pragma once
#include "lr35902.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <memory>

class Cartridge;
class IO;

class Bus {
public:
	Bus();
	~Bus();

private: 
	std::unique_ptr<Cartridge> cart;
	IO* io = nullptr;
	std::array<std::array<uint8_t, 0x2000>, 2> VRAM{}; // 8000
  std::array<uint8_t, 0x2000> WRAM{}; // C000
  std::array<uint8_t, 0x00A0> OAM{};  // FE00
  std::array<uint8_t, 0x007F> HRAM{}; // FF80
  uint8_t IE{};
  std::array<uint8_t, 0x100> dmgBoot{};
  std::array<uint8_t, 0x900> cgbBoot{};
  bool bootRomEnabled = true;
  bool dmaActive = false;
  uint16_t dmaSource = 0;
  uint16_t dmaTicks = 0;
  uint8_t dmaIndex = 0;
  uint8_t vramBank = 0;

public:
  void write(uint16_t addr, uint8_t data);
  uint8_t read(uint16_t addr);
  uint8_t rawRead(uint16_t addr);
  void rawWrite(uint16_t addr, uint8_t data);
  uint8_t readVRAM(uint8_t bank, uint16_t offset) { return VRAM[bank][offset]; }
  bool isCGB();
  bool isDMAActive();
  void resetIE() { IE = 0x00; }
  void reset();
  void startDMA(uint8_t val);
  void tickDMA();
  bool loadBootRom(const std::string& path);
  void enableBootRom() { bootRomEnabled = true; }
  void attachCart(std::unique_ptr<Cartridge> c);
  Cartridge* getCart() { return cart.get(); }
  void attachIO(IO* i);
};
