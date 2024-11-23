#version 430 core

in vec2 texturePosition; 
in vec2 atlasMin; 
in vec2 atlasMax; 

in vec3 color; 
out vec4 fragColor;

/*generated end*/

uniform sampler2D fontAtlas;

void main() {
	vec2 p = texturePosition;
	p = clamp(p, atlasMin, atlasMax);
	float d = texture(fontAtlas, p).r;
	float smoothing = fwidth(d) * 2.0;
	d -= 0.5 - smoothing;
	d = smoothstep(0.0, smoothing, d);
	fragColor = vec4(vec3(1.0), d);
}