#include "GameUpdate.hpp"
#include <engine/Math/Constants.hpp>
#include <array>
#include <engine/Input/Input.hpp>
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
	//const auto maxAllowedLength = Constants::boundary.radius - 0.001f;
	const auto maxAllowedLength = Constants::boundary.radius;
	if (length > maxAllowedLength) {
		v *= maxAllowedLength / length;
	}
	return v;
}

void GameState::update(GameEntities& e) {
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

	static f32 angle = 0.0f;
	const auto direction = Vec2::oriented(angle);
	Dbg::line(Vec2(0.0f), direction * 0.1f, 0.01f, Color3::CYAN);
	const auto rotationSpeed = TAU<f32> * 1.5f;
	const auto da = rotationSpeed * Constants::dt;
	if (Input::isKeyHeld(KeyCode::A)) {
		angle += da;
	}
	if (Input::isKeyHeld(KeyCode::D)) {
		angle -= da;
	}
	const auto movementSpeed = 2.0f;
	auto movement = [&direction](f32 speed) {
		return Quat(speed * Constants::dt, cross(Vec3(0.0f, 0.0f, 1.0f), Vec3(direction.x, direction.y, 0.0f)));
	};

	std::optional<Quat> transformationChange;
	if (Input::isKeyHeld(KeyCode::W)) {
		transformationChange = movement(movementSpeed);
	} 
	if (Input::isKeyHeld(KeyCode::S)) {
		transformationChange = movement(-movementSpeed);
	}
	if (transformationChange.has_value()) {
		accumulatedTransformation = *transformationChange * accumulatedTransformation;
		accumulatedTransformation = accumulatedTransformation.normalized();
	}
	
	auto applyTransformation = [](Vec2 v, Quat transformation) {
		return toStereographic(transformation * fromStereographic(v));
	};

	auto transformWallLikeObject = [&applyTransformation](const Vec2* initialEndpoints, Vec2* endpoints, Quat transformation) {
		bool bothOutside = true;
		for (i32 i = 0; i < 2; i++) {
			endpoints[i] = applyTransformation(initialEndpoints[i], transformation);
			bothOutside &= endpoints[i].length() > Constants::boundary.radius;
		}
		if (bothOutside) {
			for (i32 i = 0; i < 2; i++) {
				auto& endpoint = endpoints[i];
				endpoint = antipodalPoint(endpoint);
			}
		}
	};

	for (auto wall : e.walls) {
		transformWallLikeObject(wall->initialEndpoints, wall->endpoints, accumulatedTransformation);
	}

	for (auto door : e.doors) {
		transformWallLikeObject(door->initialEndpoints, door->endpoints, accumulatedTransformation);
	}

	for (auto trigger : e.triggers) {
		trigger->position = applyTransformation(trigger->initialPosition, accumulatedTransformation);
	}

	for (auto target : e.targets) {
		target->position = applyTransformation(target->initialPosition, accumulatedTransformation);
	}

	auto transformWithNormalAngle = [&applyTransformation](Vec2& position, f32& normalAngle, Quat transformation) {
		const auto tangentAngle = normalAngle + PI<f32> / 2.0f;
		const auto secondPoint = moveOnStereographicGeodesic(position, tangentAngle, 0.1f);

		const auto initialLine = stereographicLine(position, secondPoint);

		auto newPosition = applyTransformation(position, transformation);

		bool flipped = false;
		if (newPosition.length() > Constants::boundary.radius) {
			flipped = true;
			newPosition = antipodalPoint(newPosition);
		}
		const auto newSecondPoint = applyTransformation(secondPoint, transformation);

		const auto newLine = stereographicLine(newPosition, newSecondPoint);

		auto newNormal = stereographicLineNormalAt(newLine, newPosition);
		//auto newNormal = normalAtEndpoint0(newPosition, newSecondPoint);

		/*if (flipped) {
			newNormal = -newNormal;
		}*/

		/*auto newTangent = -newNormal.rotBy90deg();
		if (flipped) {
			newTangent = -newTangent;
		} 
		if (dot(newTangent, newSecondPoint - newPosition) < 0.0f) {
			newNormal = -newNormal;
		}*/

		const Vec2 pointOnNormalSide = position + Vec2::oriented(normalAngle) * 0.1f;
		Vec2 newPointOnNormalSide = applyTransformation(pointOnNormalSide, transformation);
		if (flipped) {
			newPointOnNormalSide = antipodalPoint(newPointOnNormalSide);
		}
		/*Dbg::disk(pointOnNormalSide, 0.02f, Color3::RED);
		Dbg::disk(newPointOnNormalSide, 0.02f, Color3::GREEN);*/
		if (dot(newNormal, newPointOnNormalSide - newPosition) < 0.0f) {
			newNormal = -newNormal;
		}

		//if ()
		//if (flipped != (dot(newNormal, newPointOnNormalSide - newPosition) < 0.0f)) {
		//	//CHECK_NOT_REACHED();
		//	//newNormal = -newNormal;
		//}
		/*if (dot(newNormal, newPointOnNormalSide - newPosition) < 0.0f) {
			newNormal = -newNormal;
		}*/
		
		/*if (initialLine.type == StereographicLine::Type::LINE) {
			const auto pointOnNormalSide = position + initialLine.lineNormal;
			auto newPointOnNormalSide = applyTransformation(position, transformation);
			if (newPointOnNormalSide.length() > Constants::boundary.radius) {
				newPointOnNormalSide = antipodalPoint(newPointOnNormalSide);
			}

			if (newLine.type == StereographicLine::Type::LINE) {
				if (dot(newNormal, newPointOnNormalSide - newPosition) < 0.0f) {
					newNormal = -newNormal;
				}
			} else {
				if (newPointOnNormalSide.distanceTo(newLine.circle.center) > newLine.circle.radius) {
					newNormal = -newNormal;
				}
			}
		} else {
			const auto pointOnNormalSide = initialLine.circle.center;
			auto newPointOnNormalSide = applyTransformation(position, transformation);
			if (newPointOnNormalSide.length() > Constants::boundary.radius) {
				newPointOnNormalSide = antipodalPoint(newPointOnNormalSide);
			}

			if (newLine.type == StereographicLine::Type::LINE) {
				if (dot(newNormal, newPointOnNormalSide - newPosition) < 0.0f) {
					newNormal = -newNormal;
				}
			} else {
				if (newPointOnNormalSide.distanceTo(newLine.circle.center) > newLine.circle.radius) {
					newNormal = -newNormal;
				}
			}
		}*/
		
		/*if (newPosition.length() > Constants::boundary.radius) {
			auto newNormal = normalAtEndpoint0(newPosition, newSecondPoint);

			newPosition = antipodalPoint(newPosition);
		}*/
		position = newPosition;
		normalAngle = newNormal.angle();
	};

	if (transformationChange.has_value()) {
		for (auto mirror : e.mirrors) {
			transformWithNormalAngle(mirror->center, mirror->normalAngle, *transformationChange);
		}
		for (auto portalPair : e.portalPairs) {
			for (auto& portal : portalPair->portals) {
				transformWithNormalAngle(portal.center, portal.normalAngle, *transformationChange);
			}
		}
		for (auto laser : e.lasers) {
			// Don't know why this works with a tangent angle without modifications. 
			// It breaks if you try to add and subtruct pi/2 when you move into the boundary.
			transformWithNormalAngle(laser->position, laser->angle, *transformationChange);
		}
	}

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

					/*const auto epsilon = 0.001f;
					const auto lineDirection = (endpoint1 - endpoint0).normalized();
					const auto dAlong0 = dot(lineDirection, endpoint0) - epsilon;
					const auto dAlong1 = dot(lineDirection, endpoint1) + epsilon;
					const auto intersectionDAlong = dot(lineDirection, intersection);
					if (intersectionDAlong <= dAlong0 || intersectionDAlong >= dAlong1) {
						continue;
					}*/
					// TODO: Should there be epsilon checks?
					if (!isPointOnLineAlsoOnStereographicSegment(line, endpoint0, endpoint1, intersection, 0.01f)) {
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

				const auto segments = splitStereographicSegment(endpoint0, endpoint1);

				if (segments.size() == 1) {
					processLineSegmentIntersections(segments[0].endpoints[0], segments[0].endpoints[1], id, index, false);
				} else if (segments.size() == 2) {
					processLineSegmentIntersections(segments[0].endpoints[0], segments[0].endpoints[1], id, index, false);
					processLineSegmentIntersections(segments[1].endpoints[0], segments[1].endpoints[1], id, index, true);
				}
			};

			for (const auto& wall : e.walls) {
				processSplitLineSegmentIntersections(wall->endpoints[0], wall->endpoints[1], EditorEntityId(wall.id));
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
				//Dbg::line(hit.point, hit.point + laserTangentAtHitPoint * 0.1f, 0.01f, Color3::RED);

				/*auto laserTangentAtHitPoint = stereographicLineNormalAt(laserLine, hit.point).rotBy90deg();
				Dbg::disk(hit.point + laserTangentAtHitPoint * 0.05f, 0.01f, Color3::YELLOW);
				if (hit.wrappedAround) {
					if (distance(hit.point + laserTangentAtHitPoint * 0.05f, laserPosition) < distance(hit.point, laserPosition)) {
						laserTangentAtHitPoint = -laserTangentAtHitPoint;
					}
				} else {
					if (distance(hit.point + laserTangentAtHitPoint * 0.05f, laserPosition) > distance(hit.point, laserPosition)) {
						laserTangentAtHitPoint = -laserTangentAtHitPoint;
					}
				}*/
				//if (distance(hit.point + laserTangentAtHitPoint * 0.05f, laserPosition) > distance(hit.point, laserPosition)) {
				//	/*if (!hit.wrappedAround) {
				//	}*/
				//	laserTangentAtHitPoint = -laserTangentAtHitPoint;

				//} else {
				//	/*if (hit.wrappedAround) {
				//		laserTangentAtHitPoint = -laserTangentAtHitPoint;
				//	}*/
				//}
				/*if (dot(laserTangentAtHitPoint, laserDirection) > 0.0f) {
					laserTangentAtHitPoint = -laserTangentAtHitPoint;
				}*/
				//Dbg::line(hit.point, hit.point + laserTangentAtHitPoint * 0.1f, 0.02f, Color3::RED);

				//auto normalAtHitPoint = stereographicLineNormalAt(hit.line, hit.point);
				//if (dot(normalAtHitPoint, laserTangentAtHitPoint) < 0.0f) {
				//	normalAtHitPoint = -normalAtHitPoint;
				//}
				//Dbg::line(hit.point, hit.point + hitObjectNormalAtHitPoint * 0.1f, 0.01f);

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
					//const auto r = 0.3f;

					//const auto r0 = angleRangeBetweenPointsOnCircle(Vec2(0.0f), laserTangentAtHitPoint, normalAtHitPoint);
					///*Dbg::circleArc(laserDirection.angle(), normalAtHitPoint.angle(), hit.point, 0.05f, 0.01f, Color3::RED);*/
					//Dbg::circleArc(r0.min, r0.max, hit.point, r / 2.0f, 0.01f / 2.0f, Color3::RED);

					laserDirection = laserTangentAtHitPoint.reflectedAroundNormal(hitObjectNormalAtHitPoint);

					//const auto r1 = angleRangeBetweenPointsOnCircle(Vec2(0.0f), laserDirection, normalAtHitPoint);
					///*Dbg::circleArc(laserDirection.angle(), normalAtHitPoint.angle(), hit.point, 0.05f, 0.01f, Color3::RED);*/
					//Dbg::circleArc(r1.min, r1.max, hit.point, r / 2.0f, 0.01f / 2.0f, Vec3(1.0f) - Color3::RED);

					//Dbg::line(hit.point, hit.point + normalAtHitPoint.normalized() * r, 0.01f, Color3::GREEN);
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
					if (hit.objectMirrored) {
						laserDirection = laserDirection.reflectedAroundNormal(outPortalNormalAtOutPoint);
					}
					if (outMirrored) {
						laserDirection = laserDirection.reflectedAroundNormal(outPortalNormalAtOutPoint);
					}

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
			auto doubleIntersectionCheck = [](const Hit& hit, std::optional<f32> secondClosestDistance) {
				// If the closest hit is to close to the laser position sometimes bugs happen.
				if (hit.distance < 0.001f) {
					// There was a bug where there was a wall mirror on the boundary and a laser hit it wrapped around. This kinda fixes the bug, but also ends the laser where it shouldn't really end. When designing a level to deal with this issue could just create the wall on both sides.
					//return true;
				}

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

			auto areNearlyAntipodal = [](Vec2 e0, Vec2 e1) {
				return 
					(abs(e0.length() - 1.0f) < 0.01f) &&
					(abs(e1.length() - 1.0f) < 0.01f) && 
					(e0 + e1).length() < 0.01f;
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

				const auto nearlyAntipodal = areNearlyAntipodal(e0, e1);
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
				//Dbg::disk(closestToWrappedAround->point, 0.01f, Color3::CYAN);
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
			//Dbg::line(laserPosition, laserPosition + laserDirection * 0.1f, 0.01f, Color3::YELLOW);
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
