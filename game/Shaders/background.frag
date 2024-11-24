#version 430 core

in vec2 worldPosition; 

in mat3x2 clipToWorld; 
in vec2 cameraPosition; 
in float time; 
out vec4 fragColor;

/*generated end*/

float t = time / 20.0;

#include "noise2d.glsl"

// x - distance to closest
// y - closest cell index
// z - absolute difference of closest and second closest cell distance.
vec3 voronoi(in vec2 x)
{
    vec2 n = floor(x);
    vec2 f = fract(x);

	vec3 closest = vec3( 8.0 );
    float secondClosest = 1000.0;
    for (int j = -1; j <= 1; j++) {
        for (int i = -1; i <= 1; i++) {
            vec2 g = vec2(float(i), float(j));
            vec2 o = hash(n + g); 
	        vec2 r = g - f + (0.5 + 0.5 * sin(t + 6.2831 * o));
            //float d = length(r);
            //float d = abs(r.x) + abs(r.y);
            //float d = abs(r.x) + abs(r.y) + length(r);
            float d = length(r);
            //float d = max(abs(r.x), abs(r.y)) + length(r);
            if (d < closest.x) {
                secondClosest = closest.x;
                closest = vec3(d, o);
            } else if (d < secondClosest) {
                secondClosest = d;
            }
        }
    }
    
    return vec3(closest.x, closest.y + closest.z, abs(closest.x - secondClosest));
}


vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 p = worldPosition * 0.8;

    int octaves = 2;
    p += vec2(octave01(p + vec2(123), octaves), octave01(p, octaves));

    vec3 c = voronoi(p);

    vec3 col;
    float d = c.z;

    d = 1.0 / d / 50.0;

	col = vec3(d);

    float x = c.y;

   col *= hsv2rgb(vec3(c.y, 1.0, 1.0));

    //col = clamp(col, 0.0, 2.0);
    col *= 0.3;
    //col *= 0.1;
    //col *= 1.0 - c.x;
    //col *= 0.1;
    //col *= clamp(1.0 - 0.9 * c.x * c.x, 0.0, 1.0);
    //col *= vec3(c.x);
    //col -= (1.0-smoothstep( 0.08, 0.09, c.x));
    fragColor = vec4( col, 1.0 );

   // col = vec3(d);

	fragColor = vec4(col, 1.0);
}
