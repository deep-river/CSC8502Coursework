#include "Renderer.h"
#include "../nclgl/Light.h"
#include "../nclgl/Heightmap.h"
#include "../nclgl/Shader.h"
#include "../nclgl/Camera.h"
#include <algorithm>
#include "../nclgl/TerrainNode.h"
#include "../nclgl/WaterNode.h"
#include "../nclgl/StaticMeshNode.h"
#include "../nclgl/AnimObjNode.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"
const int POST_PASSES = 10;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	quad = Mesh::GenerateQuad();
	postquad = Mesh::GenerateQuad();

	LoadTextures();
	LoadShaders();
	LoadMeshes();

	Vector3 heightmapSize = heightMap->GetHeightmapSize();

	camera = new Camera(-30.0f, 0.0f,
			heightmapSize * Vector3(0.5f, 2.0f, 0.5f));
	light = new Light(heightmapSize * Vector3(0.75f, 2.5f, 0.5f),
			Vector4(1, 1, 1, 1), heightmapSize.x * 5.0f);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
			(float)width / (float)height, 45.0f);

	// build SceneNodes
	root = new SceneNode();

	root->AddChild(new TerrainNode(lightShader, camera, heightMap,
											earthTex, earthBump));
	root->AddChild(new WaterNode(reflectShader, waterTex, quad, 
											cubeMap, heightmapSize));
	/*root->AddChild(new StaticMeshNode(meshShader, biomeMesh, biomeMaterial,
			Vector3(2800.0f, 320.0f, 2800.0f), 25.0f, 0.0f));*/
	root->AddChild(new AnimObjNode(animMeshShader, dynamicObjMesh, dynamicObjAnim, dynamicObjMaterial,
			Vector3(2000.0f, 320.0f, 2800.0f), 80.0f, 0.0f, true));


	glGenTextures(1, &bufferDepthTex);
	glBindTexture(GL_TEXTURE_2D, bufferDepthTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height,
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

	for (int i = 0; i < 2; ++i) {
		glGenTextures(1, &bufferColourTex[i]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}
	glGenFramebuffers(1, &bufferFBO);
	glGenFramebuffers(1, &processFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
		GL_TEXTURE_2D, bufferDepthTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bufferColourTex[0], 0);
	// We can check FBO attachment success using this command !
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
		GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex || !bufferColourTex[0]) {
		return;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


	//glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	postProcessingSwitch = false;
	waterRotate = 0.0f;
	waterCycle = 0.0f;
	init = true;
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

	delete biomeMaterial;
	delete meshShader;

	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(1, &bufferDepthTex);
	glDeleteFramebuffers(1, &bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
}

void Renderer::LoadTextures() {
	heightMap = new HeightMap(TEXTUREDIR"snowdon.png");

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
}

void Renderer::LoadShaders() {
	reflectShader = new Shader(
		"reflectVertex.glsl", "reflectFragment.glsl");
	skyboxShader = new Shader(
		"skyboxVertex.glsl", "skyboxFragment.glsl");
	lightShader = new Shader(
		"PerPixelVertex.glsl", "PerPixelFragment.glsl");
	terrainShader = new Shader(
			"BumpVertex.glsl", "TerrainFragment.glsl");
	animMeshShader = new Shader(
		"SkinningVertex.glsl", "TexturedFragment.glsl");
	meshShader = new Shader(
		"PerPixelVertex.glsl", "PerPixelFragment.glsl");
	sceneShader = new Shader(
		"TexturedVertex.glsl", "TexturedFragment.glsl");
	processShader = new Shader(
		"TexturedVertex.glsl", "processfrag.glsl");

	if (!reflectShader->LoadSuccess() ||
		!skyboxShader->LoadSuccess() ||
		!lightShader->LoadSuccess() ||
		!meshShader->LoadSuccess() ||
		!animMeshShader->LoadSuccess() ||
		!sceneShader->LoadSuccess() ||
		!processShader->LoadSuccess()) {
		return;
	}
}

void Renderer::LoadMeshes() {
	biomeMesh = Mesh::LoadFromMeshFile("tree-maple-low-poly-Anim.msh");
	biomeMaterial = new MeshMaterial("tree-maple-low-poly-Anim.mat");

	/*biomeMesh = Mesh::LoadFromMeshFile("CommonTree_4.msh");
	biomeMaterial = new MeshMaterial("CommonTree_4.mat");*/

	dynamicObjMesh = Mesh::LoadFromMeshFile("Role_T.msh");
	dynamicObjAnim = new MeshAnimation("Role_T.anm");
	dynamicObjMaterial = new MeshMaterial("Role_T.mat");
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
	if (postProcessingSwitch) {
		projMatrix = Matrix4::Perspective(1.0f, 25000.0f, 
							(float)width / (float)height, 45.0f);
		glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | 
										GL_STENCIL_BUFFER_BIT);
		BuildNodeLists(root);
		SortNodeLists();
		DrawSkybox();
		DrawNodes();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		DrawPostProcess();
		PresentScene();
		ClearNodeLists();
	}
	else {
		projMatrix = Matrix4::Perspective(1.0f, 15000.0f,
					(float)width / (float)height, 45.0f);
		viewMatrix = camera->BuildViewMatrix();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		BuildNodeLists(root);
		SortNodeLists();
		DrawSkybox();
		DrawNodes();
		ClearNodeLists();
	}
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bufferColourTex[1], 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(processShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);

	glActiveTexture(GL_TEXTURE0);
	glUniform1i(glGetUniformLocation(
		processShader->GetProgram(), "sceneTex"), 0);
	for (int i = 0; i < POST_PASSES; ++i) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, bufferColourTex[1], 0);
		glUniform1i(glGetUniformLocation(processShader->GetProgram(),
			"isVertical"), 0);

		glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
		postquad->Draw();
		// Now to swap the colour buffers , and do the second blur pass
		glUniform1i(glGetUniformLocation(processShader->GetProgram(),
			"isVertical"), 1);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_2D, bufferColourTex[0], 0);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
		postquad->Draw();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	BindShader(sceneShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(
		sceneShader->GetProgram(), "diffuseTex"), 0);
	postquad->Draw();
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