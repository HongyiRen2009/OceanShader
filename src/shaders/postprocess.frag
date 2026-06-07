#version 460 core

in vec2 vUv;
out vec4 FragColor;

uniform sampler2D sceneColor;
uniform sampler2D sceneDepth;
uniform samplerCube environmentMap;
uniform mat4 invProj;
uniform mat4 invView;

vec3 getWorldDir(vec2 uv) {
	vec4 ndc = vec4(uv * 2.0 - 1.0, 1.0, 1.0);
	vec4 view = invProj * ndc;
	view /= view.w;
	vec3 dirView = normalize(view.xyz);
	vec3 dirWorld = normalize((invView * vec4(dirView, 0.0)).xyz);
	return dirWorld;
}

vec3 toneMapHdr(vec3 hdr) {
	vec3 mapped = hdr / (hdr + vec3(1.0));
	return pow(mapped, vec3(1.0 / 2.2));
}

void main() {
	float depth = texture(sceneDepth, vUv).r;
	vec3 color = texture(sceneColor, vUv).rgb;
	if (depth >= 0.9999) {
		vec3 dir = getWorldDir(vUv);
		color = toneMapHdr(texture(environmentMap, dir).rgb);
	}
	FragColor = vec4(color, 1.0);
}