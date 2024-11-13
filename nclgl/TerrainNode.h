#pragma once
#include "../nclgl/ShadedSceneNode.h"

class TerrainNode :
    public ShadedSceneNode
{
public:
	TerrainNode(Shader* shader, Camera* camera, Mesh* mesh,
		GLuint earthTex, GLuint earthBump);

	void Draw(const OGLRenderer& r);

protected:
	Camera*		camera;
	GLuint		earthTex;
	GLuint		earthBump;
};

