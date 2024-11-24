#include <game/GameUi.hpp>
#include <game/Animation.hpp>
#include <engine/Input/Input.hpp>;

bool button(Ui::RectMinMax rect, f32& hoverAnimationT, Vec2 cursorPos) {
	const auto hover = Ui::isPointInRect(rect, cursorPos);
	updateConstantSpeedT(hoverAnimationT, buttonHoverAnimationDuration, hover);
	return hover && Input::isMouseButtonDown(MouseButton::LEFT);
}

bool buttonPosSize(Vec2 pos, Vec2 size, f32& hoverAnimationT, Vec2 cursorPos) {
	return button(Ui::RectMinMax::fromPosSize(pos, size), hoverAnimationT, cursorPos);
}
