#pragma once
#include <cstdint>

class APU;
class PPU;
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
		OBP1 = 0xFF49,
		VBK = 0xFF4F,
		SVBK = 0xFF70,
		KEY1 = 0xFF4D,
		HDMA1 = 0xFF51,
		HDMA2 = 0xFF52,
		HDMA3 = 0xFF53,
		HDMA4 = 0xFF54,
		HDMA5 = 0xFF55
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

	void setKEY1(uint8_t val) { KEY1 = val; }
	bool isDoubleSpeed() const { return (KEY1 & 0x80) != 0; }
	//stop delay behaviour stuff
	void setStopStall(bool s, uint8_t mode) { stopStalling = s; stopPPUMode = mode; }
	uint8_t getPPUMode() const { return stopStalling ? stopPPUMode : (STAT & 0x03); }
	bool stopStalling = false;
	uint8_t stopPPUMode = 0;

	uint8_t getVBK() const { return VBK; }
	uint8_t getSVBK() const { return SVBK; }
	void attachAPU(APU* a);
	void attachPPU(PPU* p);
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
	uint8_t VBK = 0;
	uint8_t SVBK = 1;
	uint8_t KEY1 = 0;
	uint8_t HDMA1 = 0;
	uint8_t HDMA2 = 0;
	uint8_t HDMA3 = 0;
	uint8_t HDMA4 = 0;
	uint8_t HDMA5 = 0xFF;
	uint8_t SB = 0x00;
	uint8_t SC = 0x00;  

private:
	APU* apu = nullptr;
	PPU* ppu = nullptr;
	Joypad* j = nullptr;
};