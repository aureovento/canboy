#include "ppu.h"
#include "Bus.h"
#include <iostream>

void PPU::tick() {
	uint8_t lcdc = io.read(IO::REG::LCDC);
	bit7 = lcdc & 0x80;
	if (bit7Prev && !bit7) {
		dotcount = 0;
		ly = 0;
		mode = 0;
		io.setLY(0);
		io.setSTATMode(0);
		prevMatch = false;
		wLine = 0;
		wActive = 0;
		wUsed = 0;
	}
	if (!bit7Prev && bit7) {
		dotcount = 0;
		ly = 0;
		mode = 2;
		io.setLY(0);
		io.setSTATMode(2);
		prevMatch = false;
	}
	if (!bit7) {
		bit7Prev = bit7;
		return;
	}
	dotcount++;
	if (dotcount == 456) {
		dotcount = 0;
		uint8_t prevLY = ly;
		ly++;
		xPixel = 0;
		if (ly >= 154) {
			ly = 0;
			wLine = 0;
		}
		io.setLY(ly);
		uint8_t wy = io.read(IO::REG::WY);
		if (ly < wy) {
			wLine = 0;
		}
		if (wUsed) wLine++;
		wUsed = false;
		if (prevLY == 143 && ly == 144) {
			frameReady = true;
			io.reqINT(IO::INT::VBlank);
		}
	}

	bool currentMatch = (ly == io.read(IO::REG::LYC));
	io.setSTATFlag(currentMatch);
	uint8_t stat = io.read(IO::REG::STAT);
	if (!prevMatch && currentMatch) {
		if (stat & 0x40) {
			io.reqINT(IO::INT::LCDStat);
		}
	}

	uint8_t prevMode = mode;
	if (ly <= 143) {
		if (dotcount < 80) mode = 2;
		else if (dotcount < 252 || xPixel < 160) mode = 3; // might break cuz of dotcount condition?
		else mode = 0;
	} else {
		mode = 1;
	}
	io.setSTATMode(mode);
	stat = io.read(IO::REG::STAT);
	if (mode != prevMode) {
		if (mode == 2) {
			scanOAM();
		}
		if (prevMode == 2 && mode == 3) {
			enterMode3();
		}
		// INT
		if (mode == 0) {
			if (stat & 0x08) {
				io.reqINT(IO::INT::LCDStat);
			}
		}
		else if (mode == 1) {
			if (stat & 0x10) {
				io.reqINT(IO::INT::LCDStat);
			}
		}
		else if (mode == 2) {
			if (stat & 0x20) {
				io.reqINT(IO::INT::LCDStat);
			}
		}
	}
	if (mode == 3 && ly <= 143) {
		uint8_t lcdc = io.read(IO::REG::LCDC);
		uint8_t wx = io.read(IO::REG::WX);
		uint8_t wy = io.read(IO::REG::WY);
		int16_t trigger = static_cast<int16_t>(wx) - 7;
		if (trigger < 0) trigger = 0;
		if (!wActive && (lcdc & 0x20) && ly >= wy && xPixel >= trigger) {   // xpixel < 160 maybe
			wActive = true;
			wUsed = true;
			bgFIFO.clear();
			state = FState::getTile;
			fdotcounter = 0;
			fetcherX = 0;
			pixelSkip = 0;
		}
		tickFetcher();
		if (!bgFIFO.empty() && xPixel < 160) {
			BGPixel pixel = bgFIFO.front();
			bgFIFO.pop_front();
			uint8_t color = pixel.color;
			uint8_t palette = pixel.palette;
			bool objEnabled = lcdc & 0x02;
			bool objSize = lcdc & 0x04;
			if (pixelSkip > 0) {
				pixelSkip--;
			}
			else {
				uint8_t shade; // bg shade
				uint16_t rgb = 0;
				if (bus->isCGB()) {
					uint8_t addr = palette * 8 + color * 2;
					uint8_t lo = bgPaletteRAM[addr];
					uint8_t hi = bgPaletteRAM[addr + 1];
					rgb = lo | (hi << 8);
					uint8_t bgp = io.read(IO::REG::BGP);
					shade = (bgp >> (color * 2)) & 0x03;
				}
				else {
					if (io.read(IO::REG::LCDC) & 0x01) {
						uint8_t bgp = io.read(IO::REG::BGP);
						shade = (bgp >> (color * 2)) & 0x03;
					}
					else {
						shade = 0;
					}
				}
				// sprite shade overwrite
				uint8_t spriteShade;
				if (getSpriteShade(pixel, objEnabled, objSize, spriteShade)) {
					shade = spriteShade;
				}
				if (bus->isCGB()) framebuffer[ly * 160 + xPixel] = toRGB(rgb);
				else framebuffer[ly * 160 + xPixel] = dmgPalette[shade];
				xPixel++;
			}
		}
	}
	prevMatch = currentMatch;
	bit7Prev = bit7;
}


const std::array<uint32_t, 160 * 144>& PPU::getFrameBuffer() const {
	return framebuffer;
}

void PPU::enterMode3() {
	bgFIFO.clear();
	state = FState::getTile;
	fdotcounter = 0;
	fetcherX = 0;
	xPixel = 0;
	uint8_t scx = io.read(IO::REG::SCX);
	pixelSkip = scx & 7;
	wActive = false;
	tileAttr = 0;
	attrXFlip = false;
	attrYFlip = false;
	attrBank = 0;
}

void PPU::tickFetcher() {
	switch (state) {
	case FState::getTile:
		fdotcounter++;
		if (fdotcounter == 2) {
			fdotcounter = 0;
			if (wActive) {
				uint8_t lcdc = io.read(IO::REG::LCDC);
				uint8_t tileY = wLine / 8;
				uint8_t tileX = fetcherX;
				uint16_t tileMapBase = (lcdc & 0x40) ? 0x9C00 : 0x9800;
				uint16_t addr = tileMapBase + (tileY * 32 + tileX);
				tileNo = bus->rawRead(addr);
				if (bus->isCGB()) tileAttr = bus->readVRAM(1, addr - 0x8000);
				else tileAttr = 0;
				attrPalette = tileAttr & 0x07;
				attrPriority = (tileAttr & 0x80) != 0;
				attrXFlip = (tileAttr & 0x20) != 0;
				attrYFlip = (tileAttr & 0x40) != 0;
				attrBank = (tileAttr >> 3) & 1;
			}
			else {
				uint8_t scx = io.read(IO::REG::SCX);
				uint8_t scy = io.read(IO::REG::SCY);
				uint8_t lcdc = io.read(IO::REG::LCDC);
				uint8_t bgY = (scy + ly) & 0xFF;
				uint8_t tileY = bgY / 8;
				uint8_t tileX = ((scx / 8) + fetcherX) & 31;
				uint16_t tileMapBase = (lcdc & 0x08) ? 0x9C00 : 0x9800;
				uint16_t addr = tileMapBase + (tileY * 32 + tileX);
				tileNo = bus->rawRead(addr);
				if (bus->isCGB()) tileAttr = bus->readVRAM(1, addr - 0x8000);
				else tileAttr = 0;
				attrPalette = tileAttr & 0x07;
				attrPriority = (tileAttr & 0x80) != 0;
				attrXFlip = (tileAttr & 0x20) != 0;
				attrYFlip = (tileAttr & 0x40) != 0;
				attrBank = (tileAttr >> 3) & 1;
			}
			state = FState::getLow;
		}
		break;
	case FState::getLow:
		fdotcounter++;
		if (fdotcounter == 2) {
			fdotcounter = 0;
			if (wActive) {
				tileRow = wLine & 0x07;
				if (bus->isCGB() && attrYFlip) tileRow = 7 - tileRow;
			}
			else {
				uint8_t scy = io.read(IO::REG::SCY);
				uint8_t bgY = (scy + ly) & 0xFF;
				tileRow = bgY & 0x07;
				if (bus->isCGB() && attrYFlip) tileRow = 7 - tileRow;
			}
			uint8_t lcdc = io.read(IO::REG::LCDC);
			tileBase = (lcdc & 0x10) ? (0x8000 + tileNo * 16) : (0x9000 + static_cast<int8_t>(tileNo) * 16);
			if (bus->isCGB()) tileBase += attrBank * 0x2000;
			tileLow = bus->rawRead(tileBase + tileRow * 2);
			state = FState::getHigh;
		}
		break;
	case FState::getHigh:
		fdotcounter++;
		if (fdotcounter == 2) {
			fdotcounter = 0;
			tileHigh = bus->rawRead(tileBase + (tileRow * 2 + 1));
			state = FState::sleep;
		}
		break;
	case FState::sleep:
		fdotcounter++;
		if (fdotcounter == 2) {
			fdotcounter = 0;
			state = FState::push;
		}
		break;
	case FState::push:
		if (bgFIFO.size() <= 8) {
			for (uint8_t i = 0; i < 8; i++) {
				uint8_t bit;
				if (bus->isCGB())bit = attrXFlip ? i: (7 - i);
				else bit = 7 - i;
				uint8_t lowBit = (tileLow >> (bit)) & 1;
				uint8_t highBit = (tileHigh >> (bit)) & 1;
				BGPixel p;
				p.color = (highBit << 1) | lowBit;
				p.palette = attrPalette;
				p.priority = attrPriority;
				bgFIFO.push_back(p);
			}
			fetcherX++;
			state = FState::getTile;
		}
		break;
	}
}

bool PPU::isFrameReady() {
	return frameReady;
}

void PPU::clrFrameFlag() {
	frameReady = false;
}

void PPU::scanOAM() {
	sprites.clear();
	uint8_t lcdc = io.read(IO::REG::LCDC);
	bool objSize = lcdc & 0x04;  // 8x16 if set

	uint8_t spriteHeight = objSize ? 16 : 8;

	for (int i = 0; i < 40; i++) {
		uint16_t base = 0xFE00 + i * 4;

		uint8_t y = bus->rawRead(base);
		uint8_t x = bus->rawRead(base + 1);
		uint8_t tile = bus->rawRead(base + 2);
		uint8_t attr = bus->rawRead(base + 3);

		int16_t spriteY = y - 16;
		int16_t spriteX = x - 8;

		if (ly >= spriteY && ly < spriteY + spriteHeight) {
			if (sprites.size() < 10) {
				sprites.push_back({ spriteY, spriteX, tile, attr });
			}
		}
	}
}

bool PPU::getSpriteShade(const BGPixel& bg, bool objEn, bool objSize, uint8_t& shade) {
	if (!objEn) return false;
	for (const Sprite& s : sprites) {
		if (xPixel < s.x || xPixel >= s.x + 8) continue;
		int16_t row = ly - s.y;
		int16_t col = xPixel - s.x;
		if (s.attr & 0x40) {
			row = (objSize ? 15 : 7) - row;
		}
		if (s.attr & 0x20) {
			col = 7 - col;
		}
		uint8_t tileIndex = s.tile;
		if (objSize) {
			tileIndex &= 0xFE;
			if (row >= 8) {
				tileIndex += 1;
				row -= 8;
			}
		}
		uint8_t bank = 0;
		if (bus->isCGB()) bank = (s.attr & 0x08) ? 1 : 0;
		uint8_t tileAddr = tileIndex * 16;
		uint8_t low;
		uint8_t high;
		if (bus->isCGB()) {
			low = bus->readVRAM(bank, tileAddr + row * 2);
			high = bus->readVRAM(bank, tileAddr + row * 2 + 1);
		}
		else {
			uint16_t addr = 0x8000 + tileAddr;
			low = bus->rawRead(tileAddr + row * 2);
			high = bus->rawRead(tileAddr + row * 2 + 1);
		}
		uint8_t bit = 7 - col;
		uint8_t sColor = ((high >> bit) & 1) << 1 | ((low >> bit) & 1);
		if (sColor == 0) continue;
		if (bus->isCGB()) {
			if (bg.priority && bg.color != 0) continue;
		}
		if (s.attr & 0x80) {
			if (bg.color != 0) continue;
		}
		uint8_t palette = (s.attr & 0x10) ? io.read(IO::REG::OBP1) : io.read(IO::REG::OBP0);
		shade = (palette >> (sColor * 2)) & 0x03;
		return true;
	}
	return false;
}

void PPU::reset() {
	mode = 2;          
	dotcount = 0;             
	xPixel = 0;         
	ly = 0;              
	frameReady = false;
	sprites.clear(); 
	framebuffer.fill(0); 
}

void PPU::writeBGPI(uint8_t v) {
	BGPI = v;
}

void PPU::writeBGPD(uint8_t v) {
	uint8_t i = BGPI & 0x3F;
	bgPaletteRAM[i] = v;
	if (BGPI & 0x80) {
		i = (i + 1) & 0x3F;
		BGPI = (BGPI & 0x80) | i;
	}
}

uint8_t PPU::readBGPI() {
	return BGPI;
}

uint8_t PPU::readBGPD() {
	uint8_t index = BGPI & 0x3F;
	return bgPaletteRAM[index];
}

uint32_t PPU::toRGB(uint16_t c) {
	uint8_t r = (c & 0x1F) * 255 / 31;
	uint8_t g = ((c >> 5) & 0x1F) * 255 / 31;
	uint8_t b = ((c >> 10) & 0x1F) * 255 / 31;
	return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

void PPU::resetBoot() {
	mode = 2;
	ly = 0;
	dotcount = 0;
}