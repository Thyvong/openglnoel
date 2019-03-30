#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/scene_loading.hpp>
#include <tiny_gltf.h>


typedef struct mat {
	GLuint texId = 0;

	glm::vec4 diffus = glm::vec4(1);
	glm::vec3 emission = glm::vec3(1);

}PBRMat;

class Application
{
public:
    Application(int argc, char** argv);

    int run();
	void InitDefaultMat();
	void SceneLoadingGLTF();

	void GeometryPassInit();
	void ShadingPassInit();
	void initGBuffer();

	void initShadowMapDir();
	void initShadowMapDirShaders();

	void initScreenTriangle();

	static glm::vec3 computeDirectionVector(float phiRadians, float thetaRadians)
	{
		const auto cosPhi = glm::cos(phiRadians);
		const auto sinPhi = glm::sin(phiRadians);
		const auto sinTheta = glm::sin(thetaRadians);
		return glm::vec3(sinPhi * sinTheta, glm::cos(thetaRadians), cosPhi * sinTheta);
	}
	static glm::vec3 computeDirectionVectorUp(float phiRadians, float thetaRadians)
	{
		const auto cosPhi = glm::cos(phiRadians);
		const auto sinPhi = glm::sin(phiRadians);
		const auto cosTheta = glm::cos(thetaRadians);
		return -glm::normalize(glm::vec3(sinPhi * cosTheta, -glm::sin(thetaRadians), cosPhi * cosTheta));
	};

private:
    const size_t m_nWindowWidth = 1280;
    const size_t m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

	// global

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;
	const glmlv::fs::path m_AssetsRootPath;

	// programmes des shaders

	glmlv::GLProgram m_geopassProg;
	glmlv::GLProgram m_shadepassProg;

	// gltf

	

	tinygltf::Model m_model;
	GLuint m_gltfvao;

	// mats

	GLuint m_defaultTexture;
	PBRMat m_defaultMat;
	std::vector<GLuint> m_textures;
	std::vector<PBRMat> m_gltfMaterials;


	// uniforme des transfo

	GLuint m_sampler = 0;

	GLint m_uMVP;
	GLint m_uMV;
	GLint m_uNormal;

	GLint m_uDiffus;
	GLint m_uEmission;

	GLint m_uDirLightDir;
	GLint m_uDirLightIntensity;

	GLint m_uPointLightPos;
	GLint m_uPointLightIntensity;

	
	// GBuffer
	enum GBufferTextureType
	{
		GPosition = 0,
		GNormal,
		GAmbient,
		GDiffuse,
		GGlossyShininess,
		GDepth,
		GBufferTextureCount
	};
	const GLenum m_GBufferTextureFormat[GBufferTextureCount] = { GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGB32F, GL_RGBA32F, GL_DEPTH_COMPONENT32F };
	const char * m_GBufferTexNames[GBufferTextureCount + 1] = { "position", "normal", "ambient", "diffuse", "glossyShininess", "depth", "beauty" };

	GLuint m_GBufferTextures[GBufferTextureCount]; // textures rendu par le geopass
	GLuint m_GBufferFBO; // Framebuffer object, pour envoyer les textures au gpu
	
	GLint m_uGBuffer[GBufferTextureCount];

	GLuint m_triangleVBO;
	GLuint m_triangleVAO;

	// shadow map pour directional light
	GLuint m_directionalSMTexture; // texture pour la depthmap calculé depuis la light
	GLuint m_directionalSMFBO; // pour lier les textures durant l'écriture du shader
	GLuint m_directionalSMSampler; // sampler pour lire la texture depuis les shaders
	int32_t m_nDirectionalSMResolution = 512;

	glmlv::GLProgram m_directionalSMProgram;
	GLint m_uDirLightViewProjMatrix; // envoyé dans m_directionalSMProgram
	GLint m_uDirLightShadowMap;
	GLint m_uDirLightShadowMapBias;

	GLint m_uDirLightViewProjMatrix_shadingpass; // envoyé dans shadepassProg

	// closest filtering point
	GLint m_uDirLightShadowMapSampleCount ; 
	GLint m_uDirLightShadowMapSpread ;

};

/*
d'abord on génére des textures dans des gluint
on crée un fbo qui va dire où le shader doit écrire les données, cad, dans la texture
on crée un sampler que le shader utilisera en shading pass pour lire dans la texture qu'on vient de générer
enjoy


*/