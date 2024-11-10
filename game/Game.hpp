#pragma once

#include <game/GameRenderer.hpp>
#include <game/GameEntities.hpp>

struct Game {
	void update(GameRenderer& renderer);

	void loadLevel(std::string_view path);
};