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
	return vec3(2.0 * p.x, 2.0 * p.y, -1.0 + p.x * p.x + p.y * p.y) / d;
}

float sphericalDistance(vec3 a, vec3 b) {
    return acos(clamp(dot(a, b), -1.0, 1.0));
    //return acos(dot(normalize(a), normalize(b)));
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
    return sphericalDistance(closest, p);
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
// When checking if along was in the right range with the spherical points it would also calculate the anitipodal ones.
// Technically couting them would be more correct, but would also require adding the rectange to the anitpodal line in the rendering.

//    vec2 as = toStereographic(a);
//    vec2 bs = toStereographic(b);
//    vec2 t = bs - as;
//    float along = dot(t, toStereographic(p));
//
//    if (along < dot(as, t)) {
//        return sphericalDistance(a, p);
//    }
//
//    if (along > dot(bs, t)) {
//        return sphericalDistance(b, p);
//    }

    
    if (along < dot(a, t)) {
        return sphericalDistance(a, p);
    }

    if (along > dot(b, t)) {
        return sphericalDistance(b, p);
    }

    return distanceFromGeodesicLine(a, b, p);
}

float distanceFromGeodesicSegment(vec2 a, vec2 b, vec2 p) {
    return distanceFromGeodesicSegment(
        fromStereographic(a), 
        fromStereographic(b), 
        fromStereographic(p)
    );
}

void main() {
    // When the outer hemisphere becomes the shorter segment the endpoints of lines wrap around which casues small parts of the wrapped around endpoint to be renderer (I think not sure). This happens for example if one endpoint is one the boundary.
    if (dot(worldPosition, worldPosition) > 1.0)  {
        return;
    }

	fragColor = vec4(worldPosition, 0.0, 1.0);
    float d = distanceFromGeodesicSegment(endpoint0, endpoint1, worldPosition);
    float halfWidth = 0.011;

    float smoothing = fwidth(d) * 2.0;
	d -= halfWidth  - smoothing;
	d = smoothstep(smoothing, 0.0, d);
    vec3 col = vec3(d);
    fragColor = vec4(col, d);
}
