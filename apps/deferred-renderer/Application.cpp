#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int Application::run()
{
	// Camera
	glmlv::ViewController camera{ m_GLFWHandle.window(), 3.f };
	float scenesize = glm::length(m_scene.bboxMax - m_scene.bboxMin);
	camera.setSpeed(scenesize*0.1f);
	float clearColor[3] = { 0, 0, 0 };
	
	// Put here code to run before rendering loop
	glm::mat4 Proj = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight, 0.01f * scenesize, scenesize);
	glm::mat4 View  = camera.getViewMatrix();
	glm::mat4 Normal;
	glm::mat4 Model;
	glm::mat4 MV;
	glm::mat4 MVP;
	
	glm::vec3 DirLightColor = glm::vec3(1);
	float DirLightIntensity = 20;
	float DirLightPhiAngleDegrees =40;
	float DirLightThetaAngleDegrees =40;
	glm::vec3 DirLightDirection = computeDirectionVector(glm::radians(DirLightPhiAngleDegrees), glm::radians(DirLightThetaAngleDegrees));
	
	glm::vec3 PointLightColor = glm::vec3(1);
	float PointLightIntensity =40;
	glm::vec3 PointLightPosition = glm::vec3(1,1,1);


	// Loop until the user closes the window
	for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
	{
		const auto seconds = glfwGetTime();

		// Put here rendering code

		m_geopassProg.use();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBufferFBO);

		glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		// Same sampler for all texture units
		for (GLuint i : {0, 1, 2, 3})
			glBindSampler(i, m_sampler);

		// Set texture unit of each sampler
		glUniform1i(m_uKaTextureUnit, 0);
		glUniform1i(m_uKdTextureUnit, 1);
		glUniform1i(m_uKsTextureUnit, 2);
		glUniform1i(m_uShinyTextureUnit, 3);

		glBindVertexArray(m_vao);
		int indexOffset = 0;
		// We draw each shape by specifying how much indices it carries, and with an offset in the global index buffer
		for (int i =0 ; i < m_scene.shapeCount ; i++)
		{
			glmlv::PhongMaterial material = m_scene.materialIDPerShape[i] >= 0 ? m_sceneMaterials[m_scene.materialIDPerShape[i]] : m_defaultMaterial;
			glUniform3fv(m_uKa, 1, glm::value_ptr(material.Ka));
			glUniform3fv(m_uKd, 1, glm::value_ptr(material.Kd));
			glUniform3fv(m_uKs, 1, glm::value_ptr(material.Ks));
			glUniform1fv(m_uShiny, 1, &material.shininess);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, material.KaTextureId);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, material.KdTextureId);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, material.KsTextureId);
			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, material.shininessTextureId);
			

			Model = m_scene.localToWorldMatrixPerShape[i];
			MV = View * Model;
			MVP = Proj * MV;
			Normal = glm::transpose(glm::inverse(MV));

			glUniformMatrix4fv(m_uMVP, 1, GL_FALSE, glm::value_ptr(MVP));
			glUniformMatrix4fv(m_uMV, 1, GL_FALSE, glm::value_ptr(MV));
			glUniformMatrix4fv(m_uNormal, 1, GL_FALSE, glm::value_ptr(Normal));

			
			glDrawElements(GL_TRIANGLES, m_scene.indexCountPerShape[i], GL_UNSIGNED_INT, (const GLvoid*)(indexOffset * sizeof(GLuint)));
			indexOffset += m_scene.indexCountPerShape[i];
		}

		for (GLuint i : {0, 1, 2, 3})
			glBindSampler(i, 0);

		glBindVertexArray(0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);


		// GUI code:
		glmlv::imguiNewFrame();

		{
			ImGui::Begin("GUI");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			if (ImGui::ColorEdit3("clearColor", clearColor)) {
				glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.f);
			}
			
			if (ImGui::CollapsingHeader("Directional Light"))
			{
				ImGui::ColorEdit3("DirLightColor", glm::value_ptr(DirLightColor));
				ImGui::DragFloat("DirLightIntensity", &DirLightIntensity, 0.1f, 0.f, 100.f);
				if (ImGui::DragFloat("Phi Angle", &DirLightPhiAngleDegrees, 1.0f, 0.0f, 360.f) ||
					ImGui::DragFloat("Theta Angle", &DirLightThetaAngleDegrees, 1.0f, 0.0f, 180.f)) {
					DirLightDirection = computeDirectionVector(glm::radians(DirLightPhiAngleDegrees), glm::radians(DirLightThetaAngleDegrees));
				}
			}

			if (ImGui::CollapsingHeader("Point Light"))
			{
				ImGui::ColorEdit3("PointLightColor", glm::value_ptr(PointLightColor));
				ImGui::DragFloat("PointLightIntensity", &PointLightIntensity, 0.1f, 0.f, 16000.f);
				ImGui::InputFloat3("Position", glm::value_ptr(PointLightPosition));
			}
			/*
			if (ImGui::CollapsingHeader("GBuffer"))
			{
				for (int32_t i = GPosition; i <= GBufferTextureCount; ++i)
				{
					if (ImGui::RadioButton(m_GBufferTexNames[i], m_CurrentlyDisplayed == i))
						m_CurrentlyDisplayed = GBufferTextureType(i);
				}
			}*/

			ImGui::End();
		}

		glmlv::imguiRenderFrame();

		/* Poll for and process events */
		glfwPollEvents();

		/* Swap front and back buffers*/
		m_GLFWHandle.swapBuffers();

		auto ellapsedTime = glfwGetTime() - seconds;
		auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
		if (!guiHasFocus) {
			camera.update(float(ellapsedTime));
		}
	}

	return 0;
}

void Application::InitDefaultMat() {
	// default tex
	glGenTextures(1, &m_defaultTexture);
	glBindTexture(GL_TEXTURE_2D, m_defaultTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, 1, 1);
	glm::vec4 white(1.f, 1.f, 1.f, 1.f);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_FLOAT, &white);
	glBindTexture(GL_TEXTURE_2D, 0);

	// default mat
	m_defaultMaterial.Ka = glm::vec3(0);
	m_defaultMaterial.Kd = glm::vec3(1);
	m_defaultMaterial.Ks = glm::vec3(1);
	m_defaultMaterial.shininess = 32.f;
	m_defaultMaterial.KaTextureId = m_defaultTexture;
	m_defaultMaterial.KdTextureId = m_defaultTexture;
	m_defaultMaterial.KsTextureId = m_defaultTexture;
	m_defaultMaterial.shininessTextureId = m_defaultTexture;

}

void Application::SceneLoading() {

	const auto objPath = m_AssetsRootPath / "sponza.obj";
	loadObjScene(objPath, m_scene);

	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ibo);
	glGenVertexArrays(1, &m_vao);

	//  VBO
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferStorage(GL_ARRAY_BUFFER, m_scene.vertexBuffer.size() * sizeof(glmlv::Vertex3f3f2f), m_scene.vertexBuffer.data(), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//  IBO
	glBindBuffer(GL_ARRAY_BUFFER, m_ibo);
	glBufferStorage(GL_ARRAY_BUFFER, m_scene.indexBuffer.size() * sizeof(uint32_t), m_scene.indexBuffer.data(), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//  VAO
	glBindVertexArray(m_vao);

	const GLint positionAttrLocation = 0;
	const GLint normalAttrLocation = 1;
	const GLint texCoordsAttrLocation = 2;

	// We tell OpenGL what vertex attributes our VAO is describing:
	glEnableVertexAttribArray(positionAttrLocation);
	glEnableVertexAttribArray(normalAttrLocation);
	glEnableVertexAttribArray(texCoordsAttrLocation);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo); // We bind the VBO because the next 3 calls will read what VBO is bound in order to know where the data is stored

	glVertexAttribPointer(positionAttrLocation, 3, GL_FLOAT, GL_FALSE, sizeof(glmlv::Vertex3f3f2f), (const GLvoid*)offsetof(glmlv::Vertex3f3f2f, position));
	glVertexAttribPointer(normalAttrLocation, 3, GL_FLOAT, GL_FALSE, sizeof(glmlv::Vertex3f3f2f), (const GLvoid*)offsetof(glmlv::Vertex3f3f2f, normal));
	glVertexAttribPointer(texCoordsAttrLocation, 2, GL_FLOAT, GL_FALSE, sizeof(glmlv::Vertex3f3f2f), (const GLvoid*)offsetof(glmlv::Vertex3f3f2f, texCoords));

	glBindBuffer(GL_ARRAY_BUFFER, 0); // We can unbind the VBO because OpenGL has "written" in the VAO what VBO it needs to read when the VAO will be drawn

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo); // Binding the IBO to GL_ELEMENT_ARRAY_BUFFER while a VAO is bound "writes" it in the VAO for usage when the VAO will be drawn

	glBindVertexArray(0);


	// tex&mats collection
	m_textures = std::vector<GLuint>(m_scene.textures.size());
	m_sceneMaterials = std::vector<glmlv::PhongMaterial>(m_scene.materials.size());

	// init scene textures into opengl objects
	for (int i = 0; i < m_scene.textures.size(); i++) {
		glGenTextures(1, &m_textures[i]);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, m_scene.textures[i].width(), m_scene.textures[i].height());
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_scene.textures[i].width(), m_scene.textures[i].height(), GL_RGBA, GL_UNSIGNED_BYTE, m_scene.textures[i].data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// storing materials
	for (int i = 0; i < m_scene.materials.size(); i++) {
		m_sceneMaterials[i].Ka = m_scene.materials[i].Ka;
		m_sceneMaterials[i].Kd = m_scene.materials[i].Kd;
		m_sceneMaterials[i].Ks = m_scene.materials[i].Ks;
		m_sceneMaterials[i].shininess = m_scene.materials[i].shininess;
		m_sceneMaterials[i].KaTextureId = m_scene.materials[i].KaTextureId >= 0 ? m_textures[m_scene.materials[i].KaTextureId] : m_defaultTexture;
		m_sceneMaterials[i].KdTextureId = m_scene.materials[i].KdTextureId >= 0 ? m_textures[m_scene.materials[i].KdTextureId] : m_defaultTexture;
		m_sceneMaterials[i].KsTextureId = m_scene.materials[i].KsTextureId >= 0 ? m_textures[m_scene.materials[i].KsTextureId] : m_defaultTexture;
		m_sceneMaterials[i].shininessTextureId = m_scene.materials[i].shininessTextureId >= 0 ? m_textures[m_scene.materials[i].shininessTextureId] : m_defaultTexture;

	}
}

void Application::GeometryPassInit() {
	m_geopassProg = glmlv::compileProgram({ m_ShadersRootPath / m_AppName / "geometryPass.vs.glsl", m_ShadersRootPath / m_AppName / "geometryPass.fs.glsl" });

	m_uMVP = glGetUniformLocation(m_geopassProg.glId(), "uMVP");
	m_uMV = glGetUniformLocation(m_geopassProg.glId(), "uMV");
	m_uNormal = glGetUniformLocation(m_geopassProg.glId(), "uNormal");

	m_uKa = glGetUniformLocation(m_geopassProg.glId(), "uKa");
	m_uKd = glGetUniformLocation(m_geopassProg.glId(), "uKd");
	m_uKs = glGetUniformLocation(m_geopassProg.glId(), "uKs");
	m_uShiny = glGetUniformLocation(m_geopassProg.glId(), "uShiny");
	m_uKaTextureUnit = glGetUniformLocation(m_geopassProg.glId(), "uKaTextureUnit");
	m_uKdTextureUnit = glGetUniformLocation(m_geopassProg.glId(), "uKdTextureUnit");
	m_uKsTextureUnit = glGetUniformLocation(m_geopassProg.glId(), "uKsTextureUnit");
	m_uShinyTextureUnit = glGetUniformLocation(m_geopassProg.glId(), "uShinyTextureUnit");

}

void Application::ShadingPassInit() {
	
	m_shadepassProg = glmlv::compileProgram({ m_ShadersRootPath / m_AppName / "shadingPass.vs.glsl", m_ShadersRootPath / m_AppName / "shadingPass.fs.glsl" });
	/*
	m_uGBufferSamplerLocations[GPosition] = glGetUniformLocation(m_shadepassProg.glId(), "uGPosition");
	m_uGBufferSamplerLocations[GNormal] = glGetUniformLocation(m_shadepassProg.glId(), "uGNormal");
	m_uGBufferSamplerLocations[GAmbient] = glGetUniformLocation(m_shadepassProg.glId(), "uGAmbient");
	m_uGBufferSamplerLocations[GDiffuse] = glGetUniformLocation(m_shadepassProg.glId(), "uGDiffuse");
	m_uGBufferSamplerLocations[GGlossyShininess] = glGetUniformLocation(m_shadepassProg.glId(), "uGGlossyShininess");

	*/
	m_uDirLightDir = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightDir");
	m_uDirLightIntensity = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightIntensity");

	m_uPointLightPos = glGetUniformLocation(m_shadepassProg.glId(), "uPointLightPos");
	m_uPointLightIntensity = glGetUniformLocation(m_shadepassProg.glId(), "uPointLightIntensity");
	
}

void Application::initGBuffer()
{
	// Init GBuffer
	glGenTextures(GBufferTextureCount, m_GBufferTextures);

	for (int32_t i = GPosition; i < GBufferTextureCount; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, m_GBufferTextureFormat[i], m_nWindowWidth, m_nWindowHeight);
	}

	glGenFramebuffers(1, &m_GBufferFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBufferFBO);
	for (int32_t i = GPosition; i < GDepth; ++i)
	{
		glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, m_GBufferTextures[i], 0);
	}
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_GBufferTextures[GDepth], 0);

	// we will write into 5 textures from the fragment shader
	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
	glDrawBuffers(5, drawBuffers);

	GLenum status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cerr << "FB error, status: " << status << std::endl;
		throw std::runtime_error("FBO error");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

Application::Application(int argc, char** argv) :
	m_AppPath{ glmlv::fs::path{ argv[0] } },
	m_AppName{ m_AppPath.stem().string() },
	m_ImGuiIniFilename{ m_AppName + ".imgui.ini" },
	m_ShadersRootPath{ m_AppPath.parent_path() / "shaders" },
	m_AssetsRootPath{ m_AppPath.parent_path() / "assets" }
{
	ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

	// Put here initialization code
	InitDefaultMat();
	SceneLoading();
	GeometryPassInit();
	initGBuffer();
	//initScreenTriangle();
	// sampler
	glGenSamplers(1, &m_sampler);
	glSamplerParameteri(m_sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(m_sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glEnable(GL_DEPTH_TEST);
}

void Application::initScreenTriangle()
{
	glGenBuffers(1, &m_triangleVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_triangleVBO);

	GLfloat data[] = { -1, -1, 3, -1, -1, 3 };
	glBufferStorage(GL_ARRAY_BUFFER, sizeof(data), data, 0);

	glGenVertexArrays(1, &m_triangleVAO);
	glBindVertexArray(m_triangleVAO);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}