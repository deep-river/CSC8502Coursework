#pragma once
#include "SceneNode.h"
#include "MeshMaterial.h"

class MatSceneNode : public SceneNode
{
public:
	MatSceneNode(Shader* s, Mesh* m, MeshMaterial* mmat);
	~MatSceneNode();

	void Draw(const OGLRenderer& r) override;
protected:
	MeshMaterial* meshMat;
	vector <GLuint > matTextures;
	vector <GLuint > bumpTextures;
};