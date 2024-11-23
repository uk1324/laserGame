#pragma once

#include <game/Ui.hpp>
#include <game/SettingsData.hpp>
#include <variant>

struct MainMenu {
	// Setting this to a big number so if accessed it hopefully crashes.
	static constexpr i32 INVALID = 0xFFFFFF;

	MainMenu();

	struct Button {
		i32 id = INVALID;
		std::string_view text;
		f32 hoverAnimationT = 0.0f;
	};

	static Ui::RectMinMax buttonRect(const GameRenderer& renderer, const Ui::CenteredHorizontalListLayout& layout, const Button& button);

	struct SliderInput {
		std::string_view name;
		i32 id = INVALID;
		f32 value = 0.5f; // from 0 to 1
		f32 hoverAnimationT = 0.0f;
	};

	enum class Result {
		NONE,
		GO_TO_LEVEL_SELECT,
		GO_TO_SOUND_SETTINGS,
	};

	enum class SoundSettingsResult {
		GO_BACK,
		NONE,
	};

	Result update(GameRenderer& renderer);
	SoundSettingsResult soundSettingsUpdate(GameRenderer& renderer);
	void setSoundSettings(const SettingsAudio& audio);
	SettingsAudio getSoundSettings() const;

	void drawText(GameRenderer& r, std::string_view text, const Ui::CenteredHorizontalListLayout& layout, i32 id);

	void drawButton(GameRenderer& r, const Ui::CenteredHorizontalListLayout& layout, const Button& button);

	struct MenuUi {
		Ui::CenteredHorizontalListLayout layout;
		i32 titleId = INVALID;
		std::vector<Button> buttons;
	} menuUi;

	struct SoundSettingsUi {
		Ui::CenteredHorizontalListLayout layout;
		i32 titleId = INVALID;
		std::vector<SliderInput> sliderInputs;
		std::vector<Button> buttons;

		i32 masterVolumeSliderIndex = INVALID;
		i32 soundEffectVolumeSliderIndex = INVALID;
		i32 musicVolumeSliderIndex = INVALID;

		struct GrabbedSlider {
			i32 index;
			Vec2 grabOffset;
		};
		std::optional<GrabbedSlider> grabbedSlider;
	} soundSettingsUi;

	ColorRng rng;
};