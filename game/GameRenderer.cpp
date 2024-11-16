#include "GameRenderer.hpp"
#include <engine/Math/Color.hpp>
#include <Overloaded.hpp>
#include <game/Shaders/backgroundData.hpp>
#include <engine/Math/Constants.hpp>
#include <game/Stereographic.hpp>
#include <glad/glad.h>
#include <gfx/Instancing.hpp>
#include <engine/Window.hpp>
#include <StructUtils.hpp>
#include <gfx/ShaderManager.hpp>

#define MAKE_VAO(Type) \
	createInstancingVao<Type##Shader>(gfx.quad2dPtVbo, gfx.quad2dPtIbo, gfx.instancesVbo)

GameRenderer GameRenderer::make() {
	auto gfx = Gfx2d::make();

	return GameRenderer{
		.backgroundVao = MAKE_VAO(Background),
		.backgroundShader = MAKE_GENERATED_SHADER(BACKGROUND),
		MOVE(gfx)
	};
}

Vec3 GameRenderer::wallColor(EditorWallType type) {
	return type == EditorWallType::REFLECTING
		? reflectingColor
		: absorbingColor;
}

void GameRenderer::multicoloredSegment(const std::array<Vec2, 2>& endpoints, f32 normalAngle, Vec3 normalColor, Vec3 backColor) {
	const auto normal = Vec2::oriented(normalAngle);
	const auto forward = normal * 0.005f;
	const auto back = -normal * 0.005f;
	stereographicSegment(endpoints[0] + forward, endpoints[1] + forward, normalColor);
	stereographicSegment(endpoints[0] + back, endpoints[1] + back, backColor);
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
	for (const auto endpoint : endpoints) {
		gfx.disk(endpoint, grabbableCircleRadius, movablePartColor(false));
	}
	gfx.disk(mirror.center, grabbableCircleRadius, movablePartColor(mirror.positionLocked));
}

void GameRenderer::renderWalls() {
	gfx.drawLines();
	gfx.drawFilledTriangles();
	gfx.drawDisks();
}

void GameRenderer::stereographicSegmentOld(Vec2 e0, Vec2 e1, Vec3 color) {
	const auto line = stereographicLineOld(e0, e1);

	const auto range = angleRangeBetweenPointsOnCircle(line.center, e0, e1);

	gfx.circleArcTriangulated(line.center, line.radius, range.min, range.max, Constants::wallWidth, color, 1000);
}

void GameRenderer::stereographicSegment(Vec2 e0, Vec2 e1, Vec3 color) { 
	stereographicSegment(e0, e1, Vec4(color, 1.0f));
}

void GameRenderer::stereographicSegment(Vec2 e0, Vec2 e1, Vec4 color, f32 width) {
	const auto stereographicLine = ::stereographicLine(e0, e1);
	if (stereographicLine.type == StereographicLine::Type::LINE) {
		gfx.lineTriangulated(e0, e1, width, color);
	} else {
		const auto& line = stereographicLine.circle;

		const auto range = angleRangeBetweenPointsOnCircle(line.center, e0, e1);
		gfx.circleArcTriangulated(line.center, line.radius, range.min, range.max, width, color, 1000);
	}
}

void GameRenderer::renderClear() {
	gfx.camera.aspectRatio = Window::aspectRatio();
	gfx.camera.zoom = 0.9f;
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, GLsizei(Window::size().x), GLsizei(Window::size().y));

	renderBackground();
	gfx.diskTriangulated(Vec2(0.0f), 1.0f, Vec4(Color3::BLACK, 1.0f));
	gfx.drawFilledTriangles();
}

void GameRenderer::render(GameEntities& e, const GameState& s, bool editor, bool validGameState) {
	i32 drawnSegments = 0;
	// Given objects and alpha transparency doesn't add much. With thin lines its barerly visible. Also it causes flicker sometimes when double overlap from the same laser appears.
	// srcAlpha * srcColor + 1 * dstColor
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	for (const auto& segment : s.laserSegmentsToDraw) {
		if (segment.ignore) {
			continue;
		}
		drawnSegments++;
		/*renderer.stereographicSegment(segment.endpoints[0], segment.endpoints[1], Vec4(segment.color, 0.5f), 0.05f);*/
		stereographicSegment(segment.endpoints[0], segment.endpoints[1], Vec4(segment.color));
	}
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (const auto& laser : e.lasers) {
		gfx.disk(laser->position, 0.02f, movablePartColor(laser->positionLocked));
		const auto arrowhead = laserArrowhead(laser.entity);
		/*renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), grabbableCircleRadius, movablePartColor(false));*/
		gfx.lineTriangulated(arrowhead.tip, arrowhead.ears[0], 0.01f, movablePartColor(false));
		gfx.lineTriangulated(arrowhead.tip, arrowhead.ears[1], 0.01f, movablePartColor(false));
	}

	const auto activatableObjectColor = [](Vec3 baseColor, bool isActivated) {
		return isActivated ? baseColor : baseColor / 2.0f;
	};

	//const auto darkGreen = Vec3(2, 48, 32) / 255.0f;
	//const auto darkRed = Vec3(255, 87, 51) / 255.0f;
	if (validGameState) {
		gfx.circleTriangulated(Vec2(0.0f), 1.0f, 0.01f, Color3::GREEN / 2.0f);
	} else {
		gfx.circleTriangulated(Vec2(0.0f), 1.0f, 0.01f, Color3::RED / 2.0f);
	}

	for (const auto& wall : e.walls) {
		this->wall(wall->endpoints[0], wall->endpoints[1], wallColor(wall->type), editor);
	}

	for (const auto& door : e.doors) {
		const auto info = GameState::triggerInfo(e.triggers, door->triggerIndex);
		Vec3 color = absorbingColor;
		if (info.has_value()) {
			color = activatableObjectColor(info->color, info->active);
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

	renderWalls();

	for (const auto& mirror : e.mirrors) {
		this->mirror(mirror.entity);
	}

	gfx.drawCircles();
	gfx.drawFilledTriangles();
	gfx.drawDisks();

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
			for (const auto endpoint : endpoints) {
				gfx.disk(endpoint, grabbableCircleRadius, movablePartColor(portal.rotationLocked));
			}
			gfx.disk(portal.center, grabbableCircleRadius, movablePartColor(portal.positionLocked));
		}
	}

	auto drawOrb = [&](Vec2 center, const Circle& circle, Vec3 color) {
		gfx.circle(circle.center, circle.radius, 0.01f, color);
		gfx.disk(center, 0.01f, color);
	};

	for (const auto& target : e.targets) {
		const auto circle = target->calculateCircle();
		drawOrb(target->position, circle, activatableObjectColor(Color3::MAGENTA, target->activated));
	}

	for (const auto& trigger : e.triggers) {
		const auto circle = trigger->circle();
		drawOrb(trigger->position, circle, activatableObjectColor(trigger->color, trigger->activated));
	}

	gfx.drawFilledTriangles();
	gfx.drawCircles();
	gfx.drawDisks();
	gfx.drawLines();
}

void GameRenderer::renderBackground() {
	backgroundShader.use();
	std::vector<BackgroundInstance> instances;
	static f32 elapsed = 0.0f;
	elapsed += Constants::dt;

	BackgroundInstance instance{
		.clipToWorld = gfx.camera.clipSpaceToWorldSpace(),
		.cameraPosition = gfx.camera.pos,
		.time = elapsed,
	};
	drawInstances(backgroundVao, gfx.instancesVbo, constView(instance), [](usize count) {
		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, count);
	});
}

Vec3 movablePartColor(bool isPositionLocked) {
	const auto movableColor = Color3::YELLOW;
	const auto nonMovableColor = Vec3(0.3f);
	return isPositionLocked
		? nonMovableColor
		: movableColor;
}
