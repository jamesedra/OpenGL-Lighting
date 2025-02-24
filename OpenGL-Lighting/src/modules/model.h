#pragma once
#include <iostream>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "../../shaders/shader.h"
#include "mesh.h"

class Model {
public:
	Model(const char* path) {
		loadModel(path);
	}
	void Draw(Shader& shader);
	void DrawInstanced(Shader& shader, unsigned int count);

	const std::vector<Mesh>& getMeshes() const; // may be temporary for getting mesh array
private:
	std::vector<Mesh> meshes;
	std::vector<Texture> textures_loaded;
	std::string directory;

	void loadModel(std::string path);
	void processNode(aiNode* node, const aiScene* scene);
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
	unsigned int TextureFromFile(const char* path, const std::string& directory);
};