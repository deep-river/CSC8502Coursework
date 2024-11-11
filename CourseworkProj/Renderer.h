#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"

class Camera;
class Shader;
class HeightMap;
class SceneNode;
class Mesh;

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;
	
protected:
	void BuildNodeLists(SceneNode* from);
	void SortNodeLists();
	void ClearNodeLists();
	void DrawNodes();
	void DrawNode(SceneNode* n);

	SceneNode*	root;
	Frustum		frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	void DrawHeightmap();
	void DrawWater();
	void DrawSkybox();
	
	Shader *	lightShader;
	Shader *	reflectShader;
	Shader *	skyboxShader;
	Shader *	SceneNodeShader;
	
	HeightMap * heightMap;
	Mesh *		quad;
	Mesh *		cube;
	
	Light *		light;
	Camera *	camera;
	
	GLuint		cubeMap;
	GLuint		waterTex;
	GLuint		earthTex;
	GLuint		earthBump;
	GLuint		SceneNodeTexture;
	
	float		waterRotate;
	float		waterCycle;
};