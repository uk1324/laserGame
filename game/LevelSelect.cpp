#include <game/LevelSelect.hpp>
#include <engine/Input/Input.hpp>

LevelSelect::Result LevelSelect::update(GameRenderer& renderer) {
	Result result = ResultNone();
	auto& r = renderer;

	const auto cursorPos = Ui::cursorPosUiSpace();

	const auto boxHeight = 0.7f;
	const auto squareCountY = 4;
	const auto squareSizeY = boxHeight / squareCountY;
	const auto squareSize = Ui::equalSizeReativeToY(r, squareSizeY);
	const auto squareCountX = 6;
	const auto boxWidth = Ui::ySizeToXSize(r, boxHeight) * f32(squareCountX) / f32(squareCountY);
	Vec2 boxSize(boxWidth, boxHeight);
	const auto boxPos = Vec2(0.0f);
	Ui::rectPosSize(r, boxPos, boxSize, 0.01f, Color3::WHITE);
	const auto box = Ui::RectMinMax::fromPosSize(boxPos, boxSize);

	f32 squarePaddingScale = 0.1f;
	for (i32 yi = 0; yi < squareCountY; yi++) {
		for (i32 xi = 0; xi < squareCountX; xi++) {
			Vec2 squarePos = Vec2(box.min.x, box.max.y) + Vec2(xi + 0.5f, -yi - 0.5f) * squareSize;
			Ui::rectPosSize(r, squarePos, squareSize * (1.0f - squarePaddingScale), 0.01f, Color3::GREEN);

			if (Ui::isPointInRectPosSize(squarePos, squareSize, cursorPos) &&
				Input::isMouseButtonDown(MouseButton::LEFT)) {
				result = ResultGoToLevel();
			}
		}
	}

	renderer.gfx.drawLines();
	return result;
}