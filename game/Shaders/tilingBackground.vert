#version 430 core

layout(location = 0) in vec2 vertexPosition; 
layout(location = 1) in vec2 vertexTexturePosition; 

uniform float aspectRatio; 

out vec2 position; 

/*generated end*/

void main() {
	gl_Position = vec4(vertexPosition, 0.0, 1.0);

	position = vertexTexturePosition;
    position -= 0.5;
    position.x *= aspectRatio;

}
