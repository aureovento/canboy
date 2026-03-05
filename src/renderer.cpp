#include "renderer.h"
#include <iostream>

Renderer::Renderer() {}
Renderer::~Renderer() {
	if (sdlTexture) SDL_DestroyTexture(sdlTexture);
	if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
	if (sdlWindow) SDL_DestroyWindow(sdlWindow);
	SDL_Quit();
}

bool Renderer::init(const char* title, int scale) {
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	int w = WIDTH * scale;
	int h = HEIGHT * scale;
	sdlWindow = SDL_CreateWindow(title, w, h, 0);
	if (sdlWindow == nullptr) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	sdlRenderer = SDL_CreateRenderer(sdlWindow, nullptr);
	if (sdlRenderer == nullptr) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	sdlTexture = SDL_CreateTexture(
		sdlRenderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		WIDTH,
		HEIGHT
	);
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);

	return true;
}

void Renderer::render(const std::array<uint8_t, WIDTH* HEIGHT>& framebuffer) {
	for (int i = 0; i < WIDTH * HEIGHT; i++) {
		uint8_t color = framebuffer[i];
		uint32_t argb;

		switch (color) {
		case 0: argb = 0xFFFFFFFF; break; // white
		case 1: argb = 0xFFAAAAAA; break; // light grey
		case 2: argb = 0xFF555555; break; // fdark grey
		case 3: argb = 0xFF000000; break; // black
		default: argb = 0xFFFF00FF; break;
		}

		rgbbuffer[i] = argb;
	}
	SDL_UpdateTexture(
		sdlTexture,
		nullptr,
		rgbbuffer.data(),
		WIDTH * sizeof(uint32_t)
	);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderTexture(sdlRenderer, sdlTexture, nullptr, nullptr);
	SDL_RenderPresent(sdlRenderer);
}

bool Renderer::procEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			return false;
		}
		if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
			return false;
		}
	}
	return true;
}