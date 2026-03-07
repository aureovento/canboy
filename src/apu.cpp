#include "apu.h"

uint8_t APU::read(uint16_t addr) {
	if (addr == 0xFF26) return NR52 | 0x70;
    switch (addr) {
    case 0xFF16: return NR21;
    case 0xFF17: return NR22;
    case 0xFF18: return NR23;
    case 0xFF19: return NR24;
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

    switch (addr) {
    case 0xFF16:
        NR21 = val;
        ch2.duty = (val >> 6) & 0x03;
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
    }
}


void APU::tick() {
    if (!(NR52 & 0x80)) return;

    if (ch2.enabled) {
        if (ch2.timer > 0) ch2.timer--;
        if (ch2.timer == 0) {
            ch2.timer = (2048 - ch2.frequency) * 4;
            ch2.dutyPos = (ch2.dutyPos + 1) & 7;
        }
    }
    sampleCounter++;
    if (sampleCounter >= 87) {
        sampleCounter = 0;
        generateSample();
    }
}



const uint8_t APU::dutyTable[4][8] =
{
    {0,0,0,0,0,0,0,1}, // 12.5%
    {1,0,0,0,0,0,0,1}, // 25%
    {1,0,0,0,0,1,1,1}, // 50%
    {0,1,1,1,1,1,1,0}  // 75%
};


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

    // test
    ch2.enabled = true;
    ch2.duty = 2;
    ch2.frequency = 440;
}

void APU::generateSample() {
    int16_t sample = 0;
    if (ch2.enabled) {
        int amp = ch2.volume * 200;
        sample = dutyTable[ch2.duty][ch2.dutyPos] ? amp : -amp;
    }

    audioBuffer.push_back(sample);
    audioBuffer.push_back(sample);

    if (audioBuffer.size() >= 4096) {
        if (SDL_GetAudioStreamQueued(stream) > 48000) return;
        SDL_PutAudioStreamData(stream, audioBuffer.data(), audioBuffer.size() * sizeof(int16_t));
        audioBuffer.clear();
    }
}

void APU::frameSequencer() {
    frameStep = (frameStep + 1) & 7;
    switch (frameStep) {
    case 0:
    case 2:
    case 4:
    case 6:
        if (ch2.lengthEnable && ch2.length > 0) {
            ch2.length--;
            if (ch2.length == 0) ch2.enabled = false;
        }
        break;
    case 7:
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
        break;
    }
}