#version 430 core

in vec2 worldPosition; 

in mat3x2 clipToWorld; 
in vec2 cameraPosition; 
in float time; 
out vec4 fragColor;

/*generated end*/

float t = time / 20.0;

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p) * 18.5453);
}

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

float perlin01(vec2 p) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

    vec2  i = floor(p + (p.x + p.y) * K1);
    vec2  a = p - i + (i.x + i.y) * K2;
    float m = step(a.y, a.x);
    vec2  o = vec2(m, 1.0 - m);
    vec2  b = a - o + K2;
    vec2  c = a - 1.0 + 2.0 * K2;
    vec3  h = max(0.5 - vec3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
    vec3  n = h * h * h * h * vec3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
    return (dot(n, vec3(70.0)) + 1.0) * 0.5;
}

float octave01(vec2 p, int octaves) {
    float amplitude = .5;
    float frequency = 0.;
    float value = 0.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * perlin01(p);
        p *= 2.;
        amplitude *= .5;
    }
    return value;
}

void main() {
    //vec3 col;

    //vec2 p = worldPosition * 0.7;
    vec2 p = worldPosition * 0.8;
    int octaves = 2;
    p += vec2(octave01(p + vec2(123), octaves), octave01(p, octaves));
    vec3 c = voronoi(p);

    // colorize
    vec3 col;
    //col = 0.5 + 0.5*cos( c.y*6.2831 + vec3(0.0,1.0,2.0) );	
    //col = vec3(clamp(1.0 - 0.4c.x*c.x,0.0,1.0));
    float d = c.z;
    //d *= octave01(p, 2);

    //d = 1.0 - d;
    d = 1.0 / d / 50.0;

    //d = 1.0;
	col = vec3(d);

    //col *= exp(4.0 * g - 1.0);
    float x = c.y;

    //col *= colormap(abs(x)).xyz;
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