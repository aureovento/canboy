#pragma once
#include <cstdint>
#include <array>
#include <deque>
#include <vector>
#include "io.h"

class Bus;

class PPU {
public:
	PPU(IO& io, Bus *b) : io(io), bus(b){ io.setSTATMode(mode); }
private:
	IO& io;
	std::array<uint8_t, 160 * 144> framebuffer{};
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
	std::deque<uint8_t> bgFIFO;
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
	bool getSpriteShade(uint8_t color, bool objEn, bool objSize, uint8_t& shade);

public:
	Bus* bus = nullptr;
	const std::array<uint8_t, 160 * 144>& getFrameBuffer() const;
	bool isFrameReady();
	void clrFrameFlag();
	void tick();
};