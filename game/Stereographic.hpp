#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>

struct Circle {
	Circle(Vec2 center, f32 radius);

	Vec2 center;
	f32 radius;
};

// The 3d coordinate system is the set of points satisfying x^2 + y^2 + z^2 = 1, z <= 0, hemisphere.
// The 2d coordinate system is the set of points satisfying x^2 + y^2 <= 1, circle

Vec2 toStereographic(Vec3 p);
Vec3 fromStereographic(Vec2 p);

Vec2 antipodalPoint(Vec2 p);

Circle circleThroughPoints(Vec2 p0, Vec2 p1, Vec2 p2);

Circle stereographicLine(Vec2 p0, Vec2 p1);

f32 angleToRangeZeroTau(f32 a);

std::optional<std::array<Vec2, 2>> circleCircleIntersection(const Circle& a, const Circle& b);
