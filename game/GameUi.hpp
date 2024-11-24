#pragma once

#include <game/Ui.hpp>

static constexpr f32 buttonHoverAnimationDuration = 0.2f;
static constexpr f32 navigationButtonTextHeight = 0.08f;

bool button(Ui::RectMinMax rect, f32& hoverAnimationT, Vec2 cursorPos);
bool buttonPosSize(Vec2 pos, Vec2 size, f32& hoverAnimationT, Vec2 cursorPos);

struct GameUiButton {
	f32 hoverAnimationT = 0.0f;
};
