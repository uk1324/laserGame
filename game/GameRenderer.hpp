#pragma once

#include <gfx2d/Gfx2d.hpp>
#include <game/GameUpdate.hpp>
#include <game/Constants.hpp>
#include <game/Shaders/stereographicLineData.hpp>
#include <game/Shaders/stereographicDiskData.hpp>
#include <game/Shaders/gameTextData.hpp>
#include <game/SettingsData.hpp>
#include <game/ProjectivePolyhedron.hpp>
#include <random>

struct ColorRng {
	ColorRng();
	void seed(u32 seed);

	Vec3 colorRandomHue(f32 s, f32 v);

	std::default_random_engine rng;
	std::uniform_real_distribution<f32> dist;
};

struct ColoredText {
	std::string chars;
	std::vector<Vec3> colors;

	void clear();

	void add(std::string_view text, Vec3 color = Color3::WHITE);
};

const auto grabbableCircleRadius = 0.015f;

struct GameRenderer {
	static GameRenderer make();

	//static constexpr Vec3 absorbingColor = Color3::WHITE / 5.0f;
	static Vec3 absorbingColor;
	static Vec3 reflectingColor;

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
	void render(GameEntities& e, const GameState& s, bool editor, f32 invalidGameStateAnimationT, std::optional<f32> movementDirectionAngle);

	void renderBackground();
	void renderTilingBackground();

	Vao backgroundVao;
	ShaderProgram& backgroundShader;

	bool tilingBackgroundIcosahedralIfTrueDodecahedralIfFalse;
	Vao tilingBackgroundVao;
	ShaderProgram& tilingBackgroundShader;

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

	Vao gameTextVao;
	ShaderProgram& gameTextShader;
	std::vector<GameTextInstance> gameTextInstances;
	ColorRng textColorRng;
	u32 textColorRngSeed;
	void changeTextColorRngSeed();
	void randomize();

	void gameText(
		Vec2 bottomLeftPosition,
		float maxHeight,
		std::string_view text,
		f32 hoverT = 0.0f,
		std::optional<Vec3> color = std::nullopt);

	void gameTextColored(
		Vec2 bottomLeftPosition,
		float maxHeight,
		const ColoredText& colored,
		f32 hoverT = 0.0f);

	void gameTextCentered(
		Vec2 position,
		float maxHeight,
		std::string_view text,
		f32 hoverT = 0.0f,
		std::optional<Vec3> color = std::nullopt);

	void gameTextColoredCentered(
		Vec2 position,
		float maxHeight,
		const ColoredText& colored,
		f32 hoverT = 0.0f);

	void renderGameText(bool useAdditiveBlending);

	Vao glowingArrowVao;
	ShaderProgram& glowingArrowShader;
	void glowingArrow(const Aabb& rect, f32 hoverAnimationT, f32 opacity);

	bool simplifiedGraphics = false;

	Font font;

	SettingsGraphics settings;

	Gfx2d gfx;
};

Vec3 movablePartColor(bool isPositionLocked);