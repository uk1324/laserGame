#include <engine/Engine.hpp>
#include <engine/EngineUpdateLoop.hpp>
#include <game/MainLoop.hpp>
#include <game/Paths.hpp>
#include <engine/Audio/Audio.hpp>

int main() {
 	Engine::initAll(Window::Settings{
		.maximized = true,
		.multisamplingSamplesPerPixel = 16
	}, FONT_PATH);
	Audio::init();

	EngineUpdateLoop updateLoop(60.0f);
	MainLoop mainLoop;

	while (updateLoop.isRunning()) {
		mainLoop.update();
	}

	Audio::deinit();
	Engine::terminateAll();
}

#ifdef FINAL_RELEASE

#ifdef WIN32
#include <Windows.h>
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	return main();
}
#endif

#endif