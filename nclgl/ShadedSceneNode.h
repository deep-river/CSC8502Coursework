#pragma once
#include "SceneNode.h"
class ShadedSceneNode :
    public SceneNode
{
public:
    ShadedSceneNode(Shader* shader, Mesh* mesh, GLuint = 0);
    
	virtual void Draw(const OGLRenderer& r);

	// Shader* GetShader() const { return shader; }

protected:
    void    UpdateShaderMatrices();
    void    LoadTexture();
};

