#include "GameUpdate.hpp"
#include <engine/Math/Constants.hpp>
#include <array>
#include <game/Constants.hpp>
#include <game/Stereographic.hpp>

#include <imgui/imgui.h>
#include <engine/gfx2d/DbgGfx2d.hpp>
#include <engine/Math/MarchingSquares.hpp>
#include <Array2d.hpp>
#include <engine/Math/Interpolation.hpp>

void GameState::update(GameEntities& e) {

	//ImGui::SliderFloat("test", &eccentricity, 0.0f, 12.0f);
	//ImGui::SliderFloat2("f0", focus[0].data(), -1.0f, 1.0f);
	//ImGui::SliderFloat2("f1", focus[1].data(), -1.0f, 1.0f);
	//{
	//	auto dist = [](Vec2 a, Vec2 b) {
	//		const auto as = fromStereographic(a);
	//		const auto bs = fromStereographic(b);
	//		return std::min(
	//			sphericalDistance(as, bs),
	//			sphericalDistance(as, -bs));
	//		};

	//	const auto gridSize = 200;
	//	auto f = Array2d<f32>::uninitialized(gridSize, gridSize);

	//	auto indexToPos = [](f32 xi, f32 yi) -> Vec2 {
	//		const auto xt = xi / f32(gridSize);
	//		const auto yt = yi / f32(gridSize);
	//		const auto x = lerp(-1.0f, 1.0f, xt);
	//		const auto y = lerp(-1.0f, 1.0f, yt);
	//		return Vec2(x, y);
	//	};

	//	for (i32 yi = 0; yi < gridSize; yi++) {
	//		for (i32 xi = 0; xi < gridSize; xi++) {
	//			const auto p = indexToPos(xi, yi);
	//			f(xi, yi) = dist(p, focus[0]) + dist(p, focus[1]);
	//		}
	//	}
	//	std::vector<MarchingSquaresLine> lines;
	//	marchingSquares2(lines, constView2d(f), eccentricity, true);
	//	for (auto& line : lines) {
	//		line.a = indexToPos(line.a.x, line.a.y);
	//		line.b = indexToPos(line.b.x, line.b.y);
	//	}
	//	for (const auto& line : lines) {
	//		const auto aOutside = line.a.lengthSq() > 1.0f;
	//		const auto bOutside = line.b.lengthSq() > 1.0f;
	//		if (aOutside && bOutside) {
	//			continue;
	//		}
	//		if (aOutside != bOutside) {
	//			Vec2 start = line.a;
	//			Vec2 end = line.b;
	//			if (aOutside && !bOutside) {
	//				std::swap(start, end);
	//			}
	//			const auto hit = circleRaycast(start, end, Constants::boundary);
	//			if (hit.has_value()) {
	//				Dbg::line(start, hit->pos, 0.01f);
	//			}
	//		} else {
	//			Dbg::line(line.a, line.b, 0.01f);
	//		}
	//	}
	//	Dbg::disk(focus[0], 0.02f, Color3::RED);
	//	Dbg::disk(focus[1], 0.02f, Color3::RED);
	//	segments.clear();
	//	for (const auto& line : lines) {
	//		segments.push_back(Seg{ .a = line.a, .b = line.b });
	//	}
	//}

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
			};
			std::optional<Hit> closest;
			std::optional<Hit> closestToWrappedAround;

			auto processLineSegmentIntersections = [&laserLine, &laserPosition, &boundaryIntersectionWrappedAround, &laserDirection, &closest, &closestToWrappedAround, &hitOnLastIteration](
				Vec2 endpoint0,
				Vec2 endpoint1,
				EditorEntityId id,
				i32 index = 0) {

					const auto line = stereographicLine(endpoint0, endpoint1);
					const auto intersections = stereographicLineVsStereographicLineIntersection(line, laserLine);

					for (const auto& intersection : intersections) {
						if (const auto outsideBoundary = intersection.length() > 1.0f) {
							continue;
						}

						const auto epsilon = 0.001f;
						const auto lineDirection = (endpoint1 - endpoint0).normalized();
						const auto dAlong0 = dot(lineDirection, endpoint0) - epsilon;
						const auto dAlong1 = dot(lineDirection, endpoint1) + epsilon;
						const auto intersectionDAlong = dot(lineDirection, intersection);
						if (intersectionDAlong <= dAlong0 || intersectionDAlong >= dAlong1) {
							continue;
						}

						const auto distance = intersection.distanceTo(laserPosition);
						const auto distanceToWrappedAround = intersection.distanceSquaredTo(boundaryIntersectionWrappedAround);

						const auto hitLastTime = hitOnLastIteration.has_value() && hitOnLastIteration->id == id && hitOnLastIteration->partIndex == index;

						// hitLastTime should be only in the if not the else. You can hit an object from the front on one iteration and the from the wrapped around on the other.
						if (!hitLastTime && dot(intersection - laserPosition, laserDirection) > 0.0f) {
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
				processLineSegmentIntersections(endpoints[0], endpoints[1], EditorEntityId(mirror.id));
			}

			for (const auto& portalPair : e.portalPairs) {
				for (i32 i = 0; i < 2; i++) {
					const auto& portal = portalPair->portals[i];
					const auto endpoints = portal.endpoints();
					processLineSegmentIntersections(endpoints[0], endpoints[1], EditorEntityId(portalPair.id), i);
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

				auto hitOnNormalSide = [&normalAtHitPoint](f32 hitObjectNormalAngle) {
					return dot(Vec2::oriented(hitObjectNormalAngle), normalAtHitPoint) > 0.0f;
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

				default:
					break;

				}

				CHECK_NOT_REACHED();
				return HitResult::END;
				};

			auto checkLaserVsTargetCollision = [&](const Segment& segment) {
				for (auto target : e.targets) {
					const auto intersections = stereographicSegmentVsCircleIntersection(laserLine, segment.endpoints[0], segment.endpoints[1], target->calculateCircle());
					if (intersections.size() > 0) {
						target->activated = true;
					}
				}
				};

			auto checkLaserVsTriggerCollision = [&](const Segment& segment) {
				for (auto trigger : e.triggers) {
					const auto intersections = stereographicSegmentVsCircleIntersection(laserLine, segment.endpoints[0], segment.endpoints[1], trigger->circle());
					if (intersections.size() > 0) {
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

			if (closest.has_value()) {
				const auto s = Segment{ laserPosition, closest->point, laser->color };
				processLaserSegment(s);
				const auto result = processHit(*closest);
				if (result == HitResult::END) {
					break;
				}
			} else if (closestToWrappedAround.has_value()) {
				const auto s0 = Segment{ laserPosition, boundaryIntersection, laser->color };
				const auto s1 = Segment{ boundaryIntersectionWrappedAround, closestToWrappedAround->point, laser->color };
				processLaserSegment(s0);
				processLaserSegment(s1);

				const auto result = processHit(*closestToWrappedAround);
				if (result == HitResult::END) {
					break;
				}
			} else {
				const auto s0 = Segment{ laserPosition, boundaryIntersection, laser->color };
				const auto s1 = Segment{ laserPosition, boundaryIntersectionWrappedAround, laser->color };
				processLaserSegment(s0);
				processLaserSegment(s1);
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

			const auto epsilon = 0.01f;
			const auto epsilonSquared = epsilon * epsilon;
			if (a.endpoints[0].distanceSquaredTo(b.endpoints[0]) < epsilonSquared
				&& a.endpoints[1].distanceSquaredTo(b.endpoints[1]) < epsilonSquared) {
				b.ignore = true;
			}
		}
	}

	for (auto door : e.doors) {
		const auto info = triggerInfo(e.triggers, door->triggerIndex);
		const auto isOpening = info.has_value() && info->active;
		const auto speed = 1.5f;
		door->openingT += speed * Constants::dt * (isOpening ? 1.0f : -1.0f);
		door->openingT = std::clamp(door->openingT, 0.0f, 1.0f);
	}
}

std::optional<GameState::TriggerInfo> GameState::triggerInfo(TriggerArray& triggers, i32 triggerIndex) {
	for (const auto& trigger : triggers) {
		if (trigger->index == triggerIndex) {
			return TriggerInfo{ .color = trigger->color, .active = trigger->activated };
		}
	}
	return std::nullopt;
}
