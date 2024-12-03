#pragma once

#include <game/Ui.hpp>
#include <game/GameAudio.hpp>

static constexpr f32 buttonHoverAnimationDuration = 0.2f;
static constexpr f32 navigationButtonTextHeight = 0.08f;

bool gameButton(GameAudio& audio, Ui::RectMinMax rect, f32& hoverAnimationT, Vec2 cursorPos);
bool gameButtonPosSize(GameAudio& audio, Vec2 pos, Vec2 size, f32& hoverAnimationT, Vec2 cursorPos);

struct GameUiButton {
	f32 hoverAnimationT = 0.0f;
};
