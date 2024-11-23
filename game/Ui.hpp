#pragma once

#include <game/GameRenderer.hpp>

namespace Ui {
	/*
	Ui coordinates go from [-0.5, -0.5] at the bottom left corner to [0.5, 0.5] at the top left corner.
	*/

	struct RectMinSize {
		RectMinSize(Vec2 min, Vec2 size);
		Vec2 min;
		Vec2 size;
	};

	struct RectMinMax {
		RectMinMax(Vec2 min, Vec2 max);
		static RectMinMax fromPosSize(Vec2 pos, Vec2 size);
		Vec2 min;
		Vec2 max;
	};

	Vec2 posToWorldSpace(const GameRenderer& r, Vec2 p);
	f32 ySizeToWorldSpace(const GameRenderer& r, f32 ySize);
	RectMinMax rectToWorldSpace(const GameRenderer& r, Vec2 min, Vec2 max);

	Vec2 cursorPosUiSpace();

	bool isPointInRectMinMax(Vec2 min, Vec2 max, Vec2 p);
	bool isPointInRectPosSize(Vec2 pos, Vec2 size, Vec2 p);
	bool isPointInRect(const RectMinMax& rect, Vec2 p);

	void rectMinMax(GameRenderer& r, Vec2 min, Vec2 max, f32 width, Vec3 color);
	void rectPosSize(GameRenderer& r, Vec2 pos, Vec2 size, f32 width, Vec3 color);
	void rect(GameRenderer& r, const RectMinMax& rect, f32 width, Vec3 color);
	void rectMinMaxFilled(GameRenderer& r, Vec2 min, Vec2 max, Vec4 color);
	void rectMinMaxFilled(GameRenderer& r, Vec2 min, Vec2 max, Vec3 color);
	void rectPosSizeFilled(GameRenderer& r, Vec2 pos, Vec2 size, Vec4 color);
	void rectPosSizeFilled(GameRenderer& r, Vec2 pos, Vec2 size, Vec3 color);
	void triFilled(GameRenderer& r, Vec2 v0, Vec2 v1, Vec2 v2, Vec4 color);
	void triFilled(GameRenderer& r, Vec2 v0, Vec2 v1, Vec2 v2, Vec3 color);
	void line(GameRenderer& r, Vec2 e0, Vec2 e1, f32 width, Vec3 color);
	//void diskX(GameRenderer& r, Vec2 center, f32 radiusRelativeToWidth, Vec3 color);
	void diskY(GameRenderer& r, Vec2 center, f32 radiusRelativeToHeight, Vec3 color);

	f32 xSizeToYSize(const GameRenderer& r, f32 xSize);
	f32 ySizeToXSize(const GameRenderer& r, f32 ySize);
	Vec2 equalSizeReativeToX(const GameRenderer& r, f32 xSize);
	Vec2 equalSizeReativeToY(const GameRenderer& r, f32 ySize);

	// offset is in the coordinates such that the origin is set in such a way that the corner of the rect is at the corner. offset increases from the corner to (0, 0)
	Vec2 rectPositionRelativeToCorner(Vec2 corner, Vec2 rectSize, Vec2 offset);

	struct CenteredHorizontalListLayout {
		f32 totalSizeY = 0.0f;
		struct Block {
			// positions are precentages of full screen size
			f32 yPosition;
			// from 0 to 1 where 1 is the full screen.
			f32 sizeY;
			// world space
			f32 worldPositionTopY = -1.0f;
			f32 worldPositionBottomY = -1.0f;

			f32 worldCenter() const;
			f32 worldSize() const;
		};
		std::vector<Block> blocks;

		void addPadding(f32 sizeY);
		i32 addBlock(f32 sizeY);

		void update(const Camera& camera);
	};
}