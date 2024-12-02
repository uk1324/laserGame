#version 430 core

uniform mat3 transformation; 
uniform float time; 

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

uniform vec2 vertices[20];
uniform int indices[40];
uniform int indexCount;

void main() {
    vec2 p = position;    
    p *= 25.0;
//    fragColor = vec4(vec3(length(p)), 1.0);
//    return;

    vec3 col;
        
    vec3 sp = fromStereographic(p);
    {
        float t = time / 40.0;
        //t = 0.34;
        float u = perlin01(t);
        float v = perlin01(t + 1321.0);
        float w = perlin01(t + 1521.21);
        vec4 q = vec4(sqrt(1-u) * sin(TAU * v), sqrt(1-u) * cos(TAU * v), sqrt(u) * sin(TAU * w), sqrt(u) * cos(TAU * w));
        //sp = qtransform(q, sp);
        //sp *= rotationXyz(u * TAU, v * TAU, w * TAU);
        //sp *= transformation;
    }
    vec2 stp = toStereographic(sp);
    //stp = p;
    
    float d = 1000.0;

    for (int i = 0; i < indexCount; i += 2) {
        vec2 v0 = vertices[indices[i]];
        vec2 v1 = vertices[indices[i + 1]];
        d = min(d, distanceFromGeodesicSegment(v0, v1, stp));
        //d = min(d, distanceFromGeodesicSegment(toStereographic(-fromStereographic(v0)), toStereographic(-fromStereographic(v1)), stp));
    }
//    int n = 6;
//    for (int i = 0; i < n; i++) {
//        float t0 = float(i) / float(n);
//        float t1 = float(i + 1) / float(n);
//        float a0 = TAU * t0;
//        float a1 = TAU * t1;
//        vec2 v0 = vec2(cos(a0), sin(a0));
//        vec2 v1 = vec2(cos(a1), sin(a1));
//        //d = min(d, distanceFromGeodesicSegment(vec2(0.0), v0, stp));
//        d = min(d, distanceFromGeodesicSegment(v0, v1, stp));
//        
//        float am = (a0 + a1) / 2.0;
//        vec2 vm = vec2(cos(am), sin(am));
//        
//        //d = min(d, distanceFromGeodesicSegment(v0, vm, stp));
//        //d = min(d, distanceFromGeodesicSegment(v1, vm, stp));
//    }

    //col = mix(col, vec3(1.0), smoothstep(0.97, 0.99, length(p)));
    //col *= smoothstep(1.0, 0.99, length(p));


    float width = 0.04;
    float smoothing = 0.005;
    //d /= 2.0;
    d *= 0.6;
    d = smoothstep(width + smoothing, width, d);
    col = vec3(d);

    fragColor = vec4(col, 1.0);

    //float d = distanceFromGeodesicSegment(vec2(0.0), vec2(1.0, 0.0), stp);
//    float d = 1000.0;
//    
//    const int n = 8;
//    vec3 vertices[n] = vec3[n](vec3(0.0, 1.0, 1.618033988749895), vec3(0.0, -1.0, 1.618033988749895), vec3(1.0, 1.618033988749895, 0.0), vec3(-1.0, 1.618033988749895, 0.0), vec3(1.0, -1.618033988749895, 0.0), vec3(-1.0, -1.618033988749895, 0.0), vec3(1.618033988749895, 0.0, 1.0), vec3(-1.618033988749895, 0.0, 1.0));
//    vec2 v[n];
//    for (int i = 0; i < n; i++) {
//        vertices[i] = normalize(vertices[i]);
//        v[i] = toStereographic(vertices[i]);
//    }
//
//    #define M(i0, i1) d = min(d, distanceFromGeodesicSegment(v[i0], v[i1], stp)); d = min(d, distanceFromGeodesicSegment(-v[i0], -v[i1], stp));
//    M(0, 1);
//    M(5, 1);
//    M(7, 1);
//    M(4, 1);
//    M(6, 1);
//    
//    M(0, 7);
//    M(0, 3);
//    M(0, 2);
//    M(0, 6);
//    
//    // Had to split the line into 2 parts.
//    d = min(d, distanceFromGeodesicSegment(v[6], vec2(1.0, 0.0), stp));
//    d = min(d, distanceFromGeodesicSegment(v[7], vec2(-1.0, 0.0), stp));
//    
//    M(5, 4);
//    
//    M(5, 7);
//    M(7, 3);
//    
//    M(2, 6);
//    M(6, 4);
//    
//    M(7, 6);
//    
//    float width = 0.04;
//    float smoothing = 0.005;
//    //d /= 2.0;
//    d *= 0.6;
//    d = smoothstep(width + smoothing, width, d);
//    col = vec3(d);
//
//    fragColor = vec4(col, 1.0);
}
