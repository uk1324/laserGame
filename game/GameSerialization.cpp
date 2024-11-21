#include <game/GameSerialization.hpp>
#include <game/LevelData.hpp>
#include <engine/dependencies/Json/JsonPrinter.hpp>
#include <game/Level.hpp>
#include <JsonFileIo.hpp>
#include <RefOptional.hpp>
#include <fstream>

// TODO: Maybe validate the inputs. For example calmping the values to a certain range.
// TODO: Maybe put the try catch only around loading of each element and continue if there is an error.

Json::Value::ArrayType& makeArrayAt(Json::Value& v, std::string_view at) {
	return (v[std::string(at)] = Json::Value::emptyArray()).array();
}

Json::Value saveGameLevelToJson(GameEntities& e) {
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

		for (const auto& wall : e.walls) {
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
		for (const auto& laser : e.lasers) {
			const auto levelLaser = LevelLaser{
				.position = laser->position,
				.angle = laser->angle,
				.color = laser->color,
				.positionLocked = laser->positionLocked
			};
			jsonLasers.push_back(toJson(levelLaser));
		}
	}

	{
		auto& jsonMirrors = makeArrayAt(level, levelMirrorsName);
		for (const auto& e : e.mirrors) {
			auto convertWallType = [](EditorMirrorWallType type) {
				switch (type) {
					using enum EditorMirrorWallType;
				case REFLECTING: return LevelMirrorWallType::REFLECTING;
				case ABSORBING: return LevelMirrorWallType::ABSORBING;
				}
				return LevelMirrorWallType::REFLECTING;
			};

			const auto levelMirror = LevelMirror{
				.center = e->center,
				.normalAngle = e->normalAngle,
				.length = e->length,
				.positionLocked = e->positionLocked,
				.wallType = convertWallType(e->wallType)
			};
			jsonMirrors.push_back(toJson(levelMirror));
		}
	}

	{
		auto& jsonTargets = makeArrayAt(level, levelTargetsName);
		for (const auto& e : e.targets) {
			const auto& levelTarget = LevelTarget{
				.position = e->position,
				.radius = e->radius
			};
			jsonTargets.push_back(toJson(levelTarget));
		}
	}

	{
		auto& jsonPortalPairs = makeArrayAt(level, levelPortalPairsName);
		for (const auto& e : e.portalPairs) {
			auto convertWallType = [](EditorPortalWallType type) {
				switch (type) {
					using enum EditorPortalWallType;
				case REFLECTING: return LevelPortalWallType::REFLECTING;
				case ABSORBING: return LevelPortalWallType::ABSORBING;
				case PORTAL: return LevelPortalWallType::PORTAL;
				}
				return LevelPortalWallType::PORTAL;
			};

			auto convert = [&convertWallType](const EditorPortal& portal) {
				return LevelPortal{
					.center = portal.center,
					.normalAngle = portal.normalAngle,
					.wallType = convertWallType(portal.wallType),
					.positionLocked = portal.positionLocked,
					.rotationLocked = portal.rotationLocked,
				};
			};

			const auto levelPortalPair = LevelPortalPair{
				.portal0 = convert(e->portals[0]),
				.portal1 = convert(e->portals[1])
			};
			jsonPortalPairs.push_back(toJson(levelPortalPair));
		}
	}

	{
		auto& jsonTriggers = makeArrayAt(level, levelTriggersName);
		for (const auto& e : e.triggers) {
			const auto levelE = LevelTrigger{
				.position = e->position,
				.color = e->color,
				.index = e->index,
			};
			jsonTriggers.push_back(toJson(levelE));
		}
	}

	{
		auto& jsonDoors = makeArrayAt(level, levelDoorsName);
		for (const auto& e : e.doors) {
			const auto levelE = LevelDoor{
				.endpoint0 = e->endpoints[0],
				.endpoint1 = e->endpoints[1],
				.triggerIndex = e->triggerIndex,
				.openByDefault = e->openByDefault
			};
			jsonDoors.push_back(toJson(levelE));
		}
	}

	{
		LevelLockedCells levelLockedCells{
			.ringCount = e.lockedCells.ringCount,
			.segmentCount = e.lockedCells.segmentCount
		};
		for (const auto& cell : e.lockedCells.cells) {
			levelLockedCells.cells.push_back(cell);
		}
		level[levelLockedCellsName] = toJson(levelLockedCells);

	}
	return level;
}

bool trySaveGameLevelToFile(GameEntities& e, std::string_view path) {
	const auto level = saveGameLevelToJson(e);
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

bool tryLoadGameLevelFromJson(GameEntities& e, const Json::Value& json) {
	try {

		{
			const auto& jsonWalls = json.at(levelWallsName).array();
			for (const auto& jsonWall : jsonWalls) {
				const auto levelWall = fromJson<LevelWall>(jsonWall);
				auto wall = e.walls.create();

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
				auto laser = e.lasers.create();
				laser.entity = EditorLaser{
					.position = levelLaser.position,
					.angle = levelLaser.angle,
					.color = levelLaser.color,
					.positionLocked = levelLaser.positionLocked,
				};
			}
		}

		if (const auto& jsonMirrors = tryArrayAt(json, levelMirrorsName); jsonMirrors.has_value()) {
			for (const auto& jsonMirror : *jsonMirrors) {
				auto convertWallType = [](LevelMirrorWallType type) {
					switch (type) {
						using enum LevelMirrorWallType;
					case REFLECTING: return EditorMirrorWallType::REFLECTING;
					case ABSORBING: return EditorMirrorWallType::ABSORBING;
					}
					return EditorMirrorWallType::REFLECTING;
				};

				const auto levelMirror = fromJson<LevelMirror>(jsonMirror);
				auto mirror = e.mirrors.create();
				mirror.entity = EditorMirror(levelMirror.center, levelMirror.normalAngle, levelMirror.length, levelMirror.positionLocked, convertWallType(levelMirror.wallType));
			}
		}

		if (const auto& jsonTargets = tryArrayAt(json, levelTargetsName); jsonTargets.has_value()) {
			for (const auto& jsonTarget : *jsonTargets) {
				const auto levelTarget = fromJson<LevelTarget>(jsonTarget);
				auto target = e.targets.create();
				target.entity = EditorTarget{
					.position = levelTarget.position,
					.radius = levelTarget.radius
				};
			}
		}

		if (const auto& jsonPortalPairs = tryArrayAt(json, levelPortalPairsName); jsonPortalPairs.has_value()) {
			for (const auto& jsonPortalPair : *jsonPortalPairs) {
				const auto levelPortalPair = fromJson<LevelPortalPair>(jsonPortalPair);
				auto portalPair = e.portalPairs.create();
				auto convertPortalWallType = [](LevelPortalWallType type) {
					switch (type) {
						using enum LevelPortalWallType;
					case PORTAL: return EditorPortalWallType::PORTAL;
					case REFLECTING: return EditorPortalWallType::REFLECTING;
					case ABSORBING: return EditorPortalWallType::ABSORBING;
					}
					return EditorPortalWallType::PORTAL;
				};
				auto convertPortal = [&convertPortalWallType](const LevelPortal& levelPortal) {
					return EditorPortal{
						.center = levelPortal.center,
						.normalAngle = levelPortal.normalAngle,
						.wallType = convertPortalWallType(levelPortal.wallType),
						.positionLocked = levelPortal.positionLocked,
						.rotationLocked = levelPortal.rotationLocked,
					};
				};
				portalPair.entity = EditorPortalPair{
					.portals = { convertPortal(levelPortalPair.portal0), convertPortal(levelPortalPair.portal1) }
				};
			}
		}

		if (const auto& jsonTriggers = tryArrayAt(json, levelTriggersName); jsonTriggers.has_value()) {
			for (const auto& jsonTrigger : *jsonTriggers) {
				const auto levelTrigger = fromJson<LevelTrigger>(jsonTrigger);
				auto trigger = e.triggers.create();
				trigger.entity = EditorTrigger{
					.position = levelTrigger.position,
					.color = levelTrigger.color,
					.index = levelTrigger.index,
				};
			}
		}

		if (const auto& jsonDoors = tryArrayAt(json, levelDoorsName); jsonDoors.has_value()) {
			for (const auto& jsonDoor : *jsonDoors) {
				const auto levelDoor = fromJson<LevelDoor>(jsonDoor);
				auto door = e.doors.create();
				door.entity = EditorDoor{
					.endpoints = { levelDoor.endpoint0, levelDoor.endpoint1 },
					.triggerIndex = levelDoor.triggerIndex,
					.openByDefault = levelDoor.openByDefault
				};
			}
		}

		if (json.contains(levelLockedCellsName)) {
			auto levelLockedCells = fromJson<LevelLockedCells>(json.at(levelLockedCellsName));
			e.lockedCells = LockedCells{
				.cells = std::move(levelLockedCells.cells),
				.segmentCount = levelLockedCells.segmentCount,
				.ringCount = levelLockedCells.ringCount,
			};
		}

	} catch (const Json::Value::Exception&) {
		return false;
	}

	return true;
}

bool tryLoadGameLevelFromFile(GameEntities& e, std::string_view path) {
	const auto jsonOpt = tryLoadJsonFromFile(path);
	if (!jsonOpt.has_value()) {
		return false;
	}
	return tryLoadGameLevelFromJson(e, *jsonOpt);
}
