#pragma once

#include <game/EditorEntities.hpp>
#include <Json/JsonValue.hpp>

Json::Value saveGameLevelToJson(GameEntities& e);
bool trySaveGameLevelToFile(GameEntities& e, std::string_view path);
bool tryLoadGameLevelFromJson(GameEntities& e, const Json::Value& json);
bool tryLoadGameLevelFromFile(GameEntities& e, std::string_view path);