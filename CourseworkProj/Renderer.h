#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"

class Camera;
class Shader;
class HeightMap;
class Mesh;
class MeshMaterial;
class MeshAnimation;
class SceneNode;

class Renderer : public OGLRenderer {
public:
	Renderer(Window & parent);
	~Renderer(void);

	void RenderScene() override;
	void UpdateScene(float dt) override;

	void TogglePostProcessing();
	
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
	Shader *	terrainShader;
	Shader *	skyboxShader;
	Shader *	SceneNodeShader;
	Shader *	staticShader;
	
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

	Mesh*			biomeMesh;
	MeshAnimation*	biomeAnim;
	MeshMaterial*	biomeMaterial;

	Mesh*			dynamicObjMesh;
	MeshAnimation*	dynamicObjAnim;
	MeshMaterial*	dynamicObjMaterial;

	Shader*		meshShader;
	Shader*		animMeshShader;
	
	float		waterRotate;
	float		waterCycle;

	void PresentScene();
	void DrawPostProcess();

	bool		postProcessingSwitch;
	Shader*		processShader;
	Shader*		sceneShader;
	Mesh*		postquad;
	GLuint		bufferFBO;
	GLuint		processFBO;
	GLuint		bufferColourTex[2];
	GLuint		bufferDepthTex;

	void LoadTextures();
	void LoadShaders();
	void LoadMeshes();
};