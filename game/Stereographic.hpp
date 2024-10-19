#pragma once
#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Math/Line.hpp>
#include <StaticList.hpp>
#include <variant>

struct Circle {
	Circle(Vec2 center, f32 radius);

	Vec2 center;
	f32 radius;
};

struct StereographicLine {
	enum class Type {
		LINE, CIRCLE
	};

	Type type;

	StereographicLine(const StereographicLine& other);
	StereographicLine(Circle circle);
	StereographicLine(Vec2 lineNormal);

	void operator=(const StereographicLine& other);

	union {
		Circle circle;
		Vec2 lineNormal;
	};
};

// The 3d coordinate system is the set of points satisfying x^2 + y^2 + z^2 = 1, z <= 0, hemisphere.
// The 2d coordinate system is the set of points satisfying x^2 + y^2 <= 1, circle

Vec2 toStereographic(Vec3 p);
Vec3 fromStereographic(Vec2 p);

Vec2 antipodalPoint(Vec2 p);

Circle circleThroughPoints(Vec2 p0, Vec2 p1, Vec2 p2);
std::optional<Circle> circleThroughPointsWithNormalAngle(Vec2 p0, f32 angle0, Vec2 p1);
// The center would need to lie on the midpoint of the segment p0 p1
Circle circleThroughPointsWithCenter(Vec2 center, Vec2 p0, Vec2 p1);

Circle stereographicLineOld(Vec2 p0, Vec2 p1);
StereographicLine stereographicLine(Vec2 p0, Vec2 p1);

f32 angleToRangeZeroTau(f32 a);

f32 circularArcDistance(Vec2 p, Circle circle, f32 startAngle, f32 endAngle);

StaticList<Vec2, 2> circleVsCircleIntersection(const Circle& a, const Circle& b);
StaticList<Vec2, 2> lineVsCircleIntersection(Vec2 linePoint, Vec2 lineDirection, const Circle& circle);
StaticList<Vec2, 2> stereographicLineVsCircleIntersection(const StereographicLine& l, const Circle& c);
StaticList<Vec2, 2> stereographicLineVsStereographicLineIntersection(const StereographicLine& a, const StereographicLine& b);