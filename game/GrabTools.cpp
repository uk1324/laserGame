#include "GrabTools.hpp"
#include <engine/Input/Input.hpp>
#include <game/Constants.hpp>

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

			updateGrabbed(arrowhead.actualTip, arrowhead.distanceTo(cursorPos), LaserGrabTool::LaserPart::DIRECTION);

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
				if (enforceConstraints) {
					// TODO: Could move this into the game logic instead of grab logic.
					const auto length = laser->position.length();
					const auto maxAllowedLength = Constants::boundary.radius - 0.05f;
					if (length > maxAllowedLength) {
						laser->position *= maxAllowedLength / length;
					}
				}
				break;
			case DIRECTION:
				laser->angle = (newPosition - laser->position).angle();
				break;
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
