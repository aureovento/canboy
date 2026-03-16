#include "Emu.h"
#include <iostream>

Emu::Emu(): io(), bus(), timer(io), cpu(&bus), ppu(io, &bus), apu(io), j(io){
	io.attachAPU(&apu);
	io.attachPPU(&ppu);
	io.attachJoypad(&j);
	bus.attachCPU(&cpu);
	bus.attachIO(&io);
	cpu.addListener([this]() {timer.tick(); });
	cpu.addListener([this]() {ppu.tick(); });
	cpu.addListener([this]() {apu.tick(); });
}


bool Emu::init() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	if (!r.init("CanBoy", 6)) {
		return false;
	}
	romLoaded = false;
	#ifdef _WIN32
		HWND hwnd = (HWND)SDL_GetPointerProperty(
			SDL_GetWindowProperties(r.getWindow()),
			SDL_PROP_WINDOW_WIN32_HWND_POINTER,
			nullptr
		);
		createMenu(hwnd);
		setOverlay(this);
	#endif
	
	apu.init();
	return true;
}

bool Emu::run() {
	uint64_t frameStart = SDL_GetTicksNS();
	if (!r.procEvents()) return false;

	const bool* keys = SDL_GetKeyboardState(nullptr);
	static bool spacePrev = false;
	bool spaceNow = keys[SDL_SCANCODE_SPACE];
	if (spaceNow && !spacePrev) paused = !paused;
	spacePrev = spaceNow;
	static bool tabPrev = false;
	bool tabNow = keys[SDL_SCANCODE_TAB];
	if (tabNow && !tabPrev) {
		fastForward = !fastForward;
		apu.setFF(fastForward);
	}
	tabPrev = tabNow;

	if (!romLoaded) {
		r.idle();
		SDL_Delay(16);
		return true;
	}
	if (paused) { 
		r.render(ppu.getFrameBuffer());
		SDL_Delay(16);
		return true;
	}
	j.poll();

	if (fastForward) {
		static const int FF_FACTOR = 3;
		for (int f = 0; f < FF_FACTOR; f++) {
			while (!ppu.isFrameReady()) cpu.clock();
			auto cart = bus.getCart();
			if (cart && cart->didSRAMchange()) cart->saveTimer();
			ppu.clrFrameFlag();
		}
		r.render(ppu.getFrameBuffer());
		uint64_t frameTime = SDL_GetTicksNS() - frameStart;
		const uint64_t frameTarget = 16740000ULL;
		if (frameTime < frameTarget) SDL_DelayNS(frameTarget - frameTime);
		return true;
	}

	while (!ppu.isFrameReady()) cpu.clock();

	auto cart = bus.getCart();
	if (cart && cart->didSRAMchange()) cart->saveTimer();
	ppu.clrFrameFlag();
	r.render(ppu.getFrameBuffer());
	uint64_t frameTime = SDL_GetTicksNS() - frameStart;
	const uint64_t frameTarget = 16740000;
	if (frameTime < 16740000) SDL_DelayNS(frameTarget - frameTime);
	return true;
}

bool Emu::loadCart(const std::string& path) {
	auto cart = Cartridge::loadFile(path);
	if (!cart) {
		return false;
	}
	bool isCGB = cart->isCGB();
	bus.attachCart(std::move(cart));
	reset();
	bool bootLoaded = false;
	if (isCGB) bootLoaded = bus.loadBootRom("cgb_boot.bin");
	else bootLoaded = bus.loadBootRom("dmg_boot.bin");
	if (!bootLoaded) (isCGB ? skipbootCGB() : skipbootDMG());
	else bus.enableBootRom();
	romLoaded = true;
	return true;
}

void Emu::shutdown() {
	auto cart = bus.getCart();
	if (cart) {
		auto& sram = cart->getRAM();
		cart->forceSave();
	}
}

void Emu::reset() {
	bus.reset();
	bus.resetIE();
	io.reset();
	timer.reset();
	j.reset();
	apu.reset();
	ppu.reset();
	cpu.reset();
}

void Emu::skipbootDMG() {
	bus.reset();
	ppu.resetBoot();
	cpu.regs.a = 0x01;
	cpu.regs.f = 0xB0;
	cpu.regs.b = 0x00;
	cpu.regs.c = 0x13;
	cpu.regs.d = 0x00;
	cpu.regs.e = 0xD8;
	cpu.regs.h = 0x01;
	cpu.regs.l = 0x4D;
	cpu.regs.sp = 0xFFFE;
	cpu.regs.pc = 0x0100;

	bus.rawWrite(0xFF00, 0xCF);

	bus.rawWrite(0xFF01, 0x00);
	bus.rawWrite(0xFF02, 0x7E);
	//timer
	bus.rawWrite(0xFF04, 0x00);
	bus.rawWrite(0xFF05, 0x00);
	bus.rawWrite(0xFF06, 0x00);
	bus.rawWrite(0xFF07, 0xF8);

	bus.rawWrite(0xFF0F, 0xE1);

	// ch1
	bus.rawWrite(0xFF10, 0x80);
	bus.rawWrite(0xFF11, 0xBF);
	bus.rawWrite(0xFF12, 0xF3);
	bus.rawWrite(0xFF13, 0xFF);
	bus.rawWrite(0xFF14, 0xBF);
	// ch2
	bus.rawWrite(0xFF16, 0x3F);
	bus.rawWrite(0xFF17, 0x00);
	bus.rawWrite(0xFF18, 0xFF);
	bus.rawWrite(0xFF19, 0xBF);
	// ch3
	bus.rawWrite(0xFF1A, 0x7F);
	bus.rawWrite(0xFF1B, 0xFF);
	bus.rawWrite(0xFF1C, 0x9F);
	bus.rawWrite(0xFF1D, 0xFF);
	bus.rawWrite(0xFF1E, 0xBF);
	// ch4
	bus.rawWrite(0xFF20, 0xFF);
	bus.rawWrite(0xFF21, 0x00);
	bus.rawWrite(0xFF22, 0x00);
	bus.rawWrite(0xFF23, 0xBF);
	// sound control
	bus.rawWrite(0xFF24, 0x77);
	bus.rawWrite(0xFF25, 0xF3);
	bus.rawWrite(0xFF26, 0xF1);
	// LCD
	bus.rawWrite(0xFF40, 0x91);
	bus.rawWrite(0xFF41, 0x85);
	bus.rawWrite(0xFF42, 0x00);
	bus.rawWrite(0xFF43, 0x00);
	bus.rawWrite(0xFF44, 0x00);
	bus.rawWrite(0xFF45, 0x00);
	//dma
	bus.rawWrite(0xFF46, 0xFF);
	// palettes
	bus.rawWrite(0xFF47, 0xFC);
	bus.rawWrite(0xFF48, 0xFF);
	bus.rawWrite(0xFF49, 0xFF);
	// window
	bus.rawWrite(0xFF4A, 0x00);
	bus.rawWrite(0xFF4B, 0x00);

	bus.rawWrite(0xFF50, 0x01);

	// IE
	bus.rawWrite(0xFFFF, 0x00);
}

void Emu::skipbootCGB() {
	bus.reset();
	ppu.resetBoot();
	cpu.regs.a = 0x11;
	cpu.regs.f = 0x80;
	cpu.regs.b = 0x00;
	cpu.regs.c = 0x00;
	cpu.regs.d = 0xFF;
	cpu.regs.e = 0x56;
	cpu.regs.h = 0x00;
	cpu.regs.l = 0x0D;
	cpu.regs.sp = 0xFFFE;
	cpu.regs.pc = 0x0100;
	cpu.STOP = false;
	cpu.HALT = false;
	cpu.doubleSpeed = false;
	cpu.speedCounter = 0;
	bus.rawWrite(0xFF00, 0xCF);

	bus.rawWrite(0xFF01, 0x00);
	bus.rawWrite(0xFF02, 0x7E);
	//timer
	bus.rawWrite(0xFF04, 0x00);
	bus.rawWrite(0xFF05, 0x00);
	bus.rawWrite(0xFF06, 0x00);
	bus.rawWrite(0xFF07, 0xF8);

	// int
	bus.rawWrite(0xFF0F, 0xE1);
	bus.rawWrite(0xFFFF, 0x00);

	// ch1
	bus.rawWrite(0xFF10, 0x80);
	bus.rawWrite(0xFF11, 0xBF);
	bus.rawWrite(0xFF12, 0xF3);
	bus.rawWrite(0xFF13, 0xFF);
	bus.rawWrite(0xFF14, 0xBF);
	// ch2
	bus.rawWrite(0xFF16, 0x3F);
	bus.rawWrite(0xFF17, 0x00);
	bus.rawWrite(0xFF18, 0xFF);
	bus.rawWrite(0xFF19, 0xBF);
	// ch3
	bus.rawWrite(0xFF1A, 0x7F);
	bus.rawWrite(0xFF1B, 0xFF);
	bus.rawWrite(0xFF1C, 0x9F);
	bus.rawWrite(0xFF1D, 0xFF);
	bus.rawWrite(0xFF1E, 0xBF);
	// ch4
	bus.rawWrite(0xFF20, 0xFF);
	bus.rawWrite(0xFF21, 0x00);
	bus.rawWrite(0xFF22, 0x00);
	bus.rawWrite(0xFF23, 0xBF);
	// sound control
	bus.rawWrite(0xFF24, 0x77);
	bus.rawWrite(0xFF25, 0xF3);
	bus.rawWrite(0xFF26, 0xF1);
	// LCD
	bus.rawWrite(0xFF40, 0x91);
	bus.rawWrite(0xFF41, 0x85);
	bus.rawWrite(0xFF42, 0x00);
	bus.rawWrite(0xFF43, 0x00);
	bus.rawWrite(0xFF44, 0x00);
	bus.rawWrite(0xFF45, 0x00);
	//dma
	bus.rawWrite(0xFF46, 0xFF);
	// palettes
	bus.rawWrite(0xFF47, 0xFC);
	bus.rawWrite(0xFF48, 0xFF);
	bus.rawWrite(0xFF49, 0xFF);
	// window
	bus.rawWrite(0xFF4A, 0x00);
	bus.rawWrite(0xFF4B, 0x00);
	// CGB registers
	bus.rawWrite(0xFF4C, 0x80);
	bus.rawWrite(0xFF4D, 0x00); // KEY1
	bus.rawWrite(0xFF4F, 0x00); // VRAM bank
	bus.rawWrite(0xFF70, 0x01); // WRAM bank
	// palette index registers
	bus.rawWrite(0xFF68, 0x00);
	bus.rawWrite(0xFF69, 0x00);
	bus.rawWrite(0xFF6A, 0x00);
	bus.rawWrite(0xFF6B, 0x00);

	//hdma
	bus.rawWrite(0xFF51, 0x00);
	bus.rawWrite(0xFF52, 0x00);
	bus.rawWrite(0xFF53, 0x00);
	bus.rawWrite(0xFF54, 0x00);
	bus.rawWrite(0xFF55, 0xFF);

	bus.rawWrite(0xFF6C, 0xFE); // undocumented
	// disable bootrom
	bus.rawWrite(0xFF50, 0x01);
}