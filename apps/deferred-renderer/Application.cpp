#include "Application.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "gltf_method.hpp"

int Application::run()
{
	// Camera
	glmlv::ViewController camera{ m_GLFWHandle.window(), 3.f };
	float scenesize = 10;
	camera.setSpeed(scenesize*0.1f);
	GLfloat clearColor[3] = { 0.4, 0.4, 0.4 };
	
	// Put here code to run before rendering loop
	glm::mat4 Proj = glm::perspective(70.f, float(m_nWindowWidth) / m_nWindowHeight, 0.01f * scenesize, scenesize);
	glm::mat4 View  = camera.getViewMatrix();
	glm::mat4 Normal;
	glm::mat4 Model= glm::mat4(1);
	glm::mat4 MV;
	glm::mat4 MVP;
	
	int indexOffset = 0;
	const auto sceneCenter = 0.5f * (glm::vec3(0));
	const float sceneRadius = scenesize * 0.5f;


	glm::vec3 DirLightColor = glm::vec3(1);
	float DirLightIntensity = 1;
	float DirLightPhiAngleDegrees =40;
	float DirLightThetaAngleDegrees =40;
	glm::vec3 DirLightDirection = computeDirectionVector(glm::radians(DirLightPhiAngleDegrees), glm::radians(DirLightThetaAngleDegrees));
	
	glm::vec3 PointLightColor = glm::vec3(1);
	float PointLightIntensity =5;
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
		glClearColor(clearColor[0], clearColor[1], clearColor[2],1);
		
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
			
			glBindSampler(0, m_sampler);


			drawModel(m_gltfvao, m_model);


			glBindSampler(0, 0);

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

			recalculateSMDir = false;
		}
		// Put here rendering code

		m_geopassProg.use();
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_GBufferFBO);

		glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		// pour l'instant on a qu'une texture à gérer (la diffuse), donc on a besoin que d'un slot/texture unit
		// on utilise la texture unit 0 GL_TEXTURE0
		//for (GLuint i : {0, 1, 2, 3})
			glBindSampler(0, m_sampler);

		// Set texture unit of sampler
		glUniform1i(m_uDiffusTex, 0);

		glBindVertexArray(m_gltfvao);
		int indexOffset = 0;
		
			View = camera.getViewMatrix();
			MV = View * Model;
			MVP = Proj * MV;
			Normal = glm::transpose(glm::inverse(MV));

			glUniformMatrix4fv(m_uMVP, 1, GL_FALSE, glm::value_ptr(MVP));
			glUniformMatrix4fv(m_uMV, 1, GL_FALSE, glm::value_ptr(MV));
			glUniformMatrix4fv(m_uNormal, 1, GL_FALSE, glm::value_ptr(Normal));

			
			// le bidning est dans la méthode
			drawModel(m_gltfvao, m_model);
		
		//for (GLuint i : {0, 1, 2, 3})
			glBindSampler(0, 0);

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
				glBindSampler(i, m_sampler);
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

			for (int i = GPosition; i < GDepth; ++i) {
				glBindSampler(i, 0);
			}

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
	m_defaultMat.diffus = glm::vec4(1);
	m_defaultMat.emission = glm::vec3(1);
	m_defaultMat.texId = m_defaultTexture;

}
void Application::SceneLoadingGLTF(int argc, char** argv) {

	auto objPath = m_AssetsRootPath / "Avocado.gltf";
	if (argc > 1) {
		objPath = argv[1];
		std::cout << argv[1] << std::endl;

	}
	
	std::string path = objPath.string();
	std::cout << objPath.string().c_str() << std::endl;
	loadModel(m_model, path.c_str());

	m_gltfvao = bindModel(m_model);
	InitMats(m_model);
	std::cout << "init done ?" << std::endl;
	int i = 0;
	for (auto tex : m_textures) {
		std::cout << " text"<< i << " : " << tex << std::endl;
		i++;
	}
}
// modifier les shaders
void Application::GeometryPassInit() {
	m_geopassProg = glmlv::compileProgram({ m_ShadersRootPath / "geometryPass.vs.glsl", m_ShadersRootPath / "geometryPass.fs.glsl" });

	m_uMVP = glGetUniformLocation(m_geopassProg.glId(), "uMVP");
	m_uMV = glGetUniformLocation(m_geopassProg.glId(), "uMV");
	m_uNormal = glGetUniformLocation(m_geopassProg.glId(), "uNormal");

	m_uDiffus = glGetUniformLocation(m_geopassProg.glId(), "uDiffus");
	m_uDiffusTex = glGetUniformLocation(m_geopassProg.glId(), "uDiffusTex");
	m_uEmission = glGetUniformLocation(m_geopassProg.glId(), "uEmission");

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
	SceneLoadingGLTF( argc,  argv);
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



#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#define BUFFER_OFFSET(i) ((char *)NULL + (i))
/*
void SceneLoadingGLTF(glmlv::fs::path path) {

tinygltf::Model model;
const auto objPath =path.string;
loadModel(model, objPath);

std::vector<GLuint> buffers(model.buffers.size()); // un par tinygltf::Buffer

// ibo ou vbo ?
glGenBuffers(buffers.size(), buffers.data());
for (int i =0 ; i<buffers.size();i++)
{
glBindBuffer(GL_ARRAY_BUFFER, buffers[i]);
glBufferStorage(GL_ARRAY_BUFFER, model.buffers[i].data.size() * sizeof(unsigned char), model.buffers[i].data.data(), 0);
glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::vector<GLuint> vaos;
std::vector<tinygltf::Primitive> primitives; // Pour chaque VAO on va aussi stocker les données de la primitive associé car on doit l'utiliser lors du rendu

std::map<std::string, int> attribIndexOf{ {"POSITION", 0}, {"NORMAL", 1}, {"TEXCOORDS", 2} };
std::map<int, int> numberOfComponentOf{
{ TINYGLTF_TYPE_SCALAR , 1 },
{ TINYGLTF_TYPE_VEC2 , 2 },
{ TINYGLTF_TYPE_VEC3 , 3},
{ TINYGLTF_TYPE_VEC4 , 4},
{ TINYGLTF_TYPE_MAT2 , 4 },
{ TINYGLTF_TYPE_MAT3 , 9 },
{ TINYGLTF_TYPE_MAT4 , 16 }
};

for (int i = 0; i < model.meshes.size(); i++)
{
for (int j = 0; j < model.meshes[i].primitives.size(); j++)
{
GLuint vaoId;
glGenVertexArrays(1,&vaoId);
glBindVertexArray(vaoId);
// quel accessor contient notre primitive
tinygltf::Accessor indexAccessor = model.accessors[model.meshes[i].primitives[j].indices];
// quel bufferview contient notre primitive
tinygltf::BufferView bufferView = model.bufferViews[indexAccessor.bufferView];
// quel buffer du bufferview
int bufferIndex = bufferView.buffer;
// on bind le vbo qui nous intéresse
glBindBuffer(GL_ARRAY_BUFFER, buffers[bufferIndex]);
// pour chaque attributs
for (const auto entry : model.meshes[i].primitives[j].attributes)
{
// on cherche de nouveau quel accesor/bufferview/buffer contient
indexAccessor = model.accessors[entry.second]; // key est "POSITION", ou "NORMAL", ou autre (voir l'image de spec du format)
bufferView = model.bufferViews[indexAccessor.bufferView];
bufferIndex = bufferView.buffer;
//Ici je suppose qu'on a prérempli une map attribIndexOf
//qui associe aux strings genre "POSITION" un index d'attribut du vertex shader
//(les location = XXX du vertex shader); dans les TPs on utilisait 0 pour position, 1 pour normal et 2 pour tex coords

glEnableVertexAttribArray(attribIndexOf[entry.first]);
// Ici encore il faut avoir remplit une map numberOfComponentOf
//qui associe un type gltf (comme "VEC2")
//au nombre de composantes (2 pour "VEC2", 3 pour "VEC3")

glVertexAttribPointer(attribIndexOf[entry.first], numberOfComponentOf[indexAccessor.type], indexAccessor.componentType, bufferView.byteStride, (const void*)(bufferView.byteOffset + indexAccessor.byteOffset));

}
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
glBindVertexArray(0);
vaos.push_back(vaoId);
primitives.push_back(model.meshes[i].primitives[j]);
}
}

}
*/

bool Application::loadModel(tinygltf::Model &model, const char *filename) {
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	if (!warn.empty()) {
		std::cout << "WARN: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cout << "ERR: " << err << std::endl;
	}

	if (!res)
		std::cout << "Failed to load glTF: " << filename << std::endl;
	else
		std::cout << "Loaded glTF: " << filename << std::endl;

	return res;
}

std::map<int, GLuint> Application::bindMesh(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Mesh &mesh) {
	for (size_t i = 0; i < model.bufferViews.size(); ++i) {
		const tinygltf::BufferView &bufferView = model.bufferViews[i];
		if (bufferView.target == 0) {
			std::cout << "WARN: bufferView.target is zero - Unsupported bufferView" << std::endl;
			continue;
		}

		tinygltf::Buffer buffer = model.buffers[bufferView.buffer];
		std::cout << "bufferview.target " << bufferView.target << std::endl;

		GLuint vbo;
		glGenBuffers(1, &vbo);
		vbos[i] = vbo;
		glBindBuffer(bufferView.target, vbo);

		std::cout << "buffer.data.size = " << buffer.data.size()
			<< ", bufferview.byteOffset = " << bufferView.byteOffset
			<< std::endl;

		glBufferData(bufferView.target, bufferView.byteLength,
			&buffer.data.at(0) + bufferView.byteOffset, GL_STATIC_DRAW);
	}

	for (size_t i = 0; i < mesh.primitives.size(); ++i) {
		tinygltf::Primitive primitive = mesh.primitives[i];
		tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

		for (auto &attrib : primitive.attributes) {
			tinygltf::Accessor accessor = model.accessors[attrib.second];
			int byteStride =
				accessor.ByteStride(model.bufferViews[accessor.bufferView]);
			glBindBuffer(GL_ARRAY_BUFFER, vbos[accessor.bufferView]);

			int size = 1;
			if (accessor.type != TINYGLTF_TYPE_SCALAR) {
				size = accessor.type;
			}

			int vaa = -1;
			if (attrib.first.compare("POSITION") == 0) vaa = 0;
			if (attrib.first.compare("NORMAL") == 0) vaa = 1;
			if (attrib.first.compare("TEXCOORD_0") == 0) vaa = 2;
			if (vaa > -1) {
				glEnableVertexAttribArray(vaa);
				glVertexAttribPointer(vaa, size, accessor.componentType,
					accessor.normalized ? GL_TRUE : GL_FALSE,
					byteStride, BUFFER_OFFSET(accessor.byteOffset));
			}
			else
				std::cout << "vaa missing: the following attributes is not implemented yet - " << attrib.first << std::endl;
		}

		GLuint texid;
		glGenTextures(1, &texid);

		tinygltf::Texture &tex = model.textures[0];
		tinygltf::Image &image = model.images[tex.source];

		glBindTexture(GL_TEXTURE_2D, texid);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLenum format = GL_RGBA;

		if (image.component == 1) {
			format = GL_RED;
		}
		else if (image.component == 2) {
			format = GL_RG;
		}
		else if (image.component == 3) {
			format = GL_RGB;
		}
		else {
			// ???
		}

		GLenum type = GL_UNSIGNED_BYTE;
		/*
		if (image.bits == 16) {
		type = GL_UNSIGNED_SHORT;
		}
		*/
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
			format, type, &image.image.at(0));

		// lets try to save this texture to our existing template
	}

	return vbos;
}

void Application::bindModelNodes(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Node &node) {
	
	if(node.mesh != -1)
		bindMesh(vbos, model, model.meshes[node.mesh]);
	for (size_t i = 0; i < node.children.size(); i++) {
		bindModelNodes(vbos, model, model.nodes[node.children[i]]);
	}
}

GLuint Application::bindModel(tinygltf::Model &model) {
	std::map<int, GLuint> vbos;
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	const tinygltf::Scene &scene = model.scenes[model.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		bindModelNodes(vbos, model, model.nodes[scene.nodes[i]]);
	}

	glBindVertexArray(0);
	// cleanup vbos
	for (size_t i = 0; i < vbos.size(); ++i) {
		glDeleteBuffers(1, &vbos[i]);
	}

	return vao;
}

void Application::InitMats(tinygltf::Model &model) {

	m_textures = std::vector<GLuint>(model.textures.size());
	m_gltfMaterials = std::vector<PBRMat>(model.materials.size());


	// storing textures
	for (int i = 0; i < model.textures.size(); i++) {
		// tinygltf textures info are separated in "Texture" objects and "Image" Object
		tinygltf::Image tex = model.images[model.textures[i].source];

		glGenTextures(1, &m_textures[i]);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, tex.width, tex.height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.width, tex.height, GL_RGBA, GL_UNSIGNED_BYTE, &tex.image.at(0));
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// storing materials
	for (int i = 0; i < model.materials.size(); i++) {
		tinygltf::ParameterMap material = model.materials[i].values;

		m_gltfMaterials[i].diffus = (material.count("baseColorFactor")) ? glm::make_vec4(material["baseColorFactor"].number_array.data()) : glm::vec4(1);
		m_gltfMaterials[i].texId = (material.count("baseColorTexture")) ? m_textures[material["baseColorTexture"].TextureIndex()] : -1;
		m_gltfMaterials[i].emission = (material.count("emissiveFactor")) ? glm::make_vec3(material["emissiveFactor"].number_array.data()) : glm::vec3(0);

	}
}


void Application::drawMesh(tinygltf::Model &model, tinygltf::Mesh &mesh) {
	for (size_t i = 0; i < mesh.primitives.size(); ++i) {
		std::cout << "drawing : " << mesh.name << std::endl;
		tinygltf::Primitive primitive = mesh.primitives[i];
		tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

		glUniform4fv(m_uDiffus, 1, glm::value_ptr(m_gltfMaterials[primitive.material].diffus));
		//sur quelle texture unit binder ?
		glActiveTexture(GL_TEXTURE0);
		GLint id = (m_gltfMaterials[primitive.material].texId == -1)? m_defaultTexture : m_gltfMaterials[primitive.material].texId;
		
		glBindTexture(GL_TEXTURE_2D, id);


		glDrawElements(primitive.mode, indexAccessor.count,
			indexAccessor.componentType,
			BUFFER_OFFSET(indexAccessor.byteOffset));
	}
}

// recursively draw node and children nodes of model
void Application::drawModelNodes(tinygltf::Model &model, tinygltf::Node &node) {
	if (node.mesh != -1)
		drawMesh(model, model.meshes[node.mesh]);
	for (size_t i = 0; i < node.children.size(); i++) {
		drawModelNodes(model, model.nodes[node.children[i]]);
	}
}

void Application::drawModel(GLuint vao, tinygltf::Model &model) {
	glBindVertexArray(vao);

	const tinygltf::Scene &scene = model.scenes[model.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		drawModelNodes(model, model.nodes[scene.nodes[i]]);
	}

	glBindVertexArray(0);
}