#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include <algorithm>
#include "../nclgl/TerrainNode.h"
#include "../nclgl/WaterNode.h"

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();

	heightMap = new HeightMap(TEXTUREDIR"noise.png");

	waterTex = SOIL_load_OGL_texture(
			TEXTUREDIR"water.TGA", SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	earthTex = SOIL_load_OGL_texture(
			TEXTUREDIR"brown_gravel_terrain.png", SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
		
	earthBump = SOIL_load_OGL_texture(
			TEXTUREDIR"brown_gravel_terrain_NormalMap.png", SOIL_LOAD_AUTO,
			SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
		
	cubeMap = SOIL_load_OGL_cubemap(
			TEXTUREDIR"rusted_west.jpg", TEXTUREDIR"rusted_east.jpg",
			TEXTUREDIR"rusted_up.jpg", TEXTUREDIR"rusted_down.jpg",
			TEXTUREDIR"rusted_south.jpg", TEXTUREDIR"rusted_north.jpg",
			SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);
		
	if (!earthTex || !earthBump || !cubeMap || !waterTex) {
		return;
	}

	SetTextureRepeating(earthTex, true);
	SetTextureRepeating(earthBump, true);
	SetTextureRepeating(waterTex, true);

	reflectShader = new Shader(
			"reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader(
			"skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader(
		"PerPixelVertex.glsl", "PerPixelFragment.glsl");
	
	if (!reflectShader->LoadSuccess() ||
		!skyboxShader->LoadSuccess() ||
		!lightShader->LoadSuccess()) {
		return;
	}

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(-45.0f, 0.0f,
			heightmapSize * Vector3(0.5f, 5.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(0.5f, 1.5f, 0.5f),
			Vector4(1, 1, 1, 1), heightmapSize.x * 0.5f);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
			(float)width / (float)height, 45.0f);

	// build SceneNodes
	root = new SceneNode();

	root->AddChild(new TerrainNode(lightShader, camera, heightMap,
											earthTex, earthBump));
	root->AddChild(new WaterNode(reflectShader, waterTex, quad, 
											cubeMap, heightmapSize));

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;

	postProcessingSwitch = false;
}

Renderer::~Renderer(void) {
	delete root;

	delete camera;
	delete heightMap;
	delete quad;
	delete reflectShader;
	delete skyboxShader;
	delete lightShader;
	delete light;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();

	// culling and NodeScene update
	frameFrustum.FromMatrix(projMatrix * viewMatrix);
	root->Update(dt);
}

void Renderer::BuildNodeLists(SceneNode* from) {
	// check whether node is inside the frustum
	if (frameFrustum.InsideFrustum(*from) || from->GetMesh()) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector()
			- camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}

	for (vector<SceneNode*>::const_iterator
		i = from->GetChildIteratorStart();
		i != from->GetChildIteratorEnd(); ++i) {
		// recursion
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), // r stands for reverse sorting
		transparentNodeList.rend(),
		SceneNode::CompareByCameraDistance);

	std::sort(nodeList.begin(),
		nodeList.end(),
		SceneNode::CompareByCameraDistance);
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

//todo: DrawSkybox, DrawHeightmap, DrawWater here.
void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Shader* shader = n->GetShader();
		if (shader) {
			BindShader(shader);
			SetShaderLight(*light);
			glUniform3fv(glGetUniformLocation(shader->GetProgram(),
					"cameraPos"), 1, (float*)&camera->GetPosition());

			UpdateShaderMatrices();



			n->Draw(*this);
		}
	}
}

void Renderer::RenderScene() {

	BuildNodeLists(root);
	SortNodeLists();

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	DrawSkybox();
	//DrawHeightmap();
	//DrawWater();
	DrawNodes();
	ClearNodeLists();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);
	UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawHeightmap() {
	BindShader(lightShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(lightShader->GetProgram(),
		"cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
		"diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, earthTex);

	glUniform1i(glGetUniformLocation(lightShader->GetProgram(),
		"bumpTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, earthBump);

	modelMatrix.ToIdentity();
	textureMatrix.ToIdentity();

	UpdateShaderMatrices();

	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform3fv(glGetUniformLocation(reflectShader->GetProgram(),
		"cameraPos"), 1, (float*)&camera->GetPosition());

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(),
		"diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(),
		"cubeTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, waterTex);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightmapSize();

	modelMatrix =	Matrix4::Translation(hSize * 0.5f) *
					Matrix4::Scale(hSize * 0.5f) *
					Matrix4::Rotation(90, Vector3(1, 0, 0));

	textureMatrix = Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
					Matrix4::Scale(Vector3(10, 10, 10)) *
					Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	UpdateShaderMatrices();

	quad->Draw();
}

void Renderer::TogglePostProcessing() {
	postProcessingSwitch = !postProcessingSwitch;
}