#pragma once
#include <SDL3/SDL.h>
#include <array>

class Renderer {
public:
	Renderer();
	~Renderer();
	static constexpr int WIDTH = 160;
	static constexpr int HEIGHT = 144;
	bool init(const char* title, int scale);
	void render(const std::array<uint32_t, WIDTH* HEIGHT>& framebuffer);
	bool procEvents();
	void idle();
	SDL_Window* getWindow() { return sdlWindow; }
private:
	int wWidth = WIDTH;
	int wHeight = HEIGHT;
	SDL_Window* sdlWindow = nullptr;
	SDL_Renderer* sdlRenderer = nullptr;
	SDL_Texture* sdlTexture = nullptr;
public:
	std::array<uint32_t, WIDTH * HEIGHT> rgbbuffer;
	std::array<uint32_t, WIDTH * HEIGHT> prevFrame{};
};