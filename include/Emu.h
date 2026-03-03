#pragma once
#include "Bus.h"
#include "lr35902.h"
#include "cartridge.h"
#include "io.h"
#include "timer.h"
#include "ppu.h"
#include "renderer.h"
#include "joypad.h"

class Emu {
public:
	Emu();

	Bus bus;
	lr35902 cpu;
	IO io;
	Timer timer;
	PPU ppu;
	Renderer r;
	Joypad j;
	void run();
	bool init();
	bool loadCart(const std::string& path);
};