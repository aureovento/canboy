#include "Emu.h"
#include <iostream>

Emu::Emu(): io(), bus(), timer(io), cpu(&bus), ppu(io, &bus), apu(io), j(io){
	io.attachAPU(&apu);
	io.attachJoypad(&j);
	bus.attachIO(&io);
	cpu.addListener([this]() {timer.tick(); });
	cpu.addListener([this]() {ppu.tick(); });
	cpu.addListener([this]() {apu.tick(); });
}


bool Emu::init(const std::string& path) {
	if (!loadCart(path)) {
		std::cerr << "Failed to load ROM\n";
		return false;
	}

	if (!bus.loadBootRom("boot/bootROM.cb")) {
		std::cerr << "Failed to load boot ROM\n";
		return false;
	}
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	if (!r.init("CanBoy", 6)) {
		return false;
	}
	apu.init();
	return true;
}

bool Emu::run() {
	uint64_t frameStart = SDL_GetTicksNS();
	j.poll();
	while (!ppu.isFrameReady()) {
		cpu.clock();
	}
	auto cart = bus.getCart();
	if (cart && cart->didSRAMchange()) cart->saveTimer();
	ppu.clrFrameFlag();
	r.render(ppu.getFrameBuffer());
	if(!r.procEvents()) return false;
	uint64_t frameTime = SDL_GetTicksNS() - frameStart;
	const uint64_t frameTarget = 16740000;
	if (frameTime < frameTarget) SDL_DelayNS(frameTarget - frameTime);
	return true;
}

bool Emu::loadCart(const std::string& path) {
	auto cart = Cartridge::loadFile(path);
	if (!cart) {
		return false;
	}
	bus.attachCart(std::move(cart));
	return true;
}

void Emu::shutdown() {
	auto cart = bus.getCart();
	if (cart) {
		auto& sram = cart->getRAM();
		cart->forceSave();
	}
}
