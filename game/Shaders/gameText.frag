#version 430 core

uniform float time; 

in vec2 texturePosition; 
in vec2 atlasMin; 
in vec2 atlasMax; 

in vec3 color; 
in float randomValue; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

float sampleTexture(vec2 p) {
	float d = texture(fontAtlas, p).r;
	// Clamping creates edge bleeding.
	if (p.x < atlasMin.x || p.y < atlasMin.y || p.x > atlasMax.x || p.y > atlasMax.y) {
		d = 0.0;
	}
	return d;
}

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

//	vec2 pp = p * 20.0;
//	float t = time / 100.0;
//	vec2 o = vec2(octave01(t, 2), octave01(t + 117.215, 2)) * 50.0;
//	d += octave01(pp + o, 2) * 0.15;
//	d = 1.0 / (d * d);
//	{
//		vec2 pp = p * 40.0;
//		float t = time;
//		d += octave01(vec3(pp * 2.0, t * 1.0), 2) * 0.2;
//		d = 1.0 / (d * d);
//	}

	// There are issues with implementing falloff with fonts, because the sdf are only valid up to some distance.
	// Multiplying by a scaled version of the texture seems to work fine.
	// One issue is that it doesn't add shows for example inside 'O'.

	d = 1.0 / (d * d);
	d /= 15.0;
	// similar to 1/x^2 * (-x/b + a)
	//d *= min(d2, d3);
	d *= d2;
	//d = clamp(d, 0.0, 1.0);
	d *= octave01(vec2(randomValue, time / 2.0), 2);
	//d *= max(d2, d3);
	//d *= d2 + d3;
	//vec3 c = colr * exp(4.0 * g - 1.0);
	fragColor = vec4(color, d);
	//fragColor = vec4(vec3(max(d2, d3)), 1.0);
	//fragColor = vec4(vec3(d2 + d3), 1.0);
}