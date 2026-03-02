#include "Emu.h"
#include <iostream>
Emu::Emu(): io(), bus(), timer(io), cpu(&bus), ppu(io, &bus){
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
	int debugCount = 0;
	while (!ppu.isFrameReady()) {
		cpu.clock();
		if (cpu.regs.pc == 0x0100) {
			std::cout << "Jumped to cartridge entry\n";
		}
		static int counter = 0;
		if (counter++ % 50000 == 0) {
			std::cout << "PC=" << std::hex << cpu.regs.pc << "\n";
		}
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
