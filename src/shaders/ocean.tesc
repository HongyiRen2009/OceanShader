#version 460 core
layout(vertices = 4) out;

uniform float meshResolution;
uniform vec3 cameraPos;

float getTessLevel(vec3 pos, vec3 camPos, float maxRes) {
    float dist = distance(camPos, pos);
    return mix(maxRes, 2.0, clamp(dist / 50.0, 0.0, 1.0));
}

void main()
{
    if(gl_InvocationID == 0) {	
        vec3 p0 = gl_in[0].gl_Position.xyz;
        vec3 p1 = gl_in[1].gl_Position.xyz;
        vec3 p2 = gl_in[2].gl_Position.xyz;
        vec3 p3 = gl_in[3].gl_Position.xyz;

        vec3 edgeMidpoint0 = (p0 + p3) * 0.5; 
        vec3 edgeMidpoint1 = (p0 + p1) * 0.5; 
        vec3 edgeMidpoint2 = (p1 + p2) * 0.5; 
        vec3 edgeMidpoint3 = (p2 + p3) * 0.5; 

        gl_TessLevelOuter[0] = getTessLevel(edgeMidpoint0, cameraPos, meshResolution);
        gl_TessLevelOuter[1] = getTessLevel(edgeMidpoint1, cameraPos, meshResolution);
        gl_TessLevelOuter[2] = getTessLevel(edgeMidpoint2, cameraPos, meshResolution);
        gl_TessLevelOuter[3] = getTessLevel(edgeMidpoint3, cameraPos, meshResolution);
        gl_TessLevelInner[0] = (gl_TessLevelOuter[1] + gl_TessLevelOuter[3]) * 0.5;
        gl_TessLevelInner[1] = (gl_TessLevelOuter[0] + gl_TessLevelOuter[2]) * 0.5;
    }
    
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}