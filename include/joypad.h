#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <io.h>

enum KeyIdx {
	KEY_A = 0, KEY_B, KEY_SELECT, KEY_START,
	KEY_RIGHT, KEY_LEFT, KEY_UP, KEY_DOWN,
	KEY_COUNT
};

static const char* keyActionNames[KEY_COUNT] = {
	"A", "B", "Select", "Start", "Right", "Left", "Up", "Down"
};

static const char* keyConfigNames[KEY_COUNT] = {
	"a", "b", "select", "start", "right", "left", "up", "down"
};

struct KeyConfig {
	SDL_Scancode keys[KEY_COUNT] = {
		SDL_SCANCODE_F, SDL_SCANCODE_C, SDL_SCANCODE_Z, SDL_SCANCODE_X,
		SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_W, SDL_SCANCODE_S
	};
};

class Joypad {
public:
	Joypad(IO& io) : io(io) {};
	~Joypad() {};
	uint8_t getJOYP();
	void setSelect(uint8_t bits);
	void poll();
	void reset();
	void loadConfig();
	void saveConfig();
	KeyConfig cfg;

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