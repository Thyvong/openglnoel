#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec3 fAmbient;
layout(location = 3) out vec3 fDiffuse;
layout(location = 4) out vec4 fGlossyShininess;


uniform vec3 uKa;
uniform vec3 uKd;
uniform vec3 uKs;
uniform float uShiny;

uniform sampler2D uKaTextureUnit;
uniform sampler2D uKdTextureUnit;
uniform sampler2D uKsTextureUnit;
uniform sampler2D uShinyTextureUnit;

void main()
{
	fPosition = vViewSpacePosition;
	fNormal = normalize(vViewSpaceNormal);

	vec3 ka = uKa * vec3(texture(uKaTextureUnit, vTexCoords));
	vec3 kd = uKd * vec3(texture(uKdTextureUnit, vTexCoords));
	vec3 ks = uKs * vec3(texture(uKsTextureUnit, vTexCoords));
	float shininess = uShiny * vec3(texture(uShinyTextureUnit, vTexCoords)).x;

	fAmbient = ka;
	fDiffuse = kd;
	fGlossyShininess = vec4(ks, shininess);
}