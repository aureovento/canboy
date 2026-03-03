#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

class Cartridge {
public:
	virtual uint8_t read(uint16_t addr) = 0;
	virtual void write(uint16_t addr, uint8_t val) = 0;
	virtual ~Cartridge() = default;

	static std::unique_ptr<Cartridge> loadFile(const std::string& fname);
};

class MBC0 : public Cartridge {
private:
	std::vector<uint8_t> ROM;
	std::vector<uint8_t> RAM{};
	
public:
	MBC0(std::vector<uint8_t>&& rom);
	
	uint8_t read(uint16_t addr) override;
	void write(uint16_t addr, uint8_t val) override;
};

class MBC1 : public Cartridge {
public:
	MBC1(std::vector<uint8_t>&& rom);
	uint8_t read(uint16_t addr) override;
	void write(uint16_t addr, uint8_t val) override;
private:
	std::vector<uint8_t> ROM;
	std::vector<uint8_t> RAM{};
	bool enRAM = false;
	uint8_t ROMBN = 1;
	uint8_t RAMBN = 0;
	uint8_t bMode = 0;
	size_t romBanks;
	size_t ramBanks;
};