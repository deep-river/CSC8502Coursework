#include "ShadedSceneNode.h"

ShadedSceneNode::ShadedSceneNode(Shader* shader, Mesh* mesh, GLuint tex) : SceneNode(mesh) {
	this->shader = shader;
	this->texture = tex;
}

void ShadedSceneNode::Draw(const OGLRenderer& r) {
	if (mesh) {
		LoadTexture();
		UpdateShaderMatrices();
		mesh->Draw();
	}
}

void ShadedSceneNode::UpdateShaderMatrices() {
	glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), "modelMatrix"), 1, false, transform.values);
}

void ShadedSceneNode::LoadTexture() {
	glUniform1i(glGetUniformLocation(shader->GetProgram(), 
										"diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
}