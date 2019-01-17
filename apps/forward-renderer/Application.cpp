#include "Application.hpp"

#include <iostream>
#include "glmlv/simple_geometry.hpp"
#include "glmlv/filesystem.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int Application::run()
{
	glClearColor(1, 0, 0, 1);
	// Put here code to run before rendering loop
	const auto viewportSize = m_GLFWHandle.framebufferSize();
	glViewport(0, 0, viewportSize.x, viewportSize.y);

	bool test=false;

	glm::mat4 ModelCube = glm::translate(glm::mat4(1), glm::vec3(-2, 0, 0));
	glm::mat4 ModelSphere = glm::translate(glm::mat4(1), glm::vec3(2, 0, 0));
	glm::mat4 ModelViewProj;
	glm::mat4 Normal;
	
	glm::vec3 Kd = glm::vec3(1, 1, 1);
	glm::vec3 DirectionalLightDir = glm::vec3(2, 0, 0);
	glm::vec3 DirectionalLightIntensity = glm::vec3(1,1,1);
	glm::vec3 PointLightPosition = glm::vec3(2, 0, 0);
	glm::vec3 PointLightIntensity = glm::vec3(1, 1, 1);

	glmlv::ViewController camera = glmlv::ViewController(m_GLFWHandle.window());
	camera.setSpeed(5.0f);
	camera.setViewMatrix(glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));
	

	glUniform3f(uKd, Kd.x,Kd.y,Kd.z);
	glUniform3f(uDirectionalLightDir, DirectionalLightDir.x, DirectionalLightDir.y, DirectionalLightDir.z);
	glUniform3f(uDirectionalLightIntensity, DirectionalLightIntensity.x, DirectionalLightIntensity.y, DirectionalLightIntensity.z);
	glUniform3f(uPointLightPosition, PointLightPosition.x, PointLightPosition.y, PointLightPosition.z);
	glUniform3f(uPointLightIntensity, PointLightIntensity.x, PointLightIntensity.y, PointLightIntensity.z);
	glUniform3f(uPointLightIntensity, PointLightIntensity.x, PointLightIntensity.y, PointLightIntensity.z);



	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uKdSampler, 0); // Set the uniform to 0 because we use texture unit 0
	glBindSampler(0, sampler);

    // Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
    {
        const auto seconds = glfwGetTime();

        // Put here rendering code
		//const auto fbSize = m_GLFWHandle.framebufferSize();
		//glViewport(0, 0, fbSize.x, fbSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ModelViewProj = 
			glm::perspective(glm::radians(70.f), float(viewportSize.x) / viewportSize.y, 0.01f, 100.f) 
			* camera.getViewMatrix()
			* ModelCube;
		Normal = glm::transpose(glm::inverse(camera.getViewMatrix()* ModelCube));
		
		glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()* ModelCube));
		glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(ModelViewProj));
		glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(Normal));
		
		glBindTexture(GL_TEXTURE_2D, texobject1);

		glBindVertexArray(vaocube);
		glDrawElements(GL_TRIANGLES, cube.indexBuffer.size(),GL_UNSIGNED_INT,0);
		glBindVertexArray(0);
		
		glBindTexture(GL_TEXTURE_2D, 0);

		ModelViewProj =
			glm::perspective(glm::radians(70.f), float(viewportSize.x) / viewportSize.y, 0.01f, 100.f)
			* camera.getViewMatrix()
			* ModelSphere;
		Normal = glm::transpose(glm::inverse(camera.getViewMatrix()* ModelSphere));

		glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()* ModelSphere));
		glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(ModelViewProj));
		glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(Normal));

		glBindTexture(GL_TEXTURE_2D, texobject2);

		glBindVertexArray(vaosphere);
		glDrawElements(GL_TRIANGLES, sphere.indexBuffer.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
		glBindTexture(GL_TEXTURE_2D, 0);

		
		
        // GUI code:
		glmlv::imguiNewFrame();

        {
            ImGui::Begin("GUI");
			
			if (ImGui::CollapsingHeader("Directional Light"))
			{
				ImGui::DragFloat("DirLightIntensity", &DirectionalLightIntensity[0], 0.1f, 0.f, 100.f);
				ImGui::DragFloat("Phi Angle", &DirectionalLightDir[1], 1.0f, 0.0f, 360.f);
				ImGui::DragFloat("Theta Angle", &DirectionalLightDir[2], 1.0f, 0.0f, 180.f);
				
			}

			if (ImGui::CollapsingHeader("Point Light"))
			{
				ImGui::DragFloat("PointLightIntensity", &PointLightIntensity[0], 0.1f, 0.f, 16000.f);
				ImGui::InputFloat3("Position", glm::value_ptr(PointLightPosition));
			}

			if (ImGui::CollapsingHeader("Materials"))
			{
				ImGui::ColorEdit3("Cube Kd", glm::value_ptr(Kd));
				ImGui::ColorEdit3("Sphere Kd", glm::value_ptr(Kd));
			}

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

		glmlv::imguiRenderFrame();

        glfwPollEvents(); // Poll for and process events

        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
			// Put here code to handle user interactions
			camera.update(float(ellapsedTime));
        }

		m_GLFWHandle.swapBuffers(); // Swap front and back buffers
    }
	// delete les buffers
	glBindSampler(0, 0);

	glDeleteBuffers(1, &vbocube);
	glDeleteVertexArrays(1, &vaocube);

	glDeleteBuffers(1, &vbosphere);
	glDeleteVertexArrays(1, &vaosphere);

	/* ???
	glBufferStorage();
	glDisableVertexAttribArray();
	*/
    return 0;
}

Application::Application(int argc, char** argv):
    m_AppPath { glmlv::fs::path{ argv[0] } },
    m_AppName { m_AppPath.stem().string() },
    m_ImGuiIniFilename { m_AppName + ".imgui.ini" },
    m_ShadersRootPath { m_AppPath.parent_path() / "shaders" }
	

{
    ImGui::GetIO().IniFilename = m_ImGuiIniFilename.c_str(); // At exit, ImGUI will store its windows positions in this file
	
    // Put here initialization code
	glmlv::fs::path vertexShader,fragmentShader;
	glmlv::fs::path texpath1, texpath2;

	vertexShader = m_ShadersRootPath / m_AppName / "forward.vs.glsl";
	fragmentShader = m_ShadersRootPath / m_AppName / "forward.fs.glsl";
	texpath1 = m_AppPath.parent_path() / "assets" / m_AppName  /  "textures" /"tex1.png";
	texpath2 = m_AppPath.parent_path() / "assets" /  m_AppName  / "textures" / "tex2.jpg";
	texture1 = glmlv::readImage(texpath1);
	texture2 = glmlv::readImage(texpath2);
	program = glmlv::compileProgram({vertexShader,fragmentShader});
	program.use();
	
	uModelViewProjMatrix = glGetUniformLocation(program.glId(), "uModelViewProjMatrix");
	uModelViewMatrix = glGetUniformLocation(program.glId(), "uModelViewMatrix");
	uNormalMatrix = glGetUniformLocation(program.glId(), "uNormalMatrix");

	uKd = glGetUniformLocation(program.glId(), "uKd");
	uDirectionalLightDir = glGetUniformLocation(program.glId(), "uDirectionalLightDir"); 
	uDirectionalLightIntensity = glGetUniformLocation(program.glId(), "uDirectionalLightIntensity");
	uPointLightPosition = glGetUniformLocation(program.glId(), "uPointLightPosition");
	uPointLightIntensity = glGetUniformLocation(program.glId(), "uPointLightIntensity");
	
	uKdSampler = glGetUniformLocation(program.glId(), "uKdSampler");
	

	glGenTextures(1,&texobject1);
	glGenTextures(1,&texobject2);

	glActiveTexture(GL_TEXTURE0);

	glGenTextures(1, &texobject1);
	glBindTexture(GL_TEXTURE_2D, texobject1);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, texture1.width(), texture1.height());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture1.width(), texture1.height(), GL_RGBA, GL_UNSIGNED_BYTE, texture1.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenTextures(1, &texobject2);
	glBindTexture(GL_TEXTURE_2D, texobject2);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, texture2.width(), texture2.height());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture2.width(), texture2.height(), GL_RGBA, GL_UNSIGNED_BYTE, texture2.data());
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenSamplers(1, &sampler);
	glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// VBO
	
	// remplir les points
	
	glGenBuffers(1,&vbocube);
	glGenBuffers(1,&vbosphere);
	// binding des vbos
	// vbo du triangle
	glBindBuffer(GL_ARRAY_BUFFER,vbocube);
	glBufferData(GL_ARRAY_BUFFER, cube.vertexBuffer.size() * sizeof(glmlv::Vertex3f3f2f), &cube.vertexBuffer[0] ,GL_STATIC_DRAW );
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// vbo de la sphere
	glBindBuffer(GL_ARRAY_BUFFER, vbosphere);
	glBufferData(GL_ARRAY_BUFFER, sphere.vertexBuffer.size() * sizeof(glmlv::Vertex3f3f2f), &sphere.vertexBuffer[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// IBO
	glGenBuffers(1, &ibocube);
	glGenBuffers(1, &ibosphere);
	
	
	// ibo du triangle
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibocube);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, cube.indexBuffer.size() * sizeof(uint32_t), &cube.indexBuffer[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	// ibo de la sphere
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibosphere);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphere.indexBuffer.size() * sizeof(uint32_t), &sphere.indexBuffer[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// VAO

	glGenVertexArrays(1,&vaocube);
	glGenVertexArrays(1,&vaosphere);

	// vao de triangle
	glBindVertexArray(vaocube);
	// bind ibo
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibocube);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	// le vbo de triangle est rappelé pour l'attribution des specs
	glBindBuffer(GL_ARRAY_BUFFER, vaocube);
	// attributs
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) offsetof(glmlv::Vertex3f3f2f, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) offsetof(glmlv::Vertex3f3f2f, normal));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*) offsetof(glmlv::Vertex3f3f2f, texCoords));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// debind de vao
	glBindVertexArray(0);

	// vao de sphere
	glBindVertexArray(vaosphere);
	// bind ibo
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibosphere);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	// le vbo de sphere est rappelé pour l'attribution des specs
	glBindBuffer(GL_ARRAY_BUFFER, vaosphere);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)offsetof(glmlv::Vertex3f3f2f, position));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)offsetof(glmlv::Vertex3f3f2f, normal));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)offsetof(glmlv::Vertex3f3f2f, texCoords));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// debind de vao
	glBindVertexArray(0);
	
	glEnable(GL_DEPTH_TEST); 



}