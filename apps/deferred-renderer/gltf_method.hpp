#pragma once
#include "Application.hpp"
#include <iostream>
#include <tiny_gltf.h>

class gltf_method {
public:
	bool loadModel(tinygltf::Model &model, const char *filename);
	std::map<int, GLuint> bindMesh(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Mesh &mesh);
	void bindModelNodes(std::map<int, GLuint> vbos, tinygltf::Model &model, tinygltf::Node &node);
	GLuint bindModel(tinygltf::Model &model);
	void drawMesh(tinygltf::Model &model, tinygltf::Mesh &mesh);
	void drawModelNodes(tinygltf::Model &model, tinygltf::Node &node);
	void drawModel(GLuint vao, tinygltf::Model &model);
	static void error_callback(int error, const char *description) {
		(void)error;
		fprintf(stderr, "Error: %s\n", description);
	}
};