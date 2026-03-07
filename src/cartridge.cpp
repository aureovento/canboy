#include "cartridge.h"
#include <iostream>

Cartridge::Cartridge(bool battery, const std::string& path)
	: battery(battery), savePath(path) {
	if (!battery) return;
	namespace fs = std::filesystem;
	fs::path rom(path);
	std::string gameName = rom.stem().string();
	fs::path saveDir = fs::path("save") / gameName;
	fs::create_directories(saveDir);
	savePath = (saveDir / (gameName + ".sav")).string();
}

void Cartridge::saveRAM(const std::vector<uint8_t>& RAM) {
	if (!battery) return;
	std::ofstream file(savePath, std::ios::binary);
	if (!file) return;

	file.write(reinterpret_cast<const char*>(RAM.data()), RAM.size());
}

void Cartridge::loadRAM(std::vector<uint8_t>& RAM) {
	if (!battery) return;
	std::ifstream file(savePath, std::ios::binary);
	if (!file) return;

	file.read(reinterpret_cast<char*>(RAM.data()), RAM.size());
}

void Cartridge::saveTimer() {
	if (!battery) return;
	if (!sramWrite) return;
	auto now = std::chrono::steady_clock::now();
	auto timePassed = std::chrono::duration_cast<std::chrono::seconds>(now - lastWrite).count();
	if (timePassed >= 1) {
		auto &sram = getRAM();
		saveRAM(sram);
		sramWrite = false;
	}
}

void Cartridge::forceSave() {
	if (!battery) return;
	auto& sram = getRAM();
	saveRAM(sram);
}

std::unique_ptr<Cartridge> Cartridge::loadFile(const std::string& fname) {
	std::ifstream file(fname, std::ios::binary);
	if (!file) return nullptr;

	file.seekg(0, std::ios::end);
	size_t size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<uint8_t> rom(size);
	file.read(reinterpret_cast<char*>(rom.data()), size);
	uint8_t bank = rom[0x147];
	bool battery = Cartridge::hasBattery(bank);
	switch (bank) {
	case 0x00:
		return std::make_unique<MBC0>(std::move(rom));
	case 0x01: 
	case 0x02:
	case 0x03:
		return std::make_unique<MBC1>(std::move(rom), battery, fname);
	case 0x05:
	case 0x06:
		return std::make_unique<MBC2>(std::move(rom), battery, fname);
	case 0x0F:
	case 0x10:
	case 0x11:
	case 0x12:
	case 0x13:
		return std::make_unique<MBC3>(std::move(rom), battery, fname);
	default:
		return nullptr;
	}
}

bool Cartridge::hasBattery(uint8_t type) {
	switch (type) {
	case 0x03:
	case 0x06:
	case 0x09:
	case 0x0D:
	case 0x0F:
	case 0x10:
	case 0x13:
	case 0x1B:
	case 0x1E:
	case 0x22:
	case 0xFF:
		return true;
	default:
		return false;
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
}

MBC1::MBC1(std::vector<uint8_t>&& rom, bool battery, const std::string& path)
	: ROM(std::move(rom)), Cartridge(battery, path) {
	romBanks = ROM.size() / 0x4000;
	uint8_t ramSizeCode = 0;
	if (ROM.size() > 0x149) ramSizeCode = ROM[0x149];
	size_t ramSizeBytes = 0;
	switch (ramSizeCode) {
	case 0x00: ramSizeBytes = 0; break;
	case 0x01: ramSizeBytes = 2 * 1024; break;
	case 0x02: ramSizeBytes = 8 * 1024; break;
	case 0x03: ramSizeBytes = 32 * 1024; break;
	case 0x04: ramSizeBytes = 128 * 1024; break;
	case 0x05: ramSizeBytes = 64 * 1024; break;
	}
	RAM.resize(ramSizeBytes);
	ramBanks = RAM.size() / 0x2000;
	loadRAM(RAM);
	enRAM = false;
	ROMBN = 1;
	RAMBN = 0;
	bMode = 0;
}

uint8_t MBC1::read(uint16_t addr) {
	if (addr < 0x4000) {
		uint8_t bank = 0;
		if (bMode == 0) {
			bank = 0;
		}
		else if (bMode == 1) {
			bank = (ROMBN & 0b11100000) >> 5;
		}
		if (romBanks == 0) return 0xFF;
		bank %= romBanks;
		return ROM[(bank * 0x4000) + addr];
	}
	else if (addr >= 0x4000 && addr < 0x8000) {
		uint8_t bank = 0;
		if (bMode == 0) {
			bank = ROMBN;
		}
		else if (bMode == 1) {
			bank = ROMBN & 0x1F;
		}
		if ((bank & 0x1F) == 0) {
			bank |= 1;
		}
		bank %= romBanks;
		return ROM[(bank * 0x4000) + (addr - 0x4000)];
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		uint8_t bank = 0;
		if (!enRAM) return 0xFF;
		else {
			if (bMode == 0) {
				bank = 0;
			}
			else {
				bank = RAMBN;
			}
		}
		if (ramBanks == 0) return 0xFF;
		bank %= ramBanks;
		return RAM[(bank * 0x2000) + (addr - 0xA000)];
	}
	else {
		return 0xFF;
	}
}

void MBC1::write(uint16_t addr, uint8_t val) {
	if(addr < 0x2000){
		enRAM = ((val & 0x0F) == 0x0A);
	}
	else if (addr < 0x4000) {
		ROMBN = (ROMBN & 0b11100000) | (val & 0b00011111);
		if ((ROMBN & 0x1F) == 0) {
			ROMBN |= 1;
		}
	}
	else if (addr < 0x6000) {
		if (bMode == 0) {
			ROMBN = (ROMBN & 0b00011111) | ((val & 0x03) << 5);
			if ((ROMBN & 0x1F) == 0) {
				ROMBN |= 1;
			}
		}
		else if (bMode == 1) {
			RAMBN = val & 0x03;
		}
	}
	else if (addr < 0x8000) {
		bMode = val & 0x01;
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (enRAM && (ramBanks > 0)) {
			uint8_t bank = (bMode == 0) ? 0 : RAMBN;
			bank %= ramBanks;
			RAM[(bank * 0x2000) + (addr - 0xA000)] = val;
			sramWrite = true;
			lastWrite = std::chrono::steady_clock::now();
		}
	}
}

MBC2::MBC2(std::vector<uint8_t>&& rom, bool battery, const std::string& path)
	: ROM(std::move(rom)), Cartridge(battery, path) {
	romBanks = ROM.size() / 0x4000;
	RAM.resize(512);
	loadRAM(RAM);
	ROMBN = 1;
	enRAM = false;
}
uint8_t MBC2::read(uint16_t addr) {
	if (addr < 0x4000) {
		return ROM[addr];
	}
	else if (addr >= 0x4000 && addr < 0x8000) {
		uint8_t bank = ROMBN;
		bank %= romBanks;
		return ROM[(bank * 0x4000) + (addr - 0x4000)];
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!enRAM) return 0xFF;
		return RAM[((addr - 0xA000) & 0x1FF)] | 0xF0;
	}
	else {
		return 0xFF;
	}
}

void MBC2::write(uint16_t addr, uint8_t val) {
	if (addr < 0x4000) {
		if (addr & 0x0100) {
			ROMBN = val & 0x0F;
			if (ROMBN == 0) ROMBN = 1;
		}
		else {
			enRAM = (val & 0x0F) == 0x0A;
		}
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!enRAM) return;
		RAM[(addr - 0xA000) & 0x01FF] = val & 0x0F;
		sramWrite = true;
		lastWrite = std::chrono::steady_clock::now();
	}
}

MBC3::MBC3(std::vector<uint8_t>&& rom, bool battery, const std::string& path) 
	: ROM(std::move(rom)), Cartridge(battery, path) {
	romBanks = ROM.size() / 0x4000;
	uint8_t ramSizeCode = 0;
	if (ROM.size() > 0x149) ramSizeCode = ROM[0x149];
	size_t ramSizeBytes = 0;
	switch (ramSizeCode) {
	case 0x00: ramSizeBytes = 0; break;
	case 0x01: ramSizeBytes = 2 * 1024; break;
	case 0x02: ramSizeBytes = 8 * 1024; break;
	case 0x03: ramSizeBytes = 32 * 1024; break;
	case 0x04: ramSizeBytes = 128 * 1024; break;
	case 0x05: ramSizeBytes = 64 * 1024; break;
	}
	RAM.resize(ramSizeBytes);
	ramBanks = RAM.size() / 0x2000;
	loadRAM(RAM);
	std::ifstream file(savePath, std::ios::binary);
	if (file) {
		file.seekg(RAM.size());
		file.read((char*)&RTCS, 1);
		file.read((char*)&RTCM, 1);
		file.read((char*)&RTCH, 1);
		file.read((char*)&RTCD, 2);
		file.read((char*)&rtcHalt, 1);
		file.read((char*)&rtcOF, 1);

		int64_t savedTime;
		file.read((char*)&savedTime, sizeof(savedTime));
		if (file) {
			auto now = std::chrono::system_clock::now();
			int64_t current = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
			int64_t elapsed = current - savedTime;

			if (elapsed > 0) {
				lastTick = std::chrono::steady_clock::now() - std::chrono::seconds(elapsed);
				tickRTC();
			}
		}
	}
	lastTick = std::chrono::steady_clock::now();
}

void MBC3::write(uint16_t addr, uint8_t val) {
	tickRTC();
	if (addr < 0x2000) {
		enRAM = ((val & 0x0F) == 0x0A);
	}
	else if (addr < 0x4000) {
		ROMBN = val & 0x7F;
		if (ROMBN == 0) ROMBN = 1;
	}
	else if (addr < 0x6000) {
		regSelect = val;
	}
	else if (addr < 0x8000) {
		if (lastLatch == 0 && val == 1) {
			latchS = RTCS;
			latchM = RTCM;
			latchH = RTCH;
			latchDL = RTCD & 0xFF;
			latchDH = ((RTCD >> 8) & 1) | (rtcHalt << 6) | (rtcOF << 7);
		}
		lastLatch = val;
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!enRAM) return;
		if (regSelect <= 3) {
			uint8_t bank = regSelect % ramBanks;
			RAM[(bank * 0x2000) + (addr - 0xA000)] = val;
			sramWrite = true;
			lastWrite = std::chrono::steady_clock::now();
		}
		else {
			switch (regSelect) {
			case 0x08:
				RTCS = val % 60;
				break;
			case 0x09:
				RTCM = val % 60;
				break;
			case 0x0A:
				RTCH = val % 24;
				break;
			case 0x0B:
				RTCD = (RTCD & 0x100) | val;
				break;
			case 0x0C:
				RTCD = (RTCD & 0xFF) | ((val & 1) << 8);
				rtcHalt = (val >> 6) & 1;
				rtcOF = (val >> 7) & 1;
				break;
			}
		}
	}
}

uint8_t MBC3::read(uint16_t addr) {
	tickRTC();
	if (addr < 0x4000) {
		return ROM[addr];
	}
	else if (addr < 0x8000) {
		uint8_t bank = ROMBN % romBanks;
		return ROM[(bank * 0x4000) + (addr - 0x4000)];
	}
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (!enRAM) return 0xFF;
		if (ramBanks == 0) return 0xFF;
		if (regSelect <= 3) {
			uint8_t bank = regSelect % ramBanks;
			return RAM[(bank * 0x2000) + (addr - 0xA000)];
		}
		else {
			switch (regSelect) {
			case 0x08: return latchS;
			case 0x09: return latchM; 
			case 0x0A: return latchH; 
			case 0x0B: return latchDL; 
			case 0x0C: return latchDH; 
			default: return 0xFF;
			}
		}
	}
	return 0xFF;
}

void MBC3::tickRTC() {
	if (rtcHalt) return;
	auto now = std::chrono::steady_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTick).count();  // duration cast truncates fractional seconds
	if (elapsed < 1) return;
	// advances by whole seconds elapsed so truncated remainder is preserved
	lastTick += std::chrono::seconds(elapsed); // makes up for time lost because of casting time_point to whole seconds 
	uint64_t totalS = (uint64_t)RTCD * 86400 + (uint64_t)RTCH * 3600 + (uint64_t)RTCM * 60 + RTCS;
	totalS += elapsed;
	uint64_t days = totalS / 86400;
	totalS %= 86400;
	uint8_t hours = totalS / 3600;
	totalS %= 3600;
	uint8_t minutes = totalS / 60;
	uint8_t seconds = totalS % 60;

	if (days > 511) {
		rtcOF = true;
		days %= 512;
	}

	RTCD = days;
	RTCH = hours;
	RTCM = minutes;
	RTCS = seconds;
}

void MBC3::saveRAM(const std::vector<uint8_t>& RAM) {
	if (!battery) return;
	std::ofstream file(savePath, std::ios::binary);
	if (!file) return;

	file.write(reinterpret_cast<const char*>(RAM.data()), RAM.size());

	file.write((char*)&RTCS, 1);
	file.write((char*)&RTCM, 1);
	file.write((char*)&RTCH, 1);
	file.write((char*)&RTCD, 2);
	file.write((char*)&rtcHalt, 1);
	file.write((char*)&rtcOF, 1);

	auto now = std::chrono::system_clock::now();
	int64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
	file.write((char*)&epoch, sizeof(epoch));
}