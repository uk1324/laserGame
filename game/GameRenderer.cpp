#include "GameRenderer.hpp"
#include <engine/Math/Color.hpp>
#include <Overloaded.hpp>
#include <engine/Math/Interpolation.hpp>
#include <game/Shaders/backgroundData.hpp>
#include <game/Shaders/glowingArrowData.hpp>
#include <engine/Math/Constants.hpp>
#include <game/Stereographic.hpp>
#include <glad/glad.h>
#include <gfx/Instancing.hpp>
#include <engine/Window.hpp>
#include <StructUtils.hpp>
#include <gfx/ShaderManager.hpp>
#include <gfx2d/Quad2dPt.hpp>
#include <game/Paths.hpp>
#include <gfx2d/DbgGfx2d.hpp>

#define MAKE_VAO(Type) \
	createInstancingVao<Type##Shader>(gfx.quad2dPtVbo, gfx.quad2dPtIbo, gfx.instancesVbo)

GameRenderer GameRenderer::make() {
	auto gfx = Gfx2d::make();

	return GameRenderer{
		.backgroundVao = MAKE_VAO(Background),
		.backgroundShader = MAKE_GENERATED_SHADER(BACKGROUND),
		.stereographicLineVao = MAKE_VAO(StereographicLine),
		.stereographicLineShader = MAKE_GENERATED_SHADER(STEREOGRAPHIC_LINE),
		.stereographicDiskVao = MAKE_VAO(StereographicDisk),
		.stereographicDiskShader = MAKE_GENERATED_SHADER(STEREOGRAPHIC_DISK),
		.gameTextVao = MAKE_VAO(GameText),
		.gameTextShader = MAKE_GENERATED_SHADER(GAME_TEXT),
		.textColorRngSeed = u32(time(NULL)),
		.glowingArrowVao= MAKE_VAO(GlowingArrow),
		.glowingArrowShader = MAKE_GENERATED_SHADER(GLOWING_ARROW),
		.font = Font::loadSdfWithCachingAtDefaultPath(FONT_FOLDER, "RobotoMono-Regular"),
		MOVE(gfx),
	};
}

Vec3 GameRenderer::wallColor(EditorWallType type) {
	return type == EditorWallType::REFLECTING
		? reflectingColor
		: absorbingColor;
}

void GameRenderer::multicoloredSegment(const std::array<Vec2, 2>& endpoints, f32 normalAngle, Vec3 normalColor, Vec3 backColor) {
	if (simplifiedGraphics) {
		const auto normal = Vec2::oriented(normalAngle);
		const auto forward = normal * 0.005f;
		const auto back = -normal * 0.005f;
		stereographicSegment(endpoints[0] + forward, endpoints[1] + forward, normalColor);
		stereographicSegment(endpoints[0] + back, endpoints[1] + back, backColor);
	} else {
		stereographicSegmentComplex(endpoints[0], endpoints[1], backColor, normalColor, 0.035f);
	}
}

void GameRenderer::wall(Vec2 e0, Vec2 e1, Vec3 color, bool editor) {
	if (editor) {
		gfx.disk(e0, grabbableCircleRadius, Color3::RED);
		gfx.disk(e1, grabbableCircleRadius, Color3::RED);
	}

	stereographicSegment(e0, e1, color);
}

void GameRenderer::mirror(const EditorMirror& mirror) {
	const auto endpoints = mirror.calculateEndpoints();
	switch (mirror.wallType) {
		using enum EditorMirrorWallType;
	case ABSORBING:
		multicoloredSegment(endpoints, mirror.normalAngle, reflectingColor, absorbingColor);
		break;
	case REFLECTING:
		stereographicSegment(endpoints[0], endpoints[1], reflectingColor);
		break;
	}
	for (const auto& endpoint : endpoints) {
		gfx.disk(endpoint, grabbableCircleRadius, movablePartColor(false));
	}
	gfx.disk(mirror.center, grabbableCircleRadius, movablePartColor(mirror.positionLocked));
}

void GameRenderer::renderWalls() {
	gfx.drawLines();
	gfx.drawFilledTriangles();
	gfx.drawDisks();
}

void GameRenderer::stereographicSegmentSimple(Vec2 e0, Vec2 e1, Vec3 color) {
	stereographicSegmentSimple(e0, e1, Vec4(color, 1.0f));
}

void GameRenderer::stereographicSegmentSimple(Vec2 e0, Vec2 e1, Vec4 color, f32 width) {
	const auto stereographicLine = ::stereographicLine(e0, e1);
	if (stereographicLine.type == StereographicLine::Type::LINE) {
		gfx.lineTriangulated(e0, e1, width, color);
	} else {
		const auto& line = stereographicLine.circle;

		const auto range = angleRangeBetweenPointsOnCircle(line.center, e0, e1);
		gfx.circleArcTriangulated(line.center, line.radius, range.min, range.max, width, color, 1000);
	}
}

void GameRenderer::stereographicSegment(Vec2 e0, Vec2 e1, Vec3 color) {
	if (simplifiedGraphics) {
		stereographicSegmentSimple(e0, e1, color);
	} else {
		stereographicSegmentComplex(e0, e1, color, color, 0.025f);
	}
}

void GameRenderer::lockedCell(const LockedCells& cells, i32 index, Vec4 color) {
	const auto c = cells.cellBounds(index);
	gfx.circleArcTriangulated(Vec2(0.0f), c.maxR - (c.maxR - c.minR) / 2.0f, c.minA, c.maxA, c.maxR - c.minR, color);
}

void GameRenderer::renderClear() {
	gfx.camera.zoom = 0.9f;

	if (settings.drawBackgrounds) {
		renderBackground();
	} else {
		const auto v = 0.1f;
		glClearColor(v, v, v, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	}
	gfx.diskTriangulated(Vec2(0.0f), 1.0f, Vec4(Color3::BLACK, 1.0f));
	gfx.drawFilledTriangles();

}

void GameRenderer::render(GameEntities& e, const GameState& s, bool editor, f32 invalidGameStateAnimationT) {

	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	{
		glColorMask(0, 0, 0, 0); // Disable color buffer writing.
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // Replace if the stencil test passes else do nothing.
		glStencilFunc(GL_ALWAYS, 1, 0xFF); // Always pass stencil test.

		gfx.diskTriangulated(Vec2(0.0f), 1.0f, Vec4(Color3::BLACK, 1.0f));
		gfx.drawFilledTriangles();

		glColorMask(1, 1, 1, 1); // Enable color buffer writing.
		glStencilFunc(GL_EQUAL, 1, 0xFF); // Pass stencil test only in the stencil set region.
	}


	// If you triangulate things consistently that is if things share a side then they are made of triangles that share sides then you don't have to worry about gaps. When triangulating circular arc you just need to make sure that the number of discretization points is dependent only on the arc could make it constant or dependent on length for example.
	for (const auto& cell : e.lockedCells.cells) {
		lockedCell(e.lockedCells, cell, GameRenderer::lockedCellColor);
	}

	// Given objects and alpha transparency doesn't add much. With thin lines its barerly visible. Also it causes flicker sometimes when double overlap from the same laser appears.
	// srcAlpha * srcColor + 1 * dstColor
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	i32 drawnSegments = 0;
	for (const auto& segment : s.laserSegmentsToDraw) {
		if (segment.ignore) {
			continue;
		}
		drawnSegments++;
		/*renderer.stereographicSegment(segment.endpoints[0], segment.endpoints[1], Vec4(segment.color, 0.5f), 0.05f);*/
		stereographicSegment(segment.endpoints[0], segment.endpoints[1], segment.color);
	}
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const auto activatableObjectColor = [](Vec3 baseColor, bool isActivated) {
		return isActivated ? baseColor : baseColor / 2.0f;
	};

	//const auto darkGreen = Vec3(2, 48, 32) / 255.0f;
	//const auto darkRed = Vec3(255, 87, 51) / 255.0f;

	for (const auto& wall : e.walls) {
		this->wall(wall->endpoints[0], wall->endpoints[1], wallColor(wall->type), editor);
	}

	for (const auto& door : e.doors) {
		const auto info = GameState::triggerInfo(e.triggers, door->triggerIndex);
		Vec3 color = absorbingColor;
		if (info.has_value()) {
			color = info->currentColor;
		}

		if (editor) {
			gfx.disk(door->endpoints[0], grabbableCircleRadius, Color3::RED);
			gfx.disk(door->endpoints[1], grabbableCircleRadius, Color3::RED);
		}

		const auto segments = door->segments();
		for (const auto& segment : segments) {
			stereographicSegment(segment.endpoints[0], segment.endpoints[1], color);
		}
	}

	//renderWalls();

	for (const auto& mirror : e.mirrors) {
		this->mirror(mirror.entity);
	}

	//gfx.drawCircles();
	//gfx.drawFilledTriangles();
	//gfx.drawDisks();

	for (const auto& portalPair : e.portalPairs) {
		for (const auto& portal : portalPair->portals) {
			const auto endpoints = portal.endpoints();

			// Instead of rendering 2 segments could render on that has 2 colors by making a custom triangulation. Then the rendering would need to swap wether the inside or outside of the circle is the side with the normal.
			const auto portalColor = (Color3::RED + Color3::YELLOW) / 2.0f;
			switch (portal.wallType) {
				using enum EditorPortalWallType;
			case PORTAL:
				stereographicSegment(endpoints[0], endpoints[1], portalColor);
				break;
			case ABSORBING:
				multicoloredSegment(endpoints, portal.normalAngle, portalColor, absorbingColor);
				break;

			case REFLECTING:
				multicoloredSegment(endpoints, portal.normalAngle, portalColor, reflectingColor);
				break;
			}
			for (const auto& endpoint : endpoints) {
				gfx.disk(endpoint, grabbableCircleRadius, movablePartColor(portal.rotationLocked));
			}
			gfx.disk(portal.center, grabbableCircleRadius, movablePartColor(portal.positionLocked));
		}
	}

	auto drawOrb = [&](Vec2 center, f32 radius, Vec3 color, f32 activationAnimationT) {
		const auto currentColor = currentOrbColor(color, activationAnimationT);
		//const auto c = activatableObjectColor(color, activationT);
		const auto centers = splitStereographicCircle(center, radius);
		for (const auto& center : centers) {
			if (simplifiedGraphics) {
				const auto circle = stereographicCircle(center, radius);
				gfx.circle(circle.center, circle.radius, 0.01f, currentColor);
				gfx.disk(center, 0.01f, currentColor);
			} else {
				addStereographicDisk(center, radius, currentColor, color / 2.0f);
			}
		}
	};

	for (const auto& target : e.targets) {
		drawOrb(target->position, target->radius, Color3::MAGENTA, target->activationAnimationT);
	}

	for (const auto& trigger : e.triggers) {
		drawOrb(trigger->position, EditorTrigger::defaultRadius, trigger->color, trigger->activationAnimationT);
	}

	renderStereographicSegmentsComplex();
	renderStereographicDisks();

	glDisable(GL_STENCIL_TEST);

	{
		const auto width = 0.01f;
		const auto radius = Constants::boundary.radius + width / 2.0f;
		const auto color = lerp(Color3::GREEN / 3.0f, Color3::RED / 3.0f, invalidGameStateAnimationT);
		gfx.circleTriangulated(Constants::boundary.center, radius, width, color);
	}
	gfx.drawFilledTriangles();

	for (const auto& laser : e.lasers) {
		gfx.disk(laser->position, 0.02f, movablePartColor(laser->positionLocked));
		const auto arrowhead = laserArrowhead(laser.entity);
		/*renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), grabbableCircleRadius, movablePartColor(false));*/
		gfx.lineTriangulated(arrowhead.tip, arrowhead.ears[0], 0.01f, movablePartColor(false));
		gfx.lineTriangulated(arrowhead.tip, arrowhead.ears[1], 0.01f, movablePartColor(false));
	}

	gfx.drawFilledTriangles();
	gfx.drawCircles();
	gfx.drawDisks();
	gfx.drawLines();
}

void GameRenderer::renderBackground() {
	backgroundShader.use();
	static f32 elapsed = 0.0f;
	elapsed += Constants::dt;

	BackgroundInstance instance{
		.clipToWorld = gfx.camera.clipSpaceToWorldSpace(),
		.cameraPosition = gfx.camera.pos,
		.time = elapsed,
	};
	drawInstances(backgroundVao, gfx.instancesVbo, constView(instance), quad2dPtDrawInstances);
}

void GameRenderer::addStereographicSegmentComplex(Vec2 endpoint0, Vec2 endpoint1, Vec3 color0, Vec3 color1, f32 width) {
	//const auto l0 = endpoint0.length();
	//const auto l1 = endpoint1.length();
	//if (l0 > 1.0f) {
	//	endpoint0 /= l0;
	//}
	//if (l1 > 1.0f) {
	//	endpoint1 /= l1;
	//}

	const auto line = stereographicLine(endpoint0, endpoint1);
	f32 rectangleWidth;
	Vec2 center(0.0f);
	const auto chordCenter = (endpoint0 + endpoint1) / 2.0f;
	const auto additionalWidth = 0.04f;
	if (line.type == StereographicLine::Type::LINE) {
		rectangleWidth = additionalWidth * 2.0f;
		center = chordCenter;
	} else {
		rectangleWidth = (line.circle.radius - distance(chordCenter, line.circle.center));
		const auto circleCenterToChord = chordCenter - line.circle.center;
		center = chordCenter + circleCenterToChord.normalized() * (rectangleWidth / 2.0f);
	}
	const auto angle = (endpoint1 - endpoint0).angle();
	const auto size = Vec2((endpoint1 - endpoint0).length() + additionalWidth, rectangleWidth + additionalWidth);
	StereographicLineInstance instance{
		// The rectangle isn't the most optimal shape, could divide the shape into multiple parts to optimize further. When both ends are on the boundary the line coves quite a bit of the screen. Could make the rect calculation for parts of the curve and then rendering with the same endpoints specified.
		.transform = gfx.camera.makeTransform(center, angle, size / 2.0f),
		//.transform = Mat3x2::identity, // Makes it fullscreen
		.endpoint0 = endpoint0,
		.endpoint1 = endpoint1,
		.color0 = color0,
		.color1 = color1,
		.halfWidth = width / 2.0f
	};
	stereographicLines.push_back(instance);
}

void GameRenderer::renderStereographicSegmentsComplex() {
	stereographicLineShader.use();
	StereographicLineVertUniforms uniforms{
		.clipToWorld = gfx.camera.clipSpaceToWorldSpace()
	};
	shaderSetUniforms(stereographicLineShader, uniforms);
	drawInstances(stereographicLineVao, gfx.instancesVbo, constView(stereographicLines), quad2dPtDrawInstances);
	stereographicLines.clear();
}

void GameRenderer::stereographicSegmentComplex(Vec2 endpoint0, Vec2 endpoint1, Vec3 color0, Vec3 color1, f32 width) {
	if (abs(endpoint0.length() - 1.0f) < 0.001f && abs(endpoint1.length() - 1.0f) < 0.001f && (endpoint0 + endpoint1).length() < 0.001f) {
		addStereographicSegmentComplex(endpoint0, Vec2(0.0f), color0, color1, width);
		addStereographicSegmentComplex(endpoint1, Vec2(0.0f), color0, color1, width);
		return;
	}

	addStereographicSegmentComplex(endpoint0, endpoint1, color0, color1, width);
	if (endpoint0.length() >= 1.0f || endpoint1.length() >= 1.0f) {
		// This is done for example to make thing like walls on the boundary appear on both sides of the circle.
		const auto line = stereographicLine(endpoint0, endpoint1);
		const auto a0 = antipodalPoint(endpoint0);
		const auto a1 = antipodalPoint(endpoint1);
		// Adding the antipodal line might create a lot of points the are renderered but lie outside the circle.
		// The above no longer holds, because a stencil buffer is used.
		addStereographicSegmentComplex(a0, a1, color1, color0, width);
	}
}

void GameRenderer::renderStereographicDisks() {
	stereographicDiskShader.use();
	StereographicDiskVertUniforms uniforms{
		.clipToWorld = gfx.camera.clipSpaceToWorldSpace()
	};
	shaderSetUniforms(stereographicDiskShader, uniforms);
	drawInstances(stereographicDiskVao, gfx.instancesVbo, constView(stereographicDisks), quad2dPtDrawInstances);
	stereographicDisks.clear();
}

void GameRenderer::addStereographicDisk(Vec2 center, f32 radius, Vec3 colorInside, Vec3 colorBorder) {
	const auto circle = stereographicCircle(center, radius);
	StereographicDiskInstance instance{
		.transform = gfx.camera.makeTransform(circle.center, 0.0f, Vec2(circle.radius + 0.01f)),
		.center = center,
		.radius = radius,
		.colorInside = colorInside,
		.colorBorder = colorBorder
	};
	stereographicDisks.push_back(instance);
}

void GameRenderer::changeTextColorRngSeed() {
	textColorRngSeed = u32(time(NULL));
}

#include <game/Ui.hpp>

//void GameRenderer::gameText(Vec2 bottomLeftPosition, float maxHeight, std::string_view text, Vec3 color, f32 hoverT) {
//	const auto toUiSpace = Mat3x2::scale(Vec2(2.0f)) * gfx.camera.worldToCameraToNdc();
//
//	TextRenderInfoIterator iterator(font, bottomLeftPosition, toUiSpace, maxHeight, text);
//	for (auto info = iterator.next(); info.has_value(); info = iterator.next()) {
//		gameTextInstances.push_back(GameTextInstance{
//			.transform = info->transform,
//			.offsetInAtlas = info->offsetInAtlas,
//			.sizeInAtlas = info->sizeInAtlas,
//			.color = textColorRng.colorRandomHue(1.0f, 1.0f),
//			.randomValue = textColorRng.dist(textColorRng.rng),
//			.hoverT = hoverT
//		});
//	}
//}

void GameRenderer::gameText(Vec2 bottomLeftPosition, float maxHeight, std::string_view text, f32 hoverT, std::optional<Vec3> color) {
	const auto toUiSpace = Mat3x2::scale(Vec2(2.0f)) * gfx.camera.worldToCameraToNdc();

	TextRenderInfoIterator iterator(font, bottomLeftPosition, toUiSpace, maxHeight, text);
	for (auto info = iterator.next(); info.has_value(); info = iterator.next()) {
		Vec3 c(0.0f);
		if (color.has_value()) {
			c = *color;
		} else {
			c = textColorRng.colorRandomHue(1.0f, 1.0f);
		}

		gameTextInstances.push_back(GameTextInstance{
			.transform = info->transform,
			.offsetInAtlas = info->offsetInAtlas,
			.sizeInAtlas = info->sizeInAtlas,
			.color = c,
			.randomValue = textColorRng.dist(textColorRng.rng),
			.hoverT = hoverT
		});
	}
}

Vec2 textCenteredPosition(const Font& font, Vec2 center, f32 maxHeight, std::string_view text) {
	const auto info = font.textInfo(maxHeight, text);
	Vec2 position = center;
	position.y -= info.bottomY;
	position -= info.size / 2.0f;
	return position;
}

void GameRenderer::gameTextCentered(Vec2 position, float maxHeight, std::string_view text, f32 hoverT, std::optional<Vec3> color) {
	position = Ui::posToWorldSpace(*this, position) / 2.0f;
	gameText(textCenteredPosition(font, position, maxHeight, text), maxHeight, text, hoverT, color);
}

void GameRenderer::renderGameText() {
	glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blending
	gameTextShader.use();
	gameTextShader.setTexture("fontAtlas", 0, font.fontAtlas);
	static f32 elapsed = 0.0f;
	elapsed += Constants::dt;
	shaderSetUniforms(gameTextShader, GameTextFragUniforms{
		.time = elapsed
	});
	drawInstances(gameTextVao, gfx.instancesVbo, constView(gameTextInstances), quad2dPtDrawInstances);
	gameTextInstances.clear();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GameRenderer::glowingArrow(const Aabb& rect, f32 hoverAnimationT, f32 opacity) {
	glowingArrowShader.use();
	static f32 elapsed = 0.0f;
	elapsed += Constants::dt;
	GlowingArrowInstance instance{
		.transform = gfx.camera.makeTransform(rect.center(), 0.0f, rect.size() / 2.0f),
		.widthOverHeight = rect.size().x / rect.size().y,
		.hoverT = hoverAnimationT,
		.opacity = opacity
	};
	drawInstances(glowingArrowVao, gfx.instancesVbo, constView(instance), quad2dPtDrawInstances);
}

Vec3 movablePartColor(bool isPositionLocked) {
	const auto movableColor = Color3::YELLOW;
	const auto nonMovableColor = Vec3(0.3f);
	return isPositionLocked
		? nonMovableColor
		: movableColor;
}

ColorRng::ColorRng()
	: dist(0.0f, 1.0f) {}

void ColorRng::seed(u32 seed) {
	rng.seed(seed);
}

Vec3 ColorRng::colorRandomHue(f32 s, f32 v) {
	const auto blueMin = 208.0f / 360.0f;
	const auto blueMax = 259.0f / 360.0f;
	const auto nonBlueArea0 = blueMin;
	const auto nonBlueArea1 = 1.0f - blueMax;
	const auto totalArea = nonBlueArea0 + nonBlueArea1;
	// blue text is hard to read.
	f32 h;
	if (std::bernoulli_distribution(nonBlueArea0 / totalArea)(rng)) {
		h = std::uniform_real_distribution<f32>(0.0f, blueMin)(rng);
	} else {
		h = std::uniform_real_distribution<f32>(blueMax, 1.0f)(rng);
	}
	return Color3::fromHsv(h, s, v);
}
