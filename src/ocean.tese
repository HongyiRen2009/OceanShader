#version 460 core
layout(quads, fractional_even_spacing, ccw) in;
uniform mat4 matView;
uniform mat4 matProj;
uniform float time;
uniform sampler2D heightmap;
uniform sampler2D dxTexture;
uniform sampler2D dyTexture;
uniform sampler2D displacementxTexture;
uniform sampler2D displacementyTexture;
uniform vec2 oceanMeshSize;
uniform float displacementScale;
uniform float frequency;
uniform float choppiness;
out vec3 worldPos;
out vec2 fragUV;

in gl_PerVertex
{
    vec4 gl_Position;
} gl_in[];

vec2 getUV(vec3 pos){
	vec2 uv = (pos.xz / oceanMeshSize) + 0.5;
	uv *= frequency;
	return uv;
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

	float height = texture(heightmap, uv).r * displacementScale;
	pos.y += height;

	vec2 displacement = vec2(texture(displacementxTexture, uv).r, texture(displacementyTexture, uv).r) * choppiness * displacementScale;
	pos.xz += displacement;

	worldPos = pos;
	gl_Position = matProj * matView * vec4(pos, 1.0);
}