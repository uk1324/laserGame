#define HASHSCALE 0.1031

float hash(float p)
{
	vec3 p3  = fract(vec3(p) * HASHSCALE);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

float fade(float t) { return t*t*t*(t*(6.*t-15.)+10.); }

float grad(float hash, float p)
{
    int i = int(1e4*hash);
	return (i & 1) == 0 ? p : -p;
}

float perlin(float p) {
	float pi = floor(p), pf = p - pi, w = fade(pf);
    return mix(grad(hash(pi), pf), grad(hash(pi + 1.0), pf - 1.0), w) * 2.0;
}

float perlin01(float p)
{
	return (perlin(p) + 1) * 0.5;
}

float octave01(float p, int octaves) {
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
