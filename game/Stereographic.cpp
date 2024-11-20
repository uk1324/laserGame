#include "Stereographic.hpp"
#include <engine/Math/Constants.hpp>
#include <array>
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
	return Vec3(2.0f * p.x, 2.0f * p.y, -1.0f + p.x * p.x + p.y * p.y) / d;
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

StereographicLine stereographicLineThroughPointWithTangent(Vec2 p, f32 tangentAngle, f32 translation) {
	//if (p == Vec2(0.0f)) {
	//	return StereographicLine(Vec2::oriented(tangentAngle + PI<f32> / 2.0f));
	//}
	const auto pointAhead = moveOnStereographicGeodesic(p, tangentAngle, translation);
	return stereographicLine(p, pointAhead);
}

Circle stereographicLineOld(Vec2 p0, Vec2 p1) {
	// A great circle always passes through the antipodal point.
	// Maybe doing the computation in S_2 would be more stable.
	Vec2 antipodal = antipodalPoint(p0);
	return circleThroughPoints(p0, p1, antipodal);
}

StereographicLine stereographicLine(Vec2 p0, Vec2 p1) {
	const auto line = Line(p0, p1);
	/*const auto goesThroughOrigin = distance(line, Vec2(0.0f)) < 0.0001f;*/
	/*const auto goesThroughOrigin = distance(line, Vec2(0.0f)) < 0.001f;*/
	const auto goesThroughOrigin = distance(line, Vec2(0.0f)) < 0.005f;
	if (goesThroughOrigin) {
		return StereographicLine(line.n);
	}
	return stereographicLineOld(p0, p1);
}

f32 angleToRangeZeroTau(f32 a) {
	a -= floor(a / TAU<f32>) * TAU<f32>;
	return a;
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

#include <gfx2d/DbgGfx2d.hpp>

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
	//const auto epsilon = 0.001f;
	const auto lineDirection = (lineEndpoint1 - lineEndpoint0).normalized();
	const auto dAlong0 = dot(lineDirection, lineEndpoint0);
	const auto dAlong1 = dot(lineDirection, lineEndpoint1);

	StaticList<Vec2, 2> result;
	for (const auto& intersection : intersections) {
		const auto intersectionDAlong = dot(lineDirection, intersection);
		// Using the regular segment check works, because the stereographic segment is convex.
		if (intersectionDAlong >= dAlong0 && intersectionDAlong <= dAlong1) {
			result.add(intersection);
		}
	}
	return result;
}

bool intersectionsOnSegment(Vec2 e0, Vec2 e1, Vec2 i) {
	const auto dir = e1 - e0;
	const auto along = dot(i, dir);
	return along > dot(e0, dir) && along < dot(e1, dir);
}

StaticList<Vec2, 2> stereographicSegmentVsStereographicSegmentIntersection(const StereographicSegment& a, const StereographicSegment& b) {
	const auto intersections = stereographicLineVsStereographicLineIntersection(a.line, b.line);

	StaticList<Vec2, 2> result;
	for (const auto& intersection : intersections) {
		if (intersectionsOnSegment(a.endpoints[0], a.endpoints[1], intersection) &&
			intersectionsOnSegment(b.endpoints[0], b.endpoints[1], intersection)) {
			result.add(intersection);
		}
	}
	return result;
}

bool stereographicSegmentVsSegmentCollision(const StereographicSegment& a, Vec2 endpoint0, Vec2 endpoint1) {
	switch (a.line.type) {
		using enum StereographicLine::Type;
	case CIRCLE: {
		const auto intersections = lineVsCircleIntersection(endpoint0, endpoint1 - endpoint0, a.line.circle);
		for (const auto& intersection : intersections) {
			if (intersectionsOnSegment(a.endpoints[0], a.endpoints[1], intersection) &&
				intersectionsOnSegment(endpoint0, endpoint1, intersection)) {
				return true;
			}
		}
		break;
	}

	case LINE: {
		return LineSegment(endpoint0, endpoint1).intersection(LineSegment(a.endpoints[0], a.endpoints[1])).has_value();
	}

	}
	return false;
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

bool AngleRange::isInRange(f32 angle) {
	/*
	min = a0 + m tau
	max = a1 + m tau
	min and max both have the same multiple of tau added because they are a range. That is max is measured with respect to min.
	angle = a + n tau

	subtracting floor(p / TAU<f32>) * TAU<f32> brings the angles into the same range
	*/
	angle -= floor(angle / TAU<f32>) * TAU<f32>;
	{
		const auto offset = floor(min / TAU<f32>) * TAU<f32>;
		const auto mn = min - offset;
		const auto mx = max - offset;
		if (angle >= mn && angle <= mx) {
			return true;
		}
	}
	{
		const auto offset = floor(max / TAU<f32>) * TAU<f32>;
		const auto mn = min - offset;
		const auto mx = max - offset;
		return angle >= mn && angle <= mx;
	}
}

StereographicSegment::StereographicSegment(Vec2 e0, Vec2 e1)
	: line(stereographicLine(e0, e1))
	, endpoints{ e0, e1 } {}
