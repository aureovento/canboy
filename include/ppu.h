#pragma once
#include <cstdint>
#include <array>
#include <deque>
#include <vector>
#include "io.h"

class Bus;

static const uint32_t dmgPalette[4] = {
	0xFFF0F8D8,
	0xFFA8D080,
	0xFF507860,
	0xFF101820
};

class PPU {
public:
	PPU(IO& io, Bus *b) : io(io), bus(b){ io.setSTATMode(mode); }
private:
	IO& io;
	std::array<uint32_t, 160 * 144> framebuffer{};
	std::array<uint8_t, 0x40>bgPaletteRAM{};
	std::array<uint8_t, 0x40>objPaletteRAM{};
	uint8_t BGPI = 0;
	uint8_t OBPI = 0;
	uint16_t dotcount = 0;
	uint8_t ly = 0;
	uint8_t mode = 2;
	bool prevMatch = false;
	bool bit7Prev = true;
	bool bit7 = true;
	uint8_t xPixel = 0;
	enum class FState {
		getTile,
		getLow,
		getHigh,
		sleep,
		push
	};
	FState state;
	bool frameReady = false;
	uint8_t fdotcounter = 0;
	uint8_t tileAttr = 0;
	bool attrXFlip = false;
	bool attrYFlip = false;
	uint8_t attrBank = 0;
	uint8_t attrPalette = 0;
	uint8_t attrPriority = 0;
	uint8_t tileNo = 0;
	uint16_t tileBase = 0;
	uint8_t tileRow = 0;
	uint8_t tileLow = 0;
	uint8_t tileHigh = 0;
	uint8_t fetcherX = 0;
	uint8_t pixelSkip = 0;
	bool wActive = false;
	bool wUsed = false;
	uint8_t wLine = 0;
	struct BGPixel {
		uint8_t color;
		uint8_t palette;
		bool priority;
	};
	std::deque<BGPixel> bgFIFO;
	void enterMode3();
	void tickFetcher();
	struct Sprite {
		int16_t y;
		int16_t x;
		uint8_t tile;
		uint8_t attr;
	};
	std::vector<Sprite> sprites;
	void scanOAM();
	bool getSpriteShade(const BGPixel& bg, bool objEn, bool objSize, uint16_t& shade);
	uint32_t toRGB(uint16_t c);

public:
	Bus* bus = nullptr;
	const std::array<uint32_t, 160 * 144>& getFrameBuffer() const;
	bool isFrameReady();
	void clrFrameFlag();
	void tick();
	void reset();

	void writeBGPI(uint8_t v);
	void writeBGPD(uint8_t v);
	uint8_t readBGPI();
	uint8_t readBGPD();
	void writeOBPI(uint8_t v);
	void writeOBPD(uint8_t v);
	uint8_t readOBPI();
	uint8_t readOBPD();
	void resetBoot();
};