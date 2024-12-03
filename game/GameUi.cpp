#include <game/GameUi.hpp>
#include <game/Animation.hpp>
#include <engine/Input/Input.hpp>

bool gameButton(GameAudio& audio, Ui::RectMinMax rect, f32& hoverAnimationT, Vec2 cursorPos) {
	const auto hover = Ui::isPointInRect(rect, cursorPos);
	updateConstantSpeedT(hoverAnimationT, buttonHoverAnimationDuration, hover);
	const auto on = hover && Input::isMouseButtonDown(MouseButton::LEFT);
	if (on) {
		audio.playSoundEffect(audio.uiClickSound);
	}
	return on;
}

bool gameButtonPosSize(GameAudio& audio, Vec2 pos, Vec2 size, f32& hoverAnimationT, Vec2 cursorPos) {
	return gameButton(audio, Ui::RectMinMax::fromPosSize(pos, size), hoverAnimationT, cursorPos);
}
