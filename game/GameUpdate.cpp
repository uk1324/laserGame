#include "GameUpdate.hpp"
#include <engine/Math/Constants.hpp>
#include <array>
#include <game/Constants.hpp>
#include <game/Stereographic.hpp>
#include <game/Animation.hpp>
#include <imgui/imgui.h>
#include <engine/gfx2d/DbgGfx2d.hpp>
#include <engine/Math/MarchingSquares.hpp>
#include <Array2d.hpp>
#include <engine/Math/Interpolation.hpp>

auto areNearlyAntipodal(Vec2 e0, Vec2 e1, f32 epsilon) {
	return
		(abs(e0.length() - 1.0f) < epsilon) &&
		(abs(e1.length() - 1.0f) < epsilon) &&
		(e0 + e1).length() < epsilon;
};

Vec2 snapPositionsOutsideBoundary(Vec2 v) {
	const auto length = v.length();
	//const auto maxAllowedLength = Constants::boundary.radius - 0.001f;
	const auto maxAllowedLength = Constants::boundary.radius;
	if (length > maxAllowedLength) {
		v *= maxAllowedLength / length;
	}
	return v;
}

void GameState::snapObjectPositionsInsideBoundary(GameEntities& e) {
	for (auto laser : e.lasers) {
		laser->position = snapPositionsOutsideBoundary(laser->position);
	}
	for (auto mirror : e.mirrors) {
		mirror->center = snapPositionsOutsideBoundary(mirror->center);
	}
	for (auto portalPair : e.portalPairs) {
		for (auto& portal : portalPair->portals) {
			portal.center = snapPositionsOutsideBoundary(portal.center);
		}
	}
	auto snapAntipodalPoints = [](Vec2& e0, Vec2& e1) {
		if (areNearlyAntipodal(e0, e1, 0.01f)) {
			e0 = -e0;
		}
	};

	for (auto wall : e.walls) {
		for (auto& endpoint : wall->endpoints) {
			//endpoint = snapPositionsOutsideBoundary(endpoint);
		}
		snapAntipodalPoints(wall->endpoints[0], wall->endpoints[1]);
	}
	for (auto door : e.doors) {
		for (auto& endpoint : door->endpoints) {
			endpoint = snapPositionsOutsideBoundary(endpoint);
		}
		snapAntipodalPoints(door->endpoints[0], door->endpoints[1]);
	}
	for (auto target : e.targets) {
		target->position = snapPositionsOutsideBoundary(target->position);
	}
	for (auto trigger : e.triggers) {
		trigger->position = snapPositionsOutsideBoundary(trigger->position);
	}

	for (auto target : e.targets) {
		target->activated = false;
	}
	for (auto trigger : e.triggers) {
		trigger->activated = false;
	}
}

auto pointNearlyOnGeodesic(const StereographicLine& line, Vec2 p, f32 epsilon) {
	if (line.type == StereographicLine::Type::LINE) {
		return pointsNearlyColinear(Vec2(0.0f), line.lineNormal.rotBy90deg(), p, epsilon);
	} else {
		return (abs(line.circle.center.distanceTo(p) - line.circle.radius) < epsilon);
	}
};

void GameState::update(GameEntities& e) {
	anyTargetsTurnedOn = false;

	
	laserSegmentsToDraw.clear();
	
	

	auto nearlyOnTheSameGeodesic = [](const StereographicSegment& a, const StereographicSegment& b) {
		if (a.line.type == StereographicLine::Type::LINE) {
			return pointsNearlyColinear(a.endpoints[0], a.endpoints[1], b.endpoints[0], 0.001f) &&
				pointsNearlyColinear(a.endpoints[0], a.endpoints[1], b.endpoints[1], 0.001f);
		} else {
			return (abs(a.line.circle.center.distanceTo(b.endpoints[0]) - a.line.circle.radius) < 0.001f) &&
				(abs(a.line.circle.center.distanceTo(b.endpoints[1]) - a.line.circle.radius) < 0.001f);
		}
	};
		
	std::vector<Vec2> corners;
	{
		// Not using splitStereographicSegment, because there are issues with corners near the boundary not getting detected.
		std::vector<StereographicSegment> segments;
		for (const auto& wall : e.walls) {
			/*const auto s = splitStereographicSegment(wall->endpoints[0], wall->endpoints[1]);
			for (const auto& segment : s) {
				segments.push_back(StereographicSegment(segment.endpoints[0], segment.endpoints[1]));
			}*/
 			segments.push_back(StereographicSegment(wall->endpoints[0], wall->endpoints[1]));
		}
		for (const auto& door : e.doors) {
			if (!door->isFullyOpen()) {
				//const auto s = splitStereographicSegment(door->endpoints[0], door->endpoints[1]);
				//for (const auto& segment : s) {
				//	segments.push_back(StereographicSegment(segment.endpoints[0], segment.endpoints[1]));
				//}

				segments.push_back(StereographicSegment(door->endpoints[0], door->endpoints[1]));
			}
		}
		for (i32 i = 0; i < segments.size(); i++) {
			const auto& a = segments[i];
			for (i32 j = i + 1; j < segments.size(); j++) {
				const auto& b = segments[j];

				if (nearlyOnTheSameGeodesic(a, b)) {
					continue;
				}

				for (i32 m = 0; m < 2; m++) {
					const auto& f = a.endpoints[m];
					for (i32 n = 0; n < 2; n++) {
						const auto& g = b.endpoints[n];

						auto checkPair = [&corners](Vec2 f, Vec2 g) {
							const auto isCorner = f.distanceTo(g) < 0.005f;
							if (isCorner) {
								corners.push_back(g);
							}
						};

						checkPair(f, g);

						bool fAntipodalFinite = f.length() > 0.01f;
						bool gAntipodalFinite = g.length() > 0.01f;
						if (fAntipodalFinite) {
							checkPair(antipodalPoint(f), g);
						}
						if (gAntipodalFinite) {
							checkPair(f, antipodalPoint(g));
						}
						if (fAntipodalFinite && gAntipodalFinite) {
							checkPair(antipodalPoint(f), antipodalPoint(g));
						}

						// If the lines are on the same line then treat them as a single line and don't detect a corner.
					}
				}
			}
		}
	}

	for (const auto& corner : corners) {
		//Dbg::disk(corner, 0.01f, Color3::CYAN);
	}
	//for (i32 i = 0; i < e.walls.)

	for (const auto& laser : e.lasers) {
		laserUpdate(laser.entity, e, corners);
	}

	for (auto& s : laserSegmentsToDraw) {
		// Lexographically order the endpoints.
		if (s.endpoints[0].x > s.endpoints[1].x
			|| (s.endpoints[0].x == s.endpoints[1].x && s.endpoints[0].y > s.endpoints[1].y)) {
			std::swap(s.endpoints[0], s.endpoints[1]);
		}
	}

	// This only deals with exacly overlapping segments. There is also the case of partial overlaps that often occurs.
	for (i64 i = 0; i < i64(laserSegmentsToDraw.size()); i++) {
		const auto& a = laserSegmentsToDraw[i];
		for (i64 j = i + 1; j < i64(laserSegmentsToDraw.size()); j++) {

			auto& b = laserSegmentsToDraw[j];

			// > 0.01f Buggy.
			const auto epsilon = 0.001f;
			const auto epsilonSquared = epsilon * epsilon;
			if (a.endpoints[0].distanceSquaredTo(b.endpoints[0]) < epsilonSquared
				&& a.endpoints[1].distanceSquaredTo(b.endpoints[1]) < epsilonSquared) {
				b.ignore = true;
			}
		}
	}

	for (auto target : e.targets) {
		if (!target->activatedLastFrame && target->activated && target->activationAnimationT == 0.0f) {
			anyTargetsTurnedOn = true;
		}
		target->activatedLastFrame = target->activated;
	}

	auto updateActivationAnimationT = [](f32& t, bool isActive) {
		updateConstantSpeedT(t, 0.25f, isActive);
	};

	for (auto target : e.targets) {
		updateActivationAnimationT(target->activationAnimationT, target->activated);
	}
	for (auto trigger : e.triggers) {
		updateActivationAnimationT(trigger->activationAnimationT, trigger->activated);
	}

	for (auto door : e.doors) {
		const auto info = triggerInfo(e.triggers, door->triggerIndex);
		const auto isOpening = info.has_value() && info->active;
		updateConstantSpeedT(door->openingT, 0.7f, isOpening);
	}

}

// I tried many different things to correctly detect everything, but I can't think of any way to make it work in all cases. I have to choose between not detecting collision when they are there and detecting them when they aren't there. I think it might be better to choose detecting them when they aren't there.
void GameState::laserUpdate(EditorLaser& laser, GameEntities& e, const std::vector<Vec2>& corners) {
	/*renderer.gfx.disk(laser->position, 0.02f, movablePartColor(laser->positionLocked));
		renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), grabbableCircleRadius, movablePartColor(false));*/

	auto laserPosition = laser.position;
	auto laserDirection = Vec2::oriented(laser.angle);

	struct EditorEntity {
		EditorEntityId id;
		i32 partIndex; // Used for portals.
	};
	std::optional<EditorEntity> hitOnLastIteration;
	for (i64 iteration = 0; iteration < maxReflections; iteration++) {
		const auto laserLine = stereographicLineThroughPointWithTangent(laserPosition, laserDirection.angle());
		//ImGui::Text("%g", laserDirection.angle() / TAU<f32>);
		//if (laserLine.type == StereographicLine::Type::CIRCLE) {
		//	ImGui::Text("%g", (laser->position - laserLine.circle.center).rotBy90deg().angle() / TAU<f32>);
		//}

		const auto boundaryIntersections = stereographicLineVsCircleIntersection(laserLine, Constants::boundary);


		if (boundaryIntersections.size() == 0) {
			// Shouldn't happen if the points are inside, because the antipodal point is always outside.
			// For now do nothing.
			return;
		}

		Vec2 boundaryIntersection = boundaryIntersections[0].normalized();
		Vec2 boundaryIntersectionWrappedAround = -boundaryIntersection;

		if (dot(boundaryIntersection - laserPosition, laserDirection) < 0.0f) {
			std::swap(boundaryIntersection, boundaryIntersectionWrappedAround);
		}

		struct Hit {
			Vec2 point;
			f32 distance;
			StereographicLine line;
			EditorEntityId id;
			i32 index;
			bool objectMirrored;
			bool wrappedAround;
		};
		std::optional<Hit> closest;
		std::optional<f32> secondClosestDistance;
		std::optional<Hit> closestToWrappedAround;
		std::optional<f32> secondClosestToWrappedAroundDistance;

		auto processLineSegmentIntersections = [&laserLine, &laserPosition, &boundaryIntersectionWrappedAround, &laserDirection, &closest, &closestToWrappedAround, &hitOnLastIteration, &secondClosestDistance, &secondClosestToWrappedAroundDistance](
			Vec2 endpoint0,
			Vec2 endpoint1,
			EditorEntityId id,
			i32 index = 0,
			bool objectMirrored = false) {

				if (distance(endpoint0, endpoint1) < 0.001f) {
					// bug with doors creating steregraphic lines with infinite position and radius, when opening.
					return;
				}

				const auto line = stereographicLine(endpoint0, endpoint1);
				const auto intersections = stereographicLineVsStereographicLineIntersection(line, laserLine);

				for (const auto& intersection : intersections) {
					// The epsilon added, because there was a floating precision bug that caused a laser to go through a wall
					if (const auto outsideBoundary = intersection.length() > 1.0001f) {
						continue;
					}
					//Dbg::disk(intersection, 0.02f, Color3::CYAN);

					if (!isPointOnLineAlsoOnStereographicSegment(line, endpoint0, endpoint1, intersection, 0.01f)) {
						continue;
					}

					//Dbg::disk(intersection, 0.01f, Color3::RED);

					const auto distance = intersection.distanceTo(laserPosition);
					const auto distanceToWrappedAround = intersection.distanceTo(boundaryIntersectionWrappedAround);

					const auto hitLastTime = hitOnLastIteration.has_value() && hitOnLastIteration->id == id && hitOnLastIteration->partIndex == index;

					// hitLastTime should be only in the if not the else. You can hit an object from the front on one iteration and the from the wrapped around on the other.
					if (!hitLastTime && dot(intersection - laserPosition, laserDirection) > 0.0f) {
						if (!closest.has_value() || distance < closest->distance) {
							if (closest.has_value()) {
								secondClosestDistance = closest->distance;
							}
							closest = Hit{ intersection, distance, line, id, index, objectMirrored, false };
						} else if (!secondClosestDistance.has_value()) {
							secondClosestToWrappedAroundDistance = distance;
						}
					} else {
						if (!closestToWrappedAround.has_value()
							|| distanceToWrappedAround < closestToWrappedAround->distance) {

							if (closestToWrappedAround.has_value()) {
								secondClosestToWrappedAroundDistance = closestToWrappedAround->distance;
							}
							closestToWrappedAround = Hit{
								intersection,
								distanceToWrappedAround,
								line,
								id,
								index,
								objectMirrored,
								true
							};
						} else if (!secondClosestToWrappedAroundDistance.has_value()) {
							secondClosestToWrappedAroundDistance = distanceToWrappedAround;
						}
					}
				}
		};

		auto processSplitLineSegmentIntersections = [&processLineSegmentIntersections](
			Vec2 endpoint0,
			Vec2 endpoint1,
			EditorEntityId id,
			i32 index = 0) {

				/*Dbg::disk(endpoint0, 0.03f, Color3::CYAN);
				Dbg::disk(endpoint1, 0.03f, Color3::CYAN);*/
				const auto segments = splitStereographicSegment(endpoint0, endpoint1);

				if (segments.size() == 1) {
					processLineSegmentIntersections(segments[0].endpoints[0], segments[0].endpoints[1], id, index, false);
				} else if (segments.size() == 2) {
					/*Dbg::disk(segments[0].endpoints[0], 0.01f, Color3::RED);
					Dbg::disk(segments[0].endpoints[1], 0.01f, Color3::RED);

					Dbg::disk(segments[1].endpoints[0], 0.01f, Color3::GREEN);
					Dbg::disk(segments[1].endpoints[1], 0.01f, Color3::GREEN);*/

					processLineSegmentIntersections(segments[0].endpoints[0], segments[0].endpoints[1], id, index, false);
					processLineSegmentIntersections(segments[1].endpoints[0], segments[1].endpoints[1], id, index, true);
				}
		};

		for (const auto& wall : e.walls) {
			processSplitLineSegmentIntersections(wall->endpoints[0], wall->endpoints[1], EditorEntityId(wall.id));
			//processLineSegmentIntersections(wall->endpoints[0], wall->endpoints[1], EditorEntityId(wall.id));
		}

		for (const auto& mirror : e.mirrors) {
			const auto endpoints = mirror->calculateEndpoints();
			processSplitLineSegmentIntersections(endpoints[0], endpoints[1], EditorEntityId(mirror.id));
		}

		for (const auto& portalPair : e.portalPairs) {
			for (i32 i = 0; i < 2; i++) {
				const auto& portal = portalPair->portals[i];
				const auto endpoints = portal.endpoints();
				processSplitLineSegmentIntersections(endpoints[0], endpoints[1], EditorEntityId(portalPair.id), i);
			}
		}

		for (const auto& door : e.doors) {
			const auto segments = door->segments();
			for (const auto& segment : segments) {
				// Should this include index? Probably not.
				processSplitLineSegmentIntersections(segment.endpoints[0], segment.endpoints[1], EditorEntityId(door.id));
			}
		}
		// TODO: Could check if the intersection is near a cusp. First check if 2 lines are non nearly colinear by checking if either the 4 points are co circular or 4 points are coplanar then do nothing else check if the hit point is near any of the endpoints of multiple lines at the same time.

		enum HitResult {
			CONTINUE, END
		};

		auto processHit = [&](const Hit& hit) -> HitResult {
			const Vec2 laserOrigin = hit.wrappedAround ? boundaryIntersectionWrappedAround : laserPosition;

			Vec2 hitObjectNormalAtHitPoint = Vec2(0.0f);
			if (hit.line.type == StereographicLine::Type::CIRCLE) {
				auto& circle = hit.line.circle;
				// Not sure if I should handle the equality case.

				hitObjectNormalAtHitPoint = (hit.point - circle.center).normalized(); // pointing outside of circle

				if (const auto insideCircle = circle.center.distanceTo(laserOrigin) < circle.radius) {
					hitObjectNormalAtHitPoint = -hitObjectNormalAtHitPoint;
				}
			} else {
				hitObjectNormalAtHitPoint = hit.line.lineNormal;
				const auto fromHitPointToOrigin = laserOrigin - hit.point;
				if (dot(fromHitPointToOrigin, hitObjectNormalAtHitPoint) < 0.0f) {
					hitObjectNormalAtHitPoint = -hitObjectNormalAtHitPoint;
				}
			}

			// Put the tangent on the same side as the normal.
			auto laserTangentAtHitPoint = stereographicLineNormalAt(laserLine, hit.point).rotBy90deg();
			if (dot(hitObjectNormalAtHitPoint, laserTangentAtHitPoint) < 0.0f) {
				laserTangentAtHitPoint = -laserTangentAtHitPoint;
			}

			auto normalAngle = [](f32 normalAngle, Vec2 pointWithNormalAngle, StereographicLine lineWithNormalAngleAtPoint, bool isMirrored) {
				if (isMirrored) {
					// Mirrors the angle by calculating the difference of the normals. You can think of the hit.line as a full circle.
					const auto diff = stereographicLineNormalAt(lineWithNormalAngleAtPoint, pointWithNormalAngle).angle() - stereographicLineNormalAt(lineWithNormalAngleAtPoint, antipodalPoint(pointWithNormalAngle)).angle();
					normalAngle -= diff;
					normalAngle += PI<f32>;
					return normalAngle;
				} else {
					return normalAngle;
				}
			};

			auto hitOnNormalSide = [&hitObjectNormalAtHitPoint, &hit, &normalAngle](f32 hitObjectNormalAngle, Vec2 center) {
				hitObjectNormalAngle = normalAngle(hitObjectNormalAngle, center, hit.line, hit.objectMirrored);
				//Dbg::line(hit.point, hit.point + Vec2::oriented(hitObjectNormalAngle) * 0.1f, 0.01f, Color3::MAGENTA);
				bool result = dot(Vec2::oriented(hitObjectNormalAngle), hitObjectNormalAtHitPoint) > 0.0f;
				return result;
			};

			auto doReflection = [&] {
				laserDirection = laserTangentAtHitPoint.reflectedAroundNormal(hitObjectNormalAtHitPoint);
				laserPosition = hit.point;
				hitOnLastIteration = EditorEntity{ hit.id, hit.index };
				/*Dbg::line(laserPosition, laserPosition + laserDirection * 0.2f, 0.01f, Color3::BLUE);
				Dbg::line(laserPosition, laserPosition + laserTangentAtHitPoint * 0.2f, 0.01f, Color3::GREEN);
				Dbg::line(laserPosition, laserPosition + hitObjectNormalAtHitPoint * 0.2f, 0.01f, Color3::RED);*/
			};

			switch (hit.id.type) {
				using enum EditorEntityType;
			case WALL: {
				const auto& wall = e.walls.get(hit.id.wall());
				if (!wall.has_value()) {
					CHECK_NOT_REACHED();
					return HitResult::END;
				}

				if (wall->type == EditorWallType::ABSORBING) {
					return HitResult::END;
				} else {
					doReflection();
					return HitResult::CONTINUE;
				}
			}

			case MIRROR: {
				const auto& mirror = e.mirrors.get(hit.id.mirror());
				if (!mirror.has_value()) {
					CHECK_NOT_REACHED();
					return HitResult::END;
				}
				const auto hitHappenedOnNormalSide = hitOnNormalSide(mirror->normalAngle, mirror->center);
				if (!hitHappenedOnNormalSide) {
					switch (mirror->wallType) {
					case EditorMirrorWallType::REFLECTING:
						doReflection();
						return HitResult::CONTINUE;
					case EditorMirrorWallType::ABSORBING:
						return HitResult::END;
					}
				}
				doReflection();
				return HitResult::CONTINUE;
			}

			case PORTAL_PAIR: {
				const auto portalPair = e.portalPairs.get(hit.id.portalPair());
				if (!portalPair.has_value()) {
					CHECK_NOT_REACHED();
					return HitResult::END;
				}
				const auto inPortal = portalPair->portals[hit.index];
				const auto hitHappenedOnNormalSide = hitOnNormalSide(inPortal.normalAngle, inPortal.center);
				if (!hitHappenedOnNormalSide) {
					switch (inPortal.wallType) {
					case EditorPortalWallType::PORTAL:
						break;
					case EditorPortalWallType::REFLECTING:
						doReflection();
						return HitResult::CONTINUE;
					case EditorPortalWallType::ABSORBING:
						return HitResult::END;
					}
				}
				const auto& inPortalLine = hit.line;
				const auto outPortalIndex = (hit.index + 1) % 2;
				const auto& outPortal = portalPair->portals[outPortalIndex];
				const auto outPortalEndpoints = outPortal.endpoints();
				const auto outPortalLine = stereographicLine(outPortalEndpoints[0], outPortalEndpoints[1]);

				const auto inPortalNormalAngle = normalAngle(inPortal.normalAngle, inPortal.center, inPortalLine, hit.objectMirrored);

				const auto inPortalCenter = hit.objectMirrored ? antipodalPoint(inPortal.center) : inPortal.center;
				auto dist = stereographicDistance(inPortalCenter, hit.point);

				// Choose the positive and negative directions of distance. The normal of the portal rotated by 90deg is the negative direction.
				if (dot(hit.point - inPortalCenter, Vec2::oriented(inPortalNormalAngle + PI<f32> / 2.0f)) > 0.0f) {
					dist *= -1.0f;
				}
				// If the object is rotated the mirror normal is rotated so the positive direction is switched.
				if (hit.objectMirrored) {
					dist *= -1.0f;
				}

				if (inPortal.orientationReversing) {
					dist *= -1.0f;
				}
				if (outPortal.orientationReversing) {
					dist *= -1.0f;
				}


				laserPosition = moveOnStereographicGeodesic(outPortal.center, outPortal.normalAngle + PI<f32> / 2.0f, dist);
				bool outMirrored = false;
				// If the out point is out of the boundary the make it antipodal.
				if (laserPosition.length() >= Constants::boundary.radius + 0.001f) {
					outMirrored = true;
					laserPosition = antipodalPoint(laserPosition);
				}

				//auto normalAtHitpoint = stereographicLineNormalAt(inPortalLine, hitPoint);

				/*ImGui::Text("%g", normalAtHitPoint.angle());
				renderer.gfx.line(hitPoint, hitPoint + normalAtHitPoint * 0.05f, 0.01f, Color3::GREEN);
				renderer.gfx.line(hitPoint, hitPoint + laserTangentAtIntersection * 0.05f, 0.01f, Color3::BLUE);*/

				//if (dot(normalAtHitPoint, laserDirection) > 0.0f) {
				//	normalAtHitpoint = -normalAtHitpoint;
				//}

				const auto outPortalNormalAngle = normalAngle(outPortal.normalAngle, outPortal.center, outPortalLine, outMirrored);

				auto outPortalNormalAtOutPoint = stereographicLineNormalAt(outPortalLine, laserPosition);
				if (dot(outPortalNormalAtOutPoint, Vec2::oriented(outPortalNormalAngle)) > 0.0f) {
					outPortalNormalAtOutPoint = -outPortalNormalAtOutPoint;
				}

				if (dot(hitObjectNormalAtHitPoint, Vec2::oriented(inPortalNormalAngle)) > 0.0f) {
					outPortalNormalAtOutPoint = -outPortalNormalAtOutPoint;
				}

				// The angle the laser makes with the input mirror normal is the same as the one it makes with the output portal normal.
				auto laserDirectionAngle =
					laserTangentAtHitPoint.angle() - hitObjectNormalAtHitPoint.angle()
					+ outPortalNormalAtOutPoint.angle();
				/*if (outMirrored) {
					laserDirectionAngle += PI<f32>;
				}*/
				laserDirection = Vec2::oriented(laserDirectionAngle);
				if (hit.objectMirrored xor outMirrored xor inPortal.orientationReversing xor outPortal.orientationReversing) {
					laserDirection = laserDirection.reflectedAroundNormal(outPortalNormalAtOutPoint);
				}
				/*if (outMirrored) {
					laserDirection = laserDirection.reflectedAroundNormal(outPortalNormalAtOutPoint);
				}
				if (inPortal.orientationReversing) {
					laserDirection = laserDirection.reflectedAroundNormal(outPortalNormalAtOutPoint);
				}
				if (outPortal.orientationReversing) {
					laserDirection = laserDirection.reflectedAroundNormal(outPortalNormalAtOutPoint);
				}*/

				hitOnLastIteration = EditorEntity{ hit.id, outPortalIndex };
				return HitResult::CONTINUE;
			}

			case DOOR: {
				const auto& door = e.doors.get(hit.id.door());
				if (!door.has_value()) {
					CHECK_NOT_REACHED();
					return HitResult::END;
				}

				return HitResult::END;
			}

			default:
				break;

			}

			CHECK_NOT_REACHED();
			return HitResult::END;
		};

		auto checkOrbCollision = [&](Vec2 center, f32 radius, const Segment& segment) {
			const auto centers = splitStereographicCircle(center, radius);
			for (const auto& center : centers) {
				const auto circle = stereographicCircle(center, radius);
				const auto intersections = stereographicSegmentVsCircleIntersection(laserLine, segment.endpoints[0], segment.endpoints[1], circle);
				/*const auto intersections = stereographicSegmentVsCircleIntersection(stereographicLine(segment.endpoints[0], segment.endpoints[1]), segment.endpoints[0], segment.endpoints[1], circle);*/
				if (intersections.size() > 0) {
					/*Dbg::disk(segment.endpoints[0], 0.01f, Color3::RED);
					Dbg::disk(segment.endpoints[1], 0.01f, Color3::RED);*/
					return true;
				}
			}
			return false;
		};

		auto checkLaserVsTargetCollision = [&](const Segment& segment) {
			for (auto target : e.targets) {
				if (checkOrbCollision(target->position, target->radius, segment)) {
					target->activated = true;
				}
			}
		};

		auto checkLaserVsTriggerCollision = [&](const Segment& segment) {
			for (auto trigger : e.triggers) {
				if (checkOrbCollision(trigger->position, trigger->defaultRadius, segment)) {
					trigger->activated = true;
				}
			}
		};

		// Non interfering means that they don't change the direction and position of the laser.
		auto checkNonInterferingLaserCollisions = [&](const Segment& segment) {
			checkLaserVsTargetCollision(segment);
			checkLaserVsTriggerCollision(segment);
		};

		auto processLaserSegment = [&](const Segment& segment) {
			checkNonInterferingLaserCollisions(segment);
			laserSegmentsToDraw.push_back(segment);
		};

		// Double intersections can cause bugs. 
		// If you have a corner then there is no real way to define a reflection and it probably doesn't really ma tter if a laser end there.
		// A bigger issue is configurations like crosses made out of mirrors. Then If you point right at the intersection of 2 lines then if you process only one line intersection the laser will reflect and go through the other. One option would be to process the 2 reflections sequentially, but it probably doesn't matter much. Technically only one of the walls needs to be a mirror.
		auto doubleIntersectionCheck = [&corners](const Hit& hit, std::optional<f32> secondClosestDistance) {
			// If the closest hit is to close to the laser position sometimes bugs happen.
			if (hit.distance < 0.001f) {
				// There was a bug where there was a wall mirror on the boundary and a laser hit it wrapped around. This kinda fixes the bug, but also ends the laser where it shouldn't really end. When designing a level to deal with this issue could just create the wall on both sides.
				// 
				// This fixes some things, but there are issues with lasers starting near the boundary going around the boundary and coming back to the same point. 
				// In general thereare issues near the boundary that I am not sure how to handle. 
				// If I turn this on it casues some issues, but if I don't it causes other issues.

				// TODO: Maybe checking for near intersections with endpoints would work. It would at least be consistent and not dependent on the boundary.

				// There are also cases when 2 mirror walls are next to eachother and one is hit right at the endpoint then the other might reflect the laser to the other side.
				return true;
			}

			for (const auto& corner : corners) {
				if (hit.point.distanceTo(corner) < 0.005f) {
					return true;
				}
			}


			if (!secondClosestDistance.has_value()) {
				return false;
			}
			// This creates false positives.
			return std::abs(hit.distance - *secondClosestDistance) < 0.001f;
			//return false;
		};

		// Sometimes the laser is properly calculated for collision, but when split into segments it stops working.
		// This happens when the segment has 2 antipodal points. 
		// One way to make this happens is to start a laser from a boundary.
		// Then it hits the boundary on the other side.
		// The laser position and the hit point are antipodal, which means they are the same point in the elliptic geometry so you can't draw a unique line through them.
		// You can make similar things with lasers pointed at boundary where a mirror is (if you move slowly enough you can get it to happen).
		// The game update still works only the visuals break. This can be verified by putting a trigger there. The trigger collision code uses laserLine to check for collisions so it properly detects them.

		auto laserSegmentMidpoint = [&](Vec2 e0, Vec2 e1) {
			//return stereographicSegmentMidpoint(e0, e1); // Doesn't work with antipodal points, because it's ambigous.
			const auto chordMidpoint = (e0 + e1) / 2.0f;
			if (laserLine.type == StereographicLine::Type::LINE) {
				return chordMidpoint;
			}
			auto midpoint = laserLine.circle.center + (chordMidpoint - laserLine.circle.center).normalized() * laserLine.circle.radius;
			if (midpoint.length() > Constants::boundary.radius) {
				midpoint = laserLine.circle.center - (chordMidpoint - laserLine.circle.center).normalized() * laserLine.circle.radius;
			}
			// To check that tihs is actuall the midpoint and not is antipodal point on the circle could check the dot product of laser direction with (midpoint - laserPosition).
			//if (dot(laserDirection, midpoint) < 0.0f) {
			//	midpoint = antipodalPoint(midpoint);
			//}
			return midpoint;
		};

		auto processLaserSegmentEndpoints = [&](Vec2 e0, Vec2 e1) {
			auto projectOntoLine = [&](Vec2 p) {
				if (laserLine.type == StereographicLine::Type::CIRCLE) {
					const auto& circle = laserLine.circle;
					p -= circle.center;
					p = p.normalized();
					p *= circle.radius;
					p += circle.center;
				} else {
					p -= dot(laserLine.lineNormal, p) * laserLine.lineNormal;
				}
				return p;
			};
			e0 = projectOntoLine(e0);
			e1 = projectOntoLine(e1);

			const auto nearlyAntipodal = areNearlyAntipodal(e0, e1, 0.01f);
			if (!nearlyAntipodal) {
				const auto s = Segment{ e0, e1, laser.color };
				processLaserSegment(s);
				return;
			}
			const auto midpoint = laserSegmentMidpoint(e0, e1);
			const auto s0 = Segment{ e0, midpoint, laser.color };
			const auto s1 = Segment{ midpoint, e1, laser.color };
			processLaserSegment(s0);
			processLaserSegment(s1);
		};


		std::optional<f32> closestEndpointDistance;
		std::optional<f32> closestEndpointToWrappedAroundDistance;
		/*f32 closestEndpointDistance = std::numeric_limits<f32>::infinity();
		f32 closestEndpointToWrappedAroundDistance = std::numeric_limits<f32>::infinity();*/
		for (const auto& wall : e.walls) {
			auto checkPoint = [&](Vec2 e) {
				if (pointNearlyOnGeodesic(laserLine, e, 0.005f)) {
					if (dot(e - laserPosition, laserDirection) > 0.0f) {
						const auto distance = e.distanceTo(laserPosition);
						if (!closestEndpointDistance.has_value() || distance < closestEndpointDistance) {
							closestEndpointDistance = distance;
						}
					} else {
						const auto distanceToWrappedAround = e.distanceTo(boundaryIntersectionWrappedAround);
						if (!closestEndpointToWrappedAroundDistance.has_value() || distanceToWrappedAround < *closestEndpointToWrappedAroundDistance) {
							closestEndpointToWrappedAroundDistance = distanceToWrappedAround;
						}
					}

					//if (isPointOnLineAlsoOnStereographicSegment(laserLine, laserPosition, boundaryIntersection, e)) {
					//	const auto distance = e.distanceTo(laserPosition);
					//	closestEndpointDistance = std::min(closestEndpointDistance, distance);
					//} else if (isPointOnLineAlsoOnStereographicSegment(laserLine, laserPosition, boundaryIntersectionWrappedAround, e)) {
					//	const auto distanceToWrappedAround = e.distanceTo(boundaryIntersectionWrappedAround);
					//	closestEndpointToWrappedAroundDistance = std::min(closestEndpointToWrappedAroundDistance, distanceToWrappedAround);
					//}
				}
			};

			for (const auto& e : wall->endpoints) {
				/*const auto l = e.length();
				const auto epsilon = 0.0001f;
				if (l < 1.0f - epsilon) {
					checkPoint(e);
				} else if (l > 1.0f + epsilon) {
					checkPoint(antipodalPoint(e));
				} else {
					checkPoint(e);
					checkPoint(antipodalPoint(e));
				}*/

				if (e.length() >= 1.0f) {
					checkPoint(antipodalPoint(e));
				} else if (e.length() <= 1.0f) {
					checkPoint(e);
				}
			}
		}

		if (!closest.has_value() && closestEndpointDistance.has_value()) {
			break;
		}
		if (!closest.has_value() && !closestToWrappedAround.has_value() && closestEndpointToWrappedAroundDistance.has_value()) {
			break;
		}

		if (closest.has_value()) {
			//Dbg::disk(closest->point, 0.01f, Color3::RED);
			processLaserSegmentEndpoints(laserPosition, closest->point);

			if (closestEndpointDistance.has_value() && closestEndpointDistance < closest->distance) {
				break;
			}

			const auto result = processHit(*closest);
			if (result == HitResult::END
				|| doubleIntersectionCheck(*closest, secondClosestDistance)) {
				break;
			}
		} else if (closestToWrappedAround.has_value()) {
			processLaserSegmentEndpoints(laserPosition, boundaryIntersection);
			processLaserSegmentEndpoints(boundaryIntersectionWrappedAround, closestToWrappedAround->point);

			if (closestEndpointToWrappedAroundDistance.has_value() && closestEndpointToWrappedAroundDistance < closestToWrappedAround->distance) {
				break;
			}

			const auto result = processHit(*closestToWrappedAround);
			if (result == HitResult::END
				|| doubleIntersectionCheck(*closestToWrappedAround, secondClosestToWrappedAroundDistance)) {
				break;
			}
		} else {
			processLaserSegmentEndpoints(laserPosition, boundaryIntersection);
			processLaserSegmentEndpoints(laserPosition, boundaryIntersectionWrappedAround);
			return;
		}
	}
}

std::optional<GameState::TriggerInfo> GameState::triggerInfo(TriggerArray& triggers, i32 triggerIndex) {
	std::optional<TriggerInfo> result;
	for (const auto& trigger : triggers) {
		// If any trigger is activated the return that activated.
		const auto currentColor = currentOrbColor(trigger->color, trigger->activationAnimationT);
		if (trigger->index == triggerIndex) {
			if (trigger->activated) {
				return TriggerInfo{ 
					.color = trigger->color, 
					.currentColor = currentColor,
					.active = trigger->activated, 
				};
			} else if (!result.has_value()) {
				result = TriggerInfo{ 
					.color = trigger->color, 
					.currentColor = currentColor,
					.active = trigger->activated 
				};
			}
		}
	}
	return result;
}
