#include "MainMenu.hpp"
#include <game/GameUi.hpp>
#include <engine/Input/Input.hpp>
#include <imgui/imgui.h>
#include <engine/Window.hpp>
#include <game/Animation.hpp>
#include <engine/Math/Interpolation.hpp>

// Making a layout by calculating the total height and scaling things so that they fit would make it so that elements (like text) change size in different screens. 
// If things don't fit on a single screen then some sort of scrolling needs to be implemented. The simplest thing is to probably just specify sizes in fractions of full screen size and just don't put too many things.
// The layout builder could return an index into a position array. This index would be stored in the retained structs that would store things like the animationT.

static void addButton(
	std::vector<MainMenu::Button>& buttons, 
	Ui::CenteredHorizontalListLayout& layout, 
	std::string_view text, 
	f32 sizeY) {

	buttons.push_back(MainMenu::Button{
		.id = layout.addBlock(sizeY),
		.text = text,
		.hoverAnimationT = 0.0f,
	});
}

const char* playButtonText = "play";
const char* levelSelectButtonText = "level select";
const char* settingsButtonText = "settings";
const char* editorButtonText = "editor";
const char* exitButtonText = "exit";
static constexpr const char* masterVolumeSliderName = "master";
static constexpr const char* soundEffectVolumeSliderName = "volume";
static constexpr const char* musicVolumeSliderName = "music";
static constexpr const char* backButtonText = "back";
static constexpr const char* mainMenuButtonText = "main menu";

MainMenu::MainMenu() {
	static constexpr f32 buttonSize = 0.03f;
	f32 titleSize = 0.06f;
	f32 smallTitleSize = 0.05f;
	const auto padding = 0.008f;

	auto makeButton = [](const char* text, Ui::CenteredHorizontalListLayout& layout) {
		return MainMenu::Button{
			.id = layout.addBlock(buttonSize),
			.text = text,
			.hoverAnimationT = 0.0f,
		};
	};

	{
		auto& ui = menuUi;
		ui.titleId0 = ui.layout.addBlock(titleSize);
		ui.titleId1 = ui.layout.addBlock(titleSize);

		ui.layout.addPadding(padding); // additional padding
		ui.playButton = makeButton("play", ui.layout);
		ui.layout.addPadding(padding);
		ui.levelSelectButton = makeButton("level select", ui.layout);
		ui.layout.addPadding(padding);
		ui.editorButton = makeButton("editor", ui.layout);
		ui.layout.addPadding(padding);
		ui.settingsButton = makeButton("settings", ui.layout);
		ui.layout.addPadding(padding);
		ui.exitButton = makeButton("exit", ui.layout);
	}
	{
		auto& ui = settingsUi;
		auto sliderInput = [&](std::string_view name) -> SliderInput {
			return SliderInput{
				.name = name,
				.id = ui.layout.addBlock(buttonSize),
			};
		};
		ui.titleId = ui.layout.addBlock(smallTitleSize);
		/*soundSettingsUi.masterVolumeSliderIndex = addSliderInput(masterVolumeSliderName);
		soundSettingsUi.layout.addPadding(padding);*/

		ui.volumeSlider = sliderInput(soundEffectVolumeSliderName);
		ui.layout.addPadding(padding);

		auto toggleButton = [&](const char* text) {
			return ToggleButton{
				.id = ui.layout.addBlock(buttonSize),
				.text = text,
			};
		};

		ui.drawBackgroundsButton = toggleButton("backgrounds");
		ui.fullscreenButton = toggleButton("fullscreen");

		//soundSettingsUi.musicVolumeSliderIndex = addSliderInput(musicVolumeSliderName);
		//soundSettingsUi.layout.addPadding(padding);
		ui.backButton = makeButton("back", ui.layout);
	}
	{
		auto& ui = congratulationsUi;
		ui.titleId0 = ui.layout.addBlock(titleSize);
		ui.layout.addPadding(padding);
		ui.titleId1 = ui.layout.addBlock(0.04f);
		ui.titleId2 = ui.layout.addBlock(0.04f);
		ui.layout.addPadding(padding);
		ui.layout.addPadding(padding);
		ui.layout.addPadding(padding);


		ui.layout.addPadding(padding);
		ui.mainMenuButton = makeButton("main menu", ui.layout);
		ui.layout.addPadding(padding);
		ui.levelSelectButton = makeButton("level select", ui.layout);
	}
}

Ui::RectMinMax MainMenu::buttonRect(const GameRenderer& renderer, const Ui::CenteredHorizontalListLayout& layout, const Button& button) {
	const auto& block = layout.blocks[button.id];
	const auto pos = Vec2(0.0f, block.worldCenter());
	return Ui::centeredTextBoundingRect(pos, block.worldSize(), button.text, renderer.font, renderer);
}

bool MainMenu::button(GameRenderer& renderer, GameAudio& audio, Button& button, Ui::CenteredHorizontalListLayout& layout, Vec2 cursorPos) {
	const auto rect = buttonRect(renderer, layout, button);	
	drawButton(renderer, layout, button);
	return gameButton(audio, rect, button.hoverAnimationT, cursorPos);
}

#define BUTTON(buttonStruct) button(renderer, audio, buttonStruct, ui.layout, cursorPos)

MainMenu::Result MainMenu::update(GameRenderer& renderer, GameAudio& audio) {
	uiSceneBegin(renderer);

	auto result = Result::NONE;
	auto& ui = menuUi;

	ui.layout.update(renderer.gfx.camera);

	const auto cursorPos = Ui::cursorPosUiSpace();

	drawText(renderer, "Non-eucledian", ui.layout, ui.titleId0);
	drawText(renderer, "optics", ui.layout, ui.titleId1);

	if (BUTTON(ui.playButton)) result = Result::PLAY;
	if (BUTTON(ui.levelSelectButton)) result = Result::GO_TO_LEVEL_SELECT;
	if (BUTTON(ui.editorButton)) result = Result::GO_TO_EDITOR;
	if (BUTTON(ui.settingsButton)) result = Result::GO_TO_SETTINGS;
	if (BUTTON(ui.exitButton)) Window::close();

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText(true);
	return result;
}

MainMenu::SettingsResult MainMenu::settingsUpdate(GameRenderer& renderer, GameAudio& audio) {
	uiSceneBegin(renderer);

	auto& ui = settingsUi;

	auto result = SettingsResult::NONE;

	ui.layout.update(renderer.gfx.camera);

	const auto cursorPos = Ui::cursorPosUiSpace();
	
	const auto spacingBetween = 0.01f;

	auto drawTextAlignedRelativeToCenter = [&](std::string_view text, f32 centerY, f32 sizeY, f32 alignmentDirection, f32 hoverT = 0.0f, std::optional<Vec3> color = std::nullopt) -> Ui::RectMinMax {
		const auto info = renderer.font.textInfo(sizeY, text, Constants::additionalTextSpacing);
		Vec2 position(spacingBetween * alignmentDirection, centerY);
		if (alignmentDirection == -1.0f) {
			position.x -= info.size.x;
		}
		position.y -= info.size.y / 2.0f;
		//position.y -= sizeY / 2.0f;
		position.y -= info.bottomY;

		auto center = Vec2(0.0f + (spacingBetween + info.size.x / 2.0f) / renderer.gfx.camera.clipSpaceToWorldSpace()[0][0] * alignmentDirection, centerY);
		//if (alignmentDirection == -1.0f) {
		//	center.x += info.size.x / 4.0f;
		//}
		renderer.gameText(position, sizeY, text, hoverT, color);
		return Ui::centeredTextBoundingRect(center, sizeY, text, renderer.font, renderer);
	};

	auto slider = [&](f32& value, SliderInput& slider, i32 index) {
		const auto& block = settingsUi.layout.blocks[slider.id];
		const auto sizeY = block.worldSize();
		drawTextAlignedRelativeToCenter(slider.name, block.worldCenter(), sizeY, -1.0f);
		{
			// A hacky way to fix text alignment issues.
			static f32 offset = -0.007f;
			//ImGui::SliderFloat("slider offset", &offset, -0.01f, 0.00f);

			const auto sizeY = block.worldSize();
			Vec2 min = Vec2(spacingBetween / renderer.gfx.camera.clipSpaceToWorldSpace()[0][0], block.worldPositionBottomY + offset);
			Vec2 max = Vec2(0.2f, min.y + sizeY);
			const auto mid = (min + max) / 2.0f;

			f32 width = sizeY;
			f32 radius = width / 2.0f;

			const auto sliderMinX = min.x + radius / renderer.gfx.camera.clipSpaceToWorldSpace()[0][0];
			const auto sliderMaxX = max.x - radius / renderer.gfx.camera.clipSpaceToWorldSpace()[0][0];
			Ui::line(
				renderer,
				Vec2(sliderMinX, mid.y),
				Vec2(sliderMaxX, mid.y),
				width,
				Color3::WHITE / 2.0f);

			const auto currentValue = Vec2(lerp(sliderMinX, sliderMaxX, value), mid.y);

			Ui::line(
				renderer,
				Vec2(sliderMinX, mid.y),
				currentValue,
				width,
				Color3::WHITE);

			const auto diskPos = currentValue;
			const auto diskRadius = 0.75f * radius;
			Ui::diskY(renderer, diskPos, diskRadius, Color3::YELLOW);

			if (cursorPos.distanceTo(diskPos) < diskRadius &&
				Input::isMouseButtonDown(MouseButton::LEFT) &&
				!settingsUi.grabbedSlider.has_value()) {

				settingsUi.grabbedSlider = SettingsUi::GrabbedSlider{
					.index = index,
					.grabOffset = diskPos - cursorPos,
				};
			}

			bool active = false;
			if (Input::isMouseButtonHeld(MouseButton::LEFT) && settingsUi.grabbedSlider.has_value()) {
				const auto offset = cursorPos.x - sliderMinX + settingsUi.grabbedSlider->grabOffset.x;
				value = offset / (sliderMaxX - sliderMinX);
				value = std::clamp(value, 0.0f, 1.0f);
				active = true;
			}

			if (Input::isMouseButtonUp(MouseButton::LEFT) && settingsUi.grabbedSlider.has_value()) {
				settingsUi.grabbedSlider = std::nullopt;
			}
			return active;
		}
	};

	auto toggleButton = [&](bool& value, ToggleButton& button) {
		const auto& block = settingsUi.layout.blocks[button.id];
		const auto sizeY = block.worldSize();
		const auto text = value ? "on" : "off";
		const auto color = value ? Color3::GREEN : Color3::RED;

		drawTextAlignedRelativeToCenter(button.text, block.worldCenter(), block.worldSize(), -1.0f);

		// Hacky way to fix alignment issues
		static f32 offsetOn = 0.0f;
		static f32 offsetOff = 0.004f;
		//ImGui::SliderFloat("offsetOn", &offsetOn, 0.0f, 0.01f);
		//ImGui::SliderFloat("offsetOff", &offsetOff, 0.0f, 0.01f);

		auto offset = value ? offsetOn : offsetOff;
		const auto rect = drawTextAlignedRelativeToCenter(text, block.worldCenter() + offset, block.worldSize(), 1.0f, button.hoverAnimationT, color);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);

		bool activated = false;
		if (hovered && Input::isMouseButtonDown(MouseButton::LEFT)) {
			value = !value;
			audio.playSoundEffect(audio.uiClickSound);
			activated = true;
		}
		updateConstantSpeedT(button.hoverAnimationT, buttonHoverAnimationDuration, hovered);
		return activated;
	};

	drawText(renderer, "settings", settingsUi.layout, settingsUi.titleId);

	bool modified = false;
	modified |= toggleButton(settings.graphics.drawBackgrounds, settingsUi.drawBackgroundsButton);
	modified |= toggleButton(settings.graphics.fullscreen, settingsUi.fullscreenButton);
	modified |= slider(settings.audio.soundEffectVolume, settingsUi.volumeSlider, 0);

	if (modified) {
		result = SettingsResult::PROPAGATE_SETTINGS;
	}

	if (button(renderer, audio, ui.backButton, ui.layout, cursorPos)) {
		result = SettingsResult::GO_BACK;
	}

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText(true);
	return result;
}

void MainMenu::drawText(GameRenderer& r, std::string_view text, const Ui::CenteredHorizontalListLayout& layout, i32 id, f32 hoverT) {
	auto& block = layout.blocks[id];
	const auto center = Vec2(r.gfx.camera.pos.x, block.worldCenter());
	r.gameTextCentered(center, block.worldSize(), text, hoverT);
}

void MainMenu::drawButton(GameRenderer& r, const Ui::CenteredHorizontalListLayout& layout, const Button& button) {
	drawText(r, button.text, layout, button.id, button.hoverAnimationT);
}

void MainMenu::uiSceneBegin(GameRenderer& renderer) {
	renderer.textColorRng.seed(renderer.textColorRngSeed);
	renderer.renderTilingBackground();
}

MainMenu::CongratulationsScreenResult MainMenu::congratulationsScreenUpdate(GameRenderer& renderer, GameAudio& audio) {
	uiSceneBegin(renderer);

	auto result = CongratulationsScreenResult::NONE;

	auto& ui = congratulationsUi;

	ui.layout.update(renderer.gfx.camera);


	const auto cursorPos = Ui::cursorPosUiSpace();

	// https://english.stackexchange.com/questions/240468/whats-the-difference-between-you-have-completed-the-task-and-you-com
	drawText(renderer, "Congratulations", ui.layout, ui.titleId0);
	drawText(renderer, "you've completed", ui.layout, ui.titleId1);
	drawText(renderer, "all the levels", ui.layout, ui.titleId2);

	if (BUTTON(ui.levelSelectButton)) result = CongratulationsScreenResult::GO_TO_LEVEL_SELECT; 
	if (BUTTON(ui.mainMenuButton)) result = CongratulationsScreenResult::GO_TO_MAIN_MENU;

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText(true);
	return result;
}

void MainMenu::Button::update(bool hovered) {
	updateConstantSpeedT(hoverAnimationT, buttonHoverAnimationDuration, hovered);
}
