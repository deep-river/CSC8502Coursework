#include "TerrainNode.h"
#include "Camera.h"
//#include "../CourseworkProj/Renderer.h"

TerrainNode::TerrainNode(Shader* shader, Camera* camera, Mesh* mesh,
	GLuint earthTex, GLuint earthBump) : 
	ShadedSceneNode(shader, mesh, earthTex) {

	this->earthTex = earthTex;
	this->earthBump = earthBump;

	this->camera = camera;

	transform.ToIdentity();
}

void TerrainNode::Draw(const OGLRenderer& r) {
	glUniform1i(glGetUniformLocation(shader->GetProgram(),
		"diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(),
		"bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	UpdateShaderMatrices();
	mesh->Draw();
}