#include <game/Editor.hpp>
#include <game/LevelData.hpp>
#include <engine/dependencies/Json/JsonPrinter.hpp>
#include <game/Level.hpp>
#include <JsonFileIo.hpp>
#include <RefOptional.hpp>
#include <fstream>

// TODO: Maybe validate the inputs. For example calmping the values to a certain range.

//Json::Value::ArrayType& makeArrayAt(Json::Value& v, std::string_view at) {
Json::Value::ArrayType& makeArrayAt(Json::Value& v, std::string_view at) {
	return (v[std::string(at)] = Json::Value::emptyArray()).array();
}

bool Editor::trySaveLevel(std::string_view path) {
	auto level = Json::Value::emptyObject();

	{
		auto& jsonWalls = makeArrayAt(level, levelWallsName);

		auto wallTypeToLevelWallType = [](EditorWallType type) {
			switch (type) {
				using enum EditorWallType;
			case REFLECTING: return LevelWallType::REFLECTING;
			case ABSORBING: return LevelWallType::ABSORBING;
			}
			return LevelWallType::ABSORBING;
			};

		for (const auto& wall : walls) {
			const auto levelWall = LevelWall{
				.endpoint0 = wall->endpoints[0],
				.endpoint1 = wall->endpoints[1],
				.type = wallTypeToLevelWallType(wall->type)
			};
			jsonWalls.push_back(toJson(levelWall));
		}
	}

	{
		auto& jsonLasers = makeArrayAt(level, levelLasersName);
		for (const auto& laser : lasers) {
			const auto levelLaser = LevelLaser{
				.position = laser->position,
				.angle = laser->angle,
				.color = laser->color,
			};
			jsonLasers.push_back(toJson(levelLaser));
		}
	}

	{
		auto& jsonMirrors = makeArrayAt(level, levelMirrorsName);
		for (const auto& e : mirrors) {
			const auto levelMirror = LevelMirror{
				.center = e->center,
				.normalAngle = e->normalAngle,
				.length = e->length,
			};
			jsonMirrors.push_back(toJson(levelMirror));
		}
	}

	{
		auto& jsonTargets = makeArrayAt(level, levelTargetsName);
		for (const auto& e : targets) {
			const auto& levelTarget = LevelTarget{
				.position = e->position,
				.radius = e->radius
			};
			jsonTargets.push_back(toJson(levelTarget));
		}
	}

	std::ofstream file(path.data());
	Json::print(file, level);

	if (file.bad()) {
		return false;
	}
	return true;
}

std::optional<const Json::Value::ArrayType&> tryArrayAt(const Json::Value& v, std::string_view name) {
	const auto nameStr = std::string(name);
	if (!v.contains(nameStr)) {
		return std::nullopt;
	}
	const auto& a = v.at(nameStr);
	if (!a.isArray()) {
		return std::nullopt;
	}
	return a.array();
}

bool Editor::tryLoadLevel(std::string_view path) {
	const auto jsonOpt = tryLoadJsonFromFile(path);
	if (!jsonOpt.has_value()) {
		return false;
	}
	const auto& json = *jsonOpt;
	try {

		{
			const auto& jsonWalls = json.at(levelWallsName).array();
			for (const auto& jsonWall : jsonWalls) {
				const auto levelWall = fromJson<LevelWall>(jsonWall);
				auto wall = walls.create();

				auto levelWallTypeToWallType = [](LevelWallType type) {
					switch (type) {
						using enum LevelWallType;
					case REFLECTING: return EditorWallType::REFLECTING;
					case ABSORBING: return EditorWallType::ABSORBING;
					}
					return EditorWallType::ABSORBING;
				};

				wall.entity = EditorWall{
					.endpoints = { levelWall.endpoint0, levelWall.endpoint1 },
					.type = levelWallTypeToWallType(levelWall.type)
				};
			}
		}

		if (const auto jsonLasers = tryArrayAt(json, levelLasersName); jsonLasers.has_value()) {
			for (const auto& jsonLaser : *jsonLasers) {
				const auto levelLaser = fromJson<LevelLaser>(jsonLaser);
				auto laser = lasers.create();
				laser.entity = EditorLaser{
					.position = levelLaser.position,
					.angle = levelLaser.angle,
					.color = levelLaser.color,
				};
			}
		}

		if (const auto& jsonMirrors = tryArrayAt(json, levelMirrorsName); jsonMirrors.has_value()) {
			for (const auto& jsonMirror : *jsonMirrors) {
				const auto levelMirror = fromJson<LevelMirror>(jsonMirror);
				auto mirror = mirrors.create();
				mirror.entity = EditorMirror(levelMirror.center, levelMirror.normalAngle, levelMirror.length);
			}
		}

		if (const auto& jsonTargets = tryArrayAt(json, levelTargetsName); jsonTargets.has_value()) {
			for (const auto& jsonTarget : *jsonTargets) {
				const auto levelTarget = fromJson<LevelTarget>(jsonTarget);
				auto target = targets.create();
				target.entity = EditorTarget{
					.position = levelTarget.position,
					.radius = levelTarget.radius
				};
			}
		}

	} catch (const Json::Value::Exception&) {
		return false;
	}

	return true;
}