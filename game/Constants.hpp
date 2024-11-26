#pragma once

#include <game/Stereographic.hpp>

struct Constants {
	static constexpr auto wallWidth = 0.01f;
	static constexpr auto endpointGrabPointRadius = 0.04f;
	static const Circle boundary;
	static constexpr auto dt = 1.0f / 60.0f;
	static constexpr auto additionalTextSpacing = 0.005f;
};