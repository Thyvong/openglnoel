#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/scene_loading.hpp>
#include <tiny_gltf.h>


typedef struct mat {
	/*

	Exemple de material

	"name": "Material0",
		"pbrMetallicRoughness" : {
			"baseColorFactor": [0.5, 0.5, 0.5, 1.0],
			"baseColorTexture" : {
				"index": 1,
				"texCoord" : 1
			},
			"metallicFactor" : 1,
			"roughnessFactor" : 1,
			"metallicRoughnessTexture" : {
				"index": 2,
				"texCoord" : 1
			}
		},
		"normalTexture": {
			"scale": 2,
			"index" : 3,
			"texCoord" : 1
		},
		"emissiveFactor" : [0.2, 0.1, 0.0]
		
	*/
	GLuint texId = 0; // id dans m_textures, correspond aussi � l'indice dans model.textures

	glm::vec4 diffus = glm::vec4(1); // �quivalent � baseColor dans la d�nomination
	glm::vec3 emission = glm::vec3(1); // �quivalent � emissiveFactor

}PBRMat;

class Application
{
public:
    Application(int argc, char** argv);

    int run();
	void InitDefaultMat();
	void SceneLoadingGLTF(int argc, char** argv);

	void GeometryPassInit();
	void ShadingPassInit();
	void initGBuffer();

	void initShadowMapDir();
	void initShadowMapDirShaders();

	void initScreenTriangle();

	bool loadModel(tinygltf::Model &model, const char *filename);
	std::map<int, GLuint> bindMesh(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Mesh &mesh);
	void bindModelNodes(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Node &node);
	GLuint bindModel(tinygltf::Model &model);
	void InitMats(tinygltf::Model &model);
	void drawMesh(tinygltf::Model &model, tinygltf::Mesh &mesh);
	void drawModelNodes(tinygltf::Model &model, tinygltf::Node &node);
	void drawModel(GLuint vao, tinygltf::Model &model);
	static void error_callback(int error, const char *description) {
		(void)error;
		fprintf(stderr, "Error: %s\n", description);
	}


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

	GLint m_uDiffus; // pour la valeur
	GLint m_uEmission;

	GLint m_uDiffusTex; // pour transmettre la texture unit

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
	GLuint m_directionalSMTexture; // texture pour la depthmap calcul� depuis la light
	GLuint m_directionalSMFBO; // pour lier les textures durant l'�criture du shader
	GLuint m_directionalSMSampler; // sampler pour lire la texture depuis les shaders
	int32_t m_nDirectionalSMResolution = 512;

	glmlv::GLProgram m_directionalSMProgram;
	GLint m_uDirLightViewProjMatrix; // envoy� dans m_directionalSMProgram
	GLint m_uDirLightShadowMap;
	GLint m_uDirLightShadowMapBias;

	GLint m_uDirLightViewProjMatrix_shadingpass; // envoy� dans shadepassProg

	// closest filtering point
	GLint m_uDirLightShadowMapSampleCount ; 
	GLint m_uDirLightShadowMapSpread ;

};

/*
d'abord on g�n�re des textures dans des gluint
on cr�e un fbo qui va dire o� le shader doit �crire les donn�es, cad, dans la texture
on cr�e un sampler que le shader utilisera en shading pass pour lire dans la texture qu'on vient de g�n�rer
enjoy


*/