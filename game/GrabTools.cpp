#include "GrabTools.hpp"
#include <engine/Input/Input.hpp>
#include <game/Constants.hpp>
#include <engine/Math/Constants.hpp>
#include <array>

static Vec2 normalAtEndpoint0(Vec2 endpoint0, Vec2 endpoint1) {
	const auto line = stereographicLine(endpoint0, endpoint1);

	const auto correctNormalDirection = (endpoint1 - endpoint0).rotBy90deg();
	Vec2 normal(0.0f);
	switch (line.type) {
		using enum StereographicLine::Type;
	case CIRCLE:
		normal = endpoint0 - line.circle.center;
		break;
	case LINE:
		normal = line.lineNormal;
		break;
	}

	// This is done, because stereographicLine changes the center position when the line transitions from CIRCLE to LINE to CIRCLE. This also causes the normal (center - line.circle.center) to change direction, which is visible, for example when using portals.
	if (dot(normal, correctNormalDirection) < 0.0f) {
		normal = -normal;
	}
	return normal;
}

std::optional<GrabbedRotatableSegment> rotatableSegmentCheckGrab(Vec2 center, f32 normalAngle, f32 length,
	Vec2 cursorPos, bool& cursorCaptured, bool cursorExact) {

	std::optional<GrabbedRotatableSegment> grabbed;

	auto updateGrabbed = [&](Vec2 p, RotatableSegmentGizmoType gizmo) {
		if (distance(cursorPos, p) > Constants::endpointGrabPointRadius) {
			return;
		}
		grabbed = GrabbedRotatableSegment{
			.grabOffset = p - cursorPos,
			.grabbedGizmo = gizmo,
		};
		cursorCaptured = true;
	};
	updateGrabbed(center, RotatableSegmentGizmoType::TRANSLATION);
	const auto endpoints = rotatableSegmentEndpoints(center, normalAngle, length);

	if (!cursorCaptured) {
		updateGrabbed(endpoints[0], RotatableSegmentGizmoType::ROTATION_0);
	}

	if (!cursorCaptured) {
		updateGrabbed(endpoints[1], RotatableSegmentGizmoType::ROTATION_1);
	}

	return grabbed;
}

void grabbedRotatableSegmentUpdate(RotatableSegmentGizmoType type, Vec2 grabOffset,
	Vec2& center, f32& normalAngle,
	Vec2 cursorPos, bool cursorExact) {

	const auto newPosition = cursorPos + grabOffset * !cursorExact;
	switch (type) {
		using enum RotatableSegmentGizmoType;
	case TRANSLATION:
		center = newPosition;
		break;

	case ROTATION_0:
	case ROTATION_1: {
		auto normal = normalAtEndpoint0(center, newPosition);

		if (type == ROTATION_1) {
			normal = -normal;
		}

		normalAngle = normal.angle();
		break;
	}

	}
}

void LaserGrabTool::update(LaserArray& lasers, std::optional<EditorActions&> actions, Vec2 cursorPos, bool& cursorCaptured, bool cursorExact, bool enforceConstraints) {

	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !grabbed.has_value()) {
		for (const auto& laser : lasers) {
			const auto arrowhead = laserArrowhead(laser.entity);
			auto updateGrabbed = [&](Vec2 p, f32 distance, LaserGrabTool::LaserPart part) {
				if (distance > Constants::endpointGrabPointRadius) {
					return;
				}

				grabbed = LaserGrabTool::Grabbed{
					.id = laser.id,
					.part = part,
					.grabStartState = laser.entity,
					.offset = p - cursorPos,
				};
				cursorCaptured = true;
			};

			updateGrabbed(arrowhead.tip, arrowhead.distanceTo(cursorPos), LaserGrabTool::LaserPart::DIRECTION);

			const auto positionLocked = enforceConstraints && laser->positionLocked;
			if (!positionLocked && !cursorCaptured) {
				updateGrabbed(laser->position, distance(laser->position, cursorPos), LaserGrabTool::LaserPart::ORIGIN);
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && grabbed.has_value()) {
		cursorCaptured = true;
		auto laser = lasers.get(grabbed->id);
		if (laser.has_value()) {
			const auto newPosition = cursorPos + grabbed->offset * !cursorExact;
			switch (grabbed->part) {
				using enum LaserGrabTool::LaserPart;

			case ORIGIN:
				laser->position = newPosition;
				break;
			case DIRECTION: {
				laser->angle = normalAtEndpoint0(laser->position, newPosition).angle() + PI<f32> / 2.0f;
				break;
			}

			}
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && grabbed.has_value()) {
		cursorCaptured = true;
		auto laser = lasers.get(grabbed->id);
		if (actions.has_value() && laser.has_value()) {
			actions->addModifyAction<EditorLaser, EditorActionModifyLaser>(lasers, grabbed->id, std::move(grabbed->grabStartState));
		} else {
			CHECK_NOT_REACHED();
		}
		grabbed = std::nullopt;
	}
}

void MirrorGrabTool::update(MirrorArray& mirrors, std::optional<EditorActions&> actions, Vec2 cursorPos, bool& cursorCaptured, bool cursorExact, bool enforceConstraints) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !grabbed.has_value()) {
		for (const auto& mirror : mirrors) {
			const auto result = rotatableSegmentCheckGrab(mirror->center, mirror->normalAngle, mirror->length,
				cursorPos, cursorCaptured, cursorExact);

			if (!result.has_value()) {
				continue;
			}

			if (enforceConstraints && mirror->positionLocked 
				&& result->grabbedGizmo == RotatableSegmentGizmoType::TRANSLATION) {
				continue;
			}

			grabbed = MirrorGrabTool::Grabbed{
				.id = mirror.id,
				.grabStartState = mirror.entity,
				.grabbed = *result,
			};
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && grabbed.has_value()) {
		cursorCaptured = true;
		auto mirror = mirrors.get(grabbed->id);
		if (mirror.has_value()) {
			grabbedRotatableSegmentUpdate(grabbed->grabbed.grabbedGizmo, grabbed->grabbed.grabOffset,
				mirror->center, mirror->normalAngle,
				cursorPos, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && grabbed.has_value()) {
		cursorCaptured = true;
		auto mirror = mirrors.get(grabbed->id);
		if (actions.has_value() && mirror.has_value()) {
			actions->addModifyAction<EditorMirror, EditorActionModifyMirror>(mirrors, grabbed->id, std::move(grabbed->grabStartState));
		} else {
			CHECK_NOT_REACHED();
		}
		grabbed = std::nullopt;
	}
}

void PortalGrabTool::update(PortalPairArray& portalPairs, std::optional<EditorActions&> actions, Vec2 cursorPos, bool& cursorCaptured, bool cursorExact, bool enforceConstraints) {
	if (cursorCaptured) {
		return;
	}

	if (Input::isMouseButtonDown(MouseButton::LEFT) && !grabbed.has_value()) {
		for (const auto& portalPair : portalPairs) {
			for (i32 i = 0; i < 2; i++) {
				const auto& portal = portalPair->portals[i];

				const auto result = rotatableSegmentCheckGrab(portal.center, portal.normalAngle, EditorPortal::defaultLength,
					cursorPos, cursorCaptured, cursorExact);

				if (!result.has_value()) {
					continue;
				}

				if (enforceConstraints && portal.positionLocked && result->grabbedGizmo == RotatableSegmentGizmoType::TRANSLATION) {
					continue;
				}

				const auto rotationGizmoGrabbed = 
					result->grabbedGizmo == RotatableSegmentGizmoType::ROTATION_0 || 
					result->grabbedGizmo == RotatableSegmentGizmoType::ROTATION_1;
				if (enforceConstraints && portal.rotationLocked && rotationGizmoGrabbed) {
					continue;
				}

				grabbed = PortalGrabTool::Grabbed{
					.id = portalPair.id,
					.portalIndex = i,
					.grabStartState = portalPair.entity,
					.grabbed = *result,
				};
			}
		}
	}

	if (Input::isMouseButtonHeld(MouseButton::LEFT) && grabbed.has_value()) {
		cursorCaptured = true;
		auto portalPair = portalPairs.get(grabbed->id);
		if (portalPair.has_value()) {
			auto& portal = portalPair->portals[grabbed->portalIndex];
			grabbedRotatableSegmentUpdate(grabbed->grabbed.grabbedGizmo, grabbed->grabbed.grabOffset,
				portal.center, portal.normalAngle,
				cursorPos, cursorExact);
		} else {
			CHECK_NOT_REACHED();
		}
	}

	if (Input::isMouseButtonUp(MouseButton::LEFT) && grabbed.has_value()) {
		cursorCaptured = true;
		auto portalPair = portalPairs.get(grabbed->id);
		if (actions.has_value() && portalPair.has_value()) {
			actions->addModifyAction<EditorPortalPair, EditorActionModifyPortalPair>(portalPairs, grabbed->id, std::move(grabbed->grabStartState));
		} else {
			CHECK_NOT_REACHED();
		}
		grabbed = std::nullopt;
	}
}
