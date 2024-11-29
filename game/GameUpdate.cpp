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

Vec2 snapPositionsOutsideBoundary(Vec2 v) {
	const auto length = v.length();
	const auto maxAllowedLength = Constants::boundary.radius;
	if (length > maxAllowedLength) {
		v *= maxAllowedLength / length;
	}
	return v;
}

void GameState::update(GameEntities& e, bool objectsInValidState) {
	anyTargetsTurnedOn = false;

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
	for (auto wall : e.walls) {
		for (auto& endpoint : wall->endpoints) {
			endpoint = snapPositionsOutsideBoundary(endpoint);
		}
	}
	for (auto door : e.doors) {
		for (auto& endpoint : door->endpoints) {
			endpoint = snapPositionsOutsideBoundary(endpoint);
		}
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
	laserSegmentsToDraw.clear();

	for (const auto& laser : e.lasers) {
		/*renderer.gfx.disk(laser->position, 0.02f, movablePartColor(laser->positionLocked));
		renderer.gfx.disk(laserDirectionGrabPoint(laser.entity), grabbableCircleRadius, movablePartColor(false));*/

		auto laserPosition = laser->position;
		auto laserDirection = Vec2::oriented(laser->angle);

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
				break;
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

					/*const auto epsilon = 0.001f;
					const auto lineDirection = (endpoint1 - endpoint0).normalized();
					const auto dAlong0 = dot(lineDirection, endpoint0) - epsilon;
					const auto dAlong1 = dot(lineDirection, endpoint1) + epsilon;
					const auto intersectionDAlong = dot(lineDirection, intersection);
					if (intersectionDAlong <= dAlong0 || intersectionDAlong >= dAlong1) {
						continue;
					}*/
					// TODO: Should there be epsilon checks?
					if (!isPointOnLineAlsoOnStereographicSegment(line, endpoint0, endpoint1, intersection)) {
						continue;
					}

					const auto distance = intersection.distanceTo(laserPosition);
					const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);

					const auto hitLastTime = hitOnLastIteration.has_value() && hitOnLastIteration->id == id && hitOnLastIteration->partIndex == index;

					// hitLastTime should be only in the if not the else. You can hit an object from the front on one iteration and the from the wrapped around on the other.
					if (!hitLastTime && dot(intersection - laserPosition, laserDirection) > 0.0f) {
						if (!closest.has_value() || distance < closest->distance) {
							if (closest.has_value()) {
								secondClosestDistance = closest->distance;
							}
							closest = Hit{ intersection, distance, line, id, index, objectMirrored };
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
								objectMirrored
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

				const auto segments = splitStereographicSegment(endpoint0, endpoint1);

				if (segments.size() == 1) {
					processLineSegmentIntersections(segments[0].endpoints[0], segments[0].endpoints[1], id, index, false);
				} else if (segments.size() == 2) {
					processLineSegmentIntersections(segments[0].endpoints[0], segments[0].endpoints[1], id, index, false);
					processLineSegmentIntersections(segments[1].endpoints[0], segments[1].endpoints[1], id, index, true);
				}
			};

			for (const auto& wall : e.walls) {
				processLineSegmentIntersections(wall->endpoints[0], wall->endpoints[1], EditorEntityId(wall.id));
			}

			// For this to work create a reflecting wall anywhere.
			/*for (const auto& wall : walls) {
				for (const auto& segment : segments) {
					const auto endpoint0 = segment.a;
					const auto endpoint1 = segment.b;
					i32 index = &segment - segments.data();
					const auto id = EditorEntityId(wall.id);
					const auto line = stereographicLine(endpoint0, endpoint1);
					const auto intersections = stereographicLineVsStereographicLineIntersection(line, laserLine);

					for (const auto& intersection : intersections) {
						if (const auto outsideBoundary = intersection.length() > 1.0f) {
							continue;
						}

						const auto epsilon = 0.000f;
						const auto lineDirection = (endpoint1 - endpoint0).normalized();
						const auto dAlong0 = dot(lineDirection, endpoint0) - epsilon;
						const auto dAlong1 = dot(lineDirection, endpoint1) + epsilon;
						const auto intersectionDAlong = dot(lineDirection, intersection);
						if (intersectionDAlong <= dAlong0 || intersectionDAlong >= dAlong1) {
							continue;
						}

						const auto distance = intersection.distanceTo(laserPosition);
						const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);


						if (hitOnLastIteration.has_value() && hitOnLastIteration->id == id && hitOnLastIteration->partIndex == index) {
							continue;
						}

						if (dot(intersection - laserPosition, laserDirection) > 0.0f) {
							if (!closest.has_value() || distance < closest->distance) {
								closest = Hit{ intersection, distance, line, id, index };
							}
						} else {
							if (!closestToWrappedAround.has_value()
								|| distanceToWrappedAround < closestToWrappedAround->distance) {
								closestToWrappedAround = Hit{
									intersection,
									distanceToWrappedAround,
									line,
									id,
									index
								};
							}
						}
					}
				}
				break;
			}*/

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
					processLineSegmentIntersections(segment.endpoints[0], segment.endpoints[1], EditorEntityId(door.id));
				}
			}

			// The old version of this code makes the laser stop on collision with a target.
			/*for (const auto& target : targets) {
				const auto circle = target->calculateCircle();
				const auto intersections = stereographicLineVsCircleIntersection(laserLine, circle);

				for (const auto& intersection : intersections) {
					if (const auto outsideBoundary = intersection.length() > 1.0f) {
						continue;
					}

					if (dot(intersection - laserPosition, laserDirection) > 0.0f) {
						const auto distance = intersection.distanceTo(laserPosition);
						hitTargets.push_back(HitTarget{ target.id, distance });
					} else {
						const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);
						hitTargetsFromWrappedAround.push_back(HitTarget{ target.id, distanceToWrappedAround });
					}
				}
			}*/

			enum HitResult {
				CONTINUE, END
			};

			auto processHit = [&](const Hit& hit) -> HitResult {
				auto laserTangentAtHitPoint = stereographicLineNormalAt(laserLine, hit.point).rotBy90deg();
				if (dot(laserTangentAtHitPoint, laserDirection) > 0.0f) {
					laserTangentAtHitPoint = -laserTangentAtHitPoint;
				}
				//Dbg::line(hit.point, hit.point + laserTangentAtHitPoint * 0.1f, 0.02f, Color3::RED);

				auto normalAtHitPoint = stereographicLineNormalAt(hit.line, hit.point);
				if (dot(normalAtHitPoint, laserTangentAtHitPoint) < 0.0f) {
					normalAtHitPoint = -normalAtHitPoint;
				}
				//Dbg::line(hit.point, hit.point + normalAtHitPoint * 0.1f, 0.01f);

				auto hitOnNormalSide = [&normalAtHitPoint, &hit](f32 hitObjectNormalAngle) {
					bool result = dot(Vec2::oriented(hitObjectNormalAngle), normalAtHitPoint) > 0.0f;
					if (hit.objectMirrored) {
						result = !result;
					}
					return result;
				};

				auto doReflection = [&] {
					laserDirection = laserTangentAtHitPoint.reflectedAroundNormal(normalAtHitPoint);
					laserPosition = hit.point;
					hitOnLastIteration = EditorEntity{ hit.id, hit.index };
					//renderer.gfx.line(laserPosition, laserPosition + laserDirection * 0.2f, 0.01f, Color3::BLUE);
					//renderer.gfx.line(laserPosition, laserPosition + laserTangentAtIntersection * 0.2f, 0.01f, Color3::GREEN);
					//renderer.gfx.line(laserPosition, laserPosition + mirrorNormal * 0.2f, 0.01f, Color3::RED);
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
					const auto hitHappenedOnNormalSide = hitOnNormalSide(mirror->normalAngle);
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
					const auto hitHappenedOnNormalSide = hitOnNormalSide(inPortal.normalAngle);
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

					auto dist = stereographicDistance(inPortal.center, hit.point);
					if (dot(hit.point - inPortal.center, Vec2::oriented(inPortal.normalAngle + PI<f32> / 2.0f)) > 0.0f) {
						dist *= -1.0f;
					}
					laserPosition = moveOnStereographicGeodesic(outPortal.center, outPortal.normalAngle + PI<f32> / 2.0f, dist);

					//auto normalAtHitpoint = stereographicLineNormalAt(inPortalLine, hitPoint);

					/*ImGui::Text("%g", normalAtHitPoint.angle());
					renderer.gfx.line(hitPoint, hitPoint + normalAtHitPoint * 0.05f, 0.01f, Color3::GREEN);
					renderer.gfx.line(hitPoint, hitPoint + laserTangentAtIntersection * 0.05f, 0.01f, Color3::BLUE);*/

					//if (dot(normalAtHitPoint, laserDirection) > 0.0f) {
					//	normalAtHitpoint = -normalAtHitpoint;
					//}

					auto outPortalNormalAtOutPoint = stereographicLineNormalAt(outPortalLine, laserPosition);
					if (dot(outPortalNormalAtOutPoint, Vec2::oriented(outPortal.normalAngle)) > 0.0f) {
						outPortalNormalAtOutPoint = -outPortalNormalAtOutPoint;
					}

					if (dot(normalAtHitPoint, Vec2::oriented(inPortal.normalAngle)) > 0.0f) {
						outPortalNormalAtOutPoint = -outPortalNormalAtOutPoint;
					}

					// The angle the laser makes with the input mirror normal is the same as the one it makes with the output portal normal.
					auto laserDirectionAngle =
						laserTangentAtHitPoint.angle() - normalAtHitPoint.angle()
						+ outPortalNormalAtOutPoint.angle();
					laserDirection = Vec2::oriented(laserDirectionAngle);

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
					if (intersections.size() > 0) {
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
			// If you have a corner then there is no real way to define a reflection and it probably doesn't really matter if a laser end there.
			// A bigger issue is configurations like crosses made out of mirrors. Then If you point right at the intersection of 2 lines then if you process only one line intersection the laser will reflect and go through the other. One option would be to process the 2 reflections sequentially, but it probably doesn't matter much. Technically only one of the walls needs to be a mirror.
			auto doubleIntersectionCheck = [](const Hit& hit, std::optional<f32> secondClosestDistance) {
				if (!secondClosestDistance.has_value()) {
					return false;
				}
				return std::abs(hit.distance - *secondClosestDistance) < 0.001f;
			};

			// Sometimes the laser is properly calculated for collision, but when split into segments it stops working.
			// This happens when the segment has 2 antipodal points. 
			// One way to make this happens is to start a laser from a boundary.
			// Then it hits the boundary on the other side.
			// The laser position and the hit point are antipodal, which means they are the same point in the elliptic geometry so you can't draw a unique line through them.
			// You can make similar things with lasers pointed at boundary where a mirror is (if you move slowly enough you can get it to happen).
			// The game update still works only the visuals break. This can be verified by putting a trigger there. The trigger collision code uses laserLine to check for collisions so it properly detects them.

			auto laserSegmentMidpoint = [&](Vec2 e0, Vec2 e1) {
				const auto chordMidpoint = (e0 + e1) / 2.0f;
				if (laserLine.type == StereographicLine::Type::LINE) {
					return chordMidpoint;
				}
				auto midpoint = (chordMidpoint - laserLine.circle.center).normalized() * laserLine.circle.radius + laserLine.circle.center;
				if (dot(laserDirection, midpoint) < 0.0f) {
					midpoint = antipodalPoint(midpoint);
				}
				return midpoint;
			};

			auto processLaserSegmentEndpoints = [&](Vec2 e0, Vec2 e1) {
				const auto nearlyAntipodal = (e0 + e1).length() < 0.01f;
				if (!nearlyAntipodal) {
					const auto s = Segment{ e0, e1, laser->color };
					processLaserSegment(s);
					return;
				}
				const auto midpoint = laserSegmentMidpoint(e0, e1);
				const auto s0 = Segment{ e0, midpoint, laser->color };
				const auto s1 = Segment{ midpoint, e1, laser->color };
				processLaserSegment(s0);
				processLaserSegment(s1);
			};

			if (closest.has_value()) {
				processLaserSegmentEndpoints(laserPosition, closest->point);
				/*const auto s = Segment{ laserPosition, closest->point, laser->color };
				processLaserSegment(s);*/
				const auto result = processHit(*closest);
				if (result == HitResult::END 
					|| doubleIntersectionCheck(*closest, secondClosestDistance)) {
					break;
				}
			} else if (closestToWrappedAround.has_value()) {
				processLaserSegmentEndpoints(laserPosition, boundaryIntersection);
				processLaserSegmentEndpoints(boundaryIntersectionWrappedAround, closestToWrappedAround->point);
				//const auto s0 = Segment{ laserPosition, boundaryIntersection, laser->color };
				//const auto s1 = Segment{ boundaryIntersectionWrappedAround, closestToWrappedAround->point, laser->color };
				//processLaserSegment(s0);
				//processLaserSegment(s1);

				const auto result = processHit(*closestToWrappedAround);
				if (result == HitResult::END 
					|| doubleIntersectionCheck(*closestToWrappedAround, secondClosestToWrappedAroundDistance)) {
					break;
				}
			} else {
				processLaserSegmentEndpoints(laserPosition, boundaryIntersection);
				processLaserSegmentEndpoints(laserPosition, boundaryIntersectionWrappedAround);
				/*const auto s0 = Segment{ laserPosition, boundaryIntersection, laser->color };
				const auto s1 = Segment{ laserPosition, boundaryIntersectionWrappedAround, laser->color };
				processLaserSegment(s0);
				processLaserSegment(s1);*/
				/*if (abs(laserPosition.length() - 1.0f) < 0.01f) {
					auto pointAhead = moveOnStereographicGeodesic(laserPosition, laserDirection.angle(), 0.5f);
					if (pointAhead.length() >= 1.0f) {
						pointAhead = moveOnStereographicGeodesic(laserPosition, laserDirection.angle(), -0.5f);
					}
					const auto s0 = Segment{ pointAhead, laserPosition.normalized(), laser->color};
					const auto s1 = Segment{ pointAhead, -laserPosition.normalized(), laser->color};
					processLaserSegment(s0);
					processLaserSegment(s1);
				} else {
					const auto s0 = Segment{ laserPosition, boundaryIntersection, laser->color };
					const auto s1 = Segment{ laserPosition, boundaryIntersectionWrappedAround, laser->color };
					processLaserSegment(s0);
					processLaserSegment(s1);
				}*/
				/*if (nearlyTheSameAntipodalPoint && false) {
					auto pointAhead = moveOnStereographicGeodesic(laserPosition, laserDirection.angle(), 0.5f);
					if (pointAhead.length() >= 1.0f) {
						pointAhead = moveOnStereographicGeodesic(laserPosition, laserDirection.angle(), -0.5f);
					}
					const auto s0 = Segment{ pointAhead, boundaryIntersection, laser->color };
					const auto s1 = Segment{ pointAhead, boundaryIntersectionWrappedAround, laser->color };
					processLaserSegment(s0);
					processLaserSegment(s1);
				} else {
					const auto s0 = Segment{ laserPosition, boundaryIntersection, laser->color };
					const auto s1 = Segment{ laserPosition, boundaryIntersectionWrappedAround, laser->color };
					processLaserSegment(s0);
					processLaserSegment(s1);
				}*/
			}
		}
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
