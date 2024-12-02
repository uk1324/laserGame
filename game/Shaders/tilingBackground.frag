#version 430 core

uniform float time; 
uniform bool icosahedralIfTrueDodecahedralIfFalse; 

in vec2 position; 
out vec4 fragColor;

/*generated end*/

#include "stereographic.glsl"
#include "noise1.glsl"

#define TAU 6.28318530718

vec3 qtransform(vec4 q, vec3 v){ 
	return v + 2.0*cross(cross(v, q.xyz ) + q.w*v, q.xyz);
} 

mat3 rotationX(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, s),
        vec3(0, -s, c)
    );
}

mat3 rotationY(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat3(
        vec3(c, 0, -s),
        vec3(0, 1, 0),
        vec3(s, 0, c)
    );
}

mat3 rotationZ(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat3(
        vec3(c, s, 0),
        vec3(-s, c, 0),
        vec3(0, 0, 1)
    );
}

mat3 rotationXyz(float x, float y, float z) {
    return rotationZ(z) * rotationY(y) * rotationX(x);
}

vec2 antipodal(vec2 v) {
    return toStereographic(-fromStereographic(v));
}

void main() {
    vec2 p = position;    
    p *= 4;

    vec3 col;
        
    vec3 sp = fromStereographic(p);
    {
        float t = time / 40.0;
        float u = perlin01(t);
        float v = perlin01(t + 1321.0);
        float w = perlin01(t + 1521.21);
//        float u = 0.0;
//        float v = TAU / 4.0;
//        float w = 0.0;
        vec4 q = vec4(sqrt(1-u) * sin(TAU * v), sqrt(1-u) * cos(TAU * v), sqrt(u) * sin(TAU * w), sqrt(u) * cos(TAU * w));
        sp *= rotationXyz(u * TAU, v * TAU, w * TAU);
    }

    #define M(i0, i1) d = min(d, distanceFromGeodesicSegment(v[i0], v[i1], stp)); d = min(d, distanceFromGeodesicSegment(-v[i0], -v[i1], stp));

    vec2 stp = toStereographic(sp);
    float d = 1000.0;
    if (icosahedralIfTrueDodecahedralIfFalse) {
        const int n = 8;
        vec3 vertices[n] = vec3[n](vec3(0.0, 1.0, 1.618033988749895), vec3(0.0, -1.0, 1.618033988749895), vec3(1.0, 1.618033988749895, 0.0), vec3(-1.0, 1.618033988749895, 0.0), vec3(1.0, -1.618033988749895, 0.0), vec3(-1.0, -1.618033988749895, 0.0), vec3(1.618033988749895, 0.0, 1.0), vec3(-1.618033988749895, 0.0, 1.0));
        vec2 v[n];
        for (int i = 0; i < n; i++) {
            vertices[i] = normalize(vertices[i]);
            v[i] = toStereographic(vertices[i]);
        }
        M(0, 1);
        M(5, 1);
        M(7, 1);
        M(4, 1);
        M(6, 1);
        M(0, 7);
        M(0, 3);
        M(0, 2);
        M(0, 6);
        d = min(d, distanceFromGeodesicSegment(v[6], vec2(1.0, 0.0), stp));
        d = min(d, distanceFromGeodesicSegment(v[7], vec2(-1.0, 0.0), stp));
        M(5, 4);
        M(5, 7);
        M(7, 3);
        M(2, 6);
        M(6, 4);
    } else {
        const int n = 10;
        const float phi = (1.0 + sqrt(5.0)) / 2.0;
        const float iphi = 1.0 / phi;
        vec3 vertices[n];
        // TODO: This is just copy pasted from projective polyhedron. Could implement a more generic system.

      	vertices[0] = vec3(1.0f, 1.0f, -1.0f);
	    vertices[1] = vec3(-1.0f, 1.0f, -1.0f);
	    vertices[2] = vec3(1.0f, -1.0f, -1.0f);
	    vertices[3] = vec3(-1.0f, -1.0f, -1.0f);
	    vertices[4] = vec3(0.0f, phi, -iphi); 
	    vertices[5] = vec3(0.0f, -phi, -iphi);
	    vertices[6] = vec3(iphi, 0.0f, -phi); 
	    vertices[7] = vec3(-iphi, 0.0f, -phi);
	    vertices[8] = vec3(phi, iphi, 0.0f); 
	    vertices[9] = vec3(-phi, iphi, 0.0f);

        vec2 v[n];
        for (int i = 0; i < n; i++) {
            vertices[i] = normalize(vertices[i]);
            v[i] = toStereographic(vertices[i]);
        }

        M(1, 7);
	    M(0, 6);
	    M(1, 4);
	    M(0, 4);
	    M(3, 7);
	    M(2, 6);
	    M(3, 5);
	    M(2, 5);
	    M(7, 6);
	    M(1, 9);
	    M(0, 8);
        d = min(d, distanceFromGeodesicSegment(v[3], antipodal(v[8]), stp));
        d = min(d, distanceFromGeodesicSegment(v[5], antipodal(v[4]), stp));
        d = min(d, distanceFromGeodesicSegment(v[9], antipodal(v[8]), stp));
        d = min(d, distanceFromGeodesicSegment(v[2], antipodal(v[9]), stp));
    }
    
    float width = 0.04;
    d *= 0.6;
    float smoothing = 0.005;
    //d = smoothstep(width + smoothing, width, d);
    col = vec3(d);

    fragColor = vec4(col, 1.0);
}
