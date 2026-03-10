#include "joypad.h"

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

    if (state[SDL_SCANCODE_F]) buttonState |= Buttons::a;
    if (state[SDL_SCANCODE_C]) buttonState |= Buttons::b;
	if (state[SDL_SCANCODE_Z]) buttonState |= Buttons::select;
    if (state[SDL_SCANCODE_X]) buttonState |= Buttons::start;

    if (state[SDL_SCANCODE_D]) dpadState |= Dpad::right;
    if (state[SDL_SCANCODE_A]) dpadState |= Dpad::left;
    if (state[SDL_SCANCODE_W]) dpadState |= Dpad::up;
    if (state[SDL_SCANCODE_S]) dpadState |= Dpad::down;

    uint8_t low = 0x0F;

    if (!(selectBits & 0x20)) low &= ~buttonState;
    if (!(selectBits & 0x10)) low &= ~dpadState;

	if ((prevLow & ~low) != 0) {
		io.reqINT(IO::INT::Joypad);
	}

    prevLow = low;
}

void Joypad::reset() {
	buttonState = 0;
	dpadState = 0;
	selectBits = 0x30;
	prevLow = 0x0F;
}