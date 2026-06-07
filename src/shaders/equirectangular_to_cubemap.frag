#version 460 core
in vec3 localPos;
out vec4 FRAG_COLOR;
uniform sampler2D equirectangularMap;

const vec2 INV_ATAN = vec2(0.1591, 0.3183);

vec2 sampleSphericalMap(vec3 v) {
	vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
	uv *= INV_ATAN;
	uv += 0.5;
	return uv;
}

void main() {
	vec2 uv = sampleSphericalMap(normalize(localPos));
	vec3 color = texture(equirectangularMap, uv).rgb;
	FRAG_COLOR = vec4(color, 1.0);
}
