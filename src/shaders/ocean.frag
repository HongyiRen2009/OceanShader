#version 460 core
in vec3 worldPos;
in vec2 fragUV;
uniform sampler2DArray normalMap;
uniform sampler2DArray foamTexture;
uniform samplerCube environmentMap;
uniform vec3 lightDir;
uniform vec3 cameraPos;
uniform float choppiness;
uniform vec2 oceanMeshSize;
const int MAX_SPECTRUM_COUNT = 8;
uniform int numSpectrums;
uniform float frequencies[MAX_SPECTRUM_COUNT];
uniform float amplitudes[MAX_SPECTRUM_COUNT];
out vec4 FRAG_COLOR;

const vec3 OCEAN_ALBEDO = vec3(0.0, 0.48, 0.90);
const vec3 SUN_COLOR = vec3(1.0);
const vec3 SKY_COLOR = vec3(0.529, 0.808, 0.922);
const vec3 FOAM_COLOR = vec3(1.0);
const float PI = 3.14159265359;
const float k1 = 10.0;
const float k2 = 0.1;
const float k3 = 0.3;
const float k4 = 1.75;


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
vec3 scatter(vec3 normal, vec3 viewDir, float waveHeight,float foamMask) {
	vec3 Wi = -lightDir;
	vec3 Wo = -viewDir;
	float forwardScattering = k1*waveHeight*max(pow(dot(Wi,-Wo),4.0),0.0);
	float backLighting = pow(0.5-0.5*dot(Wi,normal),3.0);
	float upSwelling = k2*max(pow(dot(Wo,normal),2.0),0.0);
	float smithShadow = GeometrySmith(normal, -Wo,-Wi, 0.4+foamMask);
	vec3 scatter = (forwardScattering*backLighting+upSwelling)*OCEAN_ALBEDO*SUN_COLOR*smithShadow;
	vec3 diffuse = k3*max(dot(-Wi,normal),0.0)*OCEAN_ALBEDO*SUN_COLOR;
	scatter+=mix(OCEAN_ALBEDO,diffuse,0.4);
	vec3 foam = k4*foamMask*FOAM_COLOR*SUN_COLOR;
	scatter+=foam;
	return vec3(scatter);
}
vec3 specular(vec3 N, vec3 L, vec3 V, float roughness) {
	vec3 H = normalize(V + L);
	vec3 F0 = vec3(0.02);

	float D = DistributionGGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	vec3 specular =  (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
	return specular*SUN_COLOR*NdotL;
}

vec3 getNormal(vec2 uv){
    vec3 normal = vec3(0.0);
    for(int i = 0; i < numSpectrums; i++){
        normal += texture(normalMap, vec3(uv*frequencies[i], i)).rgb*amplitudes[i];
    }
    return normalize(normal);
} 
float getFoamMask(vec2 uv){
	float mask = 0.0;
	for(int i=0; i<numSpectrums; i++){
		mask += texture(foamTexture, vec3(uv*frequencies[i], i)).r*amplitudes[i];
	}
	return clamp(mask,0.0,1.0);
	}
void main(){
	vec3 n = getNormal(fragUV);
	vec3 l = normalize(lightDir);
	vec3 v = normalize(cameraPos - worldPos);
	float foamMask = getFoamMask(fragUV);
	vec3 scatterColor = scatter(n, v, worldPos.y, foamMask);
	vec3 spec = specular(n, l, v, foamMask+0.4);
	vec3 reflection = texture(environmentMap, reflect(-v, n)).rgb;
	float fresnel = pow(1.0 - max(dot(n, v), 0.0), 3.0) * 0.65 + 0.35;
	vec3 oceanColor = mix(scatterColor, reflection, fresnel) + spec;
	float fogFactor = exp(-distance(cameraPos, worldPos) / 50.0);
	vec3 color = mix(SKY_COLOR, oceanColor, 1.0);
	FRAG_COLOR = vec4(color,1.0);
}