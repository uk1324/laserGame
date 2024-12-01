#pragma once
#include <engine/Math/Vec3.hpp>
#include <engine/Math/Line.hpp>
#include <engine/Math/Quat.hpp>
#include <StaticList.hpp>
#include <variant>
#include <engine/Math/Angles.hpp>
#include <game/Circle.hpp>

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

struct SegmentEndpoints {
	SegmentEndpoints(Vec2 e0, Vec2 e1);

	Vec2 endpoints[2];
};

struct StereographicSegment {
	StereographicSegment(Vec2 e0, Vec2 e1);

	StereographicLine line;
	Vec2 endpoints[2];
};

struct RaycastHit {
	Vec2 pos;
	f32 t;
};
std::optional<RaycastHit> circleRaycast(Vec2 rayStart, Vec2 rayEnd, const Circle& circle);

// The 3d coordinate system is the set of points satisfying x^2 + y^2 + z^2 = 1, z <= 0, hemisphere.
// The 2d coordinate system is the set of points satisfying x^2 + y^2 <= 1, circle

Vec2 toStereographic(Vec3 p);
Vec3 fromStereographic(Vec2 p);

// The angle is such that when stereographically projected it equal the plane angle.
Quat movementOnSphericalGeodesic(Vec3 pos, f32 angle, f32 distance);
Vec3 moveOnSphericalGeodesic(Vec3 pos, f32 angle, f32 distance);

Vec2 moveOnStereographicGeodesic(Vec2 pos, f32 angle, f32 distance);

Vec2 antipodalPoint(Vec2 p);

Circle circleThroughPoints(Vec2 p0, Vec2 p1, Vec2 p2);
std::optional<Circle> circleThroughPointsWithNormalAngle(Vec2 p0, f32 angle0, Vec2 p1);
// The center would need to lie on the midpoint of the segment p0 p1
Circle circleThroughPointsWithCenter(Vec2 center, Vec2 p0, Vec2 p1);

StereographicLine stereographicLineThroughPointWithTangent(Vec2 p, f32 tangentAngle, f32 translation = 0.1f);

Circle stereographicLineOld(Vec2 p0, Vec2 p1);
StereographicLine stereographicLine(Vec2 p0, Vec2 p1);

bool isPointOnLineAlsoOnStereographicSegment(const StereographicLine& line, Vec2 endpoint0, Vec2 endpoint1, Vec2 pointThatLiesOnLine, f32 epsilon = 0.0f);

f32 circularArcDistance(Vec2 p, Circle circle, AngleRange angleRange);

Circle stereographicCircle(Vec2 center, f32 radius);

AngleRange angleRangeBetweenPointsOnCircle(Vec2 circleCenter, Vec2 pointOnCircle0, Vec2 pointOnCircle1);

Vec3 sphericalGeodesicSegmentMidpoint(Vec3 e0, Vec3 e1);
Vec2 stereographicSegmentMidpoint(Vec2 e0, Vec2 e1);

f32 sphericalDistance(Vec3 a, Vec3 b);
f32 stereographicDistance(Vec2 a, Vec2 b);

// p is assued to lie on the line.
Vec2 stereographicLineNormalAt(const StereographicLine& line, Vec2 p);

f32 eucledianDistanceToStereographicSegment(const StereographicLine& line, Vec2 eucledianPoint);
f32 eucledianDistanceToStereographicSegment(Vec2 e0, Vec2 e1, Vec2 eucledianPoint);

StaticList<Vec2, 2> circleVsCircleIntersection(const Circle& a, const Circle& b);
StaticList<Vec2, 2> lineVsCircleIntersection(Vec2 linePoint, Vec2 lineDirection, const Circle& circle);
StaticList<Vec2, 2> stereographicLineVsCircleIntersection(const StereographicLine& l, const Circle& c);
StaticList<Vec2, 2> stereographicLineVsStereographicLineIntersection(const StereographicLine& a, const StereographicLine& b);
StaticList<Vec2, 2> stereographicSegmentVsCircleIntersection(const StereographicLine& line, Vec2 lineEndpoint0, Vec2 lineEndpoint1, const Circle& circle);
StaticList<Vec2, 2> stereographicSegmentVsStereographicSegmentIntersection(const StereographicSegment& a, const StereographicSegment& b);
bool stereographicSegmentVsSegmentCollision(const StereographicSegment& a, Vec2 endpoint0, Vec2 endpoint1);


StaticList<SegmentEndpoints, 2> splitStereographicSegment(Vec2 endpoint0, Vec2 endpoint1);
StaticList<Vec2, 2> splitStereographicCircle(Vec2 center, f32 radius);

Vec2 tangentAtPointOnLine(const StereographicLine& line, Vec2 pointOnLine, Vec2 pointBeforePointOnLine);