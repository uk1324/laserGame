#version 430 core

in vec2 worldPosition; 

in vec2 center; 
in float radius; 
in vec3 colorInside; 
in vec3 colorBorder; 
out vec4 fragColor;

/*generated end*/

#include "stereographic.glsl"

void main() {
	vec3 col;
	float dist = sphericalDistance(fromStereographic(center), fromStereographic(worldPosition));
    float smoothing = fwidth(dist) * 2.0;

	float d = dist;
	{
	    d -= radius - smoothing;
	    d = smoothstep(smoothing, 0.0, d);
    }

	float colorD = dist;
	{
		float outlineWidth = 0.02;
		colorD -= radius - smoothing - outlineWidth;
		colorD = smoothstep(smoothing / 2.0, 0.0, colorD);
	}

	col = mix(colorBorder, colorInside, colorD);
	fragColor = vec4(col, d);
	//fragColor = vec4(col, 1.0);
}
