#include "Stereographic.hpp"
#include <array>
//#include <engine/Math/Constants.hpp>
#include <engine/Math/LineSegment.hpp>

// The 3d coordinate system is the set of points satisfying x^2 + y^2 + z^2 = 1, z <= 0, hemisphere.
// The 2d coordinate system is the set of points satisfying x^2 + y^2 <= 1, circle

std::optional<RaycastHit> circleRaycast(Vec2 rayStart, Vec2 rayEnd, const Circle& circle) {
	// TODO: This could probably be made faster, because by just checking if both points are inside or outside circle.
	const auto rayDirection = rayEnd - rayStart;
	const auto
		a = dot(rayDirection, rayDirection),
		b = dot(rayStart, rayDirection) * 2.0f,
		c = dot(rayStart, rayStart) - circle.radius * circle.radius;
	const auto discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f)
		return std::nullopt;

	const auto sqrtDiscriminant = sqrt(discriminant);
	const auto
		t0 = (-b + sqrtDiscriminant) / a / 2.0f,
		t1 = (-b - sqrtDiscriminant) / a / 2.0f;

	const auto t = t0 < t1 ? t0 : t1;
	if (t < 0.0f || t > 1.0f) {
		return std::nullopt;
	}
	return RaycastHit{
		.pos = rayStart + t * rayDirection,
		.t = t,
	};
}

// https://en.wikipedia.org/wiki/Stereographic_projection#First_formulation
Vec2 toStereographic(Vec3 p) {
	const auto d = 1.0f - p.z;
	return Vec2(p.x / d, p.y / d);
}

Vec3 fromStereographic(Vec2 p) {
	const auto d = p.x * p.x + p.y * p.y + 1.0f;
	return (Vec3(2.0f * p.x, 2.0f * p.y, -1.0f + p.x * p.x + p.y * p.y) / d).normalized();
}

// The angle is such that when stereographically projected it equal the plane angle.
Quat movementOnSphericalGeodesic(Vec3 pos, f32 angle, f32 distance) {
	const auto a = atan2(pos.y, pos.x);
	const auto up = Vec3(0.0f, 0.0f, 1.0f);

	Vec3 axis(0.0f, 1.0f, 0.0f);
	if (pos != -up) {
		axis = cross(pos, up).normalized();
	} else {
		// It can be verified that this edge case works by placing a mirror at the center and then this case triggers.
	}

	// The formula for the angle was kinda derived through trial and error, but it matches the results of the correct method that isn't used, because it can't handle length specification. That version is available in previous commits in the mirror draw loop.
	return Quat(-angle + a, pos) * Quat(distance, axis);
}

Vec3 moveOnSphericalGeodesic(Vec3 pos, f32 angle, f32 distance) {
	// Normalizing, because rotating introduces errors in the norm that get amplified by the projection. 
	return (pos * movementOnSphericalGeodesic(pos, angle, distance)).normalized();
}

Vec2 moveOnStereographicGeodesic(Vec2 pos, f32 angle, f32 distance) {
	return toStereographic(moveOnSphericalGeodesic(fromStereographic(pos), angle, distance));
}

Vec2 antipodalPoint(Vec2 p) {
	const auto onSphere = fromStereographic(p);
	const auto r0 = toStereographic(-onSphere);

	const auto a = 2.0f * (p.x * p.x + p.y * p.y);
	const auto r1 = Vec2(-2.0f * p.x, -2.0f * p.y) / a;
	return r1;
}


// https://www.johndcook.com/blog/2023/06/18/circle-through-three-points/
Circle circleThroughPoints(Vec2 p0, Vec2 p1, Vec2 p2) {
	const auto x1 = p0.x, y1 = p0.y, x2 = p1.x, y2 = p1.y, x3 = p2.x, y3 = p2.y;
	const auto s1 = pow(x1, 2.0f) + pow(y1, 2.0f);
	const auto s2 = pow(x2, 2.0f) + pow(y2, 2.0f);
	const auto s3 = pow(x3, 2.0f) + pow(y3, 2.0f);
	const auto m11 = x1 * y2 + x2 * y3 + x3 * y1 - (x2 * y1 + x3 * y2 + x1 * y3);
	const auto m12 = s1 * y2 + s2 * y3 + s3 * y1 - (s2 * y1 + s3 * y2 + s1 * y3);
	const auto m13 = s1 * x2 + s2 * x3 + s3 * x1 - (s2 * x1 + s3 * x2 + s1 * x3);
	const auto x0 = 0.5f * m12 / m11;
	const auto y0 = -0.5f * m13 / m11;
	const auto r0 = sqrt(pow((x1 - x0), 2.0f) + pow((y1 - y0), 2.0f));
	return Circle(Vec2(x0, y0), r0);
}

std::optional<Circle> circleThroughPointsWithNormalAngle(Vec2 p0, f32 angle0, Vec2 p1) {
	/*
	If the circle passes through 2 points it's center lies on a line.
	If there is some N normal at a A point then the center lies on the A + tN.
	So we have 2 lines. This defines the center.
	*/
	const auto midpointConstraint = Line::fromPointAndNormal((p0 + p1) / 2.0f, (p0 - p1).normalized());
	const auto normalConstraint = Line::fromParametric(p0, Vec2::oriented(angle0));

	const auto result = midpointConstraint.intersection(normalConstraint);
	if (!result.has_value()) {
		return std::nullopt;
	}

	return Circle(*result, distance(p0, *result));
}
#include <gfx2d/DbgGfx2d.hpp>
StereographicLine stereographicLineThroughPointWithTangent(Vec2 p, f32 tangentAngle, f32 translation) {
	//if (p == Vec2(0.0f)) {
	//	return StereographicLine(Vec2::oriented(tangentAngle + PI<f32> / 2.0f));
	//}
	const auto pointAhead = moveOnStereographicGeodesic(p, tangentAngle, translation);
	//Dbg::disk(pointAhead, 0.01f, Vec3(1.0f, 0.0f, 0.0f));
	return stereographicLine(p, pointAhead);
}

Circle stereographicLineOld(Vec2 p0, Vec2 p1) {
	// A great circle always passes through the antipodal point.
	// Maybe doing the computation in S_2 would be more stable.
	Vec2 antipodal = antipodalPoint(p0);
	if (abs(p0.length() - 1.0f) < 0.01f) {
		antipodal = antipodalPoint(p1);
	}
	//Dbg::disk(antipodal, 0.01f, Vec3(1.0f, 0.0f, 0.0f));
	//Dbg::disk(p0, 0.01f, Vec3(1.0f, 0.0f, 0.0f));
	//Dbg::disk(p1, 0.01f, Vec3(1.0f, 0.0f, 0.0f));
	return circleThroughPoints(p0, p1, antipodal);
}

bool nearlyColinear(Vec2 p0, Vec2 p1, Vec2 p2) {
	// https://math.stackexchange.com/questions/405966/if-i-have-three-points-is-there-an-easy-way-to-tell-if-they-are-collinear
	// https://stackoverflow.com/questions/65396833/testing-three-points-for-collinearity-in-a-numerically-robust-way
	const auto l0 = p0.distanceTo(p1);
	const auto l1 = p0.distanceTo(p2);
	const auto l2 = p1.distanceTo(p2);
	const auto longestSide = std::max(l0, std::max(l1, l2));
	const auto parallelogramArea = abs(det(p1 - p0, p2 - p0));
	const auto parallelogramHeight = parallelogramArea / longestSide;
	return parallelogramHeight < 0.0005f;
}

StereographicLine stereographicLine(Vec2 p0, Vec2 p1) {
	Vec2 p2(0.0f);
	// The antipodal point of (0, 0) is infinity so this tries to prevent the antipodal point being set to zero by choosing the one that is further from zero.
	if (p0.lengthSq() > p1.lengthSq()) {
		p2 = antipodalPoint(p0);
	} else {
		p2 = antipodalPoint(p1);
	}


	if (nearlyColinear(p0, p1, p2)) {
		return StereographicLine((p0 - p1).rotBy90deg().normalized());
	}
	return stereographicLineOld(p0, p1);
}

#include <gfx2d/DbgGfx2d.hpp>

bool isPointOnLineAlsoOnStereographicSegment(const StereographicLine& line, Vec2 endpoint0, Vec2 endpoint1, Vec2 pointThatLiesOnLine, f32 epsilon) {
	// For this to work with endpoints that are at any point outside of the unit circle doing calculations on the stereographic projection circle or line won't work, because a large part of the circle might be outside the unit circle and the shortest angle in the model won't be the shortest angle on the sphere. 
	// One issue with working on the sphere is that epsilons are in the wrong space. Because the points lie on the circle anyway the code also check if the point is epsilon distance from the endpoints.

	const auto e0 = fromStereographic(endpoint0);
	const auto e1 = fromStereographic(endpoint1);
	const auto p = fromStereographic(pointThatLiesOnLine);
	Vec3 t = e1 - e0;
	f32 along = dot(p, t);
	Vec3 sphereCenterToChordCenter = (e0 + e1) / 2.0f;
	const auto sign = dot(sphereCenterToChordCenter.normalized(), p);
	// The case when the points are antipodal is ambigous return false.
	if (sign == 0.0f) {
		return false;
	}
	if (const auto oneTheWrongHemisphere = sign < 0.0f) {
		return false;
	}
	/*auto arccosClamped = [](f32 v, f32 e) {
		v = std::clamp(v, -1.0f, 1.0f);
		return cos(std::clamp(acos(v) + e, 0.0f, PI<f32>));
	};*/
	/*return along >= dot(e0, t) && along <= dot(e1, t);*/
	//auto stereographicProjection1d = [](f32 r) {
	//	// https://en.wikipedia.org/wiki/Stereographic_projection
	//	const auto z = 
	//	const auto 
	//};

	//f32 along = 
	return along >= dot(e0, t) && along <= dot(e1, t) 
		|| distance(pointThatLiesOnLine, endpoint0) < epsilon
		|| distance(pointThatLiesOnLine, endpoint0) < epsilon;
	//return arccosClamped(along, 0.0f) >= arccosClamped(dot(e0, t), -epsilon) && along <= arccosClamped(dot(e1, t), epsilon);


	//// Checks if a point is on the line or it's antipodal version.
	//// A stereographic line is a great circle on the plane.
	//// This projects points onto the chord (e0, e1) of the great circle.

	//if (line.type == StereographicLine::Type::CIRCLE) {
	//	const auto e0 = fromStereographic(endpoint0);
	//	const auto e1 = fromStereographic(endpoint1);
	//	const auto p = fromStereographic(pointThatLiesOnLine);
	//	Vec3 t = e1 - e0;
	//	f32 along = dot(p, t);

	//	return along >= dot(e0, t) && along <= dot(e1, t);
	//	//auto angleRange = angleRangeBetweenPointsOnCircle(line.circle.center, endpoint0, endpoint1);
	//	//// arcLength = radius * angle
	//	//// angle = arcLength / radius.
	//	//const auto angleEpsilon = epsilon / line.circle.radius;
	//	//// I found a bug and the epsilon break thing even more for some reason. The level is just a chaotic laser. Not sure why it passes through. It shouldn't happen in normal gameplay so I won't bother with it for now.
	//	//angleRange.min -= angleEpsilon;
	//	//angleRange.max += angleEpsilon;
	//	//// TODO: Maybe clamp if min switches to max.

	//	//return angleRange.isInRange((pointThatLiesOnLine - line.circle.center).angle());
	//} else {
	//	const auto lineDirection = (endpoint1 - endpoint0).normalized();
	//	if (lineDirection == Vec2(0.0f)) {
	//		return false;
	//	}
	//	const auto dAlong0 = dot(lineDirection, endpoint0) - epsilon;
	//	const auto dAlong1 = dot(lineDirection, endpoint1) + epsilon;
	//	//const auto dAlong0 = dot(lineDirection, endpoint0);
	//	//const auto dAlong1 = dot(lineDirection, endpoint1);
	//	const auto intersectionDAlong = dot(lineDirection, pointThatLiesOnLine);
	//	return intersectionDAlong >= dAlong0 && intersectionDAlong <= dAlong1;
	//}
}

/*
Shadertoy test

float circularArcDistance(vec2 p, vec2 c, float r, float a0, float a1) {
	vec2 v = p - c;
	float pa = atan(v.y, v.x);

	if (pa >= a0 && pa <= a1) {
		return abs(distance(p, c) - r);
	}
	return min(
		distance(r * vec2(cos(a0), sin(a0)), p),
		distance(r * vec2(cos(a1), sin(a1)), p)
	);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
	vec2 p = fragCoord/iResolution.xy;
	p -= 0.5;
	p.x *= iResolution.x / iResolution.y;

	float a = circularArcDistance(p, vec2(0), 0.3, 0.0, 2.5);

	vec3 col = vec3(smoothstep(0.05, 0.06, a));
	fragColor = vec4(col, 1.0);
}
*/

f32 circularArcDistance(Vec2 p, Circle circle, AngleRange angleRange) {
	if (angleRange.isInRange((p - circle.center).angle())) {
		return abs(distance(p, circle.center) - circle.radius);
	}
	return std::min(
		distance(circle.center + Vec2::fromPolar(angleRange.min, circle.radius), p),
		distance(circle.center + Vec2::fromPolar(angleRange.max, circle.radius), p)
	);
}

Circle stereographicCircle(Vec2 position, f32 radius) {
	const auto center = fromStereographic(position);

	// This could be simplified, because we don't care what direction the movement is in. The previous code (in EditorEntities.hpp) doesn't handle the cross product edge case.
	const auto p0 = moveOnSphericalGeodesic(center, 0.0f, radius);
	const auto rotate = Quat(PI<f32> / 2.0f, center);
	const auto p1 = rotate * p0;
	const auto p2 = rotate * p1;
	// The stereographic projection of center isn't necessarily the center of the stereograhic of the `circle on the sphere`. For small radii this isn't very noticible.
	return circleThroughPoints(toStereographic(p0), toStereographic(p1), toStereographic(p2));
}

AngleRange angleRangeBetweenPointsOnCircle(Vec2 circleCenter, Vec2 pointOnCircle0, Vec2 pointOnCircle1) { 
	f32 a0 = angleToRangeZeroTau((pointOnCircle0 - circleCenter).angle());
	f32 a1 = angleToRangeZeroTau((pointOnCircle1 - circleCenter).angle());

	if (a0 > a1) {
		std::swap(a0, a1);
	}
	if (a1 - a0 > PI<f32>) {
		a0 += TAU<f32>;
	}
	if (a0 > a1) {
		std::swap(a0, a1);
	}

	return AngleRange{ .min = a0, .max = a1 };
}

Vec3 sphericalGeodesicSegmentMidpoint(Vec3 e0, Vec3 e1) {
	// By adding 2 vectors on a circle we get a rhombus which bisects the angle.
	return (e0 + e1).normalized();
}

Vec2 stereographicSegmentMidpoint(Vec2 e0, Vec2 e1) {
	return toStereographic(sphericalGeodesicSegmentMidpoint(fromStereographic(e0), fromStereographic(e1)));
}

f32 sphericalDistance(Vec3 a, Vec3 b) {
	return acos(dot(a.normalized(), b.normalized()));
}

f32 stereographicDistance(Vec2 a, Vec2 b) {
	return sphericalDistance(fromStereographic(a), fromStereographic(b));
}

//f32 signedSphericalDistance(Vec3 a, Vec3 b) {
//	a = a.normalized();
//	b = b.normalized();
//	const auto dist = acos(dot(a.normalized(), b.normalized()));
//	if (dot(cross(a, b), a) > 0.0f) {
//		return -dist;
//	}
//	return dist;
//}
//
//f32 signedStereographicDistance(Vec2 a, Vec2 b) {
//	return signedSphericalDistance(fromStereographic(a), fromStereographic(b));
//}

Vec2 stereographicLineNormalAt(const StereographicLine& line, Vec2 p) {
	return line.type == StereographicLine::Type::CIRCLE
		? (p - line.circle.center).normalized()
		: line.lineNormal;
}

f32 eucledianDistanceToStereographicSegment(Vec2 e0, Vec2 e1, Vec2 eucledianPoint) {
	const auto line = stereographicLine(e0, e1);
	if (line.type == StereographicLine::Type::LINE) {
		return LineSegment(e0, e1).distance(eucledianPoint);
	} else {
		const auto range = angleRangeBetweenPointsOnCircle(line.circle.center, e0, e1);
		return circularArcDistance(eucledianPoint, line.circle, range);
	}
}

// https://stackoverflow.com/questions/55816902/finding-the-intersection-of-two-circles
StaticList<Vec2, 2> circleVsCircleIntersection(const Circle& c0, const Circle& c1) {
	StaticList<Vec2, 2> out;

	const auto d = distance(c0.center, c1.center);
	if (d > c0.radius + c1.radius) {
		return out;
	}

	if (d < std::abs(c0.radius - c1.radius)) {
		return out;
	}
	
	if (d == 0.0f) {
		return out;
	}

	const auto a = (pow(c0.radius, 2.0f) - pow(c1.radius, 2.0f) + pow(d, 2.0f)) / (2.0f * d);
	const auto h = sqrt(pow(c0.radius, 2.0f) - pow(a, 2.0f));
	const auto x0 = c0.center.x;
	const auto y0 = c0.center.y;
	const auto x1 = c1.center.x;
	const auto y1 = c1.center.y;

    const auto x2 = x0 + a * (x1 - x0) / d;
    const auto y2 = y0 + a * (y1 - y0) / d;

    const auto x3 = x2 + h * (y1 - y0) / d;
    const auto y3 = y2 - h * (x1 - x0) / d;
    const auto x4 = x2 - h * (y1 - y0) / d;
	const auto y4 = y2 + h * (x1 - x0) / d;

	out.add(Vec2(x3, y3));
	out.add(Vec2(x4, y4));
	return out;
}

StaticList<Vec2, 2> lineVsCircleIntersection(Vec2 linePoint, Vec2 lineDirection, const Circle& circle) {
	StaticList<Vec2, 2> out;

	const auto start = linePoint - circle.center;
	const auto
		a = dot(lineDirection, lineDirection),
		b = dot(start, lineDirection) * 2.0f,
		c = dot(start, start) - circle.radius * circle.radius;
	const auto discriminant = b * b - 4.0f * a * c;
	if (discriminant < 0.0f) {
		return out;
	}

	const auto sqrtDiscriminant = sqrt(discriminant);
	const auto
		t0 = (-b + sqrtDiscriminant) / a / 2.0f,
		t1 = (-b - sqrtDiscriminant) / a / 2.0f;

	out.add(linePoint + lineDirection * t0);
	out.add(linePoint + lineDirection * t1);
	return out;
}

StaticList<Vec2, 2> stereographicLineVsCircleIntersection(const StereographicLine& l, const Circle& circle) {
	switch (l.type) {
		using enum StereographicLine::Type;

	case LINE: {
		return lineVsCircleIntersection(Vec2(0.0f), l.lineNormal.rotBy90deg(), circle);
	}

	case CIRCLE: {
		return circleVsCircleIntersection(l.circle, circle);
	}

	}
	return StaticList<Vec2, 2>();
}

StaticList<Vec2, 2> stereographicLineVsStereographicLineIntersection(const StereographicLine& a, const StereographicLine& b) {

	switch (a.type) {
		using enum StereographicLine::Type;

	case LINE: {
		switch (b.type) {
		case LINE: {
			StaticList<Vec2, 2> out;
			const auto result = Line(a.lineNormal, 0.0f).intersection(Line(b.lineNormal, 0.0f));
			if (result.has_value()) {
				out.add(*result);
				return out;
			}
			break;
		}

		case CIRCLE: {
			return lineVsCircleIntersection(Vec2(0.0f), a.lineNormal.rotBy90deg(), b.circle);
		}

		}
		break;
	}

	case CIRCLE: {
		switch (b.type) {
		case LINE: {
			return lineVsCircleIntersection(Vec2(0.0f), b.lineNormal.rotBy90deg(), a.circle);
		}

		case CIRCLE: {
			return circleVsCircleIntersection(a.circle, b.circle);
		}

		}
	}

	}

	return StaticList<Vec2, 2>();
}

StaticList<Vec2, 2> stereographicSegmentVsCircleIntersection(const StereographicLine& line, Vec2 lineEndpoint0, Vec2 lineEndpoint1, const Circle& circle) {
	const auto intersections = stereographicLineVsCircleIntersection(line, circle);
	StaticList<Vec2, 2> result;

	for (const auto& intersection : intersections) {
		if (isPointOnLineAlsoOnStereographicSegment(line, lineEndpoint0, lineEndpoint1, intersection)) {
			result.add(intersection);
		}
	}
	return result;

	////const auto epsilon = 0.001f;
	//const auto lineDirection = (lineEndpoint1 - lineEndpoint0).normalized();
	//const auto dAlong0 = dot(lineDirection, lineEndpoint0);
	//const auto dAlong1 = dot(lineDirection, lineEndpoint1);

	////for (const auto& intersection : intersections) {
	////	const auto intersectionDAlong = dot(lineDirection, intersection);
	////	// Using the regular segment check works, because the stereographic segment is convex.
	////	// This actually doesn't work for example if you have 2 intersections on antipodal or nearly antipodal points of the sphere. Then they will both line in the chord region (both have values of intersectionDAlong that is the length of the projection of onto the chord in the region), but obsiously the segment can't contain antipodal point.
	////	if (intersectionDAlong >= dAlong0 && intersectionDAlong <= dAlong1) {
	////		result.add(intersection);
	////	}
	////}

	//// TODO: They incorrect method with dot products should be replaced in other places where it was used.
	//if (line.type == StereographicLine::Type::CIRCLE) {
	//	const auto angleRange = angleRangeBetweenPointsOnCircle(line.circle.center, lineEndpoint0, lineEndpoint1);
	//	for (const auto& intersection : intersections) {
	//		if (angleRange.isInRange((intersection - line.circle.center).angle())) {
	//			result.add(intersection);
	//		}
	//	}
	//} else {
	//	for (const auto& intersection : intersections) {
	//		const auto intersectionDAlong = dot(lineDirection, intersection);
	//		if (intersectionDAlong >= dAlong0 && intersectionDAlong <= dAlong1) {
	//			result.add(intersection);
	//		}
	//	}
	//}
	//return result;
}

//bool intersectionsOnSegment(Vec2 e0, Vec2 e1, Vec2 i, f32 epsilon = 0.0f) {
//	const auto dir = (e1 - e0).normalized();
//	const auto along = dot(i, dir);
//	return along > dot(e0, dir) - epsilon && along < dot(e1, dir) + epsilon;
//}

StaticList<Vec2, 2> stereographicSegmentVsStereographicSegmentIntersection(const StereographicSegment& a, const StereographicSegment& b, f32 epsilon) {
	const auto intersections = stereographicLineVsStereographicLineIntersection(a.line, b.line);

	StaticList<Vec2, 2> result;

	for (const auto& intersection : intersections) {
		if (isPointOnLineAlsoOnStereographicSegment(a.line, a.endpoints[0], a.endpoints[1], intersection, epsilon) &&
			isPointOnLineAlsoOnStereographicSegment(b.line, b.endpoints[0], b.endpoints[1], intersection, epsilon)) {
			result.add(intersection);
		}
	}

	//for (const auto& intersection : intersections) {
	//	if (intersectionsOnSegment(a.endpoints[0], a.endpoints[1], intersection, epsilon) &&
	//		intersectionsOnSegment(b.endpoints[0], b.endpoints[1], intersection, epsilon)) {
	//		result.add(intersection);
	//	}
	//}
	return result;
}

bool stereographicSegmentVsSegmentCollision(const StereographicSegment& a, Vec2 endpoint0, Vec2 endpoint1) {
	auto intersectionsOnSegment = [](Vec2 e0, Vec2 e1, Vec2 i, f32 epsilon = 0.0f) {
		const auto dir = (e1 - e0).normalized();
		const auto along = dot(i, dir);
		return along > dot(e0, dir) - epsilon && along < dot(e1, dir) + epsilon;
	};

	switch (a.line.type) {
		using enum StereographicLine::Type;
	case CIRCLE: {
		const auto intersections = lineVsCircleIntersection(endpoint0, endpoint1 - endpoint0, a.line.circle);
		for (const auto& intersection : intersections) {

			if (isPointOnLineAlsoOnStereographicSegment(a.line, a.endpoints[0], a.endpoints[1], intersection) &&
				intersectionsOnSegment(endpoint0, endpoint1, intersection)) {
				return true;
			}

			/*if (intersectionsOnSegment(a.endpoints[0], a.endpoints[1], intersection) &&
				intersectionsOnSegment(endpoint0, endpoint1, intersection)) {
				return true;
			}*/
		}
		break;
	}

	case LINE: {
		return LineSegment(endpoint0, endpoint1).intersection(LineSegment(a.endpoints[0], a.endpoints[1])).has_value();
	}

	}
	return false;
}

#include <game/Constants.hpp>
#include <engine/Math/Color.hpp>

StaticList<SegmentEndpoints, 2> splitStereographicSegment(Vec2 endpoint0, Vec2 endpoint1) {
	StaticList<SegmentEndpoints, 2> result;

	const auto insideBoundary0 = endpoint0.length() < Constants::boundary.radius;
	const auto insideBoundary1 = endpoint1.length() < Constants::boundary.radius;

	if (insideBoundary0 && insideBoundary1) {
		result.add(SegmentEndpoints(endpoint0, endpoint1));
		return result;
	}

	const auto line = stereographicLine(endpoint0, endpoint1);
	auto intersections = stereographicSegmentVsCircleIntersection(line, endpoint0, endpoint1, Constants::boundary);
	for (auto& intersection : intersections) {
		intersection = intersection.normalized();
	}

	if (intersections.size() != 1) {
		// Don't know what to do here yet.
		result.add(SegmentEndpoints(endpoint0, endpoint1));
		/*Dbg::disk(endpoint0, 0.01f);
		Dbg::disk(endpoint1, 0.01f);
		Dbg::disk(intersections[0], 0.01f, Vec3(1.0f, 0.0f, 0.0f));
		Dbg::disk(intersections[1], 0.01f, Vec3(1.0f, 0.0f, 0.0f));*/
		return result;
	}

	//Dbg::disk(intersections[0], 0.03f, Color3::MAGENTA);

	const auto epsilon = 0.05f;
	auto addSegmentExtendedOutOfBoundary = [&](Vec2 pointInside, Vec2 pointOnBoundary) {
		//const auto tangent = tangentAtPointOnLine(line, pointOnBoundary, pointInside);
		/*Dbg::line(pointOnBoundary, pointOnBoundary + tangent.normalized() * 0.03f, 0.01f);
		pointOnBoundary = moveOnStereographicGeodesic(pointOnBoundary, tangent.angle(), epsilon);
		Dbg::disk(pointInside, 0.01f, Vec3(1.0f, 0.0f, 0.0f));
		Dbg::disk(pointOnBoundary, 0.01f);*/
		result.add(SegmentEndpoints(pointInside, pointOnBoundary));
	};
	
	// TODO: Code in game update relies on the order these are added. First the segment inside next the one wrapped around.
	if (insideBoundary0) {
		addSegmentExtendedOutOfBoundary(endpoint0, intersections[0]);
		addSegmentExtendedOutOfBoundary(antipodalPoint(endpoint1), -intersections[0]);
		/*result.add(SegmentEndpoints(endpoint0, intersections[0]));
		result.add(SegmentEndpoints(antipodalPoint(endpoint1), -intersections[0]));*/
	} else {
		addSegmentExtendedOutOfBoundary(endpoint1, intersections[0]);
		addSegmentExtendedOutOfBoundary(antipodalPoint(endpoint0), -intersections[0]);
		//result.add(SegmentEndpoints(endpoint1, intersections[0]));
		//result.add(SegmentEndpoints(antipodalPoint(endpoint0), -intersections[0]));
	}
	return result;
}

StaticList<Vec2, 2> splitStereographicCircle(Vec2 center, f32 radius) {
	StaticList<Vec2, 2> result;
	const auto circle = stereographicCircle(center, radius);

	result.add(center);
	if (circle.center.length() + radius < Constants::boundary.radius) {
		return result;
	}
	result.add(antipodalPoint(center));
	return result;
}

Vec2 tangentAtPointOnLine(const StereographicLine& line, Vec2 pointOnLine, Vec2 pointBeforePointOnLine) {
	const auto correctTangentDirection = (pointOnLine - pointBeforePointOnLine); 

	Vec2 tangent(0.0f);
	switch (line.type) {
		using enum StereographicLine::Type;
	case CIRCLE:
		tangent = (pointOnLine - line.circle.center).rotBy90deg();
		break;
	case LINE:
		tangent = line.lineNormal.rotBy90deg();
		break;
	}

	// This is done, because stereographicLine changes the center position when the line transitions from CIRCLE to LINE to CIRCLE. This also causes the normal (center - line.circle.center) to change direction, which is visible, for example when using portals.
	if (dot(tangent, correctTangentDirection) < 0.0f) {
		tangent = -tangent;
	}
	return tangent;
}

StereographicLine::StereographicLine(const StereographicLine& other)
	: type(other.type) {
	switch (type) {
		using enum Type;
	case LINE:
		lineNormal = other.lineNormal;
		break;

	case CIRCLE:
		circle = other.circle;
		break;
	}
}

StereographicLine::StereographicLine(Circle circle)
	: type(Type::CIRCLE)
	, circle(circle) {}

StereographicLine::StereographicLine(Vec2 lineNormal) 
	: type(Type::LINE)
	, lineNormal(lineNormal) {}

void StereographicLine::operator=(const StereographicLine& other) {
	type = other.type;
	switch (type) {
		using enum Type;
	case LINE:
		lineNormal = other.lineNormal;
		break;
	case CIRCLE:
		circle = other.circle;
		break;
	}
}

StereographicSegment::StereographicSegment(Vec2 e0, Vec2 e1)
	: line(stereographicLine(e0, e1))
	, endpoints{ e0, e1 } {}

SegmentEndpoints::SegmentEndpoints(Vec2 e0, Vec2 e1) 
	: endpoints{ e0, e1 } {
}
