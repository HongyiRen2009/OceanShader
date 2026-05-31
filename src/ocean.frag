#version 460 core
in vec3 worldPos;
in vec2 fragUV;
uniform sampler2D dxTexture;
uniform sampler2D dyTexture;
uniform sampler2D displacementxTexture;
uniform sampler2D displacementyTexture;
uniform samplerCube environmentMap;
uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float choppiness;
uniform vec2 oceanMeshSize;

out vec4 FRAG_COLOR;

const vec3 OCEAN_ALBEDO = vec3(0.0, 0.48, 0.57);
const vec3 SUN_COLOR = vec3(1.0);
const vec3 SKY_COLOR = vec3(0.529, 0.808, 0.922);
const float PI = 3.14159265359;
const float k1 = 10.0;
const float k2 = 0.1;
const float k3 = 0.3;
const float k4 = 1.75;

vec3 scatter(vec3 normal, vec3 viewDir, float waveHeight) {
	vec3 Wi = -lightDir;
	vec3 Wo = -viewDir;
	float forwardScattering = k1*waveHeight*max(pow(dot(Wi,-Wo),4.0),0.0);
	float backLighting = pow(0.5-0.5*dot(Wi,normal),3.0);
	float upSwelling = k2*max(pow(dot(Wo,normal),2.0),0.0);
	vec3 scatter = (forwardScattering*backLighting+upSwelling)*OCEAN_ALBEDO*SUN_COLOR;
	vec3 diffuse = k3*max(dot(-Wi,normal),0.0)*OCEAN_ALBEDO*SUN_COLOR;
	// Add foam here when jacobian is implemented
	scatter+=mix(OCEAN_ALBEDO,diffuse,0.4);
	return vec3(scatter);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggxV = GeometrySchlickGGX(NdotV, roughness);
	float ggxL = GeometrySchlickGGX(NdotL, roughness);
	return ggxV * ggxL;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 specular(vec3 N, vec3 L, vec3 V, float roughness) {
	vec3 H = normalize(V + L);
	vec3 F0 = vec3(0.02);

	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	return (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
}

vec3 getNormal(vec2 uv) {
	float dx = texture(dxTexture, uv).r;
	float dy = texture(dyTexture, uv).r;
	return normalize(vec3(-dx, 1.0, -dy));
}

void main(){
	vec3 n = getNormal(fragUV);
	vec3 l = normalize(lightDir);
	vec3 v = normalize(cameraPos - worldPos);

	vec3 scatterColor = scatter(n, v, worldPos.y);
	vec3 spec = specular(n, l, v, 0.4);
	scatterColor+=spec;
	vec3 reflection = texture(environmentMap, reflect(-v, n)).rgb;
	float fresnel = pow(1.0 - max(dot(n, v), 0.0), 3.0) * 0.65 + 0.35;
	vec3 oceanColor = mix(scatterColor, reflection, fresnel) + spec * SUN_COLOR;
	float fogFactor = exp(-distance(cameraPos, worldPos) / 50.0);
	vec3 color = mix(SKY_COLOR, oceanColor, fogFactor);
	FRAG_COLOR = vec4(color,1.0);
}