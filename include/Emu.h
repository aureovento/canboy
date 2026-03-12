#pragma once
#include "Bus.h"
#include "lr35902.h"
#include "cartridge.h"
#include "io.h"
#include "timer.h"
#include "ppu.h"
#include "apu.h"
#include "renderer.h"
#include "joypad.h"
#include "menu.h"

class Emu {
public:
	Emu();

	Bus bus;
	lr35902 cpu;
	IO io;
	Timer timer;
	PPU ppu;
	APU apu;
	Renderer r;
	Joypad j;
	bool run();
	bool init();
	bool loadCart(const std::string& path);
	bool romLoaded = false;
	void reset();
	void skipbootDMG();
	void skipbootCGB();
	void shutdown();
};