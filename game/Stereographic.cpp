#include "Stereographic.hpp"
#include <engine/Math/Constants.hpp>
#include <array>

// The 3d coordinate system is the set of points satisfying x^2 + y^2 + z^2 = 1, z <= 0, hemisphere.
// The 2d coordinate system is the set of points satisfying x^2 + y^2 <= 1, circle

// https://en.wikipedia.org/wiki/Stereographic_projection#First_formulation
Vec2 toStereographic(Vec3 p) {
	const auto d = 1.0f - p.z;
	return Vec2(p.x / d, p.y / d);
}

Vec3 fromStereographic(Vec2 p) {
	const auto d = p.x * p.x + p.y * p.y + 1.0f;
	return Vec3(2.0f * p.x / d, 2.0f * p.y / d, -1.0 + p.x * p.x + p.y * p.y);
}

Vec2 antipodalPoint(Vec2 p) {
	const auto onSphere = fromStereographic(p);
	return toStereographic(-onSphere);
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
	const auto x0 = 0.5 * m12 / m11;
	const auto y0 = -0.5 * m13 / m11;
	const auto r0 = sqrt(pow((x1 - x0), 2.0f) + pow((y1 - y0), 2.0f));
	return Circle(Vec2(x0, y0), r0);
}

Circle stereographicLine(Vec2 p0, Vec2 p1) {
	// A great circle always passes through the antipodal point.
	// Maybe doing the computation in S_2 would be more stable.
	Vec2 antipodal = antipodalPoint(p0);
	return circleThroughPoints(p0, p1, antipodal);
}

f32 angleToRangeZeroTau(f32 a) {
	a -= floor(a / TAU<f32>) * TAU<f32>;
	return a;
}

// https://stackoverflow.com/questions/55816902/finding-the-intersection-of-two-circles
std::optional<std::array<Vec2, 2>> circleCircleIntersection(const Circle& c0, const Circle& c1) {
	const auto d = distance(c0.center, c1.center);
	if (d > c0.radius + c1.radius) {
		return std::nullopt;
	}

	if (d < std::abs(c0.radius - c1.radius)) {
		return std::nullopt;
	}
	
	if (d == 0.0f) {
		return std::nullopt;
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

	return std::array<Vec2, 2>{ Vec2(x3, y3), Vec2(x4, y4) };
}
