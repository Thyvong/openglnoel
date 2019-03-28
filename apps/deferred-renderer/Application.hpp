#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/scene_loading.hpp>

class Application
{
public:
    Application(int argc, char** argv);

    int run();
	void InitDefaultMat();
	void SceneLoading();

	void GeometryPassInit();
	void ShadingPassInit();
	void initGBuffer();


	void initScreenTriangle();

	static glm::vec3 computeDirectionVector(float phiRadians, float thetaRadians)
	{
		const auto cosPhi = glm::cos(phiRadians);
		const auto sinPhi = glm::sin(phiRadians);
		const auto sinTheta = glm::sin(thetaRadians);
		return glm::vec3(sinPhi * sinTheta, glm::cos(thetaRadians), cosPhi * sinTheta);
	}

private:
    const size_t m_nWindowWidth = 1280;
    const size_t m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;
	const glmlv::fs::path m_AssetsRootPath;

	glmlv::GLProgram m_geopassProg;
	glmlv::GLProgram m_shadepassProg;

	glmlv::SceneData m_scene;

	GLuint m_vbo;
	GLuint m_vao;
	GLuint m_ibo;

	GLuint m_triangleVBO;
	GLuint m_triangleVAO;

	GLuint m_defaultTexture;
	glmlv::PhongMaterial m_defaultMaterial;

	std::vector<GLuint> m_textures;
	std::vector<glmlv::PhongMaterial> m_sceneMaterials;

	

	GLuint m_sampler = 0;

	GLint m_uMVP;
	GLint m_uMV;
	GLint m_uNormal;
	GLint m_uKa;
	GLint m_uKd;
	GLint m_uKs;
	GLint m_uShiny;
	GLint m_uKaTextureUnit;
	GLint m_uKdTextureUnit;
	GLint m_uKsTextureUnit;
	GLint m_uShinyTextureUnit;

	GLint m_uDirLightDir;
	GLint m_uDirLightIntensity;

	GLint m_uPointLightPos;
	GLint m_uPointLightIntensity;


	// GBuffer:
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

	GLuint m_GBufferTextures[GBufferTextureCount]; // textures rendu par le geopass
	GLuint m_GBufferFBO; // Framebuffer object, pour envoyer les textures au gpu

};