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
	int w = WIDTH * scale - 22;  // magic number fixes initial black bars caused by menu integration :D
	int h = HEIGHT * scale;
	sdlWindow = SDL_CreateWindow(title, w, h, SDL_WINDOW_RESIZABLE);
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
	if (!sdlTexture) {
		std::cout << SDL_GetError() << std::endl;
		return false;
	}
	SDL_SetTextureScaleMode(sdlTexture, SDL_SCALEMODE_NEAREST);
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);

	return true;
}

void Renderer::render(const std::array<uint32_t, WIDTH* HEIGHT>& framebuffer) {
	for (int i = 0; i < WIDTH * HEIGHT; i++) {
		rgbbuffer[i] = framebuffer[i];
	}
	for (int y = 0; y < HEIGHT - 1; y++) {     // smoothing filter
		for (int x = 0; x < WIDTH - 1; x++) {
			int i = y * WIDTH + x;

			uint32_t c = rgbbuffer[i];
			uint32_t r = rgbbuffer[i + 1];
			uint32_t d = rgbbuffer[i + WIDTH];

			uint8_t cr = (c >> 16) & 0xFF;
			uint8_t cg = (c >> 8) & 0xFF;
			uint8_t cb = c & 0xFF;

			uint8_t rr = (r >> 16) & 0xFF;
			uint8_t rg = (r >> 8) & 0xFF;
			uint8_t rb = r & 0xFF;

			uint8_t dr = (d >> 16) & 0xFF;
			uint8_t dg = (d >> 8) & 0xFF;
			uint8_t db = d & 0xFF;

			uint8_t nr = (cr * 28 + rr + dr) / 32;
			uint8_t ng = (cg * 28 + rg + dg) / 32;
			uint8_t nb = (cb * 28 + rb + db) / 32;

			rgbbuffer[i] = 0xFF000000 | (nr << 16) | (ng << 8) | nb;
		}
	}
	for (int i = 0; i < WIDTH * HEIGHT; i++) {
		uint32_t cur = rgbbuffer[i];
		uint32_t prev = prevFrame[i];

		uint8_t cr = (cur >> 16) & 0xFF;
		uint8_t cg = (cur >> 8) & 0xFF;
		uint8_t cb = cur & 0xFF;

		uint8_t pr = (prev >> 16) & 0xFF;
		uint8_t pg = (prev >> 8) & 0xFF;
		uint8_t pb = prev & 0xFF;

		uint8_t r = (cr * 7 + pr) / 8;
		uint8_t g = (cg * 7 + pg) / 8;
		uint8_t b = (cb * 7 + pb) / 8;

		uint32_t blended = 0xFF000000 | (r << 16) | (g << 8) | b;

		prevFrame[i] = blended;
		rgbbuffer[i] = blended;
	}
	SDL_UpdateTexture(
		sdlTexture,
		nullptr,
		rgbbuffer.data(),
		WIDTH * sizeof(uint32_t)
	);
	int windowW, windowH;
	SDL_GetWindowSize(sdlWindow, &windowW, &windowH);
	float scaleX = (float)windowW / WIDTH;
	float scaleY = (float)windowH / HEIGHT;
	float scale = std::min(scaleX, scaleY);
	SDL_FRect dest;
	dest.w = WIDTH * scale;
	dest.h = HEIGHT * scale;
	dest.x = (windowW - dest.w) * 0.5f;
	dest.y = (windowH - dest.h) * 0.5f;
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderTexture(sdlRenderer, sdlTexture, nullptr, &dest);
	SDL_RenderPresent(sdlRenderer);
}

bool Renderer::procEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			return false;
		}
		if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_MAXIMIZED) {
			wWidth = event.window.data1;
			wHeight = event.window.data2;
		}
	}
	return true;
}

void Renderer::idle() {
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_RenderPresent(sdlRenderer);
}