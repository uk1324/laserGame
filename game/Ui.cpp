#include "Ui.hpp"
#include <engine/Input/Input.hpp>

Ui::RectMinSize::RectMinSize(Vec2 min, Vec2 size) 
	: min(min)
	, size(size) {}

Ui::RectMinMax::RectMinMax(Vec2 min, Vec2 max)
	: min(min)
	, max(max) {}

Ui::RectMinMax Ui::RectMinMax::fromPosSize(Vec2 pos, Vec2 size) {
	return RectMinMax(pos - size / 2.0f, pos + size / 2.0f);
}

Vec2 Ui::posToWorldSpace(const GameRenderer& r, Vec2 p) {
	// @Performance: Could cache in renderer.
	return p * 2.0f * r.gfx.camera.clipSpaceToWorldSpace();
}

Ui::RectMinMax Ui::rectToWorldSpace(const GameRenderer& r, Vec2 min, Vec2 max) {
	return RectMinMax(posToWorldSpace(r, min), posToWorldSpace(r, max));
}

Vec2 Ui::cursorPosUiSpace() {
	return Input::cursorPosClipSpace() / 2.0f;
}

bool Ui::isPointInRectMinMax(Vec2 min, Vec2 max, Vec2 p) {
	return min.x < p.x && p.x < max.x
		&& min.y < p.y && p.y < max.y;
}

bool Ui::isPointInRectPosSize(Vec2 pos, Vec2 size, Vec2 p) {
	return isPointInRectMinMax(pos - size / 2.0f, pos + size / 2.0f, p);
}

void Ui::rectMinMax(GameRenderer& r, Vec2 min, Vec2 max, f32 width, Vec3 color) {
	const auto rect = rectToWorldSpace(r, min, max);
	r.gfx.rect(rect.min, rect.max - rect.min, width, color);
}

void Ui::rectPosSize(GameRenderer& r, Vec2 pos, Vec2 size, f32 width, Vec3 color) {
	rectMinMax(r, pos - size / 2.0f, pos + size / 2.0f, width, color);
}

void Ui::rectMinMaxFilled(GameRenderer& r, Vec2 min, Vec2 max, Vec3 color) {
	const auto rect = rectToWorldSpace(r, min, max);
	r.gfx.filledRect((rect.min + rect.max) / 2.0f, rect.max - rect.min, color);
}

void Ui::rectPosSizeFilled(GameRenderer& r, Vec2 pos, Vec2 size, Vec3 color) {
	rectMinMaxFilled(r, pos - size / 2.0f, pos + size / 2.0f, color);
}

void Ui::triFilled(GameRenderer& r, Vec2 v0, Vec2 v1, Vec2 v2, Vec3 color) {
	r.gfx.filledTriangle(
		posToWorldSpace(r, v0), 
		posToWorldSpace(r, v1), 
		posToWorldSpace(r, v2), 
		color);
}

f32 Ui::xSizeToYSize(const GameRenderer& r, f32 xSize) {
	return xSize * r.gfx.camera.aspectRatio;
}

f32 Ui::ySizeToXSize(const GameRenderer& r, f32 ySize) {
	return ySize / r.gfx.camera.aspectRatio;
}

Vec2 Ui::equalSizeReativeToX(const GameRenderer& r, f32 xSize) {
	return Vec2(xSize, xSizeToYSize(r, xSize));
}

Vec2 Ui::equalSizeReativeToY(const GameRenderer& r, f32 ySize) {
	return Vec2(ySizeToXSize(r, ySize), ySize);
}

Vec2 Ui::rectPositionRelativeToCorner(Vec2 corner, Vec2 rectSize, Vec2 offset) {
	const auto direction = corner.applied([](f32 f) { return f > 0.0f ? 1.0f : -1.0f; });
	return corner - (offset + rectSize / 2.0f) * direction;
}

void Ui::updateConstantSpeedT(f32& t, f32 timeToFinish, bool active) {
	const auto speed = 1.0f / (timeToFinish);
	t += speed * Constants::dt * (active ? 1.0f : -1.0f);
	t = std::clamp(t, 0.0f, 1.0f);
}

