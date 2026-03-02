#include "lr35902.h"
#include "Bus.h"
#include <iostream>

using cpu = lr35902;

cpu::lr35902(Bus *b) : bus(b){
  instSet = new instructionSet(this);
  r8[0] = &regs.b;
  r8[1] = &regs.c;
  r8[2] = &regs.d;
  r8[3] = &regs.e;
  r8[4] = &regs.h;
  r8[5] = &regs.l;
  r8[6] = nullptr; 
  r8[7] = &regs.a;
    
  r16[0] = &regs.bc;
  r16[1] = &regs.de;
  r16[2] = &regs.hl;
  r16[3] = &regs.sp;
}
cpu::~lr35902() { delete instSet; }

uint8_t cpu::fetch() {
  fetched = read(regs.pc);
  if (haltBug) {
      haltBug = false;
  } else {
      regs.pc++;
  }
  return fetched;
}

uint8_t cpu::read(uint16_t a) { return bus->read(a); }

void cpu::write(uint16_t a, uint8_t d) { bus->write(a, d); }

void cpu::reset() {
  regs = Registers{};
  fetched = 0x00;
  opcode = 0x00;
  cycles = 0;
  HALT = false;
  STOP = false;
  IME = false;
  delayedIME = false;
}

void cpu::clock() {  
    if (bus->isDMAActive()) {
        bus->tickDMA();
        for (auto& pulse : clockCallback) pulse();
        return;
    }
    bool instrCompleted = false;
    if (STOP) {
        uint8_t IE = read(0xFFFF);
        uint8_t IF = read(0xFF0F);
        if (IE & IF) STOP = false;
        for (auto& pulse : clockCallback) pulse();
        return;
    }
    if (cycles > 0) {
        cycles--;
        if (cycles == 0 && !HALT) {
            instrCompleted = true;
        }
    }
    if (instrCompleted) {
        if (imePending) {
            if (imeSkip) {
                imeSkip = false;
            } else {
                IME = true;
                imePending = false;
            }
        }
        if (serviceINT()) {
            for (auto& pulse : clockCallback) pulse();
            return;
        }
    }
    if (cycles == 0) {
        if (!HALT) {
            opcode = fetch();
            exec(opcode);
        } else {
            cycles = 1;
        }
    }
    for (auto& pulse : clockCallback)
        pulse();
}

void cpu::exec(uint8_t opcode) {
  cycles = instSet->tstates[opcode];
  instSet->decode(opcode);
}

bool cpu::serviceINT() {
  uint8_t IE = read(0xFFFF);
  uint8_t IF = read(0xFF0F);
  if ((IE & IF) == 0) {
    return false;
  } // interrupt pending after this
  STOP = false;
  if (HALT) {
      HALT = false;
      if (!IME) {
          return false;
      }
  }
  if (!IME) return false;
  // service the interrupt
  IME = false;
  imePending = false;
  uint8_t pending = IE & IF;
  if (pending & 1) {
    // vblank
    uint8_t iflags = read(0xFF0F);
    iflags &= ~(1);
    write(0xFF0F, iflags);
    instSet->push(regs.pc);
    regs.pc = 0x0040;
    cycles = 20;
  } else if ((pending >> 1) & 1) {
    // lcd
    uint8_t iflags = read(0xFF0F);
    iflags &= ~(1 << 1);
    write(0xFF0F, iflags);
    instSet->push(regs.pc);
    regs.pc = 0x0048;
    cycles = 20;
  } else if ((pending >> 2) & 1) {
    // timer
    uint8_t iflags = read(0xFF0F);
    iflags &= ~(1 << 2);
    write(0xFF0F, iflags);
    instSet->push(regs.pc);
    regs.pc = 0x0050;
    cycles = 20;
  } else if ((pending >> 3) & 1) {
    // serial
    uint8_t iflags = read(0xFF0F);
    iflags &= ~(1 << 3);
    write(0xFF0F, iflags);
    instSet->push(regs.pc);
    regs.pc = 0x0058;
    cycles = 20;
  } else if ((pending >> 4) & 1) {
    // joypad
    uint8_t iflags = read(0xFF0F);
    iflags &= ~(1 << 4);
    write(0xFF0F, iflags);
    instSet->push(regs.pc);
    regs.pc = 0x0060;
    cycles = 20;
  } else {
    return false;
  }
  return true;
}

void cpu::addListener(std::function<void()> x) {
    clockCallback.push_back(x);
}