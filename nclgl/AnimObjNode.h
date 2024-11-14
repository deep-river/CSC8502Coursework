#pragma once
#include "ShadedSceneNode.h"

class Mesh;
class MeshAnimation;
class MeshMaterial;

class AnimObjNode :
    public ShadedSceneNode
{
public:
	AnimObjNode(Shader* shader, Mesh* mesh, MeshAnimation* anim,
					MeshMaterial* mat, Vector3 pos, float scale, 
											float yRot, bool move);
protected:
	void Draw(const OGLRenderer& r);
	void Update(float dt);

	MeshAnimation*	anim;
	MeshMaterial*	mat;
	Vector3			pos;
	float			scale;
	float			yRot;
	bool			move;
	vector<GLuint>	matTextures;
	int				currentFrame;
	float			frameTime;
};

