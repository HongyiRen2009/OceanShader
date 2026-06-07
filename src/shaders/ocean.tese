#version 460 core
layout(quads, fractional_even_spacing, ccw) in;
const int MAX_SPECTRUM_COUNT = 8;
uniform mat4 matView;
uniform mat4 matProj;
uniform float time;
uniform sampler2DArray displacementMap;
uniform vec2 oceanMeshSize;
uniform int numSpectrums;
uniform float frequencies[MAX_SPECTRUM_COUNT];
uniform float amplitudes[MAX_SPECTRUM_COUNT];
out vec3 worldPos;
out vec2 fragUV;

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

vec2 getUV(vec3 pos){
	vec2 uv = (pos.xz / oceanMeshSize) + 0.5;
	return uv;
}
vec3 getDisplacement(vec2 uv){
    vec3 displacement = vec3(0.0);
    for(int i = 0; i < numSpectrums; i++){
        displacement += texture(displacementMap, vec3(uv*frequencies[i], i)).rgb*amplitudes[i];
    }
    return displacement;
} 
void main(){
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec3 p0 = gl_in[0].gl_Position.xyz;
    vec3 p1 = gl_in[1].gl_Position.xyz;
    vec3 p2 = gl_in[2].gl_Position.xyz;
    vec3 p3 = gl_in[3].gl_Position.xyz;

    vec3 leftEdge  = mix(p0, p1, u);
    vec3 rightEdge = mix(p3, p2, u);

    vec3 pos = mix(leftEdge, rightEdge, v);

	vec2 uv = getUV(pos);
	fragUV = uv;

    vec3 displacement = getDisplacement(uv);
    pos += displacement;

	worldPos = pos;
	gl_Position = matProj * matView * vec4(pos, 1.0);
}