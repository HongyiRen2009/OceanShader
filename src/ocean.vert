#version 460 core
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
in vec3 position;
out vec3 worldPos;
out vec2 fragUV;

vec2 getUV(vec3 pos){
	vec2 uv = (pos.xz / oceanMeshSize) + 0.5;
	uv *= frequency;
	return uv;
}
void main(){
	worldPos = position;
	vec2 uv = getUV(position);
	fragUV = uv;
	float height = texture(heightmap, uv).r * displacementScale;
	worldPos.y += height;
	vec2 displacement = vec2(texture(displacementxTexture, uv).r, texture(displacementyTexture, uv).r) * choppiness * displacementScale;
	worldPos.xz += displacement;
	gl_Position = matProj * matView * vec4(worldPos, 1.0);
}