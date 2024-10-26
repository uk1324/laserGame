#include "Stereographic.hpp"
#include <engine/Math/Constants.hpp>
#include <array>
#include <engine/Math/Line.hpp>

// The 3d coordinate system is the set of points satisfying x^2 + y^2 + z^2 = 1, z <= 0, hemisphere.
// The 2d coordinate system is the set of points satisfying x^2 + y^2 <= 1, circle

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

Circle::Circle(Vec2 center, f32 radius)
	: center(center)
	, radius(radius) {}


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

Circle stereographicLineOld(Vec2 p0, Vec2 p1) {
	// A great circle always passes through the antipodal point.
	// Maybe doing the computation in S_2 would be more stable.
	Vec2 antipodal = antipodalPoint(p0);
	return circleThroughPoints(p0, p1, antipodal);
}

StereographicLine stereographicLine(Vec2 p0, Vec2 p1) {
	const auto line = Line(p0, p1);
	/*const auto goesThroughOrigin = distance(line, Vec2(0.0f)) < 0.0001f;*/
	const auto goesThroughOrigin = distance(line, Vec2(0.0f)) < 0.001f;
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

f32 circularArcDistance(Vec2 p, Circle circle, f32 startAngle, f32 endAngle) {
	const auto pAngle = (p - circle.center).angle();

	if (pAngle >= startAngle && pAngle <= endAngle) {
		return abs(distance(p, circle.center) - circle.radius);
	}
	return std::min(
		distance(Vec2::fromPolar(startAngle, circle.radius), p),
		distance(Vec2::fromPolar(endAngle, circle.radius), p)
	);
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
