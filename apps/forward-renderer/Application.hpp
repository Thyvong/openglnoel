#pragma once

#include <glmlv/filesystem.hpp>
#include <glmlv/GLFWHandle.hpp>
#include <glmlv/GLProgram.hpp>
#include <glmlv/ViewController.hpp>
#include <glmlv/simple_geometry.hpp>
#include <glmlv/Image2DRGBA.hpp>
#include <glmlv/scene_loading.hpp>

class Application
{
public:
    Application(int argc, char** argv);
	const glmlv::SimpleGeometry cube = glmlv::makeCube(),
		sphere= glmlv::makeSphere(30);
	GLuint vbocube, vbosphere;
	GLuint vaocube, vaosphere;
	GLuint ibocube, ibosphere;

	GLuint vbos;
	GLuint vaos;
	GLuint ibos;

	GLint uModelViewProjMatrix, uModelViewMatrix, uNormalMatrix;

	GLint uKd;
	GLint uDirectionalLightDir, uDirectionalLightIntensity;
	GLint uPointLightPosition, uPointLightIntensity;
	GLint uKdSampler;

	glmlv::Image2DRGBA texture1, texture2;

	GLuint texobject1, texobject2 , sampler;
	std::vector<GLuint> texobjects;

	glmlv::SceneData sceneData;

    int run();
private:
    const size_t m_nWindowWidth = 1280;
    const size_t m_nWindowHeight = 720;
    glmlv::GLFWHandle m_GLFWHandle{ m_nWindowWidth, m_nWindowHeight, "Template" }; // Note: the handle must be declared before the creation of any object managing OpenGL resource (e.g. GLProgram, GLShader)

    const glmlv::fs::path m_AppPath;
    const std::string m_AppName;
    const std::string m_ImGuiIniFilename;
    const glmlv::fs::path m_ShadersRootPath;

	glmlv::GLProgram program;
};