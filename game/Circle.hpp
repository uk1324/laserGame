#pragma once
#include <engine/Math/Vec2.hpp>

struct Circle {
	Circle(Vec2 center, f32 radius);

	Vec2 center;
	f32 radius;
};