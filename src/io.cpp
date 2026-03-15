#include "io.h"
#include "apu.h"
#include "ppu.h"
#include "joypad.h"

IO::IO() {
	IF = 0;
	JOYP = 0xFF;
	TIMA = 0;
	TMA = 0;
	TAC = 0;
	DIV = 0;
}

IO::~IO() {};

uint8_t IO::read(uint16_t addr) {
	if (addr >= 0xFF10 && addr <= 0xFF3F) return apu ? apu->read(addr) : 0xFF;
	switch (addr) {
	case 0xFF00: return j ? j->getJOYP() : 0xFF; break;
	case 0xFF0F: return IF; break;
	case 0xFF04: return DIV; break;
	case 0xFF05: return TIMA; break;
	case 0xFF06: return TMA; break;
	case 0xFF07: return TAC; break;
	case 0xFF40: return LCDC; break;
	case 0xFF41: return STAT; break;
	case 0xFF42: return SCY; break;
	case 0xFF43: return SCX; break;
	case 0xFF44: return LY; break;
	case 0xFF45: return LYC; break;
	case 0xFF47: return BGP; break;
	case 0xFF48: return OBP0; break;
	case 0xFF49: return OBP1; break;
	case 0xFF4A: return WY; break;
	case 0xFF4B: return WX; break;
	case 0xFF4D: return KEY1; break;
	case 0xFF4F: return 0xFE | VBK; break;
	case 0xFF68: return ppu->readBGPI(); break;
	case 0xFF69: return ppu->readBGPD(); break;
	case 0xFF6A: return ppu->readOBPI(); break;
	case 0xFF6B: return ppu->readOBPD(); break;
	case 0xFF70: return SVBK; break;
	default: return 0xFF;
	}
}

void IO::write(uint16_t addr, uint8_t val) {
	if (addr >= 0xFF10 && addr <= 0xFF3F) {
		if (apu) apu->write(addr, val);
		return;
	}
	switch (addr) {
	case 0xFF00:
		if (j) j->setSelect(val);
		break;
	case 0xFF0F:
		IF = (val & 0x1F); 
		break;
	case 0xFF04:
		divWrite = true;
		break;
	case 0xFF05:
	    TIMA = val;
		break;
	case 0xFF06:
		TMA = val;
		break;
	case 0xFF07:
		TAC = (val & 0x07);
		break;
	case 0xFF40:
		LCDC = val;
		break;
	case 0xFF41:
		STAT = (STAT & 0x07) | (val & 0x78); // 0x78 is 3-6 bits
		break;
	case 0xFF42:
		SCY = val;
		break;
	case 0xFF43:
		SCX = val;
		break;
	case 0xFF44:
		//LY = 0;
		break;
	case 0xFF45: {
		LYC = val;

		bool match = (LY == LYC);

		if (match) STAT |= 0x04;
		else STAT &= ~0x04;

		if (match && (STAT & 0x40)) {
			reqINT(IO::INT::LCDStat);
		}
		break;
	}
	case 0xFF47:
		BGP = val;
		break;
	case 0xFF48:
		OBP0 = val;
		break;
	case 0xFF49:
		OBP1 = val;
		break;
	case 0xFF4A:
		WY = val;
		break;
	case 0xFF4B:
		WX = val;
		break;
	case 0xFF4D:
		KEY1 = (KEY1 & 0x80) | (val & 0x01);
		break;
	case 0xFF4F:
		VBK = val & 1;
		break;
	case 0xFF51: HDMA1 = val; break;
	case 0xFF52: HDMA2 = val; break;
	case 0xFF53: HDMA3 = val; break;
	case 0xFF54: HDMA4 = val; break;
	case 0xFF55:
		HDMA5 = val;
		break;
	case 0xFF68:
		ppu->writeBGPI(val);
		break;
	case 0xFF69:
		ppu->writeBGPD(val);
		break;
	case 0xFF6A:
		ppu->writeOBPI(val);
		break;
	case 0xFF6B:
		ppu->writeOBPD(val);
		break;
	case 0xFF70:
		SVBK = val & 0x07;
		if (SVBK == 0) SVBK = 1;
		break;
	default:
		break;
	}
}

void IO::reqINT(INT type) {
	IF |= (1 << static_cast<int>(type));
}

uint8_t IO::readTIMA() {
	return TIMA;
}

void IO::setTIMA(uint8_t val) {
	TIMA = val;
}

uint8_t IO::readTMA() {
	return TMA;
}

uint8_t IO::readTAC() {
	return TAC;
}

void IO::setDIV(uint8_t val) {
	DIV = val;
}

void IO::setLY(uint8_t val) {
	LY = val;
}

void IO::setSTATMode(uint8_t mode) {
	STAT &= ~(0x03);
	STAT |= (mode & 0x03);
}

void IO::setSTATFlag(bool match) {
	if (match) {
		STAT |= 0x04;
	} else {
		STAT &= ~(0x04);
	}
}

void IO::attachJoypad(Joypad* jp) {
	j = jp;
}

void IO::attachAPU(APU* a) {
	apu = a;
}

void IO::attachPPU(PPU* p) {
	ppu = p;
}

void IO::reset() {
	IF = 0x00;
	JOYP = 0xCF;
	DIV = 0x00;
	TIMA = 0x00;
	TMA = 0x00;
	TAC = 0x00;
	LCDC = 0x00;
	STAT = 0x00;
	LY = 0x00;
	LYC = 0x00;
	SCX = 0x00;
	SCY = 0x00;
	WX = 0x00;
	WY = 0x00;
	BGP = 0x00;
	OBP0 = 0x00;
	OBP1 = 0x00;
	VBK = 0x00;
	SVBK = 0x01;
	KEY1 = 0x00;
}