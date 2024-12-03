#pragma once

#include <game/Ui.hpp>
#include <game/GameAudio.hpp>
#include <game/SettingsData.hpp>
#include <variant>

// TODO: Instead of storing the settings state inside the structs. Could just pass the value from the settings struct directly.
struct MainMenu {
	// Setting this to a big number so if accessed it hopefully crashes.
	static constexpr i32 INVALID = 0xFFFFFF;

	MainMenu();

	Settings settings;

	struct Button {
		i32 id = INVALID;
		std::string_view text;
		f32 hoverAnimationT = 0.0f;

		void update(bool hovered);
	};

	static bool button(GameRenderer& renderer, GameAudio& audio, Button& button, Ui::CenteredHorizontalListLayout& layout, Vec2 cursorPos);

	static Ui::RectMinMax buttonRect(const GameRenderer& renderer, const Ui::CenteredHorizontalListLayout& layout, const Button& button);

	struct SliderInput {
		std::string_view name;
		i32 id = INVALID;
		f32 hoverAnimationT = 0.0f;
	};

	enum class Result {
		NONE,
		GO_TO_LEVEL_SELECT,
		GO_TO_SETTINGS,
		GO_TO_EDITOR,
		PLAY,
	};

	enum class SettingsResult {
		GO_BACK,
		PROPAGATE_SETTINGS,
		NONE,
	};

	Result update(GameRenderer& renderer, GameAudio& audio);
	SettingsResult settingsUpdate(GameRenderer& renderer, GameAudio& audio);

	static void drawText(GameRenderer& r, std::string_view text, const Ui::CenteredHorizontalListLayout& layout, i32 id, f32 hoverT = 0.0f);

	static void drawButton(GameRenderer& r, const Ui::CenteredHorizontalListLayout& layout, const Button& button);

	void uiSceneBegin(GameRenderer& renderer);

	struct MenuUi {
		Ui::CenteredHorizontalListLayout layout;
		i32 titleId0 = INVALID;
		i32 titleId1 = INVALID;

		Button playButton;
		Button levelSelectButton;
		Button editorButton;
		Button settingsButton;
		Button exitButton;
	} menuUi;

	struct ToggleButton {
		i32 id = INVALID;
		std::string_view text;
		f32 hoverAnimationT = 0.0f;
	};

	struct SettingsUi {
		Ui::CenteredHorizontalListLayout layout;
		i32 titleId = INVALID;

		SliderInput volumeSlider;
		i32 soundEffectVolumeSliderIndex = INVALID;
		ToggleButton drawBackgroundsButton;
		ToggleButton fullscreenButton;
		//i32 musicVolumeSliderIndex = INVALID;

		Button backButton;

		struct GrabbedSlider {
			i32 index;
			Vec2 grabOffset;
		};
		std::optional<GrabbedSlider> grabbedSlider;
	} settingsUi;

	struct CongratulationsScreenUi {
		Ui::CenteredHorizontalListLayout layout;
		i32 titleId0 = INVALID;
		i32 titleId1 = INVALID;
		i32 titleId2 = INVALID;

		Button mainMenuButton;
		Button levelSelectButton;
	} congratulationsUi;
	enum class CongratulationsScreenResult {
		NONE,
		GO_TO_MAIN_MENU,
		GO_TO_LEVEL_SELECT
	};

	CongratulationsScreenResult congratulationsScreenUpdate(GameRenderer& renderer, GameAudio& audio);

	ColorRng rng;
};