#include "StaticMeshNode.h"
#include "Camera.h"
#include "MeshMaterial.h"

StaticMeshNode::StaticMeshNode(Shader* shader, Mesh* imesh, 
    MeshMaterial* mat,Vector3 pos, float scale, float yRot)
    : ShadedSceneNode(shader, imesh) {

    this->mat = mat;
    this->pos = pos;
    this->scale = scale;
    this->yRot = yRot;

    for (int i = 0; i < mesh->GetSubMeshCount(); ++i) {
        const MeshMaterialEntry* matEntry = mat->GetMaterialForLayer(i);

        const string* filename = nullptr;
        matEntry->GetEntry("Diffuse", &filename);
        string path = TEXTUREDIR + *filename;
        GLuint texID = SOIL_load_OGL_texture(path.c_str(), SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);
        matTextures.emplace_back(texID);
    }
    mesh->GenerateNormals();
}

void StaticMeshNode::Draw(const OGLRenderer& r) {
    UpdateShaderMatrices();

    transform.ToIdentity();
    transform = transform * Matrix4::Translation(
                parent->GetTransform().GetPositionVector() + pos);
    transform = transform * Matrix4::Scale(
                Vector3(scale, scale, scale));
    transform = transform * Matrix4::Rotation(
                yRot, Vector3(0, 1, 0));
    
    // SceneNode::Draw(r);
	
    for (int i = 0; i < mesh->GetSubMeshCount(); ++i) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, matTextures[i]);
        mesh->DrawSubMesh(i);
    }
    SceneNode::Draw(r);
}