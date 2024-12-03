#include "Game.hpp"
#include <game/GameSerialization.hpp>
#include <game/GameUi.hpp>
#include <engine/Math/Interpolation.hpp>
#include <game/Animation.hpp>
#include <imgui/imgui.h>
#include <engine/Input/Input.hpp>

Game::Game() {
	tryLoadLevel("./generated/test");
}

Game::Result Game::update(GameRenderer& renderer, GameAudio& audio) {
	Result result = ResultNone();

	bool cursorCaptured = false;

	auto cursorPos = Input::cursorPosClipSpace() * renderer.gfx.camera.clipSpaceToWorldSpace();

	const bool cursorExact = false;
	const bool enforceConstrains = true;

	laserGrabTool.update(e.lasers, std::nullopt, cursorPos, cursorCaptured, cursorExact, enforceConstrains);
	mirrorGrabTool.update(e.mirrors, std::nullopt, cursorPos, cursorCaptured, cursorCaptured, enforceConstrains);
	portalGrabTool.update(e.portalPairs, std::nullopt, cursorPos, cursorCaptured, cursorCaptured, enforceConstrains);

	const auto objectsInValidState = areObjectsInValidState();
	s.update(e, objectsInValidState);
	
	bool allTargetsActivated = true;
	for (const auto& target : e.targets) {
		if (!target->activated) {
			allTargetsActivated = false;
		}
	}
	const auto allTasksComplete = objectsInValidState && allTargetsActivated;
	// Introducing a delay to prevent issue with doors having a delay in closing.
	if (allTasksComplete) {
		if (timeForWhichAllTasksHaveBeenCompleted.has_value()) {
			(*timeForWhichAllTasksHaveBeenCompleted) += Constants::dt;
		} else {
			timeForWhichAllTasksHaveBeenCompleted = 0.0f;
		}
	} else {
		timeForWhichAllTasksHaveBeenCompleted = std::nullopt;
	}
	const auto levelComplete = 
		allTasksComplete && 
		timeForWhichAllTasksHaveBeenCompleted.has_value() && 
		timeForWhichAllTasksHaveBeenCompleted > 0.5f &&
		!isEditorPreviewLevelLoaded();

	if (!objectsInValidState && !previousFrameInvalidState && timeSincePlayedInvalidState > 0.1f) {
		timeSincePlayedLevelComplete = 0.0f;
		audio.playSoundEffect(audio.errorSound);
	}
	timeSincePlayedInvalidState += Constants::dt;

	timeSincePlayedLevelComplete += Constants::dt;
	if (levelComplete && !previousFrameLevelComplete && timeSincePlayedLevelComplete > 0.1f) {
		//audio.playSoundEffect(audio.levelCompleteSound);
		timeSincePlayedLevelComplete = 0.0f;
	}

	updateConstantSpeedT(invalidGameStateAnimationT, 0.2f, !objectsInValidState);

	f32 doorOpeningT = 0.0f;
	for (const auto& door : e.doors) {
		doorOpeningT = std::max(door->openingT, doorOpeningT);
	}

	{
		static f32 riseAndFallLength = 0.4f;
		//ImGui::SliderFloat("riseAndFallLength", &riseAndFallLength, 0.0f, 0.5f);
		f32 volume;
		const auto t = doorOpeningT;
		if (t < riseAndFallLength) {
			volume = smoothstep(lerp(0.0f, 1.0f, t / riseAndFallLength));
		} else if (t < 1.0f - riseAndFallLength) {
			volume = 1.0f;
		} else {
			volume = smoothstep(lerp(1.0f, 0.0f, (t - (1.0f - riseAndFallLength)) / riseAndFallLength));
		}
		audio.setSoundEffectSourceVolume(audio.doorOpeningSource, volume);
	}

	if (s.anyTargetsTurnedOn) {
		audio.playSoundEffect(audio.targetOnSound);
	}

	previousFrameLevelComplete = levelComplete;
	previousFrameInvalidState = !objectsInValidState;

	renderer.renderClear();
	renderer.render(e, s, false, invalidGameStateAnimationT);

	if (!isEditorPreviewLevelLoaded()) {
		const auto uiResult = updateUi(renderer, audio, levelComplete);
		if (uiResult.has_value()) {
			result = *uiResult;
		}
	}

	return result;
}

std::optional<Game::Result> Game::updateUi(GameRenderer& r, GameAudio& audio, bool levelComplete) {
	std::optional<Result> result;
	auto uiCursorPos = Ui::cursorPosUiSpace();

	{
		f32 xSize = 0.2f;
		f32 ySize = 0.5f * Ui::xSizeToYSize(r, xSize);
		Vec2 size(xSize, ySize);

		const auto anchor = Vec2(0.5f, -0.5f);

		const auto pos = Ui::rectPositionRelativeToCorner(anchor, size, Ui::equalSizeReativeToX(r, 0.01f));

		updateConstantSpeedT(goToNextLevelButtonActiveT, 0.3f, levelComplete);

		auto hover = Ui::isPointInRectPosSize(pos, size, uiCursorPos);
		// Don't do hover highlighting when the level is not complete so the player doesn't try to press a button that doesn't do anything. Could instead just not have the button there when the level is not complete.
		updateConstantSpeedT(goToNextLevelButtonHoverT, buttonHoverAnimationDuration, levelComplete && hover);

		goToNextLevelButtonActiveT = std::clamp(goToNextLevelButtonActiveT, 0.0f, 1.0f);

		const auto color1 = lerp(Color3::WHITE / 15.0f, Color3::WHITE / 10.0f, goToNextLevelButtonHoverT);
		const auto color2 = lerp(1.5f * color1, Color3::WHITE, goToNextLevelButtonActiveT);
		//Ui::rectPosSizeFilled(r, pos, size, Vec4(color1, goToNextLevelButtonActiveT));

		if (levelComplete && hover && Input::isMouseButtonDown(MouseButton::LEFT)) {
			result = ResultGoToNextLevel{};
		}

		const auto padding = size * 0.1f;
		const auto insideSize = size - padding * 2.0f;
		const auto min = pos - size / 2.0f + padding;
		const auto max = pos + size / 2.0f - padding;
		const auto offset = max.x - Ui::ySizeToXSize(r, insideSize.y / 2.0f * sqrt(2.0f));
		const auto color2a = Vec4(color2, goToNextLevelButtonActiveT);

		r.glowingArrow(Aabb(Ui::posToWorldSpace(r, min), Ui::posToWorldSpace(r, max)), goToNextLevelButtonHoverT, goToNextLevelButtonActiveT);
		// TODO: Could actually check if the cursor is in the arrow not just the rect.
	}

	{
		r.textColorRng.seed(r.textColorRngSeed);
		const auto mainMenuButtonText = "main menu";
		const auto height = navigationButtonTextHeight;
		const auto padding = Ui::equalSizeReativeToX(r, 0.01f);
		{
			auto size = Ui::textBoundingRectSize(height, mainMenuButtonText, r.font, r);
			auto pos = Ui::rectPositionRelativeToCorner(Vec2(-0.5f, 0.5f), size, padding);
			r.gameTextCentered(pos, height, mainMenuButtonText, mainMenuButton.hoverAnimationT);
			if (gameButtonPosSize(audio, pos, size, mainMenuButton.hoverAnimationT, uiCursorPos)) {
				result = ResultGoToMainMenu{};
			}
		}

		{
			const auto skipButtonText = "skip";
			auto size = Ui::textBoundingRectSize(height, skipButtonText, r.font, r);
			auto pos = Ui::rectPositionRelativeToCorner(Vec2(0.5f, 0.5f), size, padding);
			r.gameTextCentered(pos, height, skipButtonText, skipButton.hoverAnimationT);
			if (gameButtonPosSize(audio, pos, size, skipButton.hoverAnimationT, uiCursorPos)) {
				result = ResultSkipLevel{};
			}
		}

		r.renderGameText(false);
	}
	return result;
}

void Game::onSwitchToGame(GameAudio& a) {
	a.doorOpeningSource.source.stop();
	a.doorOpeningSource.source.setBuffer(a.doorOpenSound);
	a.doorOpeningSource.source.setLoop(true);
	a.doorOpeningSource.source.play();
	a.setSoundEffectSourceVolume(a.doorOpeningSource, 0.0f);
}

void Game::onSwitchFromGame(GameAudio& audio) {
	for (const auto& source : audio.soundEffectSources) {
		source->source.stop();
	}
}

void Game::reset() {
	goToNextLevelButtonActiveT = 0.0f;
	e.reset();
	previousFrameLevelComplete = false;
}

bool Game::areObjectsInValidState() {
	std::vector<StereographicSegment> movableObjects;
	for (const auto& mirror : e.mirrors) {
		const auto endpoints = mirror->calculateEndpoints();
		movableObjects.push_back(StereographicSegment(endpoints[0], endpoints[1]));
	}
	for (const auto& portalPair : e.portalPairs) {
		for (const auto& portal : portalPair->portals) {
			const auto endpoints = portal.endpoints();
			movableObjects.push_back(StereographicSegment(endpoints[0], endpoints[1]));
		}
	}
	std::vector<StereographicSegment> staticObjects;
	for (const auto& wall : e.walls) {
		staticObjects.push_back(StereographicSegment(wall->endpoints[0], wall->endpoints[1]));
	}
	for (const auto& door : e.doors) {
		const auto segments = door->segments();
		for (const auto& segment : segments) {
			staticObjects.push_back(StereographicSegment(segment.endpoints[0], segment.endpoints[1]));
		}
	}

	auto collision = [](const StereographicSegment& a, const StereographicSegment& b) {
		return stereographicSegmentVsStereographicSegmentIntersection(a, b).size() > 0;
	};
	for (i32 i = 0; i < i32(movableObjects.size()) - 1; i++) {
		const auto& a = movableObjects[i];
		for (i32 j = i + 1; j < movableObjects.size(); j++) {
			const auto& b = movableObjects[j];
			if (collision(a, b)) {
				return false;
			}
		}
	}
	for (const auto& a : movableObjects) {
		for (const auto& b : staticObjects) {
			if (collision(a, b)) {
				return false;
			}
		}
	}

	for (const auto& cell : e.lockedCells.cells) {
		const auto cellBounds = e.lockedCells.cellBounds(cell);

		for (const auto& laser : e.lasers) {
			if (cellBounds.containsPoint(laser->position)) {
				return false;
			}
		}

		for (const auto& object : movableObjects) {
			for (const auto& endpoint : object.endpoints) {
				if (cellBounds.containsPoint(endpoint)) {
					return false;
				}
			}
			auto stereographicSegmentVsCircleArcCollision = [&](f32 r) {
				const auto intersections = stereographicSegmentVsCircleIntersection(object.line, object.endpoints[0], object.endpoints[1], Circle(Vec2(0.0f), r));
				for (const auto& intersection : intersections) {
					const auto a = angleToRangeZeroTau(intersection.angle());
					if (a >= cellBounds.minA && a >= cellBounds.maxA) {
						return true;
					}
				}
				return false;
			};

			if (stereographicSegmentVsCircleArcCollision(cellBounds.minR)) {
				return false;
			}
			if (stereographicSegmentVsCircleArcCollision(cellBounds.maxR)) {
				return false;
			}
			const auto v00 = Vec2::fromPolar(cellBounds.minA, cellBounds.minR);
			const auto v10 = Vec2::fromPolar(cellBounds.minA, cellBounds.maxR);
			if (stereographicSegmentVsSegmentCollision(object, v00, v10)) {
				return false;
			}
			const auto v01 = Vec2::fromPolar(cellBounds.maxA, cellBounds.minR);
			const auto v11 = Vec2::fromPolar(cellBounds.maxA, cellBounds.maxR);
			if (stereographicSegmentVsSegmentCollision(object, v11, v01)) {
				return false;
			}
		}

	}

	return true;
}

bool Game::tryLoadLevel(std::string_view path) {
	reset();
	return tryLoadGameLevelFromFile(e, path);
}

bool Game::tryLoadLevelFromJson(const Json::Value& json) {
	reset();
	return tryLoadGameLevelFromJson(e, json);
}

bool Game::tryLoadGameLevel(const Levels& levels, LevelIndex levelIndex) {
	const auto& info = levels.tryGetLevelInfo(levelIndex);
	if (!info.has_value()) {
		currentLevel = std::nullopt;
		return false;
	}
	if (tryLoadLevel(info->path)) {
		currentLevel = levelIndex;
		return true;
	}
	currentLevel = std::nullopt;
	return false;
}

bool Game::tryLoadEditorPreviewLevel(const Json::Value& json) {
	const auto result = tryLoadLevelFromJson(json);
	currentLevel = std::nullopt;
	return result;
}

bool Game::isEditorPreviewLevelLoaded() const {
	return currentLevel == std::nullopt;
}
