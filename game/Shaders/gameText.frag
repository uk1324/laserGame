#version 430 core

uniform float time; 

in vec2 texturePosition; 
in vec2 atlasMin; 
in vec2 atlasMax; 

in vec3 color; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

float f(float x) {
	// Start with inverse falloff untill a0 then get to zero linearly at a1.
	// y = ax + b
	// 1/a0 = a * a0 + b
	// 0.0 = a a1 + b

	// b = -a a1

	// 1/a0 = a * a0  - a a1
	// 1/a0 = a (a0 - a1)
	// a = 1/(a0(a0 - a1))
	float falloffTime = 1.0;
	float falloffStart = 0.1;
	float a0 = falloffStart;
	float a1 = falloffStart + falloffTime;
	if (x < a0) {
		return 1.0 / x;
	}
	if (x < a1) {
		float a = 1.0 / (a0 * (a0 - a1));
		float b = -a * a1;
		return a * x + b;
	}
	return 0.0;
}

float f2(float x) {
	// Start with inverse falloff untill a0 then get to zero linearly at a1.
	// y = ax + b
	// 1/(a0^2) = a * a0 + b
	// 0.0 = a a1 + b

	// b = -a a1

	// 1/(a0^2) = a * a0  - a a1
	// 1/(a0^2) = a (a0 - a1)
	// a = 1/(a0^2(a0 - a1))
	float falloffStart = 0.49;
	float falloffTime = 0.001;
	float a0 = falloffStart;
	float a1 = falloffStart + falloffTime;
	if (x < a0) {
		return 1.0 / (x * x);
	}
	if (x < a1) {
		float a = 1.0 / (a0 * a0 * (a0 - a1));
		float b = -a * a1;
		return a * x + b;
	}
	return 0.0;
}

float sampleTexture(vec2 p) {
	float d = texture(fontAtlas, p).r;
	// Clamping leaves artifacts.
	if (p.x < atlasMin.x || p.y < atlasMin.y || p.x > atlasMax.x || p.y > atlasMax.y) {
		d = 0.0;
	}
	return d;
}

#include "noise1.glsl"
#include "noise2.glsl"

void main() {
	vec2 p = texturePosition;
	int octaves = 2;
	//vec2 pp = p * 20.0;
	float s = 2.0;
//    p += vec2(
//		octave01(pp + vec2(123.0 + s * perlin01(t * 50.0), 0.1 + s* perlin01(t + 1234.2154)), octaves), 
//		octave01(pp + vec2(233.0 + s * perlin01(t + 21.124), 0.6 + s * perlin01(t + 1534.55)), octaves))
//		* 0.005;

	vec2 center = (atlasMin + atlasMax) / 2.0;
	vec2 p2 = p;
	p2 -= center;
	p2 /= 1.3;
	p2 += center;

	vec2 p3 = p;
	p3 -= center;
	p3 *= 1.5;
	p3 += center;

	float d = sampleTexture(p);
	float d2 = sampleTexture(p2);
	float d3 = sampleTexture(p3);
	
	d -= 0.5;
	d = -d;
	d = max(0.0, d);

	vec2 pp = p * 20.0;
	float t = time / 100.0;
	vec2 o = vec2(octave01(t, 2), octave01(t + 117.215, 2)) * 50.0;
	d += octave01(pp + o, 2) * 0.15;
	d = 1.0 / (d * d);

	// There are issues with implementing falloff with fonts, because the sdf are only valid up to some distance.
	// Multiplying by a scaled version of the texture seems to work fine.
	// One issue is that it doesn't add shows for example inside 'O'.

	d /= 15.0;
	// similar to 1/x^2 * (-x/b + a)
	//d *= min(d2, d3);
	//d *= d2;
	d *= max(d2, d3);
	//d *= d2 + d3;
	//float g = octave01(vec3(p * 4.0 + texturePosition, time), 4);
	//vec3 c = colr * exp(4.0 * g - 1.0);
	fragColor = vec4(color, d);
	//fragColor = vec4(vec3(max(d2, d3)), 1.0);
	//fragColor = vec4(vec3(d2 + d3), 1.0);
}