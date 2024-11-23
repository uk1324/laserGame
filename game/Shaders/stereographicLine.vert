#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in vec2 instanceEndpoint0; 
layout(location = 6) in vec2 instanceEndpoint1; 
layout(location = 7) in vec3 instanceColor0; 
layout(location = 8) in vec3 instanceColor1; 
layout(location = 9) in float instanceHalfWidth; 

uniform mat3x2 clipToWorld; 

out vec2 worldPosition; 

out vec2 endpoint0; 
out vec2 endpoint1; 
out vec3 color0; 
out vec3 color1; 
out float halfWidth; 

void passToFragment() {
    endpoint0 = instanceEndpoint0; 
    endpoint1 = instanceEndpoint1; 
    color0 = instanceColor0; 
    color1 = instanceColor1; 
    halfWidth = instanceHalfWidth; 
}

/*generated end*/

void main() {
	passToFragment();
	vec2 vertexClipSpace = instanceTransform * vec3(vertexPosition, 1.0);
	gl_Position = vec4(vertexClipSpace, 0.0, 1.0);
	worldPosition = clipToWorld * vec3(vertexClipSpace, 1.0);
}
