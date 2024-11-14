#include "MatSceneNode.h"

MatSceneNode::MatSceneNode(Shader* s, Mesh* m, MeshMaterial* mmat) : 
									SceneNode(m, Vector4(1, 1, 1, 1), s), meshMat(mmat) {
	for (int i = 0; i < mesh->GetSubMeshCount(); ++i) {
		const MeshMaterialEntry* matEntry = meshMat->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
		matTextures.emplace_back(texID);

		if (matEntry->GetEntry("Bump", &filename))
		{
			path = TEXTUREDIR + *filename;
			texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
			bumpTextures.emplace_back(texID);
		}
	}
	mesh->GenerateNormals();
	mesh->GenerateTangents();
}

MatSceneNode::~MatSceneNode()
{
	delete meshMat;
	delete mesh;
}

void MatSceneNode::Draw(const OGLRenderer& r)
{
	for (int i = 0; i < mesh->GetSubMeshCount(); ++i)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, matTextures[i]);
		if (!bumpTextures.empty())
		{
			//glActiveTexture(GL_TEXTURE1);
			//glBindTexture(GL_TEXTURE_2D, bumpTextures[i]);

			glUniform1i(glGetUniformLocation(shader->GetProgram(), "bumpTex"), 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, bumpTextures[i]);
		}
		mesh->DrawSubMesh(i);
	}

	SceneNode::Draw(r);
}