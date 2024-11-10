#pragma once

#include <game/EditorEntities.hpp>

bool trySaveGameLevel(GameEntities& e, std::string_view path);
bool tryLoadGameLevel(GameEntities& e, std::string_view path);