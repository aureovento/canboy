#include "cartridge.h"

std::unique_ptr<Cartridge> Cartridge::loadFile(const std::string& fname) {
	std::ifstream file(fname, std::ios::binary);
	if (!file) return nullptr;

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> rom(size);
	file.read(reinterpret_cast<char*>(rom.data()), size);
	uint8_t bank = rom[0x147];

	switch (bank) {
	case 0x00:
		return std::make_unique<MBC0>(std::move(rom));
	case 0x01:
		return std::make_unique<MBC0>(std::move(rom));
	default:
		return nullptr;
	}
}


MBC0::MBC0(std::vector<uint8_t>&& rom) : ROM(std::move(rom)){
	RAM.resize(8 * 1024);
}

uint8_t MBC0::read(uint16_t addr) {
	if (addr < 0x8000) {
		if (addr < ROM.size()) {
			return ROM[addr - 0x0000];
		}
	}
	else {
		return RAM[addr - 0xA000];
	}
	return 0xFF;
}

void MBC0::write(uint16_t addr, uint8_t val) {
	if (addr >= 0xA000 && addr <= 0xBFFF) {
		RAM[addr - 0xA000] = val;
	}
	else {
		// later mbc
	}
}