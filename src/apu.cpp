#include "apu.h"

uint8_t APU::read(uint16_t addr) {
	if (addr == 0xFF26) return NR52 | 0x70;
    if (addr >= 0xFF30 && addr <= 0xFF3F) return waveRAM[addr - 0xFF30];
    switch (addr) {
    case 0xFF10: return NR10;
    case 0xFF11: return NR11;
    case 0xFF12: return NR12;
    case 0xFF13: return NR13;
    case 0xFF14: return NR14;
    case 0xFF16: return NR21;
    case 0xFF17: return NR22;
    case 0xFF18: return NR23;
    case 0xFF19: return NR24;
    case 0xFF1A: return NR30;
    case 0xFF1B: return NR31;
    case 0xFF1C: return NR32;
    case 0xFF1D: return NR33;
    case 0xFF1E: return NR34;
    case 0xFF20: return NR41;
    case 0xFF21: return NR42;
    case 0xFF22: return NR43;
    case 0xFF23: return NR44;
    case 0xFF24: return NR50;
    case 0xFF25: return NR51;
    }
	return 0xFF;
}

void APU::write(uint16_t addr, uint8_t val)
{
    if (!(NR52 & 0x80) && addr != 0xFF26) return;
    if (addr == 0xFF26) {
        bool enable = val & 0x80;
        if (!enable) NR52 = 0;
        else NR52 |= 0x80;
    }
    if (addr >= 0xFF30 && addr <= 0xFF3F) {
        waveRAM[addr - 0xFF30] = val;
        return;
    }

    switch (addr) {
    case 0xFF10:
        NR10 = val;
        ch1SweepPeriod = (val >> 4) & 0x07;
        ch1SweepNegate = (val & 0x08) != 0;
        ch1SweepShift = val & 0x07;
        break;
    case 0xFF11:
        NR11 = val;
        ch1.duty = (val >> 6) & 0x03;
        ch1.length = 64 - (val & 0x3F);
        break;
    case 0xFF12:
        NR12 = val;
        ch1.volume = (val >> 4) & 0x0F;
        ch1.envelopeIncrease = val & 0x08;
        ch1.envelopePeriod = val & 0x07;
        ch1.envelopeTimer = ch1.envelopePeriod;
        break;
    case 0xFF13:
        NR13 = val;
        ch1.frequency = ((NR14 & 7) << 8) | NR13;
        break;
    case 0xFF14: {
        NR14 = val;
        ch1.frequency = ((NR14 & 7) << 8) | NR13;
        ch1.lengthEnable = val & 0x40;
        if (val & 0x80) {
            ch1.enabled = true;
            ch1.dutyPos = 0;
            ch1.timer = (2048 - ch1.frequency) * 4;
            ch1ShadowFreq = ch1.frequency;
            ch1SweepTimer = ch1SweepPeriod;
            if (ch1.length == 0) ch1.length = 64;
        }
        break;
    }
    case 0xFF16:
        NR21 = val;
        ch2.duty = (val >> 6) & 0x03;
        ch2.length = 64 - (val & 0x3F);
        break;
    case 0xFF17:
        NR22 = val;
        ch2.volume = (val >> 4) & 0x0F;
        ch2.envelopeIncrease = val & 0x08;
        ch2.envelopePeriod = val & 0x07;
        ch2.envelopeTimer = ch2.envelopePeriod;
        break;
    case 0xFF18:
        NR23 = val;
        break;
    case 0xFF19:
        NR24 = val;
        ch2.frequency = ((val & 0x07) << 8) | NR23;
        ch2.lengthEnable = val & 0x40;
        if (val & 0x80) { //trigger
            ch2.enabled = true;
            ch2.dutyPos = 0;
            ch2.timer = (2048 - ch2.frequency) * 4;
            ch2.volume = (NR22 >> 4) & 0x0F;
            ch2.envelopeTimer = ch2.envelopePeriod;
            if (ch2.length == 0) ch2.length = 64;
        }
        break;
    case 0xFF1A:
        NR30 = val;
        ch3.dacEnabled = val & 0x80;
        if (!ch3.dacEnabled) ch3.enabled = false;
        break;
    case 0xFF1B:
        NR31 = val;
        ch3.length = 256 - val;
        break;
    case 0xFF1C:
        NR32 = val;
        ch3.volumeShift = (val >> 5) & 3;
        break;
    case 0xFF1D:
        NR33 = val;
        ch3.frequency = ((NR34 & 7) << 8) | NR33;
        break;
    case 0xFF1E:
        NR34 = val;
        ch3.frequency = ((val & 7) << 8) | NR33;
        ch3.lengthEnable = val & 0x40;
        if (val & 0x80) {
            ch3.enabled = ch3.dacEnabled;
            ch3.position = 0;
            ch3.timer = (2048 - ch3.frequency) * 2 - 1;
            if (ch3.length == 0) ch3.length = 256;
        }
        break;
    case 0xFF20:
        NR41 = val;
        ch4.length = 64 - (val & 0x3F);
        break;
    case 0xFF21:
        NR42 = val;

        ch4.volume = (val >> 4) & 0x0F;
        ch4.envelopeIncrease = val & 0x08;
        ch4.envelopePeriod = val & 0x07;
        ch4.envelopeTimer = ch4.envelopePeriod;
        break;
    case 0xFF22:
        NR43 = val;

        ch4.clockShift = val >> 4;
        ch4.widthMode = val & 0x08;
        ch4.divisorCode = val & 0x07;
        break;
    case 0xFF23:
        NR44 = val;
        ch4.lengthEnable = val & 0x40;
        if (val & 0x80) {
            ch4.enabled = true;
            ch4.lfsr = 0x7FFF;
            if (ch4.length == 0) ch4.length = 64;
            uint8_t divisors[8] = { 8,16,32,48,64,80,96,112 };
            ch4.timer = divisors[ch4.divisorCode] << ch4.clockShift;
        }
        break;
    case 0xFF24:
        NR50 = val;
        break;
    case 0xFF25:
        NR51 = val;
        break;
    }
}


void APU::tick() {
    if (!(NR52 & 0x80)) return;
    if (ch1.enabled) {
        if (ch1.timer > 0) ch1.timer--;
        if (ch1.timer == 0) {
            ch1.timer = (2048 - ch1.frequency) * 4;
            ch1.dutyPos = (ch1.dutyPos + 1) & 7;
        }
    }
    if (ch2.enabled) {
        if (ch2.timer > 0) ch2.timer--;
        if (ch2.timer == 0) {
            ch2.timer = (2048 - ch2.frequency) * 4;
            ch2.dutyPos = (ch2.dutyPos + 1) & 7;
        }
    }
    if (ch3.enabled) {
        if (--ch3.timer <= 0) {
            ch3.timer += (2048 - ch3.frequency) * 2;
            ch3.position = (ch3.position + 1) & 31;
        }
    }
    if (ch4.enabled) {
        if (--ch4.timer <= 0) {
            uint8_t divisors[8] = { 8,16,32,48,64,80,96,112 };
            ch4.timer += divisors[ch4.divisorCode] << ch4.clockShift;
            uint16_t xorBit = (ch4.lfsr ^ (ch4.lfsr >> 1)) & 1;
            ch4.lfsr = (ch4.lfsr >> 1) | (xorBit << 14);
            if (ch4.widthMode) ch4.lfsr = (ch4.lfsr & ~(1 << 6)) | (xorBit << 6);
        }
    }

    frameCounter++;
    if (frameCounter >= 8192) {
        frameCounter = 0;
        frameSequencer();
    }
    sampleTimer += 1.0;
    if (sampleTimer >= samplePeriod) {
        sampleTimer -= samplePeriod;
        generateSample();
    }
}

void APU::init()
{
    SDL_AudioSpec spec{};
    spec.freq = 48000;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;

    device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);

    if (!device) {
        SDL_Log("Failed to open audio: %s", SDL_GetError());
        return;
    }
    stream = SDL_CreateAudioStream(&spec, &spec);
    SDL_BindAudioStream(device, stream);
    SDL_ResumeAudioDevice(device);
}

void APU::generateSample() {
    int sample = 0;
    int active = 0;
    int s1 = ch1.sample(dutyTable);
    int s2 = ch2.sample(dutyTable);
    int s3 = sampleCH3();
    int s4 = sampleCH4();
    int left = 0;
    int right = 0;

    if (NR51 & 0x10) left += s1;
    if (NR51 & 0x20) left += s2;
    if (NR51 & 0x40) left += s3;
    if (NR51 & 0x80) left += s4;

    if (NR51 & 0x01) right += s1;
    if (NR51 & 0x02) right += s2;
    if (NR51 & 0x04) right += s3;
    if (NR51 & 0x08) right += s4;

    int leftVol = ((NR50 >> 4) & 7) + 1;
    int rightVol = (NR50 & 7) + 1;

    left = (left * leftVol) / 16;
    right = (right * rightVol) / 16;

    static int prevInL = 0, prevOutL = 0;
    static int prevInR = 0, prevOutR = 0;
    int outL = left - prevInL + (prevOutL * 995) / 1000;
    int outR = right - prevInR + (prevOutR * 995) / 1000;
    prevInL = left;
    prevInR = right;
    prevOutL = outL;
    prevOutR = outR;
    static float lpL = 0.0f;
    static float lpR = 0.0f;
    const float alpha = 0.5f;
    lpL += alpha * (outL - lpL);
    lpR += alpha * (outR - lpR);
    outL = (int)lpL;
    outR = (int)lpR;
    audioBuffer.push_back(outL);
    audioBuffer.push_back(outR);
    if (audioBuffer.size() >= 1024) {
        while (SDL_GetAudioStreamQueued(stream) > 8192) SDL_Delay(1);
        SDL_PutAudioStreamData(stream, audioBuffer.data(), audioBuffer.size() * sizeof(int16_t));
        audioBuffer.clear();
    }
}

void APU::frameSequencer() {
    frameStep = (frameStep + 1) & 7;
    switch (frameStep) {
    case 0:
    case 4:
        if (ch1.lengthEnable && ch1.length > 0) {
            ch1.length--;
            if (ch1.length == 0) ch1.enabled = false;
        }
        if (ch2.lengthEnable && ch2.length > 0) {
            ch2.length--;
            if (ch2.length == 0) ch2.enabled = false;
        }
        if (ch3.lengthEnable && ch3.length > 0) {
            ch3.length--;
            if (ch3.length == 0) ch3.enabled = false;
        }
        if (ch4.lengthEnable && ch4.length > 0) {
            ch4.length--;
            if (ch4.length == 0) ch4.enabled = false;
        }
        break;
    case 2:
    case 6:
        if (ch1.lengthEnable && ch1.length > 0) {
            ch1.length--;
            if (ch1.length == 0) ch1.enabled = false;
        }
        if (ch2.lengthEnable && ch2.length > 0) {
            ch2.length--;
            if (ch2.length == 0) ch2.enabled = false;
        }
        if (ch3.lengthEnable && ch3.length > 0) {
            ch3.length--;
            if (ch3.length == 0) ch3.enabled = false;
        }
        if (ch4.lengthEnable && ch4.length > 0) {
            ch4.length--;
            if (ch4.length == 0) ch4.enabled = false;
        }
        updateSweep();
        break;
    case 7: // envelope
        if (ch1.envelopePeriod > 0) {
            if (ch1.envelopeTimer > 0) ch1.envelopeTimer--;
            if (ch1.envelopeTimer == 0) {
                ch1.envelopeTimer = ch1.envelopePeriod;
                if (ch1.envelopeIncrease) {
                    if (ch1.volume < 15) ch1.volume++;
                }
                else {
                    if (ch1.volume > 0) ch1.volume--;
                }
            }
        }
        if (ch2.envelopePeriod > 0) {
            if (ch2.envelopeTimer > 0) ch2.envelopeTimer--;
            if (ch2.envelopeTimer == 0) {
                ch2.envelopeTimer = ch2.envelopePeriod;
                if (ch2.envelopeIncrease) {
                    if (ch2.volume < 15)
                        ch2.volume++;
                }
                else {
                    if (ch2.volume > 0) ch2.volume--;
                }
            }
        }
        if (ch4.envelopePeriod > 0) {
            if (ch4.envelopeTimer > 0) ch4.envelopeTimer--;
            if (ch4.envelopeTimer == 0) {
                ch4.envelopeTimer = ch4.envelopePeriod;
                if (ch4.envelopeIncrease) {
                    if (ch4.volume < 15) ch4.volume++;
                }
                else {
                    if (ch4.volume > 0) ch4.volume--;
                }
            }
        }
        break;
    }
}

void APU::calcSweep() {
    if (ch1SweepShift == 0) return;
    uint16_t newFreq = ch1ShadowFreq >> ch1SweepShift;
    if (ch1SweepNegate) newFreq = ch1ShadowFreq - newFreq;
    else newFreq = ch1ShadowFreq + newFreq;
    if (newFreq > 2047) {
        ch1.enabled = false;
    }
    else {
        ch1ShadowFreq = newFreq;
        ch1.frequency = newFreq;
    }
}

void APU::updateSweep() {
    if (ch1SweepPeriod == 0) return;
    if (ch1SweepTimer > 0) ch1SweepTimer--;
    if (ch1SweepTimer == 0) {
        ch1SweepTimer = ch1SweepPeriod;
        calcSweep();
        if (ch1.enabled) calcSweep();
    }
}

int APU::sampleCH3() {
    if (!ch3.enabled || !ch3.dacEnabled) return 0;
    uint8_t byte = waveRAM[ch3.position >> 1];
    uint8_t sample;
    if (ch3.position & 1) sample = byte & 0x0F;
    else sample = byte >> 4;
    if (ch3.volumeShift == 0) return 0;
    switch (ch3.volumeShift) {
    case 0: sample = 0; break;
    case 1: break;
    case 2: sample >>= 1; break;
    case 3: sample >>= 2; break;
    }
    return (sample - 8) * 150;
}

int APU::sampleCH4() {
    if (!ch4.enabled) return 0;
    int bit = (~ch4.lfsr) & 1;
    int dacInput = bit ? ch4.volume : 0;
    return (dacInput - 8) * 200;
}

void APU::reset() {
    counter = 0;
    frameStep = 0;
    frameCounter = 0;
    sampleCounter = 0;
    sampleAccumulator = 0.0;
    sampleTimer = 0.0;
    audioBuffer.clear();

    NR50 = 0;
    NR51 = 0;
    NR52 = 0;
    NR10 = 0;
    NR11 = 0;
    NR12 = 0;
    NR13 = 0;
    NR14 = 0;
    NR21 = 0;
    NR22 = 0;
    NR23 = 0;
    NR24 = 0;
    NR30 = 0;
    NR31 = 0;
    NR32 = 0;
    NR33 = 0;
    NR34 = 0;
    NR41 = 0;
    NR42 = 0;
    NR43 = 0;
    NR44 = 0;

    ch1 = SquareChannel{};
    ch2 = SquareChannel{};
    ch3 = WaveChannel{};
    ch4 = NoiseChannel{};

    ch1SweepPeriod = 0;
    ch1SweepTimer = 0;
    ch1SweepShift = 0;
    ch1SweepNegate = false;
    ch1ShadowFreq = 0;

    memset(waveRAM, 0, sizeof(waveRAM));
}