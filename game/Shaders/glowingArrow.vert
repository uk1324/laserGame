#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in float instanceWidthOverHeight; 
layout(location = 6) in float instanceHoverT; 
layout(location = 7) in float instanceOpacity; 

out vec2 position; 
out float width; 

out float hoverT; 
out float opacity; 

void passToFragment() {
    hoverT = instanceHoverT; 
    opacity = instanceOpacity; 
}

/*generated end*/

void main() {
	passToFragment();

	float rectScale = 2.0;
	vec2 vertexClipSpace = instanceTransform * vec3(vertexPosition * rectScale, 1.0);
	gl_Position = vec4(vertexClipSpace, 0.0, 1.0);

	position = vertexTexturePosition;
	position -= 0.5;
	position.x *= instanceWidthOverHeight;
	position *= rectScale;
	width = instanceWidthOverHeight;
}
