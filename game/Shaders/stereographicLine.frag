#version 430 core

in vec2 worldPosition; 

in vec2 endpoint0; 
in vec2 endpoint1; 
in vec3 color0; 
in vec3 color1; 
in float halfWidth; 
out vec4 fragColor;

/*generated end*/

#include "stereographic.glsl"

void main() {
    // When the outer hemisphere becomes the shorter segment the endpoints of lines wrap around which casues small parts of the wrapped around endpoint to be renderer (I think not sure). This happens for example if one endpoint is one the boundary.
    if (dot(worldPosition, worldPosition) > 1.0)  {
        return;
    }
    // @Performance: many things like the plane can be computed in the vertex shader

    Plane plane = greatCirclePlane(fromStereographic(endpoint0), fromStereographic(endpoint1));
    float colorD = signedDistanceFromPlane(plane, fromStereographic(worldPosition));
    {
        float smoothing = fwidth(colorD);
	    colorD = smoothstep(smoothing, -smoothing, colorD);
    }
	fragColor = vec4(worldPosition, 0.0, 1.0);
    float d = distanceFromGeodesicSegment(endpoint0, endpoint1, worldPosition);
    {
        float smoothing = fwidth(d) * 2.0;
	    d -= halfWidth - smoothing;
	    d = smoothstep(smoothing, 0.0, d);
    }
    vec3 col = mix(color0, color1, colorD);
    fragColor = vec4(col, d);
    //fragColor = vec4(vec3(d) + vec3(0.1), 1.0);
}
