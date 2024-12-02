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
		auto sliderInput = [&](std::string_view name) -> SliderInput {
			return SliderInput{
				.name = name,
				.id = settingsUi.layout.addBlock(buttonSize),
			};
		};
		settingsUi.titleId = settingsUi.layout.addBlock(smallTitleSize);
		/*soundSettingsUi.masterVolumeSliderIndex = addSliderInput(masterVolumeSliderName);
		soundSettingsUi.layout.addPadding(padding);*/

		settingsUi.volumeSlider = sliderInput(soundEffectVolumeSliderName);
		settingsUi.layout.addPadding(padding);

		auto toggleButton = [&](const char* text) {
			return ToggleButton{
				.id = settingsUi.layout.addBlock(buttonSize),
				.text = text,
			};
		};

		settingsUi.drawBackgroundsButton = toggleButton("backgrounds");
		settingsUi.fullscreenButton = toggleButton("fullscreen");

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

#include <glfw/include/GLFW/glfw3.h>

MainMenu::Result MainMenu::update(GameRenderer& renderer) {
	uiSceneBegin(renderer);

	static std::optional<Vec2T<i32>> lastSize;
	static std::optional<Vec2T<i32>> lastPos;
	auto monitor = glfwGetPrimaryMonitor();
	GLFWwindow* w = (GLFWwindow*)Window::handle();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	if (Input::isKeyDown(KeyCode::A)) {
		i32 sizeX;
		i32 sizeY;
		i32 posX;
		i32 posY;
		glfwGetWindowSize(w, &sizeX, &sizeY);
		glfwGetWindowPos(w, &posX, &posY);
		lastSize = Vec2T<i32>(sizeX, sizeY);
		lastPos = Vec2T<i32>(posX, posY);

		glfwSetWindowMonitor(w, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
	if (Input::isKeyDown(KeyCode::B)) {
		glfwSetWindowMonitor(w, NULL, lastPos->x, lastPos->y, lastSize->x, lastSize->y, 0);
		Window::enableWindowedFullscreen();
	}

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
	drawText(renderer, "optics", menuUi.layout, menuUi.titleId1);
	for (const auto& button : menuUi.buttons) {
		drawButton(renderer, menuUi.layout, button);
	}

	renderer.gfx.fontRenderer.render(renderer.font, renderer.gfx.instancesVbo);
	renderer.gfx.drawFilledTriangles();
	renderer.gfx.drawLines();
	renderer.renderGameText(true);
	return result;
}

MainMenu::SettingsResult MainMenu::settingsUpdate(GameRenderer& renderer) {
	uiSceneBegin(renderer);

	auto result = SettingsResult::NONE;

	settingsUi.layout.update(renderer.gfx.camera);

	const auto cursorPos = Ui::cursorPosUiSpace();

	for (auto& button : settingsUi.buttons) {
		const auto rect = buttonRect(renderer, settingsUi.layout, button);
		const auto hovered = Ui::isPointInRect(rect, cursorPos);
		button.update(hovered);
		//Ui::rect(renderer, rect, 0.01f, Color3::WHITE);

		if (Ui::isPointInRect(rect, cursorPos) && Input::isMouseButtonDown(MouseButton::LEFT)) {
			if (button.text == backButtonText) {
				result = SettingsResult::GO_BACK;
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

	auto slider = [&](f32& value, SliderInput& slider, i32 index) {
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
			activated = true;
		}
		updateConstantSpeedT(button.hoverAnimationT, buttonHoverAnimationDuration, hovered);
		return activated;
	};


	for (const auto& button : settingsUi.buttons) {
		drawButton(renderer, settingsUi.layout, button);
	}

	bool modified = false;
	modified |= toggleButton(settings.graphics.drawBackgrounds, settingsUi.drawBackgroundsButton);
	modified |= toggleButton(settings.graphics.fullscreen, settingsUi.fullscreenButton);
	modified |= slider(settings.audio.soundEffectVolume, settingsUi.volumeSlider, 0);

	if (modified) {
		result = SettingsResult::PROPAGATE_SETTINGS;
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

MainMenu::CongratulationsScreenResult MainMenu::congratulationsScreenUpdate(GameRenderer& renderer) {
	uiSceneBegin(renderer);

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
	renderer.renderGameText(true);
	return result;
}

void MainMenu::Button::update(bool hovered) {
	updateConstantSpeedT(hoverAnimationT, buttonHoverAnimationDuration, hovered);
}
