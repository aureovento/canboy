#pragma once
#include <cstdint>

class APU;
class Joypad;

class IO {
public:
	IO();
	~IO();

	enum class INT {
		VBlank,
		LCDStat,
		Timer,
		Serial,
		Joypad
	};

	enum class REG : uint16_t {
		JOYP = 0xFF00,
		LCDC = 0xFF40,
		STAT = 0xFF41,
		SCY = 0xFF42,
		SCX = 0xFF43,
		LY = 0xFF44,
		LYC = 0xFF45,
		WY = 0xFF4A,
		WX = 0xFF4B,
		BGP = 0xFF47,
		OBP0 = 0xFF48,
		OBP1 = 0xFF49
	};

	uint8_t read(uint16_t addr);
	uint8_t read(REG r) { return read(static_cast<uint16_t>(r)); }
	void write(uint16_t addr, uint8_t val);
	void reqINT(INT type);
	// timer functions
	uint8_t readTIMA();
	void setTIMA(uint8_t val);
	uint8_t readTMA();
	uint8_t readTAC();
	void setDIV(uint8_t val);
	bool divWrite = false;
	// ppu functions
	void setLY(uint8_t val);
	void setSTATMode(uint8_t mode);
	void setSTATFlag(bool match);

	void attachAPU(APU* a);
	void attachJoypad(Joypad* jp);
	void reset();

private:
	uint8_t IF = 0;
	uint8_t JOYP = 0;
	uint8_t DIV;
	uint8_t TIMA;
	uint8_t TMA;
	uint8_t TAC;
	uint8_t LCDC = 0;
	uint8_t STAT = 0;
	uint8_t LY = 0;
	uint8_t LYC = 0;
	uint8_t SCX = 0;
	uint8_t SCY = 0;
	uint8_t WX = 0;
	uint8_t WY = 0;
	uint8_t BGP = 0;
	uint8_t OBP0 = 0;
	uint8_t OBP1 = 0;

private:
	APU* apu = nullptr;
	Joypad* j = nullptr;
};