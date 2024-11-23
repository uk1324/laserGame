#pragma once

#include <gfx2d/Gfx2d.hpp>
#include <game/GameUpdate.hpp>
#include <game/Constants.hpp>
#include <game/Shaders/stereographicLineData.hpp>
#include <game/Shaders/stereographicDiskData.hpp>

const auto grabbableCircleRadius = 0.015f;

struct GameRenderer {
	static GameRenderer make();

	static constexpr Vec3 absorbingColor = Color3::WHITE;
	static constexpr Vec3 reflectingColor = Color3::WHITE / 2.0f;

	static Vec3 wallColor(EditorWallType type);

	void multicoloredSegment(const std::array<Vec2, 2>& endpoints, f32 normalAngle, Vec3 normalColor, Vec3 backColor);
	void wall(Vec2 e0, Vec2 e1, Vec3 color, bool editor = true);
	void mirror(const EditorMirror& mirror);
	void renderWalls();
	void stereographicSegmentSimple(Vec2 e0, Vec2 e1, Vec3 color);
	void stereographicSegmentSimple(Vec2 e0, Vec2 e1, Vec4 color, f32 width = Constants::wallWidth);
	void stereographicSegment(Vec2 e0, Vec2 e1, Vec3 color);
	void lockedCell(const LockedCells& cells, i32 index, Vec4 color);

	static constexpr Vec4 lockedCellColor = Vec4(Color3::WHITE / 2.0f, 0.5f);

	void renderClear();
	void render(GameEntities& e, const GameState& s, bool editor, bool validGameState);

	void renderBackground();

	Vao backgroundVao;
	ShaderProgram& backgroundShader;

	Vao stereographicLineVao;
	ShaderProgram& stereographicLineShader;
	std::vector<StereographicLineInstance> stereographicLines;
	void addStereographicSegmentComplex(Vec2 endpoint0, Vec2 endpoint1, Vec3 color0, Vec3 color1, f32 width);
	void renderStereographicSegmentsComplex();
	void stereographicSegmentComplex(Vec2 endpoint0, Vec2 endpoint1, Vec3 color0, Vec3 color1, f32 width);

	Vao stereographicDiskVao;
	ShaderProgram& stereographicDiskShader;
	std::vector<StereographicDiskInstance> stereographicDisks;
	void renderStereographicDisks();
	void addStereographicDisk(Vec2 center, f32 radius, Vec3 colorInside, Vec3 colorBorder);

	bool simplifiedGraphics = false;

	Font font;

	Gfx2d gfx;
};

Vec3 movablePartColor(bool isPositionLocked);