#include <game/LevelSelect.hpp>

LevelSelect::LevelSelect() {
	for (i32 i = 0; i < squareCountY * squareCountX; i++) {
		buttons.push_back(GameUiButton{});
	}
}

LevelSelect::Result LevelSelect::update(GameRenderer& renderer, const Levels& levels, const GameSave& save) {
	Result result = ResultNone();
	renderer.textColorRng.seed(renderer.textColorRngSeed);
	auto& r = renderer;

	const auto cursorPos = Ui::cursorPosUiSpace();

	const auto boxHeight = 0.7f;
	const auto squareSizeY = boxHeight / squareCountY;
	const auto squareSize = Ui::equalSizeReativeToY(r, squareSizeY);
	const auto boxWidth = Ui::ySizeToXSize(r, boxHeight) * f32(squareCountX) / f32(squareCountY);
	Vec2 boxSize(boxWidth, boxHeight);
	const auto boxPos = Vec2(0.0f);
	//Ui::rectPosSize(r, boxPos, boxSize, 0.01f, Color3::WHITE);
	const auto box = Ui::RectMinMax::fromPosSize(boxPos, boxSize);

	f32 squarePaddingScale = 0.1f;
	for (i32 yi = 0; yi < squareCountY; yi++) {
		for (i32 xi = 0; xi < squareCountX; xi++) {
			Vec2 squarePos = Vec2(box.min.x, box.max.y) + Vec2(xi + 0.5f, -yi - 0.5f) * squareSize;
			//Ui::rectPosSize(r, squarePos, squareSize * (1.0f - squarePaddingScale), 0.01f, Color3::GREEN);
			const auto index = yi * squareCountX + xi;
			auto& hoverT = buttons[index].hoverAnimationT;

			if (buttonPosSize(squarePos, squareSize, hoverT, cursorPos)) {
				result = ResultGoToLevel{
					.index = index
				};
			}

			auto color = Color3::WHITE / 2.0f;

			if (save.isCompleted(index, levels)) {
				color = renderer.textColorRng.colorRandomHue(1.0f, 1.0f);
			}
			renderer.gameTextCentered(squarePos, squareSize.x, std::to_string(index + 1), hoverT, color);
		}
	}

	{
		const auto backButtonText = "back";
		const auto height = navigationButtonTextHeight;
		const auto size = Ui::textBoundingRectSize(height, backButtonText, renderer.font, renderer);
		const auto pos = Ui::rectPositionRelativeToCorner(Vec2(-0.5f, 0.5f), size, Ui::equalSizeReativeToX(renderer, 0.01f));

		if (buttonPosSize(pos, size, backButton.hoverAnimationT, cursorPos)) {
			result = ResultGoBack();
		}

		renderer.gameTextCentered(pos, height, backButtonText, backButton.hoverAnimationT);
	}

	renderer.gfx.drawLines();
	renderer.renderGameText();
	return result;
}