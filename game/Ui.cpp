#include "Ui.hpp"
#include <engine/Input/Input.hpp>
#include <engine/Math/Interpolation.hpp>

Ui::RectMinSize::RectMinSize(Vec2 min, Vec2 size) 
	: min(min)
	, size(size) {}

Ui::RectMinMax::RectMinMax(Vec2 min, Vec2 max)
	: min(min)
	, max(max) {}

Ui::RectMinMax Ui::RectMinMax::fromPosSize(Vec2 pos, Vec2 size) {
	return RectMinMax(pos - size / 2.0f, pos + size / 2.0f);
}

Ui::RectMinMax Ui::RectMinMax::fromMinSize(Vec2 min, Vec2 size) {
	return RectMinMax(min, min + size);
}

Vec2 Ui::RectMinMax::center() const {
	return (min + max) / 2.0f;
}

Vec2 Ui::RectMinMax::size() const {
	return max - min;
}

Vec2 Ui::posToWorldSpace(const GameRenderer& r, Vec2 p) {
	// @Performance: Could cache in renderer.
	return p * 2.0f * r.gfx.camera.clipSpaceToWorldSpace();
}

f32 Ui::ySizeToWorldSpace(const GameRenderer& r, f32 ySize) {
	return ySize * 2.0f * r.gfx.camera.clipSpaceToWorldSpace()[1][1];
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

bool Ui::isPointInRect(const RectMinMax& rect, Vec2 p) {
	return isPointInRectMinMax(rect.min, rect.max, p);
}

void Ui::rectMinMax(GameRenderer& r, Vec2 min, Vec2 max, f32 width, Vec3 color) {
	const auto rect = rectToWorldSpace(r, min, max);
	r.gfx.rect(rect.min, rect.max - rect.min, width, color);
}

void Ui::rectPosSize(GameRenderer& r, Vec2 pos, Vec2 size, f32 width, Vec3 color) {
	rectMinMax(r, pos - size / 2.0f, pos + size / 2.0f, width, color);
}

void Ui::rect(GameRenderer& r, const RectMinMax& rect, f32 width, Vec3 color) {
	rectMinMax(r, rect.min, rect.max, width, color);
}

void Ui::rectMinMaxFilled(GameRenderer& r, Vec2 min, Vec2 max, Vec4 color) {
	const auto rect = rectToWorldSpace(r, min, max);
	r.gfx.filledRect((rect.min + rect.max) / 2.0f, rect.max - rect.min, color);
}

void Ui::rectMinMaxFilled(GameRenderer& r, Vec2 min, Vec2 max, Vec3 color) {
	rectMinMaxFilled(r, min, max, Vec4(color, 1.0f));
}

void Ui::rectPosSizeFilled(GameRenderer& r, Vec2 pos, Vec2 size, Vec4 color) {
	rectMinMaxFilled(r, pos - size / 2.0f, pos + size / 2.0f, color);
}

void Ui::rectPosSizeFilled(GameRenderer& r, Vec2 pos, Vec2 size, Vec3 color) {
	rectMinMaxFilled(r, pos - size / 2.0f, pos + size / 2.0f, color);
}

void Ui::triFilled(GameRenderer& r, Vec2 v0, Vec2 v1, Vec2 v2, Vec4 color) {
	r.gfx.filledTriangle(
		posToWorldSpace(r, v0),
		posToWorldSpace(r, v1),
		posToWorldSpace(r, v2),
		color);
}

void Ui::triFilled(GameRenderer& r, Vec2 v0, Vec2 v1, Vec2 v2, Vec3 color) {
	triFilled(r, v0, v1, v2, Vec4(color, 1.0f));
}

void Ui::line(GameRenderer& r, Vec2 e0, Vec2 e1, f32 width, Vec3 color) {
	r.gfx.lineTriangulated(posToWorldSpace(r, e0), posToWorldSpace(r, e1), width, color);
}

void Ui::diskY(GameRenderer& r, Vec2 center, f32 radiusRelativeToHeight, Vec3 color) {
	r.gfx.diskTriangulated(posToWorldSpace(r, center), ySizeToWorldSpace(r, radiusRelativeToHeight), Vec4(color, 1.0f));
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

Ui::RectMinMax Ui::centeredTextBoundingRect(Vec2 position, f32 maxHeight, std::string_view text, const Font& font, const GameRenderer& renderer) {
	return Ui::RectMinMax::fromPosSize(position, textBoundingRectSize(maxHeight, text, font, renderer));
}

Vec2 Ui::textBoundingRectSize(f32 maxHeight, std::string_view text, const Font& font, const GameRenderer& renderer) {
	const auto info = font.textInfo(maxHeight, text, Constants::additionalTextSpacing);
	const auto size = Vec2(info.size.x / renderer.gfx.camera.clipSpaceToWorldSpace()[0][0], maxHeight);
	return size;
}

f32 Ui::CenteredHorizontalListLayout::Block::worldCenter() const {
	return (worldPositionTopY + worldPositionBottomY) / 2.0f;
}

f32 Ui::CenteredHorizontalListLayout::Block::worldSize() const {
	return worldPositionTopY - worldPositionBottomY;
}

void Ui::CenteredHorizontalListLayout::addPadding(f32 sizeY) {\
	totalSizeY += sizeY;
}

i32 Ui::CenteredHorizontalListLayout::addBlock(f32 sizeY) {
	const auto id = static_cast<i32>(blocks.size());
	blocks.push_back(Block{
		.yPosition = totalSizeY,
		.sizeY = sizeY,
	});
	totalSizeY += sizeY;
	return id;
}

void Ui::CenteredHorizontalListLayout::update(const Camera& camera) {
	const auto viewCenter = camera.pos;
	const auto viewSize = camera.aabb().size();
	const auto topY = viewCenter.y + (totalSizeY / 2.0f) * viewSize.y;
	const auto bottomY = viewCenter.y - (totalSizeY / 2.0f) * viewSize.y;
	for (auto& block : blocks) {
		const auto topT = block.yPosition / totalSizeY;
		const auto bottomT = (block.yPosition + block.sizeY) / totalSizeY;
		block.worldPositionBottomY = lerp(topY, bottomY, bottomT);
		block.worldPositionTopY = lerp(topY, bottomY, topT);
	}
}
