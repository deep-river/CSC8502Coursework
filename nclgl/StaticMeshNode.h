#pragma once
#include "ShadedSceneNode.h"
#include "MeshMaterial.h"
#include "OGLRenderer.h"

class StaticMeshNode : public ShadedSceneNode {
public:
    StaticMeshNode(Shader* shader, Mesh* imesh, MeshMaterial* mat,
        Vector3 pos, float scale, float yRot);
    ~StaticMeshNode() = default;

    void Draw(const OGLRenderer& r);

protected:
    MeshMaterial*   mat;
    Vector3         pos;
    float           scale;
    float           yRot;
    std::vector<GLuint> matTextures;
};