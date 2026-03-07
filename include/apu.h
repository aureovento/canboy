#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include "io.h"

struct SquareChannel {
	uint16_t frequency = 0;
	uint16_t timer = 0;

	uint8_t duty = 0;
	uint8_t dutyPos = 0;

	bool enabled = false;

	uint8_t length = 0;
	bool lengthEnable = false;

	uint8_t volume = 0;
	uint8_t envelopePeriod = 0;
	uint8_t envelopeTimer = 0;
	bool envelopeIncrease = false;
};

class APU {
public:
	APU(IO& io) : io(io) {}
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t val);
	void tick();
	void init();

private:
	IO& io;
	uint32_t counter = 0;
	uint8_t frameStep = 0;

	void frameSequencer();

private: //sdl3
	SDL_AudioDeviceID device = 0;
	uint32_t sampleCounter = 0;
	std::vector<int16_t> audioBuffer;
	SDL_AudioStream* stream = nullptr;

private: // apu
	uint8_t NR52 = 0;
	uint8_t NR21 = 0;
	uint8_t NR22 = 0;
	uint8_t NR23 = 0;
	uint8_t NR24 = 0;
	
	SquareChannel ch2;

	static const uint8_t dutyTable[4][8];
	void generateSample();
};