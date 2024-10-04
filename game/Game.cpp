#include "Game.hpp"
#include <JsonFileIo.hpp>
#include <game/Level.hpp>
#include <game/LevelData.hpp>

void Game::update(GameRenderer& renderer) {

	for (const auto& wall : walls) {
		renderer.wall(wall.endpoints[0], wall.endpoints[1]);
	}
	renderer.renderWalls();
}

void Game::loadLevel(std::string_view path) {
	const auto json = jsonFromFile(path);
	walls.clear();
	try {

		{
			const auto& wallsJson = json.at(levelWallsName).array();
			for (const auto& wallJson : wallsJson) {
				const auto& wallLevel = fromJson<LevelWall>(wallJson);
				walls.push_back(Wall{ .endpoints = { wallLevel.e0, wallLevel.e1 } });
			}
		}

	} catch (const Json::Value::Exception&) {
		ASSERT_NOT_REACHED();
	}
}
