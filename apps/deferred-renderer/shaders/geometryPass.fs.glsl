#version 330

in vec3 vViewSpacePosition;
in vec3 vViewSpaceNormal;
in vec2 vTexCoords;

layout(location = 0) out vec3 fPosition;
layout(location = 1) out vec3 fNormal;
layout(location = 2) out vec3 fAmbient;
layout(location = 3) out vec3 fDiffuse;
layout(location = 4) out vec4 fGlossyShininess;

uniform vec4 uDiffus;
uniform vec3 uEmission;

uniform sampler2D uDiffusTex;
void main()
{
	fPosition = vViewSpacePosition;
	fNormal = normalize(vViewSpaceNormal);

	//vec3 ka = uKa * vec3(texture(uKaTextureUnit, vTexCoords));
	

	fAmbient = uEmission;
	fDiffuse = uDiffus.xyz* vec3(texture(uDiffusTex, vTexCoords));
	fGlossyShininess = vec4(0.5,0.5,0.5, 1);
}