#pragma once
#include "ShadedSceneNode.h"

class WaterNode :
	public ShadedSceneNode
{

public:
	WaterNode(Shader* shader, GLuint tex, Mesh* mesh, 
				GLuint cubemap, Vector3 heightMap);

	void	Draw(const OGLRenderer& r);

protected:
	void	Update(float dt);
	GLuint	cubemap;
	Vector3 size;

	float	waterCycle = 0.0f;
	float	waterRotate = 0.0f;
	Matrix4 textureMatrix;
};

