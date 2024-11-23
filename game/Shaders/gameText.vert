#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 
layout(location = 2) in mat3x2 instanceTransform; 
layout(location = 5) in vec2 instanceOffsetInAtlas; 
layout(location = 6) in vec2 instanceSizeInAtlas; 
layout(location = 7) in vec3 instanceColor; 

out vec2 texturePosition; 
out vec2 atlasMin; 
out vec2 atlasMax; 

out vec3 color; 

void passToFragment() {
    color = instanceColor; 
}

/*generated end*/

void main() {
	passToFragment();
	texturePosition = vertexTexturePosition;
	texturePosition.y = 1.0 - texturePosition.y;
	texturePosition *= instanceSizeInAtlas;
	texturePosition += instanceOffsetInAtlas;
	atlasMin = instanceOffsetInAtlas;
	atlasMax = atlasMin + instanceSizeInAtlas;
	gl_Position = vec4(instanceTransform * vec3(vertexPosition, 1.0), 0.0, 1.0);
}
