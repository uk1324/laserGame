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
		.angle = 0.0f,
		.color = Vec3(0.123f)
	};
}

EditorMirror EditorMirror::DefaultInitialize::operator()() {
	return EditorMirror(Vec2(0.0f), 0.0f, 1.0f, false, EditorMirrorWallType::REFLECTING);
}

EditorTarget EditorTarget::DefaultInitialize::operator()() {
	return EditorTarget{
		.position = Vec2(0.0f) ,
		.radius = 1.0f
	};
}

EditorPortalPair EditorPortalPair::DefaultInitialize::operator()() {
	const auto portal = EditorPortal{ .center = Vec2(0.0f), .normalAngle = 0.0f };
	return EditorPortalPair{
		.portals = { portal, portal }
	};
}

EditorTrigger EditorTrigger::DefaultInitialize::operator()() {
	return EditorTrigger{ .position = Vec2(0.0f), .color = Vec3(0.0f), .index = 0};
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

EditorEntityId::EditorEntityId(const EditorPortalPairId& id)
	: type(EditorEntityType::PORTAL_PAIR)
	, index(id.index())
	, version(id.version()) {}

EditorEntityId::EditorEntityId(const EditorTriggerId& id)
	: type(EditorEntityType::TRIGGER)
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

EditorPortalPairId EditorEntityId::portalPair() const {
	ASSERT(type == EditorEntityType::PORTAL_PAIR);
	return EditorPortalPairId(index, version);
}

EditorTriggerId EditorEntityId::trigger() const {
	ASSERT(type == EditorEntityType::TRIGGER);
	return EditorTriggerId(index, version);
}

EditorMirror::EditorMirror(Vec2 center, f32 normalAngle, f32 length, bool positionLocked, EditorMirrorWallType wallType)
	: center(center)
	, normalAngle(normalAngle)
	, length(length) 
	, positionLocked(positionLocked)
	, wallType(wallType)
{}

// Could do this https://stackoverflow.com/questions/1171849/finding-quaternion-representing-the-rotation-from-one-vector-to-another

//Vec3 somePerpendicularVector(Vec3 v) {
//	// The space of all vector perpendicular to v can be found using the cross product formula
//	// (x, y, z) x (a, b, c) = 
//	// (yc - bz, -(xc - az), xb - ay) = 
//	// (yc - bz, -xc + az, xb - ay) = 
//	// a * (0, z, -y) + b * (-z, 0, x) + c * (y, -x, 0)
//
//	//if (v.x != 0.0f) {
//	//	return 
//	//}
//}

//// v0, v1 assumed to be normalized
//Quat shortestRotationThatMovesV0ToV1(Vec3 v0, Vec3 v1) {
//	Vec3 axis = cross(v0, v1).normalized();
//	f32 angle = acos(dot(v0, v1));
//
//	if (v0 == v1) {
//		return Quat::identity;
//	}
//	if (v0 == -v1) {
//		// TODO: Infinitely many options pick one.
//	}
//
//	return Quat(angle, axis);
//}
//
//// normalizedVelocity or direction
//Vec3 moveOnSphericalGeodesic(Vec3 pos, Vec3 normalizedVelocity, f32 distance) {
//	Vec3 movementAxis = cross(Vec3((0.0f, 0.0f, 1.0f)), normalizedVelocity).normalized();
//	return Quat(distance, movementAxis) * pos;
//}
//
//Vec3 moveOnSphericalGeodesic(Vec3 pos, f32 directionAngle, f32 distance) {
//	Vec3 velocity = Vec3(cos(directionAngle), sin(directionAngle), 0.0f);
//	Vec3 movementAxis = cross(Vec3((0.0f, 0.0f, 1.0f)), velocity).normalized();
//	Vec3 point = Quat(distance, movementAxis) * Vec3(0.0f, 0.0f, -1.0f);
//	point *= shortestRotationThatMovesV0ToV1(Vec3(0.0f, 0.0f, -1.0f), pos);
//	return point;
//}

//// The angle is such that when stereographically projected it equal the plane angle.
//Quat movementOnSphericalGeodesic(Vec3 pos, f32 angle, f32 distance) {
//	const auto a = atan2(pos.y, pos.x);
//	const auto up = Vec3(0.0f, 0.0f, 1.0f);
//
//	Vec3 axis(0.0f, 1.0f, 0.0f);
//	if (pos != -up) {
//		axis = cross(pos, up).normalized();
//	}
//
//	// The formula for the angle was kinda derived through trial and error, but it matches the results of the correct method that isn't used, because it can't handle length specification. That version is available in previous commits in the mirror draw loop.
//	return Quat(-angle + a, pos) * Quat(distance, axis);
//}
//
//Vec3 moveOnSphericalGeodesic(Vec3 pos, f32 angle, f32 distance) {
//	// Normalizing, because rotating introduces errors in the norm that get amplified by the projection. 
//	return (pos * movementOnSphericalGeodesic(pos, angle, distance)).normalized();
//}

std::array<Vec2, 2> EditorMirror::calculateEndpoints() const {
	return rotatableSegmentEndpoints(center, normalAngle, length);
}

Circle EditorTarget::calculateCircle() const {
	return stereographicCircle(position, radius);
}

void colorCombo(const char* label, View<const ColorEntry> colors, ColorEntry defaultColor, Vec3& selectedColor) {
	const char* selectedName = nullptr;

	for (const auto& [color, name] : colors) {
		if (selectedColor == color) {
			selectedName = name;
			break;
		}
	}

	if (selectedName == nullptr) {
		selectedColor = defaultColor.color;
	}

	if (ImGui::BeginCombo(label, selectedName)) {
		for (const auto& [color, name] : colors) {
			const auto isSelected = selectedColor == color;
			ImGui::ColorButton("##colorDisplay", ImVec4(color.x, color.y, color.z, 1.0f));
			ImGui::SameLine();
			if (ImGui::Selectable(name, isSelected)) {
				selectedColor = color;
			}

			if (isSelected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}

void wallTypeCombo(const char* label, EditorWallType& type) {
	ImGui::Combo(
		"type",
		reinterpret_cast<int*>(&type),
		"reflecting\0absorbing\0");
}

void editorLaserColorCombo(Vec3& selectedColor) {
	ColorEntry entries[]{
		EditorLaser::defaultColor,
		{ Vec3(81, 255, 13) / 255.0f, "green"},
		{ Color3::RED, "red" }
	};
	colorCombo("color", constView(entries), EditorLaser::defaultColor, selectedColor);
}

std::array<Vec2, 2> rotatableSegmentEndpoints(Vec2 center, f32 normalAngle, f32 length) {
	const auto c = fromStereographic(center);
	const auto endpoint0 = moveOnSphericalGeodesic(c, normalAngle + PI<f32> / 2.0f, length / 2.0f);
	const auto endpoint1 = Quat(PI<f32>, c) * endpoint0;
	const auto e0 = toStereographic(endpoint0.normalized());
	const auto e1 = toStereographic(endpoint1.normalized());
	return { e0, e1 };
}

void sliderHelp() {
	ImGui::TextDisabled("(?)");
	ImGui::SetItemTooltip("Click while holding ctrl to input the value directly.");
}

void sliderFloat(const char* label, f32& value, f32 min, f32 max) {
	ImGui::SliderFloat(label, &value, min, max);
	value = std::clamp(value, min, max);
	sliderHelp();
}

void editorMirrorLengthInput(f32& length) {
	sliderFloat("length", length, 0.1f, 2.0f);
}

void editorMirrorWallTypeInput(EditorMirrorWallType& type) {
	ImGui::Combo("wall type", reinterpret_cast<int*>(&type), "reflecting\0absorbing\0");
}

void editorTargetRadiusInput(f32& radius) {
	sliderFloat("radius", radius, 0.05f, 1.5f);
}

void editorTriggerColorCombo(Vec3& color) {
	ColorEntry entries[]{
		EditorTrigger::defaultColor,
		{ Vec3(0.0f, 0.5f, 1.0f), "azure"},
		{ Vec3(1.0f, 0.0f, 0.5f), "rose"}
	};
	colorCombo("color", constView(entries), EditorLaser::defaultColor, color);
}

std::array<Vec2, 2> EditorPortal::endpoints() const {
	return rotatableSegmentEndpoints(center, normalAngle, defaultLength);
}

Circle EditorTrigger::circle() const {
	return stereographicCircle(position, defaultRadius);
}
