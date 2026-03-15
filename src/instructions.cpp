#include "instructions.h"
#include "lr35902.h"
#include "bus.h"
#include "io.h"

instructionSet::instructionSet(lr35902* c) : cpu(c) { }

void instructionSet::decode(uint8_t opcode) {
    uint8_t x = opcode >> 6;
    uint8_t y = opcode >> 3 & 0x07;
    uint8_t z = opcode & 0x07;
    // clang-format off
    // empty blocks are either illegal opcodes or jumps not taken
    switch (x) {
    case 0: {
        switch (z) {
        case 0: {
            switch (y) {
            case 0: // 0x00 nop
                break;
            case 1: { // 0x08 LD [a16], sp
                uint16_t a16 = cpu->fetch();
                a16 |= cpu->fetch() << 8;
                cpu->write(a16, cpu->regs.sp & 0xFF);
                cpu->write(a16 + 1, cpu->regs.sp >> 8);
                break;
            }
            case 2: // 0x10 STOP
                if (cpu->isCGB()) {
                    uint8_t key1 = cpu->read(0xFF4D);
                    if (key1 & 0x01) {
                        uint8_t currentMode = cpu->read(0xFF41) & 0x03;
                        cpu->toggleDoubleSpeed();
                        cpu->regs.pc++;
                        cpu->cycles = 8200;
                        cpu->bus->getIO()->setStopStall(true, currentMode);
                        break;
                    }
                }
                cpu->STOP = true;
                cpu->regs.pc++;
                cpu->cycles = 0;
                break;
            case 3: { // 0x18 JR e8
                int8_t e8 = cpu->fetch();
                cpu->regs.pc += e8;
                break;
            }
            case 4: { // 0x20 JR NZ e8
                int8_t e8 = cpu->fetch();
                if (cpu->regs.checkFlag(cpu->regs.Z) == false) {
                    cpu->regs.pc += e8;
                    cpu->cycles += extraTstates[opcode];
                }
                break;
            }
            case 5: { // 0x28 JR Z, e8
                int8_t e8 = cpu->fetch();
                if (cpu->regs.checkFlag(cpu->regs.Z) == true) {
                    cpu->regs.pc += e8;
                    cpu->cycles += extraTstates[opcode];
                }
                break;
            }
            case 6: { // JR NC, e8
                int8_t e8 = cpu->fetch();
                if (cpu->regs.checkFlag(cpu->regs.C) == false) {
                    cpu->regs.pc += e8;
                    cpu->cycles += extraTstates[opcode];
                }
                break;
            }
            case 7: { // JR C, e8
                int8_t e8 = cpu->fetch();
                if (cpu->regs.checkFlag(cpu->regs.C) == true) {
                    cpu->regs.pc += e8;
                    cpu->cycles += extraTstates[opcode];
                }
                break;
            }
            }
            break;
        }
        case 1: {
            uint8_t p = y >> 1;
            uint8_t q = y & 1;
            if (q == 0) { // 4 opcodes LD rp[p], nn
                uint16_t val = cpu->fetch();
                val |= cpu->fetch() << 8;
                load16(p, val);
            }
            else { // 4 opcodes ADD HL, r16
                addhl(p);
            }
            break;
        }
        case 2: {
            uint8_t p = y >> 1;
            uint8_t q = y & 1;
            if (q == 0) {
                storeA(p);
            }
            else {
                loadA(p);
            }
            break;
        }
        case 3: {
            uint8_t p = y >> 1;
            uint8_t q = y & 1;
            if (q == 0) { // inc r16
                (*cpu->r16[p])++;
            }
            else { // dec r16
                (*cpu->r16[p])--;
            }
            break;
        }
        case 4: {
            if (y != 6) {
                *cpu->r8[y] = inc8(*cpu->r8[y]);
            }
            else {
                cpu->write(cpu->regs.hl, inc8(cpu->read(cpu->regs.hl)));
            }
            break;
        }
        case 5: {
            if (y != 6) {
                *cpu->r8[y] = dec8(*cpu->r8[y]);
            }
            else {
                cpu->write(cpu->regs.hl, dec8(cpu->read(cpu->regs.hl)));
            }
            break;
        }
        case 6: { // ld r[y], n
            uint8_t val = cpu->fetch();
            if (y != 6) {
                *cpu->r8[y] = val;
            }
            else {
                cpu->write(cpu->regs.hl, val);
            }
            break;
        }
        case 7: {
            switch (y) {
            case 0: { // RLCA
                uint8_t bit = (cpu->regs.a >> 7) & 1;
                cpu->regs.a = ((cpu->regs.a << 1) | bit) & 0xFF;
                cpu->regs.setFlag(Registers::C, bit);
                cpu->regs.setFlag(Registers::Z, false);
                cpu->regs.setFlag(Registers::N, false);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            case 1: { // RRCA
                uint8_t bit = cpu->regs.a & 1;
                cpu->regs.a = ((cpu->regs.a >> 1) | (bit << 7)) & 0xFF;
                cpu->regs.setFlag(Registers::C, bit);
                cpu->regs.setFlag(Registers::Z, false);
                cpu->regs.setFlag(Registers::N, false);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            case 2: { // RLA
                uint8_t bit = (cpu->regs.a >> 7) & 1;
                cpu->regs.a = ((cpu->regs.a << 1) | cpu->regs.checkFlag(Registers::C)) & 0xFF;
                cpu->regs.setFlag(Registers::C, bit);
                cpu->regs.setFlag(Registers::Z, false);
                cpu->regs.setFlag(Registers::N, false);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            case 3: { // RRA
                uint8_t bit = cpu->regs.a & 1;
                cpu->regs.a = ((cpu->regs.a >> 1) | (cpu->regs.checkFlag(Registers::C)) << 7) & 0xFF;
                cpu->regs.setFlag(Registers::C, bit);
                cpu->regs.setFlag(Registers::Z, false);
                cpu->regs.setFlag(Registers::N, false);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            case 4: { // DAA
                uint8_t a = cpu->regs.a;
                if (!cpu->regs.checkFlag(Registers::N)) {
                    if (cpu->regs.checkFlag(Registers::H) || (a & 0x0F) > 0x09) {
                        a += 0x06;
                    }
                    if (cpu->regs.checkFlag(Registers::C) || a > 0x99) {
                        a += 0x60;
                        cpu->regs.setFlag(Registers::C, true);
                    }
                }
                else {
                    if (cpu->regs.checkFlag(Registers::H)) {
                        a -= 0x06;
                    }
                    if (cpu->regs.checkFlag(Registers::C)) {
                        a -= 0x60;
                    }
                }
                cpu->regs.a = a & 0xFF;
                cpu->regs.setFlag(Registers::Z, cpu->regs.a == 0);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            case 5: {// CPL
                cpu->regs.a = ~cpu->regs.a;
                cpu->regs.setFlag(Registers::N, true);
                cpu->regs.setFlag(Registers::H, true);
                break;
            }
            case 6: {// SCF
                cpu->regs.setFlag(Registers::C, true);
                cpu->regs.setFlag(Registers::N, false);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            case 7: { // CCF
                cpu->regs.setFlag(Registers::C, !cpu->regs.checkFlag(Registers::C));
                cpu->regs.setFlag(Registers::N, false);
                cpu->regs.setFlag(Registers::H, false);
                break;
            }
            }
            break;
        }
              break;
        }
        break;
    }
    case 1: { // LD r[y], r[z]    x = 1
        if (y == 6 && z == 6) {
            uint8_t IE = cpu->read(0xFFFF);
            uint8_t IF = cpu->read(0xFF0F);
            if (!cpu->IME && (IE & IF) != 0) {
                cpu->haltBug = true;
                cpu->HALT = false;
            }
            else {
                cpu->HALT = true;
            }
            cpu->cycles = 0;
        }
        else if (y == 6) {
            cpu->write(cpu->regs.hl, *cpu->r8[z]);
        }
        else if (z == 6) {
            *cpu->r8[y] = cpu->read(cpu->regs.hl);
        }
        else {
            *cpu->r8[y] = *cpu->r8[z];
        }
        break;
    }
    case 2: {
        uint8_t val = (z == 6) ? cpu->read(cpu->regs.hl) : *cpu->r8[z];

        switch (y) {
        case 0: add(val); break;
        case 1: adc(val); break;
        case 2: sub(val); break;
        case 3: sbc(val); break;
        case 4: _and(val); break;
        case 5: _xor(val); break;
        case 6: _or(val); break;
        case 7: cp(val); break;
        }
        break;
    }
    case 3: {
        switch (z) {
        case 0: {
            if (y <= 3) {
                if (y == 0 && !cpu->regs.checkFlag(Registers::Z)) {
                    uint16_t addr = cpu->read(cpu->regs.sp++);
                    addr |= cpu->read(cpu->regs.sp++) << 8;
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 1 && cpu->regs.checkFlag(Registers::Z)) {
                    uint16_t addr = cpu->read(cpu->regs.sp++);
                    addr |= cpu->read(cpu->regs.sp++) << 8;
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 2 && !cpu->regs.checkFlag(Registers::C)) {
                    uint16_t addr = cpu->read(cpu->regs.sp++);
                    addr |= cpu->read(cpu->regs.sp++) << 8;
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 3 && cpu->regs.checkFlag(Registers::C)) {
                    uint16_t addr = cpu->read(cpu->regs.sp++);
                    addr |= cpu->read(cpu->regs.sp++) << 8;
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else { /* ret fails do nothing */ }
            }
            else {
                if (y == 4) {
                    uint16_t addr = 0xFF00 + cpu->fetch();
                    cpu->write(addr, cpu->regs.a);
                }
                else if (y == 5) {
                    int8_t e8 = cpu->fetch();
                    cpu->regs.setFlag(Registers::Z, false);
                    cpu->regs.setFlag(Registers::N, false);
                    cpu->regs.setFlag(Registers::H, ((cpu->regs.sp & 0x0F) + (e8 & 0x0F)) > 0x0F);
                    cpu->regs.setFlag(Registers::C, ((cpu->regs.sp & 0x0FF) + (e8 & 0x0FF)) > 0xFF);
                    cpu->regs.sp += e8;
                }
                else if (y == 6) {
                    uint16_t addr = 0xFF00 + cpu->fetch();
                    cpu->regs.a = cpu->read(addr);
                }
                else {
                    int8_t e8 = cpu->fetch();
                    cpu->regs.setFlag(Registers::Z, false);
                    cpu->regs.setFlag(Registers::N, false);
                    cpu->regs.setFlag(Registers::H, ((cpu->regs.sp & 0x0F) + (e8 & 0x0F)) > 0x0F);
                    cpu->regs.setFlag(Registers::C, ((cpu->regs.sp & 0x0FF) + (e8 & 0x0FF)) > 0xFF);
                    cpu->regs.hl = cpu->regs.sp + e8;
                }
            }
            break;
        }
        case 1: {
            uint8_t p = y >> 1;
            uint8_t q = y & 1;
            if (q == 0) {
                uint16_t val = pop();
                if (p == 3) {
                    cpu->regs.a = val >> 8;
                    cpu->regs.f = val & 0xF0;

                }
                else {
                    *cpu->r16[p] = val;
                }
            }
            else {
                if (p == 0) { // RET
                    cpu->regs.pc = pop();
                }
                else if (p == 1) { // RETI
                    cpu->regs.pc = pop();
                    cpu->IME = true;
                }
                else if (p == 2) {
                    cpu->regs.pc = cpu->regs.hl;
                }
                else {
                    cpu->regs.sp = cpu->regs.hl;
                }
            }
            break;
        }
        case 2: {
            if (y <= 3) {
                uint16_t addr = cpu->fetch();
                addr |= cpu->fetch() << 8;
                if (y == 0 && !cpu->regs.checkFlag(Registers::Z)) {
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 1 && cpu->regs.checkFlag(Registers::Z)) {
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 2 && !cpu->regs.checkFlag(Registers::C)) {
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 3 && cpu->regs.checkFlag(Registers::C)) {
                    cpu->regs.pc = addr;
                    cpu->cycles += extraTstates[opcode];
                }
                else {}
            }
            else {
                if (y == 4) {
                    cpu->write((0xFF00 + cpu->regs.c), cpu->regs.a);
                }
                else if (y == 5) {
                    uint16_t addr = cpu->fetch();
                    addr |= cpu->fetch() << 8;
                    cpu->write(addr, cpu->regs.a);
                }
                else if (y == 6) {
                    cpu->regs.a = cpu->read((0xFF00 + cpu->regs.c));
                }
                else {
                    uint16_t addr = cpu->fetch();
                    addr |= cpu->fetch() << 8;
                    cpu->regs.a = cpu->read(addr);
                }
            }
            break;
        }
        case 3: {
            switch (y) {
            case 0: {
                uint16_t addr = cpu->fetch();
                addr |= cpu->fetch() << 8;
                cpu->regs.pc = addr;
                break;
            }
            case 1:
                // CB prefixed opcodes
                cpu->cycles = cbTstates[opcode];
                decodeCB(cpu->fetch());
                break;
            case 6:
                // DI
                cpu->IME = false;
                cpu->imePending = false;
                cpu->imeSkip = false;
                break;
            case 7:
                // EI
                cpu->imePending = true;
                cpu->imeSkip = true;
                break;
            default:
                break;
            }
            break;
        }
        case 4: {
            if (y <= 3) {
                uint16_t val = cpu->fetch();
                val |= cpu->fetch() << 8;
                if (y == 0 && !cpu->regs.checkFlag(Registers::Z)) {
                    push(cpu->regs.pc);
                    cpu->regs.pc = val;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 1 && cpu->regs.checkFlag(Registers::Z)) {
                    push(cpu->regs.pc);
                    cpu->regs.pc = val;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 2 && !cpu->regs.checkFlag(Registers::C)) {
                    push(cpu->regs.pc);
                    cpu->regs.pc = val;
                    cpu->cycles += extraTstates[opcode];
                }
                else if (y == 3 && cpu->regs.checkFlag(Registers::C)) {
                    push(cpu->regs.pc);
                    cpu->regs.pc = val;
                    cpu->cycles += extraTstates[opcode];
                }
                else {}
            }
            else {}
            break;
        }
        case 5: {
            uint8_t p = y >> 1;
            if ((y % 2 == 0) && (y != 6)) { // y = 0, 2, 4
                push(*cpu->r16[p]);
            }
            else if (y == 6) {
                push(cpu->regs.af);
            }
            else if (y == 1) {
                uint16_t val = cpu->fetch();
                val |= cpu->fetch() << 8;
                push(cpu->regs.pc);
                cpu->regs.pc = val;
                cpu->cycles += extraTstates[opcode];
            }
            else {}
            break;
        }
        case 6: { // alu[y], n8
            switch (y) {
            case 0: add(cpu->fetch()); break;
            case 1: adc(cpu->fetch()); break;
            case 2: sub(cpu->fetch()); break;
            case 3: sbc(cpu->fetch()); break;
            case 4: _and(cpu->fetch()); break;
            case 5: _xor(cpu->fetch()); break;
            case 6: _or(cpu->fetch()); break;
            case 7: cp(cpu->fetch()); break;
            }
            break;
        }
        case 7: {
            push(cpu->regs.pc);
            cpu->regs.pc = y * 8;
            break;
        }
              break;
        }
    }
    }
}

void instructionSet::decodeCB(uint8_t opcode){
  uint8_t x = opcode >> 6;
  uint8_t y = opcode >> 3 & 0x07;
  uint8_t z = opcode & 0x07;

  switch(x) {
    case 0: { // rotate, switch, swap
      switch(y) {
        case 0: { // RLC
          uint8_t bit, val;
          if(z != 6){
            bit = (*cpu->r8[z] >> 7) & 1;
            val = (*cpu->r8[z] << 1) | (bit & 1);
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = (val >> 7) & 1;
            val = (val << 1) | (bit & 1);
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::C, bit);
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
        case 1: { // RRC
          uint8_t bit, val;
          if(z != 6){
            bit = *cpu->r8[z] & 1;
            val = ((*cpu->r8[z] >> 1) | ((bit & 1) << 7));
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = val & 1;
            val = (val >> 1) | ((bit & 1) << 7);
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::C, bit);
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
        case 2: { // RL
          uint8_t bit, val;
          if(z != 6){
            bit = cpu->regs.checkFlag(Registers::C);  
            cpu->regs.setFlag(Registers::C, ((*cpu->r8[z] >> 7) & 1));
            val = ((*cpu->r8[z] << 1) | (bit & 1));
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = cpu->regs.checkFlag(Registers::C);
            cpu->regs.setFlag(Registers::C, ((val >> 7) & 1));
            val = (val << 1) | (bit & 1);
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
        case 3: { // RR
          uint8_t bit, val;
          if(z != 6){
            bit = cpu->regs.checkFlag(Registers::C);  
            cpu->regs.setFlag(Registers::C, (*cpu->r8[z] & 1));
            val = ((*cpu->r8[z] >> 1) | ((bit & 1) << 7));
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = cpu->regs.checkFlag(Registers::C);
            cpu->regs.setFlag(Registers::C, (val & 1));
            val = (val >> 1) | ((bit & 1) << 7);
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
        case 4: { // SLA 
          uint8_t bit, val;
          if(z != 6){
            bit = (*cpu->r8[z] >> 7) & 1;
            val = (*cpu->r8[z] << 1) | 0;
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = (val >> 7) & 1;
            val = (val << 1) | 0;
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::C, bit);
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
        case 5: { // SRA 
          uint8_t bit, val;
          if(z != 6){
            bit = *cpu->r8[z] & 1;
            val = ((*cpu->r8[z] >> 1) | (*cpu->r8[z] & 0b10000000));
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = val & 1;
            val = (val >> 1) | (val & 0b10000000);
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::C, bit);
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
        case 6: { // SWAP 
          uint8_t val;
          if (z != 6){
            *cpu->r8[z] = ((*cpu->r8[z] & 0x0F) << 4) | ((*cpu->r8[z] & 0xF0) >> 4);
            val = *cpu->r8[z];
          } else {
            val = cpu->read(cpu->regs.hl);
            val = ((val & 0x0F) << 4) | ((val & 0xF0) >> 4);
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          cpu->regs.setFlag(Registers::C, false);
          break;
        }
        case 7: { // SRL
          uint8_t bit, val;
          if(z != 6){
            bit = *cpu->r8[z] & 1;
            val = ((*cpu->r8[z] >> 1));
            *cpu->r8[z] = val;
          } else {
            val = cpu->read(cpu->regs.hl);
            bit = val & 1;
            val = val >> 1;
            cpu->write(cpu->regs.hl, val);
          }
          cpu->regs.setFlag(Registers::C, bit);
          cpu->regs.setFlag(Registers::Z, val == 0);
          cpu->regs.setFlag(Registers::N, false);
          cpu->regs.setFlag(Registers::H, false);
          break;
        }
      }
      break;
    }
    case 1: { // BIT
      uint8_t val = (z == 6) ? cpu->read(cpu->regs.hl) : *cpu->r8[z];
      cpu->regs.setFlag(Registers::Z, ((val >> y) & 1) == 0);
      cpu->regs.setFlag(Registers::N, false);
      cpu->regs.setFlag(Registers::H, true);
      break;
    }
    case 2: { // RES
      uint8_t val = (z == 6) ? cpu->read(cpu->regs.hl) : *cpu->r8[z];
      val &= ~(1 << y);
      if(z == 6){ cpu->write(cpu->regs.hl, val); }
      else { *cpu->r8[z] = val; }
      break;
    }
    case 3: { // SET
      uint8_t val = (z == 6) ? cpu->read(cpu->regs.hl) : *cpu->r8[z];
      val |= (1 << y);
      if(z == 6){ cpu->write(cpu->regs.hl, val); }
      else { *cpu->r8[z] = val; }
      break;
    }
  }
}

// helper functions for the instructions

void instructionSet::load16(uint8_t p, uint16_t val) { *cpu->r16[p] = val; }

void instructionSet::addhl(uint8_t p) {
  uint16_t hl = cpu->regs.hl;
  uint16_t rr = *cpu->r16[p];
  uint32_t res = hl + rr;
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, ((hl & 0x0FFF) + (rr & 0x0FFF)) > 0x0FFF);
  cpu->regs.setFlag(Registers::C, res > 0xFFFF);
  cpu->regs.hl = static_cast<uint16_t>(res);
}

uint16_t instructionSet::getHL() {
    return (cpu->regs.h << 8) | cpu->regs.l;
}

void instructionSet::setHL(uint16_t v) {
    cpu->regs.h = v >> 8;
    cpu->regs.l = v & 0xFF;
}

void instructionSet::loadA(uint8_t p) {
  if (p < 2) {
    cpu->regs.a = cpu->read(*cpu->r16[p]);
  } else if (p == 2) {
    cpu->regs.a = cpu->read(cpu->regs.hl++);
  } else {
    cpu->regs.a = cpu->read(cpu->regs.hl--);
  }
}

void instructionSet::storeA(uint8_t p) {
  if (p < 2) {
    cpu->write(*cpu->r16[p], cpu->regs.a);
  } else if (p == 2) {
    cpu->write(cpu->regs.hl++, cpu->regs.a);
  } else {
    cpu->write(cpu->regs.hl--, cpu->regs.a);
  }
}

uint8_t instructionSet::inc8(uint8_t val) {
  uint8_t res = val + 1;
  cpu->regs.setFlag(Registers::Z, res == 0);
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, (val & 0x0F) == 0x0F);
  return res;
}

uint8_t instructionSet::dec8(uint8_t val) {
  uint8_t res = val - 1;
  cpu->regs.setFlag(Registers::Z, res == 0);
  cpu->regs.setFlag(Registers::N, true);
  cpu->regs.setFlag(Registers::H, (val & 0x0F) == 0x00);
  return res;
}

void instructionSet::add(uint8_t val){
  uint8_t a = cpu->regs.a;
  uint16_t res = cpu->regs.a + val;
  cpu->regs.setFlag(Registers::Z, (res & 0xFF) == 0);
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, ((a & 0x0F) + (val & 0x0F)) > 0x0F);
  cpu->regs.setFlag(Registers::C, res > 0xFF);
  cpu->regs.a = static_cast<uint8_t>(res);
}

void instructionSet::adc(uint8_t val){
  uint8_t a = cpu->regs.a;
  uint8_t c = cpu->regs.checkFlag(Registers::C);
  uint16_t res = cpu->regs.a + val + c;
  cpu->regs.setFlag(Registers::Z, (res & 0xFF) == 0);
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, ((a & 0x0F) + (val & 0x0F) + (c & 0x0F)) > 0x0F);
  cpu->regs.setFlag(Registers::C, res > 0xFF);
  cpu->regs.a = static_cast<uint8_t>(res);
}

void instructionSet::sub(uint8_t val){
  uint8_t a = cpu->regs.a;
  uint16_t res = cpu->regs.a - val;
  cpu->regs.setFlag(Registers::Z, (res & 0xFF) == 0);
  cpu->regs.setFlag(Registers::N, true);
  cpu->regs.setFlag(Registers::H, (a & 0x0F) < (val & 0x0F));
  cpu->regs.setFlag(Registers::C, a < val);
  cpu->regs.a = static_cast<uint8_t>(res);
}

void instructionSet::sbc(uint8_t val){
  uint8_t a = cpu->regs.a;
  uint8_t b = cpu->regs.checkFlag(Registers::C);
  uint16_t res = cpu->regs.a - val - b;
  cpu->regs.setFlag(Registers::Z, (res & 0xFF) == 0);
  cpu->regs.setFlag(Registers::N, true);
  cpu->regs.setFlag(Registers::H, ((a ^ val ^ res) >> 3) & 1);    // val + b?
  cpu->regs.setFlag(Registers::C, a < (val + b));
  cpu->regs.a = static_cast<uint8_t>(res);
}

void instructionSet::_and(uint8_t val){
  cpu->regs.a &= val;
  cpu->regs.setFlag(Registers::Z, cpu->regs.a == 0);
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, true);
  cpu->regs.setFlag(Registers::C, false);
}

void instructionSet::_or(uint8_t val){
  cpu->regs.a |= val;
  cpu->regs.setFlag(Registers::Z, cpu->regs.a == 0);
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, false);
  cpu->regs.setFlag(Registers::C, false);
}

void instructionSet::_xor(uint8_t val){
  cpu->regs.a ^= val;
  cpu->regs.setFlag(Registers::Z, cpu->regs.a == 0);
  cpu->regs.setFlag(Registers::N, false);
  cpu->regs.setFlag(Registers::H, false);
  cpu->regs.setFlag(Registers::C, false);
}

void instructionSet::cp(uint8_t val){
  uint8_t a = cpu->regs.a;
  uint16_t res = cpu->regs.a - val;
  cpu->regs.setFlag(Registers::Z, (res & 0xFF) == 0);
  cpu->regs.setFlag(Registers::N, true);
  cpu->regs.setFlag(Registers::H, ((a ^ val ^ res) >> 3) & 1);
  cpu->regs.setFlag(Registers::C, a < val);
}

uint16_t instructionSet::pop(){
  uint16_t popped = cpu->read(cpu->regs.sp++);
  popped |= cpu->read(cpu->regs.sp++) << 8;
  return popped;
}

void instructionSet::push(uint16_t val){
  cpu->write(--cpu->regs.sp, (val & 0xFF00) >> 8);
  cpu->write(--cpu->regs.sp, (val & 0x00FF));
}
