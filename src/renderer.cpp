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
	int w = WIDTH * scale;
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

void Renderer::render(const std::array<uint8_t, WIDTH* HEIGHT>& framebuffer) {
	const uint32_t dmgPalette[4] = {
			0xFFF0F8D8,
			0xFFA8D080,
			0xFF507860,
			0xFF101820
	};
	for (int i = 0; i < WIDTH * HEIGHT; i++) {
		rgbbuffer[i] = dmgPalette[framebuffer[i]];
	}
	for (int y = 0; y < HEIGHT - 1; y++) {
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
	SDL_UpdateTexture(
		sdlTexture,
		nullptr,
		rgbbuffer.data(),
		WIDTH * sizeof(uint32_t)
	);
	int windowW, windowH;
	SDL_GetWindowSize(sdlWindow, &windowW, &windowH);
	int scaleX = windowW / WIDTH;
	int scaleY = windowH / HEIGHT;
	int scale = std::max(1, std::min(scaleX, scaleY));
	SDL_FRect dest;
	dest.w = WIDTH * scale;
	dest.h = HEIGHT * scale;
	dest.x = (windowW - dest.w) / 2;
	dest.y = (windowH - dest.h) / 2;
	SDL_RenderTexture(sdlRenderer, sdlTexture, nullptr, &dest);
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
		if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_MAXIMIZED) {
			int w = event.window.data1;
			int h = event.window.data2;
			int scaleX = w / WIDTH;
			int scaleY = h / HEIGHT;
			int scale = std::max(1, std::min(scaleX, scaleY));
			int targetW = WIDTH * scale;
			int targetH = HEIGHT * scale;

			if (w != targetW || h != targetH) SDL_SetWindowSize(sdlWindow, targetW, targetH);
		}
	}
	return true;
}