#pragma once
#include <functional>
#include <vector>
#include "instructions.h"
#include "registers.h"
#include <cstdint>

class Bus;

class lr35902 {
  friend class instructionSet;

public:
  explicit lr35902(Bus *b);
  ~lr35902();
  void connectBus(Bus *b) { bus = b; }
  void importInstructionSet(instructionSet* is) { instSet = is; }
  Registers regs;
  void clock();
  void reset();
  uint8_t fetch();
  uint8_t fetched = 0x00;
  uint16_t cycles = 0;
  uint8_t opcode = 0x00;
  bool serviceINT();
  void exec(uint8_t opcode);
  bool isHalted() const { return HALT; }
  bool checkIME() const { return IME; }
  bool isCGB();
  void tickPeripherals();
  void toggleDoubleSpeed();
  bool isDoubleSpeed() const { return doubleSpeed;}
  void addListener(std::function<void()> x);

  bool HALT = false;
  bool STOP = false;
  bool doubleSpeed = false;
  uint8_t speedCounter = 0;
private:
  Bus *bus = nullptr;
  instructionSet *instSet;
  uint8_t* r8[8] = { &regs.b, &regs.c, &regs.d, &regs.e, &regs.h, &regs.l, nullptr, &regs.a };
  uint16_t* r16[4] = { &regs.bc, &regs.de, &regs.hl, &regs.sp };
  
  bool haltBug = false;
  bool IME = false;
  bool imePending = false;
  bool imeSkip = false;
  bool delayedIME = false;
  std::vector<std::function<void()>> clockCallback;
  void write(uint16_t a, uint8_t d);
  uint8_t read(uint16_t a);
};
