#include <game/EditorEntities.hpp>
#include <array>
#include <engine/Math/Interpolation.hpp>
#include <engine/Math/Rotation.hpp>
#include <engine/Math/Quat.hpp>
#include <engine/Math/LineSegment.hpp>
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

EditorDoor EditorDoor::DefaultInitialize::operator()() {
	return EditorDoor{ .endpoints = { Vec2(0.0f), Vec2(1.0f) }, .triggerIndex = 0, .openByDefault = false };
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

EditorEntityId::EditorEntityId(const EditorDoorId& id)
	: type(EditorEntityType::DOOR)
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

EditorDoorId EditorEntityId::door() const {
	ASSERT(type == EditorEntityType::DOOR);
	return EditorDoorId(index, version);
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

Vec3 currentOrbColor(Vec3 color, f32 activationAnimationT) {
	return lerp(color / 2.0f, color, activationAnimationT);
}

void editorLaserColorCombo(Vec3& selectedColor) {
	ColorEntry entries[]{
		EditorLaser::defaultColor,
		/*{ Vec3(81, 255, 13) / 255.0f, "green"},*/
		{ Color3::GREEN, "green" },
		{ Color3::RED, "red" }
	};
	colorCombo("color", constView(entries), EditorLaser::defaultColor, selectedColor);
}

LaserGrabPoint laserDirectionGrabPoint(const EditorLaser& laser) {
	const auto eucledianLength = 0.15f;
	const auto laserDirection = Vec2::oriented(laser.angle);
	
	const auto actualGrabPoint = laser.position + laserDirection * eucledianLength;
	const auto endpoint0 = moveOnStereographicGeodesic(laser.position, laser.angle, 0.1f);
	const auto line = stereographicLine(endpoint0, laser.position);
	if (line.type == StereographicLine::Type::LINE) {
		return LaserGrabPoint{
			.point = actualGrabPoint,
			.visualPoint = actualGrabPoint,
			.visualTangent = laserDirection
		};
	}
	auto arcLength = eucledianLength / line.circle.radius;
	const auto startAngle = (laser.position - line.circle.center).angle();
	const auto tangentAtStartAngle = Vec2::oriented(startAngle).rotBy90deg();
	if (dot(tangentAtStartAngle, laserDirection) > 0.0f) {
		arcLength = -arcLength;
	}

	const auto normalAtGrabPoint = Vec2::oriented(startAngle + arcLength);
	const auto tangentAtGrabPoint = normalAtGrabPoint.rotBy90deg() * ((arcLength > 0.0f) ? -1.0f : 1.0f);

	return LaserGrabPoint{
		.point = actualGrabPoint,
		.visualPoint = line.circle.center + normalAtGrabPoint * line.circle.radius,
		.visualTangent = tangentAtGrabPoint
	};
}

LaserArrowhead laserArrowhead(const EditorLaser& laser) {
	const auto tip = laserDirectionGrabPoint(laser);
	const auto rotation = Rotation(3.0f / 4.0f * PI<f32>);
	const auto ear0 = tip.visualPoint + rotation * tip.visualTangent * 0.03f;
	const auto ear1 = tip.visualPoint + rotation.inversed() * tip.visualTangent * 0.03f;
	return LaserArrowhead{
		.actualTip = tip.point,
		.tip = tip.visualPoint,
		.ears = { ear0, ear1 },
	};
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

void editorTriggerIndexInput(const char* label, i32& index) {
	ImGui::InputInt("index", &index);
	index = std::clamp(index, 0, 1000);
}

std::array<Vec2, 2> EditorPortal::endpoints() const {
	return rotatableSegmentEndpoints(center, normalAngle, defaultLength);
}

Circle EditorTrigger::circle() const {
	return stereographicCircle(position, defaultRadius);
}

StaticList<EditorDoorSegment, 2> EditorDoor::segments() const {
	StaticList<EditorDoorSegment, 2> result;

	auto t = openingT;
	if (openByDefault) {
		t = 1.0f - t;
	}

	if (t == 0.0f) {
		result.add(EditorDoorSegment{ .endpoints = { endpoints[0], endpoints[1] } });
		return result;
	} 
	if (t == 1.0f) {
		return result;
	}
	const auto line = stereographicLine(endpoints[0], endpoints[1]);
	const auto length = stereographicDistance(endpoints[0], endpoints[1]);

	auto tangentAtE0TowardsE1 = stereographicLineNormalAt(line, endpoints[0]).rotBy90deg();
	if (dot(tangentAtE0TowardsE1, endpoints[1] - endpoints[0]) < 0.0f) {
		tangentAtE0TowardsE1 = -tangentAtE0TowardsE1;
	}
	const auto tangentAngle = tangentAtE0TowardsE1.angle();

	const auto segmentLength = length / 2.0f * (1.0f - t);
	const auto m0 = moveOnStereographicGeodesic(endpoints[0], tangentAngle, segmentLength);
	const auto m1 = moveOnStereographicGeodesic(endpoints[0], tangentAngle, length - segmentLength);

	result.add(EditorDoorSegment{ { endpoints[0], m0 }});
	result.add(EditorDoorSegment{ { m1, endpoints[1] }});

	return result;

	// Could instead of calculating midpoint move from on endpoint d units and length - d units. This would require calculating the direction and 2 goedesic move operations.
	//const auto e0 = fromStereographic(endpoints[0]);
	//const auto e1 = fromStereographic(endpoints[1]);

	//const auto midpoint = sphericalGeodesicSegmentMidpoint(e0, e1);
	//const auto tangentAtMidpoint = stereographicLineNormalAt(line, toStereographic(midpoint)).rotBy90deg();
	//const auto moveOnSphericalGeodesic(midpoint, tangentAtMidpoint.angle(), openAmount);
}

void GameEntities::reset() {
	walls.reset();
	lasers.reset();
	mirrors.reset();
	targets.reset();
	portalPairs.reset();
	triggers.reset();
	doors.reset();
}

f32 LaserArrowhead::distanceTo(Vec2 v) const {
	return std::min(
		LineSegment(tip, ears[0]).distance(v), 
		LineSegment(tip, ears[1]).distance(v));
}

std::optional<i32> LockedCells::getIndex(Vec2 pos) const {
	const auto a = angleToRangeZeroTau(pos.angle());
	const auto r = pos.length();
	const auto ai = i32(floor(a / TAU<f32> * segmentCount));
	const auto ri = i32(floor(r * ringCount));
	if (ri >= ringCount) {
		return std::nullopt;
	}
	return ai * ringCount + ri;
}

LockedCells::CellBounds LockedCells::cellBounds(i32 index) const {
	const auto ai = index / ringCount;
	const auto ri = index % ringCount;
	const auto startR = f32(ri) / f32(ringCount);
	const auto endR = f32(ri + 1) / f32(ringCount);
	const auto startA = (f32(ai) / f32(segmentCount)) * TAU<f32>;
	const auto endA = (f32(ai + 1) / f32(segmentCount)) * TAU<f32>;
	return CellBounds{
		.minA = startA,
		.maxA = endA,
		.minR = startR,
		.maxR = endR
	};
}

bool LockedCells::CellBounds::containsPoint(Vec2 v) const {
	const auto a = angleToRangeZeroTau(v.angle());
	const auto r = v.length();
	return a >= minA && a <= maxA && r >= minR && r <= maxR;
}
