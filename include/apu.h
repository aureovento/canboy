#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include "io.h"

const uint8_t dutyTable[4][8] =
{
	{0,0,0,0,0,0,0,1}, // 12.5%
	{1,0,0,0,0,0,0,1}, // 25%
	{1,0,0,0,0,1,1,1}, // 50%
	{0,1,1,1,1,1,1,0}  // 75%
};


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
	int sample(const uint8_t dutyTable[4][8]) const {
		if (!enabled) return 0;
		int dacInput = dutyTable[duty][dutyPos] ? volume : 0;
		return (dacInput - 8) * 200;
	}
};

struct WaveChannel {
	bool enabled = false;
	bool dacEnabled = false;
	uint16_t frequency = 0;
	uint16_t timer = 0;
	uint8_t length = 0;
	bool lengthEnable = false;
	uint8_t volumeShift = 0;
	uint8_t position = 0;
};

struct NoiseChannel {
	bool enabled = false;
	uint16_t lfsr = 0x7FFF;
	uint16_t timer = 0;
	uint8_t length = 0;
	bool lengthEnable = false;
	uint8_t volume = 0;
	bool envelopeIncrease = false;
	uint8_t envelopePeriod = 0;
	uint8_t envelopeTimer = 0;
	uint8_t clockShift = 0;
	uint8_t divisorCode = 0;
	bool widthMode = false;
};

class APU {
public:
	APU(IO& io) : io(io) {}
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t val);
	void tick();
	void init();
	void reset();
	void setFF(bool ff);

private:
	IO& io;
	uint32_t counter = 0;
	uint8_t frameStep = 0;
	uint32_t frameCounter = 0;
	void frameSequencer();
	bool fastForward = false;

private: //sdl3
	SDL_AudioDeviceID device = 0;
	uint32_t sampleCounter = 0;
	double sampleAccumulator = 0.0;
	double sampleTimer = 0.0;
	const double samplePeriod = 4194304.0 / 48000.0;
	std::vector<int16_t> audioBuffer;
	SDL_AudioStream* stream = nullptr;

private: // apu
	uint8_t NR50 = 0;
	uint8_t NR51 = 0;
	uint8_t NR52 = 0;
	uint8_t NR10 = 0;
	uint8_t NR11 = 0;
	uint8_t NR12 = 0;
	uint8_t NR13 = 0;
	uint8_t NR14 = 0;
	uint8_t NR21 = 0;
	uint8_t NR22 = 0;
	uint8_t NR23 = 0;
	uint8_t NR24 = 0;
	uint8_t NR30 = 0;
	uint8_t NR31 = 0;
	uint8_t NR32 = 0;
	uint8_t NR33 = 0;
	uint8_t NR34 = 0;
	uint8_t NR41 = 0;
	uint8_t NR42 = 0;
	uint8_t NR43 = 0;
	uint8_t NR44 = 0;
	
	SquareChannel ch1;
	// sweep  
	uint8_t ch1SweepPeriod = 0;
	uint8_t ch1SweepTimer = 0;
	uint8_t ch1SweepShift = 0;
	bool ch1SweepNegate = false;
	uint16_t ch1ShadowFreq = 0;
	void calcSweep();
	void updateSweep();

	SquareChannel ch2;

	WaveChannel ch3;
	uint8_t waveRAM[16]{};
	int sampleCH3();

	NoiseChannel ch4;
	int sampleCH4();

	void generateSample();
};