#include "WaterNode.h"
#include "Camera.h"

WaterNode::WaterNode(Shader* shader, GLuint tex, Mesh* mesh, 
								GLuint cubemap, Vector3 size) : 
								ShadedSceneNode(shader, mesh, tex) {

	this->cubemap = cubemap;
	this->size = size;
	size.y *= 0.5f; // adjust water level

	transform = Matrix4::Translation(size * 0.5f) *
				Matrix4::Scale(size * 0.5f) *
				Matrix4::Rotation(90, Vector3(1, 0, 0));
}

void WaterNode::Update(float dt) {
	waterRotate += dt * 0.05f; //2 degrees a second
	waterCycle += dt * 0.25f; // 10 units a second
}

void WaterNode::Draw(const OGLRenderer& r) {
	LoadTexture();
	glUniform1i(glGetUniformLocation(shader->GetProgram(), 
											"cubeTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
					Matrix4::Scale(Vector3(10, 10, 10)) *
					Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	glUniformMatrix4fv(glGetUniformLocation(shader->GetProgram(), 
				"textureMatrix"), 1, false, textureMatrix.values);

	UpdateShaderMatrices();
	mesh->Draw();
}
