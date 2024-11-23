#include "Animation.hpp"
#include <game/Constants.hpp>

void updateConstantSpeedT(f32& t, f32 timeToFinish, bool active) {
	const auto speed = 1.0f / (timeToFinish);
	t += speed * Constants::dt * (active ? 1.0f : -1.0f);
	t = std::clamp(t, 0.0f, 1.0f);
}