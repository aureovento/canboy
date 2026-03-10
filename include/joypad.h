#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <io.h>

class Joypad {
public:
	Joypad(IO& io) : io(io) {};
	~Joypad() {};
	uint8_t getJOYP();
	void setSelect(uint8_t bits);
	void poll();
	void reset();

private:
	IO& io;
	enum Buttons {
		a = (1 << 0),
		b = (1 << 1),
		select = (1 << 2),
		start = (1 << 3)
	};
	enum Dpad {
		right = (1 << 0),
		left = (1 << 1),
		up = (1 << 2),
		down = (1 << 3)
	};
	uint8_t buttonState = 0;
	uint8_t dpadState = 0;
	uint8_t selectBits = 0x30;
	uint8_t prevLow = 0x0F;
};