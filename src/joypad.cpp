#include "joypad.h"
#include <fstream>
#include <vector>
#include <string>

uint8_t Joypad::getJOYP() {
	uint8_t low = 0x0F;
	if (!(selectBits & 0x20)) {
		low &= ~buttonState;
	}
	if (!(selectBits & 0x10)) {
		low &= ~dpadState;
	}
	uint8_t joyp = 0xC0 | selectBits | low;
	return joyp;
}

void Joypad::setSelect(uint8_t val) {
	selectBits = val & 0x30;
	uint8_t low = 0x0F;
	if (!(selectBits & 0x20)) {
		low &= ~buttonState;
	}
	if (!(selectBits & 0x10)) {
		low &= ~dpadState;
	}
	if ((prevLow & ~low) != 0) {
		io.reqINT(IO::INT::Joypad);
	}
	prevLow = low;
}

void Joypad::poll() {
	const bool* state = SDL_GetKeyboardState(NULL);

	buttonState = 0;
	dpadState = 0;

	if (state[cfg.keys[KEY_A]])      buttonState |= Buttons::a;
	if (state[cfg.keys[KEY_B]])      buttonState |= Buttons::b;
	if (state[cfg.keys[KEY_SELECT]]) buttonState |= Buttons::select;
	if (state[cfg.keys[KEY_START]])  buttonState |= Buttons::start;

	if (state[cfg.keys[KEY_RIGHT]])  dpadState |= Dpad::right;
	if (state[cfg.keys[KEY_LEFT]])   dpadState |= Dpad::left;
	if (state[cfg.keys[KEY_UP]])     dpadState |= Dpad::up;
	if (state[cfg.keys[KEY_DOWN]])   dpadState |= Dpad::down;

	uint8_t low = 0x0F;

	if (!(selectBits & 0x20)) low &= ~buttonState;
	if (!(selectBits & 0x10)) low &= ~dpadState;

	if ((prevLow & ~low) != 0) {
		io.reqINT(IO::INT::Joypad);
	}

	prevLow = low;
}

void Joypad::loadConfig() {
	std::ifstream f("config.cfg");
	if (!f) return;

	std::string line, section;
	while (std::getline(f, line)) {
		if (line.empty()) continue;
		if (line[0] == '[') {
			section = line.substr(1, line.find(']') - 1);
			continue;
		}
		if (section != "keys") continue;
		auto sep = line.find('=');
		if (sep == std::string::npos) continue;
		std::string key = line.substr(0, sep);
		int val = std::stoi(line.substr(sep + 1));
		for (int i = 0; i < KEY_COUNT; i++) {
			if (key == keyConfigNames[i]) {
				cfg.keys[i] = (SDL_Scancode)val;
				break;
			}
		}
	}
}

void Joypad::saveConfig() {
	std::vector<std::string> lines;
	{
		std::ifstream f("config.cfg");
		std::string line;
		while (std::getline(f, line)) lines.push_back(line);
	}
	bool inKeys = false;
	std::vector<std::string> kept;
	for (auto& l : lines) {
		if (!l.empty() && l[0] == '[') inKeys = (l.find("[keys]") != std::string::npos);
		if (!inKeys) kept.push_back(l);
	}
	kept.push_back("[keys]");
	for (int i = 0; i < KEY_COUNT; i++) {
		kept.push_back(std::string(keyConfigNames[i]) + "=" + std::to_string((int)cfg.keys[i]));
	}
	std::ofstream f("config.cfg");
	for (auto& l : kept) f << l << '\n';
}

void Joypad::reset() {
	buttonState = 0;
	dpadState = 0;
	selectBits = 0x30;
	prevLow = 0x0F;
}