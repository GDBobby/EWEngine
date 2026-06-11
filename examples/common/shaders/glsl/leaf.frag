#version 450

#include "leaf_common.glsl"

layout (location = 0) in vec3 fragPosWorld;
layout (location = 1) in vec3 fragNormalWorld;
//layout (location = 2) in vec2 fragTexCoord;

layout (location = 0) out vec4 outColor;

const float PI = 3.14159265359;

float DistributionGGX (vec3 N, vec3 HalfAngle, float roughness) {
    float a2    = roughness * roughness * roughness * roughness;
    float NdotH = max (dot (N, HalfAngle), 0.0);
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX (float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith (vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX (max (dot(N, L), 0.0), roughness) * 
           GeometrySchlickGGX (max (dot(N, V), 0.0), roughness);
}

vec3 FresnelSchlick (float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow (1.0 - cosTheta, 5.0);
}

//layout(set = 0, binding = 1) uniform sampler2D bindless[];

void main(){
	vec3 viewDirection = normalize(push.ir.sbo.cameraPos.xyz - fragPosWorld);
	
	vec3 surfaceNormal = normalize(fragNormalWorld);
	//albedo will always be defined
	//vec3 albedo = texture(bindless[push.texture_index], fragTexCoord).rgb;
	vec3 albedo = vec3(1.0); //TEMPORARY, SKIPPING THE TEXTURE
	float roughness = 0.5;
	float metal = 0.0;
	
	vec3 F0 = vec3(0.01);
	F0 = mix(F0, albedo, metal);
	
	//reflectance
	vec3 Lo = vec3(0.0);

	vec3 sunDir = normalize(-push.ir.sbo.sunlightDirection.xyz);
	//float sunAttenuation = 1.0 / dot(sunDir, sunDir); //this should always be 1
	float sunAttenuation = 1.0;
	//new attenuation method
	//float sunAttenuation = 1.0 / (1.0 + 0.09 + .032);
	
	//sunDir = normalize(sunDir);
	vec3 sunRadiance = push.ir.sbo.sunlightColor.rgb * sunAttenuation * push.ir.sbo.sunlightColor.w; //1.0 is sunAttenuation
	
	vec3 sunHalfAngle = normalize(viewDirection + sunDir);
	float sunNDF = DistributionGGX(surfaceNormal, sunHalfAngle, roughness);
	float sunGeo = GeometrySmith(surfaceNormal, viewDirection, sunDir, roughness);
	vec3 sunFres = FresnelSchlick(max(dot(sunHalfAngle, viewDirection), 0.0), F0);
	
	vec3 sunkD = vec3(1.0) - sunFres;
	sunkD *= 1.0 - metal;
	vec3 sunSpecular = (sunNDF * sunGeo * sunFres) / (4.0 * max(dot(surfaceNormal, viewDirection), 0.0) * max(dot(surfaceNormal, sunDir), 0.0) + .0001);
	float sunNdotL = max(dot(surfaceNormal, sunDir), 0.0);
	Lo += (sunkD * albedo / PI + sunSpecular) * sunRadiance * sunNdotL;

	vec3 ambient = vec3(0.01) * albedo;

	vec3 color = ambient + Lo;
	
	color /= (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));
	
	float tempColor = sqrt((color.x + color.y + color.z) / 3.f);
	//tempColor = 2.f - sqrt(tempColor);
	//tempColor = max(tempColor, 1.f);
	//tempColor = 1.f - tempColor;
	//tempColor = step(0.9f, tempColor);
	color= vec3(tempColor); //just do this in the texture????
	
	outColor = vec4(color, 1.0);
	
}