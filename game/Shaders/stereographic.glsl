vec2 toStereographic(vec3 p) {
	float d = 1.0 - p.z;
	return vec2(p.x / d, p.y / d);
}

vec3 fromStereographic(vec2 p) {
	float d = p.x * p.x + p.y * p.y + 1.0;
	return vec3(2.0 * p.x, 2.0 * p.y, -1.0 + p.x * p.x + p.y * p.y) / d;
}

float sphericalDistance(vec3 a, vec3 b) {
    return acos(clamp(dot(a, b), -1.0, 1.0));
    //return acos(dot(normalize(a), normalize(b)));
}