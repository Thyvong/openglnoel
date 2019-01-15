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

	glm::mat4 View = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 ModelView = View * glm::rotate(glm::translate(glm::mat4(1), glm::vec3(-2, 0, 0)), 0.2f, glm::vec3(0, 1, 0));
	glm::mat4 ModelViewProj = glm::perspective(glm::radians(70.f), float(viewportSize.x) / viewportSize.y, 0.01f, 100.f) * ModelView;
	glm::mat4 Normal = glm::transpose(glm::inverse(ModelView));
	
	glUniformMatrix4fv(uModelViewMatrix, 1, GL_FALSE, glm::value_ptr(ModelView));
	glUniformMatrix4fv(uModelViewProjMatrix, 1, GL_FALSE, glm::value_ptr(ModelViewProj));
	glUniformMatrix4fv(uNormalMatrix, 1, GL_FALSE, glm::value_ptr(Normal));
    // Loop until the user closes the window
    for (auto iterationCount = 0u; !m_GLFWHandle.shouldClose(); ++iterationCount)
    {
        const auto seconds = glfwGetTime();

        // Put here rendering code
		const auto fbSize = m_GLFWHandle.framebufferSize();
		glViewport(0, 0, fbSize.x, fbSize.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		

		if (0) {
			glBindVertexArray(vaocube);
			glDrawElements(GL_TRIANGLES, cube.indexBuffer.size(),GL_UNSIGNED_INT,0);
			glBindVertexArray(0);
		}
		else {
			glBindVertexArray(vaosphere);
			glDrawElements(GL_TRIANGLES, sphere.indexBuffer.size(),GL_UNSIGNED_INT,0);
			glBindVertexArray(0);
		}

		
        // GUI code:
		glmlv::imguiNewFrame();

        {
            ImGui::Begin("GUI");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

		glmlv::imguiRenderFrame();

        glfwPollEvents(); // Poll for and process events

        auto ellapsedTime = glfwGetTime() - seconds;
        auto guiHasFocus = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
        if (!guiHasFocus) {
            // Put here code to handle user interactions
        }

		m_GLFWHandle.swapBuffers(); // Swap front and back buffers
    }
	// delete les buffers
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
	vertexShader = m_ShadersRootPath / m_AppName / "forward.vs.glsl";
	fragmentShader = m_ShadersRootPath / m_AppName / "forward.fs.glsl";

	program = glmlv::compileProgram({vertexShader,fragmentShader});
	program.use();
	
	uModelViewProjMatrix = glGetUniformLocation(program.glId(), "uModelViewProjMatrix");
	uModelViewMatrix = glGetUniformLocation(program.glId(), "uModelViewMatrix");
	uNormalMatrix = glGetUniformLocation(program.glId(), "uNormalMatrix");
	
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