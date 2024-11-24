#include "MainMenu.hpp"
#include <game/GameUi.hpp>
#include <engine/Input/Input.hpp>
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
const char* soundSettingsButtonText = "sound settings";
const char* editorButtonText = "level editor";
const char* exitButtonText = "exit";
static constexpr const char* masterVolumeSliderName = "master";
static constexpr const char* soundEffectVolumeSliderName = "sound effect";
static constexpr const char* musicVolumeSliderName = "music";
static constexpr const char* backButtonText = "back";

MainMenu::MainMenu() {
	static constexpr f32 buttonSize = 0.04f;
	f32 titleSize = 0.10f;
	f32 smallTitleSize = 0.06f;
	const auto padding = 0.008f;

	{
		menuUi.titleId = menuUi.layout.addBlock(titleSize);

		std::string_view buttonNames[]{
			playButtonText,
			levelSelectButtonText,
			editorButtonText,
			soundSettingsButtonText,
			exitButtonText,
		};
		for (const auto& name : buttonNames) {
			addButton(menuUi.buttons, menuUi.layout, name, buttonSize);
			menuUi.layout.addPadding(padding);
		}
	}
	{
		auto addSliderInput = [&](std::string_view name) -> i32 {
			i32 index = soundSettingsUi.sliderInputs.size();
			soundSettingsUi.sliderInputs.push_back(SliderInput{
				.name = name,
				.id = soundSettingsUi.layout.addBlock(buttonSize),
			});
			return index;
		};
		soundSettingsUi.titleId = soundSettingsUi.layout.addBlock(smallTitleSize);

		soundSettingsUi.masterVolumeSliderIndex = addSliderInput(masterVolumeSliderName);
		soundSettingsUi.layout.addPadding(padding);

		soundSettingsUi.soundEffectVolumeSliderIndex = addSliderInput(soundEffectVolumeSliderName);
		soundSettingsUi.layout.addPadding(padding);

		soundSettingsUi.musicVolumeSliderIndex = addSliderInput(musicVolumeSliderName);
		soundSettingsUi.layout.addPadding(padding);

		addButton(soundSettingsUi.buttons, soundSettingsUi.layout, backButtonText, buttonSize);
	}
}

Ui::RectMinMax MainMenu::buttonRect(const GameRenderer& renderer, const Ui::CenteredHorizontalListLayout& layout, const Button& button) {
	const auto& block = layout.blocks[button.id];
	const auto pos = Vec2(0.0f, block.worldCenter());
	return Ui::centeredTextBoundingRect(pos, block.worldSize(), button.text, renderer.font, renderer);
}

MainMenu::Result MainMenu::update(GameRenderer& renderer) {
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
			} else if (button.text == soundSettingsButtonText) {
				result = Result::GO_TO_SOUND_SETTINGS;
			} else if (button.text == editorButtonText) {
				result = Result::GO_TO_EDITOR;
			} else if (button.text == playButtonText) {
				result = Result::PLAY;
			}
		}
	}

	drawText(renderer, "NAME", menuUi.layout, menuUi.titleId);
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

	soundSettingsUi.layout.update(renderer.gfx.camera);

	const auto cursorPos = Ui::cursorPosUiSpace();

	for (auto& button : soundSettingsUi.buttons) {
		const auto rect = buttonRect(renderer, soundSettingsUi.layout, button);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);
		button.update(hovered);
		//Ui::rect(renderer, rect, 0.01f, Color3::WHITE);

		if (Ui::isPointInRect(rect, cursorPos) && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (button.text == backButtonText) {
				result = SoundSettingsResult::GO_BACK;
			}
		}
	}

	drawText(renderer, "sound settings", soundSettingsUi.layout, soundSettingsUi.titleId);
	
	for (i32 sliderI = 0; sliderI < soundSettingsUi.sliderInputs.size(); sliderI++) {
		const auto& slider = soundSettingsUi.sliderInputs[sliderI];
		const auto block = soundSettingsUi.layout.blocks[slider.id];
		const auto spacingBetween = 0.01f;
		const auto sizeY = block.worldSize();
		{
			const auto info = renderer.font.textInfo(sizeY, slider.name);
			Vec2 position = Vec2(renderer.gfx.camera.pos.x - info.size.x - spacingBetween, block.worldCenter());
			position.y -= info.bottomY;
			position.y -= info.size.y / 2.0f;
			renderer.gameText(position, sizeY, slider.name);
		}
		{
			const auto sizeY = block.worldSize();
			Vec2 min = Vec2(renderer.gfx.camera.pos.x + spacingBetween, block.worldPositionBottomY);
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
				!soundSettingsUi.grabbedSlider.has_value()) {

				soundSettingsUi.grabbedSlider = SoundSettingsUi::GrabbedSlider{
					.index = sliderI,
					.grabOffset = diskPos - cursorPos,
				};
			}

			if (Input::isMouseButtonHeld(MouseButton::LEFT) && soundSettingsUi.grabbedSlider.has_value()) {
				auto& slider = soundSettingsUi.sliderInputs[soundSettingsUi.grabbedSlider->index];
				const auto offset = cursorPos.x - sliderMinX + soundSettingsUi.grabbedSlider->grabOffset.x;
				slider.value = offset / (sliderMaxX - sliderMinX);
				slider.value = std::clamp(slider.value, 0.0f, 1.0f);
			}

			if (Input::isMouseButtonUp(MouseButton::LEFT) && soundSettingsUi.grabbedSlider.has_value()) {
				soundSettingsUi.grabbedSlider = std::nullopt;
			}
		}
	}

	for (const auto& button : soundSettingsUi.buttons) {
		drawButton(renderer, soundSettingsUi.layout, button);
	}


	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText();
	return result;
}

void MainMenu::setSoundSettings(const SettingsAudio& audio) {
	soundSettingsUi.sliderInputs[soundSettingsUi.masterVolumeSliderIndex].value = audio.masterVolume;
	soundSettingsUi.sliderInputs[soundSettingsUi.musicVolumeSliderIndex].value = audio.musicVolume;
	soundSettingsUi.sliderInputs[soundSettingsUi.soundEffectVolumeSliderIndex].value = audio.soundEffectVolume;
}

SettingsAudio MainMenu::getSoundSettings() const {
	return SettingsAudio{
		.masterVolume = soundSettingsUi.sliderInputs[soundSettingsUi.masterVolumeSliderIndex].value,
		.soundEffectVolume = soundSettingsUi.sliderInputs[soundSettingsUi.soundEffectVolumeSliderIndex].value,
		.musicVolume = soundSettingsUi.sliderInputs[soundSettingsUi.musicVolumeSliderIndex].value,
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

void MainMenu::Button::update(bool hovered) {
	updateConstantSpeedT(hoverAnimationT, buttonHoverAnimationDuration, hovered);
}
