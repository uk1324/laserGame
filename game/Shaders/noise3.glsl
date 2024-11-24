float mod289(float x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 mod289(vec4 x){return x - floor(x * (1.0 / 289.0)) * 289.0;}
vec4 perm(vec4 x){return mod289(((x * 34.0) + 1.0) * x);}

float perlin(vec3 p){
    vec3 a = floor(p);
    vec3 d = p - a;
    d = d * d * (3.0 - 2.0 * d);

    vec4 b = a.xxyy + vec4(0.0, 1.0, 0.0, 1.0);
    vec4 k1 = perm(b.xyxy);
    vec4 k2 = perm(k1.xyxy + b.zzww);

    vec4 c = k2 + a.zzzz;
    vec4 k3 = perm(c);
    vec4 k4 = perm(c + 1.0);

    vec4 o1 = fract(k3 * (1.0 / 41.0));
    vec4 o2 = fract(k4 * (1.0 / 41.0));

    vec4 o3 = o2 * d.z + o1 * (1.0 - d.z);
    vec2 o4 = o3.yw * d.x + o3.xz * (1.0 - d.x);

    return o4.y * d.y + o4.x * (1.0 - d.y);
}

float perlin01(vec3 p) {
	return (perlin(p) + 1.0) * 0.5;
}

float octave01(vec3 p, int octaves, float amplitudeScale, float frequencyScale) {
    float amplitude = 1.0;
	float value = 0.0;
    for (int i = 0; i < octaves; i++) {
        value += amplitude * perlin01(p);
        p *= frequencyScale;
        amplitude *= amplitudeScale;
    }

	// sum of geometric series
	// doesn't work if the amplitude scale = 1.0
	// amplitude is equal to amplitudeScale^octaves
	value /= (1.0 - amplitude) / (1.0 - amplitudeScale);

	return value;
}

float octave01(vec3 p, int octaves) {
	return octave01(p, octaves, 0.5, 2.0);
}

float octave01(vec3 p, int octaves, float amplitudeScale) {
	return octave01(p, octaves, amplitudeScale, 2.0);
}