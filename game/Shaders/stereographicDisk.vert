#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in vec2 instanceCenter; 
layout(location = 6) in float instanceRadius; 
layout(location = 7) in vec3 instanceColorInside; 
layout(location = 8) in vec3 instanceColorBorder; 

uniform mat3x2 clipToWorld; 

out vec2 worldPosition; 

out vec2 center; 
out float radius; 
out vec3 colorInside; 
out vec3 colorBorder; 

void passToFragment() {
    center = instanceCenter; 
    radius = instanceRadius; 
    colorInside = instanceColorInside; 
    colorBorder = instanceColorBorder; 
}

/*generated end*/

void main() {
	passToFragment();
	vec2 vertexClipSpace = instanceTransform * vec3(vertexPosition, 1.0);
	gl_Position = vec4(vertexClipSpace, 0.0, 1.0);
	worldPosition = clipToWorld * vec3(vertexClipSpace, 1.0);
}
