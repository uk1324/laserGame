#include <game/EditorEntities.hpp>
#include <array>
#include <engine/Math/Quat.hpp>
#include <engine/Math/Constants.hpp>
#include <game/Stereographic.hpp>

EditorWall EditorWall::DefaultInitialize::operator()() {
	return EditorWall{
		.endpoints = { Vec2(0.0f), Vec2(0.0f) }
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
