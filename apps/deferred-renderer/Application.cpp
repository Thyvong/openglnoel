#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gltf_method.hpp"

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
	
	int indexOffset = 0;
	const auto sceneCenter = 0.5f * (m_scene.bboxMin + m_scene.bboxMax);
	const float sceneRadius = scenesize * 0.5f;


	glm::vec3 DirLightColor = glm::vec3(1);
	float DirLightIntensity = 20;
	float DirLightPhiAngleDegrees =40;
	float DirLightThetaAngleDegrees =40;
	glm::vec3 DirLightDirection = computeDirectionVector(glm::radians(DirLightPhiAngleDegrees), glm::radians(DirLightThetaAngleDegrees));
	
	glm::vec3 PointLightColor = glm::vec3(1);
	float PointLightIntensity =40;
	glm::vec3 PointLightPosition = glm::vec3(1,1,1);

	GBufferTextureType current_display = GPosition;
	int fullmode = 1;

	bool recalculateSMDir = true; // init à true pour le 1er calcul
	glm::mat4 dirLightProjMatrix = glm::mat4(1);
	glm::mat4 dirLightViewMatrix = glm::mat4(1);

	glm::vec3 dirLightUpVector = glm::vec3(1);
	glm::mat4 DirLightViewProjMatrix = glm::mat4(1);
	glm::mat4 DirLightViewProjMatrix_shadingpass = glm::mat4(1);
	float shadowBias = 0.5;

	// closest filtering
	int DirLightSMSampleCount = 16;
	float DirLightSMSpread = 0.0005f;

	// Loop until the user closes the window
	for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
	{
		const auto seconds = glfwGetTime();

		
		dirLightUpVector = computeDirectionVectorUp(glm::radians(DirLightPhiAngleDegrees), glm::radians(DirLightThetaAngleDegrees));
		dirLightViewMatrix = glm::lookAt(sceneCenter + DirLightDirection * sceneRadius, sceneCenter, dirLightUpVector); // Will not work if m_DirLightDirection is colinear to lightUpVector
		dirLightProjMatrix = glm::ortho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius, 0.01f * sceneRadius, 2.f * sceneRadius);

		if (recalculateSMDir) {

			// recalcul
			m_directionalSMProgram.use();

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
			glViewport(0, 0, m_nDirectionalSMResolution, m_nDirectionalSMResolution);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			DirLightViewProjMatrix = dirLightProjMatrix * dirLightViewMatrix;
			glUniformMatrix4fv(m_uDirLightViewProjMatrix, 1, GL_FALSE, glm::value_ptr(DirLightViewProjMatrix));
			
			glBindVertexArray(m_vao);

			indexOffset = 0;
			for (int i = 0; i < m_scene.shapeCount; i++)
			{
				glDrawElements(GL_TRIANGLES, m_scene.indexCountPerShape[i], GL_UNSIGNED_INT, (const GLvoid*)(indexOffset * sizeof(GLuint)));
				indexOffset += m_scene.indexCountPerShape[i];
			}

			glBindVertexArray(0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			recalculateSMDir = false;
		}
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
			View = camera.getViewMatrix();
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
		
		// pour éviter le vacillement et faire un refresh
		const auto viewportSize = m_GLFWHandle.framebufferSize();
		glViewport(0, 0, viewportSize.x, viewportSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		if (fullmode==1) {
			m_shadepassProg.use();

			glUniform3fv(m_uDirLightDir, 1, glm::value_ptr(glm::vec3(View * glm::vec4(glm::normalize(DirLightDirection), 0))));
			glUniform3fv(m_uDirLightIntensity, 1, glm::value_ptr(DirLightColor * DirLightIntensity));

			glUniform3fv(m_uPointLightPos, 1, glm::value_ptr(glm::vec3(View * glm::vec4(PointLightPosition, 1))));
			glUniform3fv(m_uPointLightIntensity, 1, glm::value_ptr(PointLightColor * PointLightIntensity));

			glUniform1fv(m_uDirLightShadowMapBias, 1, &shadowBias);

			glUniform1iv(m_uDirLightShadowMapSampleCount, 1, &DirLightSMSampleCount);
			glUniform1fv(m_uDirLightShadowMapSpread, 1, &DirLightSMSpread);


			DirLightViewProjMatrix_shadingpass = dirLightProjMatrix * dirLightViewMatrix;
			const auto rcpViewMatrix = camera.getRcpViewMatrix(); // Inverse de la view matrix de la caméra
			glUniformMatrix4fv(m_uDirLightViewProjMatrix_shadingpass, 1, GL_FALSE, glm::value_ptr(DirLightViewProjMatrix_shadingpass * rcpViewMatrix));

			for (int i = GPosition; i < GDepth; ++i)
			{
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(GL_TEXTURE_2D, m_GBufferTextures[i]);

				glUniform1i(m_uGBuffer[i], i);
			}
			// shadow map texture can now be used through gdepth entry
			glActiveTexture(GL_TEXTURE0 + GDepth);
			glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
			glBindSampler(GDepth, m_directionalSMSampler);
			glUniform1i(m_uDirLightShadowMap, GDepth);

			glBindVertexArray(m_triangleVAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			glBindVertexArray(0);

		}
		else {
			// GBuffer display
			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_GBufferFBO);
			glReadBuffer(GL_COLOR_ATTACHMENT0 + current_display);
			glBlitFramebuffer(0, 0, m_nWindowWidth, m_nWindowHeight,
			0, 0, m_nWindowWidth, m_nWindowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			
		}
		
		



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
				if (ImGui::DragFloat("Phi Angle", &DirLightPhiAngleDegrees, 1.0f, 0.0f, 360.f) 
					||
					ImGui::DragFloat("Theta Angle", &DirLightThetaAngleDegrees, 1.0f, 0.0f, 180.f)
					||
					ImGui::DragInt("SampleCount", &DirLightSMSampleCount, 1, 0, 16)
					||
					ImGui::DragFloat("ShadowSpread", &DirLightSMSpread, 0.00001f, 0.0f, 0.0005f)
					||
					ImGui::InputFloat("DirShadowMap Bias", &shadowBias, 1, 0, 100)

					)

				{
					DirLightDirection = computeDirectionVector(glm::radians(DirLightPhiAngleDegrees), glm::radians(DirLightThetaAngleDegrees));
					recalculateSMDir = true;
				}
			}

			if (ImGui::CollapsingHeader("Point Light"))
			{
				ImGui::ColorEdit3("PointLightColor", glm::value_ptr(PointLightColor));
				ImGui::DragFloat("PointLightIntensity", &PointLightIntensity, 0.1f, 0.f, 16000.f);
				ImGui::InputFloat3("Position", glm::value_ptr(PointLightPosition));
			}
			ImGui::DragInt("Fullmode = 1, GB-Blitter = 0", &fullmode, 1, 0, 1);
			if (ImGui::CollapsingHeader("GBuffer"))
			{
				for (int32_t i = GPosition; i <= GBufferTextureCount; ++i)
				{
					if (ImGui::RadioButton(m_GBufferTexNames[i], current_display == i))
						current_display = GBufferTextureType(i);
				}
			}
			
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
	std::cout << objPath << std::endl;
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

void Application::SceneLoadingGLTF() {

	gltf_method sceneloader;
	const auto objPath = m_AssetsRootPath / "sponza.obj";
	sceneloader.loadModel(m_model, objPath.string);

	m_gltfvao = sceneloader.bindModel(m_model);
}

void Application::GeometryPassInit() {
	m_geopassProg = glmlv::compileProgram({ m_ShadersRootPath / "geometryPass.vs.glsl", m_ShadersRootPath / "geometryPass.fs.glsl" });

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
	
	m_shadepassProg = glmlv::compileProgram({ m_ShadersRootPath  / "shadingPass.vs.glsl", m_ShadersRootPath / "shadingPass.fs.glsl" });
	
	m_uGBuffer[GPosition] = glGetUniformLocation(m_shadepassProg.glId(), "uGPosition");
	m_uGBuffer[GNormal] = glGetUniformLocation(m_shadepassProg.glId(), "uGNormal");
	m_uGBuffer[GAmbient] = glGetUniformLocation(m_shadepassProg.glId(), "uGAmbient");
	m_uGBuffer[GDiffuse] = glGetUniformLocation(m_shadepassProg.glId(), "uGDiffuse");
	m_uGBuffer[GGlossyShininess] = glGetUniformLocation(m_shadepassProg.glId(), "uGGlossyShininess");

	m_uDirLightDir = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightDir");
	m_uDirLightIntensity = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightIntensity");

	m_uPointLightPos = glGetUniformLocation(m_shadepassProg.glId(), "uPointLightPos");
	m_uPointLightIntensity = glGetUniformLocation(m_shadepassProg.glId(), "uPointLightIntensity");
	m_uDirLightViewProjMatrix_shadingpass = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightViewProjMatrix");

	m_uDirLightShadowMapSampleCount = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightShadowMapSampleCount");
	m_uDirLightShadowMapSpread = glGetUniformLocation(m_shadepassProg.glId(), "uDirLightShadowMapSpread"); 
	
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

void Application::initShadowMapDir() {
	
	glGenTextures(1, &m_directionalSMTexture);

	glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT32F, m_nDirectionalSMResolution, m_nDirectionalSMResolution);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &m_directionalSMFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_directionalSMFBO);
	glBindTexture(GL_TEXTURE_2D, m_directionalSMTexture);
	glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_directionalSMTexture, 0);

	const auto fboStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cerr << "Error on building directional shadow mapping framebuffer. Error code = " << fboStatus << std::endl;
		throw std::runtime_error("Error on building directional shadow mapping framebuffer.");
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glGenSamplers(1, &m_directionalSMSampler);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glSamplerParameteri(m_directionalSMSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

}

void Application::initShadowMapDirShaders() {

	m_directionalSMProgram = glmlv::compileProgram({ m_ShadersRootPath / "directionalSM.vs.glsl", m_ShadersRootPath / "directionalSM.fs.glsl" });
	m_uDirLightViewProjMatrix =  glGetUniformLocation(m_geopassProg.glId(), "uDirLightViewProjMatrix");
	m_uDirLightShadowMap = glGetUniformLocation(m_geopassProg.glId(), "uDirLightShadowMap");
	m_uDirLightShadowMapBias = glGetUniformLocation(m_geopassProg.glId(), "uDirLightShadowMapBias");
	
	

}

Application::Application(int argc, char** argv) :
	m_AppPath{ glmlv::fs::path{ argv[0] } },
	m_AppName{ m_AppPath.stem().string() },
	m_ImGuiIniFilename{ m_AppName + ".imgui.ini" },
	m_ShadersRootPath{ m_AppPath.parent_path() / "shaders" / m_AppName },
	m_AssetsRootPath{ m_AppPath.parent_path() / "assets" / m_AppName }
{
	ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file

	// Put here initialization code
	InitDefaultMat();
	SceneLoading();
	// deferred
	GeometryPassInit();
	ShadingPassInit();
	initScreenTriangle();
	initGBuffer();
	// shadowmap
	initShadowMapDir();
	initShadowMapDirShaders();

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