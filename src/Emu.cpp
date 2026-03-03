#include "Emu.h"
#include <iostream>

Emu::Emu(): io(), bus(), timer(io), cpu(&bus), ppu(io, &bus), j(io){
	io.attachJoypad(&j);
	bus.attachIO(&io);
	cpu.addListener([this]() {timer.tick(); });
	cpu.addListener([this]() {ppu.tick(); });
}


bool Emu::init() {
	if (!bus.loadBootRom("../../../boot/dmg_boot.bin")) {
		std::cerr << "Failed to load boot ROM\n";
		return false;
	}
	if (!r.init("CanBoy", 4)) {
		return false;
	}

	return true;
}

void Emu::run() {
	j.poll();
	while (!ppu.isFrameReady()) {
		cpu.clock();
	}
	ppu.clrFrameFlag();
	r.render(ppu.getFrameBuffer());
	r.procEvents();
}

bool Emu::loadCart(const std::string& path) {
	auto cart = Cartridge::loadFile(path);
	if (!cart) {
		return false;
	}
	bus.attachCart(std::move(cart));
	return true;
}
