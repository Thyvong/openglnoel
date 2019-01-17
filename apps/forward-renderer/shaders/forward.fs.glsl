#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

out vec3 fColor;

uniform vec3 uKd;
// lumiere direct
uniform vec3 uDirectionalLightDir;
uniform vec3 uDirectionalLightIntensity;

// lumiere ponctuelle
uniform vec3 uPointLightPosition;
uniform vec3 uPointLightIntensity;

uniform sampler2D uKdSampler;

void main()
{
	float distToPointLight = length(uPointLightPosition - vViewSpacePosition);
	vec3 dirToPointLight = (uPointLightPosition - vViewSpacePosition) / distToPointLight;
	vec3 texResult = vec3(texture(uKdSampler,vTexCoords));
	fColor = texResult * uKd *
		(uDirectionalLightIntensity
			* max(0.0, dot(vViewSpaceNormal, uDirectionalLightDir))
			+ uPointLightIntensity
			* max(0.0, dot(vViewSpaceNormal, dirToPointLight)) / (distToPointLight * distToPointLight));
	
}