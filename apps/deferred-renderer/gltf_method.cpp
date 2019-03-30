#include "Application.hpp"
#include "gltf_method.hpp"

#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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

bool gltf_method::loadModel(tinygltf::Model &model, const char *filename) {
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

std::map<int, GLuint> gltf_method::bindMesh(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Mesh &mesh) {
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
				std::cout << "vaa missing: " << attrib.first << std::endl;
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

void gltf_method::bindModelNodes(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Node &node) {
	bindMesh(vbos, model, model.meshes[node.mesh]);
	for (size_t i = 0; i < node.children.size(); i++) {
		bindModelNodes(vbos, model, model.nodes[node.children[i]]);
	}
}

GLuint gltf_method::bindModel(tinygltf::Model &model) {
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

void gltf_method::InitMats(tinygltf::Model &model, std::vector<GLuint> m_textures, std::vector<PBRMat> m_gltfMaterials) {

	m_textures = std::vector<GLuint>(model.textures.size());
	m_gltfMaterials = std::vector<PBRMat>(model.materials.size());


	// storing textures
	for (int i = 0; i < model.textures.size(); i++) {
		// tinygltf textures info are separated in "Texture" objects and "Image" Object
		tinygltf::Image tex = model.images[model.textures[i].source];

		glGenTextures(1, &m_textures[i]);
		glBindTexture(GL_TEXTURE_2D, m_textures[i]);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGB32F, tex.width, tex.height);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.width, tex.height, GL_RGBA, GL_UNSIGNED_BYTE, &tex.image.at(0) );
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	// storing materials
	for (int i = 0; i < model.materials.size(); i++) {
		tinygltf::ParameterMap material =  model.materials[i].values;
		for (auto entry : material) {
			std::cout << "material " << i << " : "<< entry.first << ", " 
				
				<< entry.second.string_value << ", " 
				<< entry.second.number_value 
				<< std::endl;
		}
		m_gltfMaterials[i].diffus.x = material["pbrMetallicRoughness"].ColorFactor()[0];
		m_gltfMaterials[i].diffus.y = material["pbrMetallicRoughness"].ColorFactor()[1];
		m_gltfMaterials[i].diffus.z = material["pbrMetallicRoughness"].ColorFactor()[2];
		m_gltfMaterials[i].diffus.w = material["pbrMetallicRoughness"].ColorFactor()[3];

		m_gltfMaterials[i].emission.x = material["emissiveFactor"].number_array[0];
		m_gltfMaterials[i].emission.y = material["emissiveFactor"].number_array[1];
		m_gltfMaterials[i].emission.z = material["emissiveFactor"].number_array[2];

	}
}


void gltf_method::drawMesh(tinygltf::Model &model, tinygltf::Mesh &mesh) {
	for (size_t i = 0; i < mesh.primitives.size(); ++i) {
		tinygltf::Primitive primitive = mesh.primitives[i];
		tinygltf::Accessor indexAccessor = model.accessors[primitive.indices];

		glDrawElements(primitive.mode, indexAccessor.count,
			indexAccessor.componentType,
			BUFFER_OFFSET(indexAccessor.byteOffset));
	}
}

// recursively draw node and children nodes of model
void gltf_method::drawModelNodes(tinygltf::Model &model, tinygltf::Node &node) {
	drawMesh(model, model.meshes[node.mesh]);
	for (size_t i = 0; i < node.children.size(); i++) {
		drawModelNodes(model, model.nodes[node.children[i]]);
	}
}

void gltf_method::drawModel(GLuint vao, tinygltf::Model &model) {
	glBindVertexArray(vao);

	const tinygltf::Scene &scene = model.scenes[model.defaultScene];
	for (size_t i = 0; i < scene.nodes.size(); ++i) {
		drawModelNodes(model, model.nodes[scene.nodes[i]]);
	}

	glBindVertexArray(0);
}
/*
void dbgModel(tinygltf::Model &model) {
	for (auto &mesh : model.meshes) {
		std::cout << "mesh : " << mesh.name << std::endl;
		for (auto &primitive : mesh.primitives) {
			const tinygltf::Accessor &indexAccessor =
				model.accessors[primitive.indices];

			std::cout << "indexaccessor: count " << indexAccessor.count << ", type "
				<< indexAccessor.componentType << std::endl;

			tinygltf::Material &mat = model.materials[primitive.material];
			for (auto &mats : mat.values) {
				std::cout << "mat : " << mats.first.c_str() << std::endl;
			}

			for (auto &image : model.images) {
				std::cout << "image name : " << image.uri << std::endl;
				std::cout << "  size : " << image.image.size() << std::endl;
				std::cout << "  w/h : " << image.width << "/" << image.height
					<< std::endl;
			}

			std::cout << "indices : " << primitive.indices << std::endl;
			std::cout << "mode     : "
				<< "(" << primitive.mode << ")" << std::endl;

			for (auto &attrib : primitive.attributes) {
				std::cout << "attribute : " << attrib.first.c_str() << std::endl;
			}
		}
	}
}
*/

/*

void displayLoop(Window &window, const std::string &filename) {
Shaders shader = Shaders();
glUseProgram(shader.pid);

// grab uniforms to modify
GLuint MVP_u = glGetUniformLocation(shader.pid, "MVP");
GLuint sun_position_u = glGetUniformLocation(shader.pid, "sun_position");
GLuint sun_color_u = glGetUniformLocation(shader.pid, "sun_color");

tinygltf::Model model;
if (!loadModel(model, filename.c_str())) return;

GLuint vao = bindModel(model);
// dbgModel(model); return;

// Model matrix : an identity matrix (model will be at the origin)
glm::mat4 model_mat = glm::mat4(1.0f);
glm::mat4 model_rot = glm::mat4(1.0f);
glm::vec3 model_pos = glm::vec3(-3, 0, -3);

// generate a camera view, based on eye-position and lookAt world-position
glm::mat4 view_mat = genView(glm::vec3(2, 2, 20), model_pos);

glm::vec3 sun_position = glm::vec3(3.0, 10.0, -5.0);
glm::vec3 sun_color = glm::vec3(1.0);

while (!window.Close()) {
window.Resize();

glClearColor(0.2, 0.2, 0.2, 1.0);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

glm::mat4 trans =
glm::translate(glm::mat4(1.0f), model_pos);  // reposition model
model_rot = glm::rotate(model_rot, glm::radians(0.8f),
glm::vec3(0, 1, 0));  // rotate model on y axis
model_mat = trans * model_rot;

// build a model-view-projection
GLint w, h;
glfwGetWindowSize(window.window, &w, &h);
glm::mat4 mvp = genMVP(view_mat, model_mat, 45.0f, w, h);
glUniformMatrix4fv(MVP_u, 1, GL_FALSE, &mvp[0][0]);

glUniform3fv(sun_position_u, 1, &sun_position[0]);
glUniform3fv(sun_color_u, 1, &sun_color[0]);

drawModel(vao, model);
glfwSwapBuffers(window.window);
glfwPollEvents();
}
}
	
}*/

