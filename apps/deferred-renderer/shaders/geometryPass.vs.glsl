#version 330

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

out vec3 vViewSpacePosition;
out vec3 vViewSpaceNormal;
out vec2 vTexCoords;

uniform mat4 uMVP;
uniform mat4 uMV;
uniform mat4 uNormal;

void main()
{
	vViewSpacePosition = vec3(uMV * vec4(aPosition, 1));
	vViewSpaceNormal = vec3(uNormal * vec4(aNormal, 0));
	vTexCoords = aTexCoords;
	gl_Position = uMVP * vec4(aPosition, 1);
}