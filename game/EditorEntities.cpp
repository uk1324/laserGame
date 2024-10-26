#include <game/EditorEntities.hpp>
#include <array>
#include <engine/Math/Quat.hpp>
#include <engine/Math/Constants.hpp>
#include <imgui/imgui.h>
#include <game/Stereographic.hpp>

EditorWall EditorWall::DefaultInitialize::operator()() {
	return EditorWall{
		.endpoints = { Vec2(0.0f), Vec2(0.0f) },
		.type = static_cast<EditorWallType>(-1)
	};
}

EditorLaser EditorLaser::DefaultInitialize::operator()() {
	return EditorLaser{
		.position = Vec2(0.0f),
		.angle = 0.0f
	};
}

EditorMirror EditorMirror::DefaultInitialize::operator()() {
	return EditorMirror{
		.center = Vec2(0.0f),
		.normalAngle = 0.0f
	};
}

EditorTarget EditorTarget::DefaultInitialize::operator()() {
	return EditorTarget{
		.position = Vec2(0.0f) 
	};
}

EditorEntityId::EditorEntityId(const EditorWallId& id) 
	: type(EditorEntityType::WALL) 
	, index(id.index())
	, version(id.version()) {}

EditorEntityId::EditorEntityId(const EditorLaserId& id)
	: type(EditorEntityType::LASER)
	, index(id.index())
	, version(id.version()) {}

EditorEntityId::EditorEntityId(const EditorMirrorId& id)
	: type(EditorEntityType::MIRROR)
	, index(id.index())
	, version(id.version()) {}

EditorEntityId::EditorEntityId(const EditorTargetId& id) 
	: type(EditorEntityType::TARGET)
	, index(id.index())
	, version(id.version()) {}

EditorWallId EditorEntityId::wall() const {
	ASSERT(type == EditorEntityType::WALL);
	return EditorWallId(index, version);
}

EditorLaserId EditorEntityId::laser() const {
	ASSERT(type == EditorEntityType::LASER);
	return EditorLaserId(index, version);
}

EditorMirrorId EditorEntityId::mirror() const {
	ASSERT(type == EditorEntityType::MIRROR);
	return EditorMirrorId(index, version);
}

EditorTargetId EditorEntityId::target() const {
	ASSERT(type == EditorEntityType::TARGET);
	return EditorTargetId(index, version);
}

std::array<Vec2, 2> EditorMirror::calculateEndpoints() const {
	f32 halfLength = 0.6f;
	const auto c = fromStereographic(center);
	const auto a = atan2(center.y, center.x);

	const auto axis = cross(c, Vec3(0.0f, 0.0f, 1.0f));

	// The formula for the angle was kinda derived through trial and error, but it matches the results of the correct method that isn't used, because it can't handle length specification. That version is available in previous commits in the mirror draw loop.
	const auto rotateLine = Quat(-normalAngle + a + PI<f32> / 2.0f, c);
	const auto endpoint0 = rotateLine * (Quat(halfLength, axis) * c);
	const auto endpoint1 = rotateLine * (Quat(-halfLength, axis) * c);
	// Normalizing, because rotating introduces errors in the norm that get amplified by the projection. 
	const auto e0 = toStereographic(endpoint0.normalized());
	const auto e1 = toStereographic(endpoint1.normalized());

	return { e0, e1 };
}

Circle EditorTarget::calculateCircle() const {
	const auto center = fromStereographic(position);
	f32 radius = 0.1f;

	const auto axis = cross(center, Vec3(0.0f, 0.0f, 1.0f));
	// Point equidistant from center in there sphere metric.
	const auto p0 = Quat(radius, axis) * center;
	const auto p1 = Quat(-radius, axis) * center;
	const auto p2 = Quat(PI<f32> / 2.0f, center) * p1;

	// The stereographic projection of center isn't necessarily the center of the stereograhic of the `circle on the sphere`. For small radii this isn't very noticible.

	return circleThroughPoints(toStereographic(p0), toStereographic(p1), toStereographic(p2));
}

void wallTypeCombo(const char* label, EditorWallType& type) {
	ImGui::Combo(
		"type",
		reinterpret_cast<int*>(&type),
		"reflecting\0absorbing\0");
}
