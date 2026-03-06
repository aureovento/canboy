#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <chrono>

class Cartridge {
public:
	virtual uint8_t read(uint16_t addr) = 0;
	virtual void write(uint16_t addr, uint8_t val) = 0;
	virtual std::vector<uint8_t>& getRAM() = 0;
	Cartridge() = default;
	Cartridge(bool battery, const std::string& path);
	virtual ~Cartridge() = default;
	static std::unique_ptr<Cartridge> loadFile(const std::string& fname);
	void saveTimer();
	bool didSRAMchange() const { return sramWrite; }
	void forceSave();
	
	// save file stuff
protected:
	std::string savePath;
	static bool hasBattery(uint8_t type);
	bool battery = false;
	bool sramWrite = false;
	void saveRAM(const std::vector<uint8_t>& RAM);
	void loadRAM(std::vector<uint8_t>& RAM);
	std::chrono::steady_clock::time_point lastWrite;
};

class MBC0 : public Cartridge {
private:
	std::vector<uint8_t> ROM;
	std::vector<uint8_t> RAM{};
	
public:
	MBC0(std::vector<uint8_t>&& rom);
	
	uint8_t read(uint16_t addr) override;
	void write(uint16_t addr, uint8_t val) override;
	std::vector<uint8_t>& getRAM() override { return RAM; }
};

class MBC1 : public Cartridge {
public:
	MBC1(std::vector<uint8_t>&& rom, bool battery, const std::string& path);
	uint8_t read(uint16_t addr) override;
	void write(uint16_t addr, uint8_t val) override;
	std::vector<uint8_t>& getRAM() override { return RAM; }
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

class MBC2 : public Cartridge {
public:
	MBC2(std::vector<uint8_t>&& rom, bool battery, const std::string& path);
	uint8_t read(uint16_t addr) override;
	void write(uint16_t addr, uint8_t val) override;
	std::vector<uint8_t>& getRAM() override { return RAM; }
private:
	std::vector<uint8_t> ROM;
	std::vector<uint8_t> RAM{};
	bool enRAM = false;
	uint8_t ROMBN = 1;
	size_t romBanks;
};

class MBC3 : public Cartridge {
public:
	MBC3(std::vector<uint8_t>&& rom, bool battery, const std::string& path);
	uint8_t read(uint16_t addr) override;
	void write(uint16_t addr, uint8_t val) override;
	std::vector<uint8_t>& getRAM() override { return RAM; }
private:
	std::vector<uint8_t> ROM;
	std::vector<uint8_t> RAM{};
	bool enRAM = false;
	uint8_t ROMBN = 1;
	uint8_t RAMBN = 0;
	size_t romBanks;
	size_t ramBanks;

	uint8_t regSelect = 0;
	uint8_t RTCS = 0;
	uint8_t RTCM = 0;
	uint8_t RTCH = 0;
	uint16_t RTCD = 0;
	bool rtcHalt = false;
	bool rtcOF = false;
	uint8_t latchS = 0;
	uint8_t latchM = 0;
	uint8_t latchH = 0;
	uint8_t latchDL = 0;
	uint8_t latchDH = 0;
	uint8_t lastLatch = 0;
	std::chrono::steady_clock::time_point lastTick;
	void tickRTC();
};