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
static constexpr const char* soundEffectVolumeSliderName = "sfx volume";
static constexpr const char* musicVolumeSliderName = "music";
static constexpr const char* backButtonText = "back";
static constexpr const char* mainMenuButtonText = "main menu";

MainMenu::MainMenu() {
	static constexpr f32 buttonSize = 0.03f;
	f32 titleSize = 0.06f;
	f32 smallTitleSize = 0.05f;
	const auto padding = 0.008f;

	{
		menuUi.titleId0 = menuUi.layout.addBlock(titleSize);
		menuUi.titleId1 = menuUi.layout.addBlock(titleSize);

		std::string_view buttonNames[]{
			playButtonText,
			levelSelectButtonText,
			editorButtonText,
			settingsButtonText,
			exitButtonText,
		};
		menuUi.layout.addPadding(padding); // additional padding
		for (const auto& name : buttonNames) {
			menuUi.layout.addPadding(padding);
			addButton(menuUi.buttons, menuUi.layout, name, buttonSize);
		}
	}
	{
		auto addSliderInput = [&](std::string_view name) -> i32 {
			i32 index = i32(settingsUi.sliderInputs.size());
			settingsUi.sliderInputs.push_back(SliderInput{
				.name = name,
				.id = settingsUi.layout.addBlock(buttonSize),
			});
			return index;
		};
		settingsUi.titleId = settingsUi.layout.addBlock(smallTitleSize);
		/*soundSettingsUi.masterVolumeSliderIndex = addSliderInput(masterVolumeSliderName);
		soundSettingsUi.layout.addPadding(padding);*/

		settingsUi.soundEffectVolumeSliderIndex = addSliderInput(soundEffectVolumeSliderName);
		settingsUi.layout.addPadding(padding);

		settingsUi.drawBackgroundsButton = ToggleButton{
			.id = settingsUi.layout.addBlock(buttonSize),
			.text = "backgrounds",
		};

		//soundSettingsUi.musicVolumeSliderIndex = addSliderInput(musicVolumeSliderName);
		//soundSettingsUi.layout.addPadding(padding);

		addButton(settingsUi.buttons, settingsUi.layout, backButtonText, buttonSize);
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
		std::string_view buttonNames[]{
			mainMenuButtonText,
			levelSelectButtonText,
		};
		for (const auto& name : buttonNames) {
			ui.layout.addPadding(padding);
			addButton(ui.buttons, ui.layout, name, buttonSize);
		}
	}
}

Ui::RectMinMax MainMenu::buttonRect(const GameRenderer& renderer, const Ui::CenteredHorizontalListLayout& layout, const Button& button) {
	const auto& block = layout.blocks[button.id];
	const auto pos = Vec2(0.0f, block.worldCenter());
	return Ui::centeredTextBoundingRect(pos, block.worldSize(), button.text, renderer.font, renderer);
}

MainMenu::Result MainMenu::update(GameRenderer& renderer) {
	renderer.renderTilingBackground();
	renderer.textColorRng.seed(renderer.textColorRngSeed);

	auto result = Result::NONE;

	menuUi.layout.update(renderer.gfx.camera);


	const auto cursorPos = Ui::cursorPosUiSpace();

	for (auto& button : menuUi.buttons) {
		const auto rect = buttonRect(renderer, menuUi.layout, button);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);
		button.update(hovered);

		if (hovered && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (button.text == exitButtonText) {
				Window::close();
			} else if (button.text == levelSelectButtonText) {
				result = Result::GO_TO_LEVEL_SELECT;
			} else if (button.text == settingsButtonText) {
				result = Result::GO_TO_SOUND_SETTINGS;
			} else if (button.text == editorButtonText) {
				result = Result::GO_TO_EDITOR;
			} else if (button.text == playButtonText) {
				result = Result::PLAY;
			}
		}
	}

	drawText(renderer, "Non-eucledian", menuUi.layout, menuUi.titleId0);
	drawText(renderer, "lasers", menuUi.layout, menuUi.titleId1);
	for (const auto& button : menuUi.buttons) {
		drawButton(renderer, menuUi.layout, button);
	}

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText();
	return result;
}

MainMenu::SoundSettingsResult MainMenu::soundSettingsUpdate(GameRenderer& renderer) {
	renderer.textColorRng.seed(renderer.textColorRngSeed);

	auto result = SoundSettingsResult::NONE;

	settingsUi.layout.update(renderer.gfx.camera);

	const auto cursorPos = Ui::cursorPosUiSpace();

	for (auto& button : settingsUi.buttons) {
		const auto rect = buttonRect(renderer, settingsUi.layout, button);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);
		button.update(hovered);
		//Ui::rect(renderer, rect, 0.01f, Color3::WHITE);

		if (Ui::isPointInRect(rect, cursorPos) && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (button.text == backButtonText) {
				result = SoundSettingsResult::GO_BACK;
			}
		}
	}

	drawText(renderer, "settings", settingsUi.layout, settingsUi.titleId);
	
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

	for (i32 sliderI = 0; sliderI < settingsUi.sliderInputs.size(); sliderI++) {
		const auto& slider = settingsUi.sliderInputs[sliderI];
		const auto& block = settingsUi.layout.blocks[slider.id];
		const auto sizeY = block.worldSize();
		drawTextAlignedRelativeToCenter(slider.name, block.worldCenter(), sizeY, -1.0f);
		{
			// A hacky way to fix text alignment issues.
			static f32 offset = -0.003f;
			//ImGui::SliderFloat("slider offset", &offset, -0.01f, 0.00f);

			const auto sizeY = block.worldSize();
			Vec2 min = Vec2(renderer.gfx.camera.pos.x + spacingBetween, block.worldPositionBottomY + offset);
			Vec2 max = Vec2(0.2f, min.y + sizeY);
			const auto mid = (min + max) / 2.0f;
			
			f32 width = sizeY;
			f32 radius = width / 2.0f;

			const auto sliderMinX = min.x + radius;
			const auto sliderMaxX = max.x - radius;
			Ui::line(
				renderer, 
				Vec2(sliderMinX, mid.y),
				Vec2(sliderMaxX, mid.y), 
				width, 
				Color3::WHITE / 2.0f);

			const auto currentValue = Vec2(lerp(sliderMinX, sliderMaxX, slider.value), mid.y);

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
					.index = sliderI,
					.grabOffset = diskPos - cursorPos,
				};
			}

			if (Input::isMouseButtonHeld(MouseButton::LEFT) && settingsUi.grabbedSlider.has_value()) {
				auto& slider = settingsUi.sliderInputs[settingsUi.grabbedSlider->index];
				const auto offset = cursorPos.x - sliderMinX + settingsUi.grabbedSlider->grabOffset.x;
				slider.value = offset / (sliderMaxX - sliderMinX);
				slider.value = std::clamp(slider.value, 0.0f, 1.0f);
			}

			if (Input::isMouseButtonUp(MouseButton::LEFT) && settingsUi.grabbedSlider.has_value()) {
				settingsUi.grabbedSlider = std::nullopt;
			}
		}
	}

	auto toggleButton = [&](ToggleButton& button) {
		const auto& block = settingsUi.layout.blocks[button.id];
		const auto sizeY = block.worldSize();
		const auto text = button.value ? "on" : "off";
		const auto color = button.value ? Color3::GREEN : Color3::RED;

		drawTextAlignedRelativeToCenter(button.text, block.worldCenter(), block.worldSize(), -1.0f);

		// Hacky way to fix alignment issues
		static f32 offsetOn = 0.0f;
		static f32 offsetOff = 0.004f;
		//ImGui::SliderFloat("offsetOn", &offsetOn, 0.0f, 0.01f);
		//ImGui::SliderFloat("offsetOff", &offsetOff, 0.0f, 0.01f);

		auto offset = button.value ? offsetOn : offsetOff;
		const auto rect = drawTextAlignedRelativeToCenter(text, block.worldCenter() + offset, block.worldSize(), 1.0f, button.hoverAnimationT, color);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);
		if (hovered && Input::isMouseButtonDown(MouseButton::LEFT)) {
			button.value = ! button.value;
		}
		updateConstantSpeedT(button.hoverAnimationT, buttonHoverAnimationDuration, hovered);
	};


	for (const auto& button : settingsUi.buttons) {
		drawButton(renderer, settingsUi.layout, button);
	}

	toggleButton(settingsUi.drawBackgroundsButton);

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText();
	return result;
}

void MainMenu::setSoundSettings(const SettingsAudio& audio) {
	//soundSettingsUi.sliderInputs[soundSettingsUi.masterVolumeSliderIndex].value = audio.masterVolume;
	//soundSettingsUi.sliderInputs[soundSettingsUi.musicVolumeSliderIndex].value = audio.musicVolume;
	settingsUi.sliderInputs[settingsUi.soundEffectVolumeSliderIndex].value = audio.soundEffectVolume;
}

SettingsAudio MainMenu::getSoundSettings() const {
	return SettingsAudio{
		//.masterVolume = soundSettingsUi.sliderInputs[soundSettingsUi.masterVolumeSliderIndex].value,
		.soundEffectVolume = std::clamp(settingsUi.sliderInputs[settingsUi.soundEffectVolumeSliderIndex].value, 0.0f, 1.0f),
		//.musicVolume = soundSettingsUi.sliderInputs[soundSettingsUi.musicVolumeSliderIndex].value,
	};
}

void MainMenu::drawText(GameRenderer& r, std::string_view text, const Ui::CenteredHorizontalListLayout& layout, i32 id, f32 hoverT) {
	auto& block = layout.blocks[id];
	const auto center = Vec2(r.gfx.camera.pos.x, block.worldCenter());
	r.gameTextCentered(center, block.worldSize(), text, hoverT);
}

void MainMenu::drawButton(GameRenderer& r, const Ui::CenteredHorizontalListLayout& layout, const Button& button) {
	drawText(r, button.text, layout, button.id, button.hoverAnimationT);
}

MainMenu::CongratulationsScreenResult MainMenu::congratulationsScreenUpdate(GameRenderer& renderer) {
	renderer.textColorRng.seed(renderer.textColorRngSeed);

	auto result = CongratulationsScreenResult::NONE;

	auto& ui = congratulationsUi;

	ui.layout.update(renderer.gfx.camera);


	const auto cursorPos = Ui::cursorPosUiSpace();

	for (auto& button : ui.buttons) {
		const auto rect = buttonRect(renderer, ui.layout, button);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);
		button.update(hovered);

		if (hovered && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (button.text == levelSelectButtonText) {
				result = CongratulationsScreenResult::GO_TO_LEVEL_SELECT;
			} else if (button.text == mainMenuButtonText) {
				result = CongratulationsScreenResult::GO_TO_MAIN_MENU;
			}
		}
	}

	drawText(renderer, "Congratulations", ui.layout, ui.titleId0);
	drawText(renderer, "you've completed", ui.layout, ui.titleId1);
	drawText(renderer, "all the levels", ui.layout, ui.titleId2);
	// https://english.stackexchange.com/questions/240468/whats-the-difference-between-you-have-completed-the-task-and-you-com
	for (const auto& button : ui.buttons) {
		drawButton(renderer, ui.layout, button);
	}

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText();
	return result;
}

void MainMenu::Button::update(bool hovered) {
	updateConstantSpeedT(hoverAnimationT, buttonHoverAnimationDuration, hovered);
}
