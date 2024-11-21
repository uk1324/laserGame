#version 430 core

in vec2 worldPosition; 

in vec2 endpoint0; 
in vec2 endpoint1; 
out vec4 fragColor;

/*generated end*/

vec2 toStereographic(vec3 p) {
	float d = 1.0 - p.z;
	return vec2(p.x / d, p.y / d);
}

vec3 fromStereographic(vec2 p) {
	float d = p.x * p.x + p.y * p.y + 1.0;
	return vec3(2.0f * p.x, 2.0f * p.y, -1.0f + p.x * p.x + p.y * p.y) / d;
}

float sphericalDistance(vec3 a, vec3 b) {
    return acos(dot(a, b));
    //return acos(dot(normalize(a), normalize(b)));
}

float ellipticDistance(vec3 a, vec3 b) {
    return min(sphericalDistance(a, b), sphericalDistance(a, -b));
}

struct Plane {
    vec3 n;
    float d;
};
// v dot n = d

Plane planeThoughPoints(vec3 a, vec3 b, vec3 c) {
    vec3 n = normalize(cross(c - a, b - a));
    float d = dot(n, a);
    Plane p;
    p.d = d;
    p.n = n;
    return p;
}

Plane greatCirclePlane(vec3 a, vec3 b) {
    return planeThoughPoints(a, b, -a);
}

float signedDistanceFromPlane(Plane p, vec3 v) {
    return p.d - dot(v, p.n);
}

vec3 closestPointOnPlane(Plane p, vec3 v) {
    return v + p.n * (p.d - dot(v, p.n));
}

float distanceFromGeodesicLine(vec3 a, vec3 b, vec3 p) {
    Plane plane = greatCirclePlane(a, b);
    vec3 closest = normalize(closestPointOnPlane(plane, p));
    return ellipticDistance(closest, p);
}

float distanceFromGeodesicLine(vec2 a, vec2 b, vec2 p) {
    return distanceFromGeodesicLine(
        fromStereographic(a), 
        fromStereographic(b), 
        fromStereographic(p)
    );
}

float distanceFromGeodesicSegment(vec3 a, vec3 b, vec3 p) {
    vec3 t = b - a;
    float along = dot(p, t);
    if (along > dot(a, t) && along < dot(b, t)) {
        return distanceFromGeodesicLine(a, b, p);
    }
    return min(ellipticDistance(a, p), ellipticDistance(b, p));
}

float distanceFromGeodesicSegment(vec2 a, vec2 b, vec2 p) {
    return distanceFromGeodesicSegment(
        fromStereographic(a), 
        fromStereographic(b), 
        fromStereographic(p)
    );
}

void main() {
	fragColor = vec4(worldPosition, 0.0, 1.0);
	//fragColor = vec4(vec3(smoothstep(0.5, 0.51, length(worldPosition))), 1.0);
    float d = distanceFromGeodesicSegment(endpoint0, endpoint1, worldPosition);
    float halfWidth = 0.02;
    float smoothing = 0.001;
    d = smoothstep(halfWidth, halfWidth + smoothing, d);
    vec3 col = vec3(d);
    fragColor = vec4(col, 1.0);
	//fragColor = vec4(vec3(worldPosition.y), 1.0);
}
