#include "M_Renderer3D.h"
#include "Primitive.h"

// OpenGL / GLEW
#include "glew.h"
#include "SDL_opengl.h"
#include <gl/GL.h>
#include <gl/GLU.h>
#include "MathGeoLib/Geometry/Plane.h"

#include "Log.h"
#include "M_Window.h"
#include "Engine.h"
#include "M_Camera3D.h"
#include "M_SceneManager.h"
#include "M_Editor.h"
#include "M_Input.h"
#include "M_Physics.h"
#include "ImGuiAppLog.h"
#include "M_FileSystem.h"
#include "R_Texture.h"
#include "M_UI.h"
#include "M_Navigation.h"

#include <imgui.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "imgui_internal.h"

#include "GameObject.h"
#include "C_Transform.h"
#include "C_Mesh.h"
#include "C_Material.h"
#include "C_Camera.h"
#include "C_RenderedUI.h"
#include "C_LightSource.h"
#include "C_Animator.h"
#include "C_RigidBody.h"

#include "R_Material.h"

#include "SceneIntro.h"

#include "PieShape.h"
#include "AnimatorClip.h"
#include "FSDefs.h"

#include "PanelViewport.h"

#include "SkyBox.h"

#include <iostream>

#include "optick.h"
#include "Quadtree.h"
#pragma comment (lib, "glu32.lib")    /* link OpenGL Utility lib     */
#pragma comment (lib, "opengl32.lib") /* link Microsoft OpenGL lib   */

#define RELOAD_SHADOWS_TIMER 1

M_Renderer3D::M_Renderer3D(KoFiEngine* engine) : Module()
{
	name = "Renderer3D";
	this->engine = engine;

	/*auto compGO = [this](GameObject* a, GameObject* b) {
		float3 cameraPosition = this->engine->GetCamera3D()->gameCamera->owner->GetTransform()->GetPosition();
		return cameraPosition.DistanceSq(a->GetTransform()->GetPosition()) < cameraPosition.DistanceSq(b->GetTransform()->GetPosition());
	};*/

	//gameObejctsToRenderDistanceOrdered = std::set<GameObject*, decltype(compGO)>(compGO);
}

// Destructor
M_Renderer3D::~M_Renderer3D()
{}

// Called before render is available
bool M_Renderer3D::Awake(Json configModule)
{
	CONSOLE_LOG("Creating 3D Renderer context");
	appLog->AddLog("Creating 3D Renderer context\n");
	bool ret = true;
	ret = InitOpenGL();
	OnResize();
	SetVsync(configModule["Vsync"].get<bool>());

	depthMapResolution = 4096; //quite high but the engine seems to handle it pretty well (2 or 3 fps below the normal shadows)
	InitDepthMapFramebufferAndTexture();
	InitFrameBuffers();

	query = new OcclusionQuery();
	enableOcclusionCulling = false;
	occlusionMat = new R_Material();
	occlusionMat->SetAssetPath("EngineConfig/Shaders/BasicShader.glsl");
	Importer::GetInstance()->materialImporter->LoadAndCreateShader("EngineConfig/Shaders/BasicShader.glsl", occlusionMat);

	outlineShader = new R_Material();
	outlineShader->SetAssetPath("Assets/Shaders/outline_shader.glsl");
	Importer::GetInstance()->materialImporter->LoadAndCreateShader("Assets/Shaders/outline_shader.glsl", outlineShader);

	ret = LoadConfiguration(configModule);

	return ret;
}

// PreUpdate: clear buffer
bool M_Renderer3D::PreUpdate(float dt)
{
	OPTICK_EVENT();

	bool ret = true;

	PrepareFrameBuffers();

	isFirstPass = true;

	return ret;
}

bool M_Renderer3D::Update(float dt)
{
	OPTICK_EVENT();
	/*for (GameObject* go : engine->GetSceneManager()->GetCurrentScene()->gameObjectList)
	{
		if (go->active && go->GetRenderGameObject() && go->GetComponent<C_Mesh>())
		{
			if (gameObejctsToRenderDistance.contains(go)) {
				gameObejctsToRenderDistanceOrdered.insert(go);
			}
		}
		else {
			if (!gameObejctsToRenderDistance.contains(go)) {
				gameObejctsToRenderDistanceOrdered.erase(go);
			}
		}
	}*/
	//float3 cameraPosition = engine->GetCamera3D()->gameCamera->owner->GetTransform()->GetPosition();
	/*std::sort(gameObejctsToRenderDistanceOrdered.begin(),
		gameObejctsToRenderDistanceOrdered.end(), 
		[&cameraPosition](const GameObject* lhs, const GameObject* rhs) {return cameraPosition.DistanceSq(lhs->GetTransform()->GetPosition()) < cameraPosition.DistanceSq(rhs->GetTransform()->GetPosition()); });*/
	
	if (!reloadShadows) 
	{
		timerShadowsReload += dt;
		if (timerShadowsReload > RELOAD_SHADOWS_TIMER)
		{
			timerShadowsReload = 0.0f;
			reloadShadows = true;
		}
	}
	return true;
}

// PostUpdate present buffer to screen
bool M_Renderer3D::PostUpdate(float dt)
{
	OPTICK_EVENT();

	//ShadowMap creation
	if (engine->GetSceneManager()->GetCurrentScene() != nullptr) {
		GameObject* light = engine->GetSceneManager()->GetCurrentScene()->GetShadowCaster();
		if (light)
		{
			if (reloadShadows) //optimization to make shadows less real time and more efficient ( timer is a #define )
			{
				glViewport(0, 0, depthMapResolution, depthMapResolution);		//configure viewport
				glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);		//bind framebuffer
				glClear(GL_DEPTH_BUFFER_BIT);						//clear only the depth buffer
				reloadShadows = false;
				FillShadowMap();
				glViewport(0, 0, engine->GetEditor()->lastViewportSize.x, engine->GetEditor()->lastViewportSize.y);
				UnbindFrameBuffers();
				PrepareFrameBuffers();
			}
		}
	}

	PassProjectionAndViewToRenderer();

	if (enableOcclusionCulling)
	{
		QueryScene2(engine->GetCamera3D()->currentCamera);
	}
	else {
		for (auto go : gameObejctsToRenderDistance) {
			go->SetRenderGameObject(true);
		}
	}
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	RenderScene(engine->GetCamera3D()->currentCamera);
	engine->GetNavigation()->DrawNavmesh();

	isFirstPass = false;

	UnbindFrameBuffers();

	SwapWindow();
	return true;
}

// Called before quitting
bool M_Renderer3D::CleanUp()
{
	CONSOLE_LOG("Destroying 3D Renderer");
	appLog->AddLog("Destroying 3D Renderer\n");

	SDL_GL_DeleteContext(context);

	ReleaseFrameBuffers();

	return true;
}

void M_Renderer3D::SwapWindow()
{
	OPTICK_EVENT();

	SDL_GL_SwapWindow(engine->GetWindow()->GetWindow());
}

bool M_Renderer3D::SaveConfiguration(Json& configModule) const
{
	configModule["Vsync"] = vsync;
	return true;
}

bool M_Renderer3D::LoadConfiguration(Json& configModule)
{
	vsync = configModule["Vsync"];

	return true;
}

bool M_Renderer3D::InspectorDraw()
{
	if (ImGui::CollapsingHeader("Renderer##"))
	{
		bool vsync = GetVsync();
		if (ImGui::Checkbox("V-Sync", &vsync))
		{
			SetVsync(vsync);
			engine->SaveConfiguration();
		}
		if (ImGui::Checkbox("Draw scene partition tree", &engine->GetSceneManager()->GetCurrentScene()->drawSceneTree)) {
			engine->SaveConfiguration();
		}
		if (ImGui::Checkbox("Enable Occlusion Culling", &enableOcclusionCulling)) {
			ResetFrustumCulling();
			engine->GetCamera3D()->currentCamera->ApplyCullings();
		}
		

		ImGui::Text("Objects in sphere %d", gameObejctsToRenderDistanceSphere.size());
		ImGui::Text("Objects in frustrum %d", gameObejctsToRenderDistance.size());
		ImGui::Text("Objects in frustrum sorted %d", gameObejctsToRenderDistanceOrdered.size());
	}

	return true;
}

bool M_Renderer3D::InitOpenGL()  
{
	bool ret = true;
	context = SDL_GL_CreateContext(engine->GetWindow()->GetWindow());
	int contextMajorVersion, contextMinorVersion;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &contextMajorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &contextMinorVersion);
	if (!context)
	{
		KOFI_ERROR(" OpenGL context could not be created! SDL_ERROR: %s\n", SDL_GetError());
		ret = false;
	}
	ret = InitGlew();
	if (ret)
	{
		if (this->vsync)
		{
			SDL_GL_SetSwapInterval(1) < 0 ?
				KOFI_ERROR(" Unable to set Vsync! SDL Error: %s\n", SDL_GetError()) : KOFI_STATUS(" Vsync is activated!");
		}
		else
		{
			SDL_GL_SetSwapInterval(0) < 0 ?
				KOFI_ERROR(" Unable to set frame update interval to immediate! SDL Error: %s\n", SDL_GetError()) : KOFI_STATUS(" Vsync is deactivated!");
		}
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		glClearDepth(1.0f);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		GLfloat LightModelAmbient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, LightModelAmbient);

		lights[0].ref = GL_LIGHT0;
		lights[0].ambient.Set(1, 1, 1, 1);
		lights[0].diffuse.Set(1, 1, 1, 1);
		//lights[0].ambient.Set(0.25f, 0.25f, 0.25f, 1.0f);
		//lights[0].diffuse.Set(0.75f, 0.75f, 0.75f, 1.0f);
		lights[0].SetPos(0.0f, 0.0f, 2.5f);
		lights[0].Init();

		GLfloat MaterialAmbient[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, MaterialAmbient);

		GLfloat MaterialDiffuse[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, MaterialDiffuse);

		SetGLFlag(GL_DEPTH_TEST, true);
		glDepthFunc(GL_LESS);  // change depth function so depth test passes when values are equal to depth buffer's content
		SetGLFlag(GL_STENCIL_TEST, true);
		SetGLFlag(GL_CULL_FACE, true);
		lights[0].Active(true);
		SetGLFlag(GL_LIGHTING, true);
		SetGLFlag(GL_COLOR_MATERIAL, true);
		SetGLFlag(GL_TEXTURE_2D, true);

		GLenum err = glGetError();
		while (err != GL_NO_ERROR)
		{
			CONSOLE_LOG("OpenGl error: %d", err);
			err = glGetError();
		}
	}

	return ret;
}

bool M_Renderer3D::InitGlew()
{
	bool ret = true;

	// Initializing glew.
	GLuint GLEWerr = glewInit();
	if (GLEWerr != GLEW_OK)
	{
		KOFI_ERROR(" GLEW could not initialize!: %s\n", glewGetErrorString(GLEWerr));
		ret = false;
	}
	return ret;
}

void M_Renderer3D::SetGLFlag(GLenum flag, bool setTo)
{
	if (setTo != (bool)glIsEnabled(flag))
	{
		setTo ? glEnable(flag) : glDisable(flag);
	}
}

void M_Renderer3D::PassProjectionAndViewToRenderer()
{

	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	M_Camera3D* currentCamera3D = engine->GetCamera3D();
	if (currentCamera3D->currentCamera)
	{
		RecalculateProjectionMatrix();
		glLoadMatrixf((GLfloat*)currentCamera3D->currentCamera->GetViewMatrix().Transposed().ptr());
		//glLoadMatrixf((GLfloat*)currentCamera3D->currentCamera->GetProjectionMatrix().Transposed().ptr());
	}
	float3 cameraPos = float3::zero;
	//TODO NEED TO CHANGE THIS TO engine->camera->currentcamera when the component camera can be set as camera.
	if (engine->GetCamera3D()->currentCamera)
	{
		cameraPos = engine->GetCamera3D()->currentCamera->GetPosition();
	}
	else {
		cameraPos = float3(0.0f, 20.0f, 0.0f);
	}
}

void M_Renderer3D::PassPreviewProjectionAndViewToRenderer()
{
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	M_Camera3D* currentCamera3D = engine->GetCamera3D();
	if (currentCamera3D->gameCamera)
	{
		RecalculateProjectionMatrix();

		glLoadMatrixf((GLfloat*)currentCamera3D->gameCamera->GetViewMatrix().Transposed().ptr());
	}
	float3 cameraPos = float3::zero;
	//TODO NEED TO CHANGE THIS TO engine->camera->currentcamera when the component camera can be set as camera.
	if (engine->GetCamera3D()->gameCamera)
	{
		cameraPos = engine->GetCamera3D()->gameCamera->GetPosition();
	}
	else {
		cameraPos = float3(0.0f, 20.0f, 0.0f);
	}
}

void M_Renderer3D::RecalculateProjectionMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (engine->GetCamera3D()->currentCamera)
	{

		glLoadMatrixf((GLfloat*)engine->GetCamera3D()->currentCamera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());
	}
	else
	{
		KOFI_ERROR(" M_Renderer3D: Could not recalculate the projection matrix!Error : Current Camera was nullptr.");
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}



void M_Renderer3D::RenderScene(C_Camera* camera)
{
	OPTICK_EVENT();

	if (engine->GetSceneManager()->GetCurrentScene() != nullptr) {
		if (engine->GetSceneManager()->GetCurrentScene()->drawSkybox == true)
			RenderSkyBox(camera, engine->GetSceneManager()->GetCurrentScene()->skybox);
	}

	auto renderGo = [this, camera](GameObject* go) {
		if (go->active && go->GetRenderGameObject())
		{
			C_Mesh* cMesh = go->GetComponent<C_Mesh>();
			if (cMesh)
			{
				//CONSOLE_LOG(go->GetName());
				RenderMeshes(camera, go);
				RenderBoundingBox(cMesh);
			}

			if (go->GetComponent<C_RigidBody>())
				engine->GetPhysics()->RenderPhysics();

		}
	};

	if (gameObejctsToRenderDistanceOrdered.size() != 0) {

		for (GameObject* go : gameObejctsToRenderDistanceOrdered)
		{
			renderGo(go);
		}
	}
	else {

		for (GameObject* go : gameObejctsToRenderDistance) {
			renderGo(go);
		}
	}
	RenderAllParticles();

	if (engine->GetSceneManager()->GetCurrentScene() != nullptr) {
		for (GameObject* go : engine->GetSceneManager()->GetCurrentScene()->gameObjectList)
		{
			if (go->active)
			{
				C_RenderedUI* cRenderedUI = go->GetComponent<C_RenderedUI>();
				if (cRenderedUI)
				{
					RenderUI(go);
				}

				C_Camera* cCamera = go->GetComponent<C_Camera>();
				if (cCamera) {
					if (!engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.empty())
					{
						int uid = engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.at(0);

						if (!cCamera->IsEngineCamera() /*&& cCamera->owner->GetUID() == uid*/)
						{
							if (cCamera->GetIsSphereCullingActive())
							{
								cCamera->DrawSphereCulling();
							}
							cCamera->DrawFrustum();
						}

					}
				}
			}
		}
	}

	//if (engine->GetSceneManager()->GetCurrentScene()->sceneTree != nullptr && engine->GetSceneManager()->GetCurrentScene()->drawSceneTree)
	//{
	//	//engine->GetSceneManager()->GetCurrentScene()->ComputeQuadTree();
	//	//engine->GetSceneManager()->GetCurrentScene()->sceneTree->Draw();
	//}


}
void M_Renderer3D::QueryScene1(C_Camera* camera)
{
	OPTICK_EVENT();

	for (GameObject* go : gameObejctsToRenderDistanceOrdered)
	{
		if (go->active)
		{
			C_Mesh* cMesh = go->GetComponent<C_Mesh>();
			if (cMesh)
			{

				if (cMesh->GetMesh() != nullptr)
				{
					//render the meshes
					uint shader = occlusionMat->shaderProgramID;
					if (shader != 0)
					{
						glUseProgram(shader);
						// Passing Shader Uniforms
						GLint model_matrix = glGetUniformLocation(shader, "model_matrix");
						glUniformMatrix4fv(model_matrix, 1, GL_FALSE, cMesh->owner->GetTransform()->GetGlobalTransform().Transposed().ptr());
						GLint view_location = glGetUniformLocation(shader, "view");
						glUniformMatrix4fv(view_location, 1, GL_FALSE, camera->GetViewMatrix().Transposed().ptr());
						GLint projection_location = glGetUniformLocation(shader, "projection");
						glUniformMatrix4fv(projection_location, 1, GL_FALSE, camera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());
						GLint color = glGetUniformLocation(shader, "color");
						glUniform4f(color, static_cast <float> (rand()) / static_cast <float> (RAND_MAX), static_cast <float> (rand()) / static_cast <float> (RAND_MAX), static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 1.0f);

						//Draw Mesh
						cMesh->GetMesh()->Draw();
				
						glUseProgram(0);

					}
				}
			}
		}
	}
}

void M_Renderer3D::QueryScene2(C_Camera* camera)
{
	OPTICK_EVENT();

	for (GameObject* go : gameObejctsToRenderDistanceOrdered)
	{
		if (go->active)
		{
			C_Mesh* cMesh = go->GetComponent<C_Mesh>();
			if (cMesh)
			{
				if (cMesh->GetMesh() != nullptr)
				{
					// Render the meshes
					uint shader = occlusionMat->shaderProgramID;
					if (shader != 0)
					{
						glUseProgram(shader);
						// Passing Shader Uniforms
						GLint model_matrix = glGetUniformLocation(shader, "model_matrix");
						glUniformMatrix4fv(model_matrix, 1, GL_FALSE, cMesh->owner->GetTransform()->GetGlobalTransform().Transposed().ptr());
						GLint view_location = glGetUniformLocation(shader, "view");
						glUniformMatrix4fv(view_location, 1, GL_FALSE, camera->GetViewMatrix().Transposed().ptr());
						GLint projection_location = glGetUniformLocation(shader, "projection");
						glUniformMatrix4fv(projection_location, 1, GL_FALSE, camera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());
						GLint color = glGetUniformLocation(shader, "color");
						glUniform4f(color, static_cast <float> (rand()) / static_cast <float> (RAND_MAX), static_cast <float> (rand()) / static_cast <float> (RAND_MAX), static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 1.0f);
	
						query->BeginQuery();
						// Draw Mesh
						cMesh->GetMesh()->Draw();
						query->EndQuery();
						bool active = query->AnySamplesPassed();
						go->SetRenderGameObject(active);
						glUseProgram(0);
				
					}
				}
			}
		}
	}
}

void M_Renderer3D::ResetFrustumCulling()
{
	gameObejctsToRenderDistanceSphere.clear();
	gameObejctsToRenderDistance.clear();
	gameObejctsToRenderDistanceOrdered.clear();

	for (auto go : gameObejctsToRenderDistance) {
		go->SetRenderGameObject(false);
	}
	
}
void M_Renderer3D::RenderBoundingBox(C_Mesh* cMesh)
{
	OPTICK_EVENT();
	for (int selectedId : engine->GetEditor()->panelGameObjectInfo.selectedGameObjects)
	{
		if (selectedId == cMesh->owner->GetUID())
		{
			if (cMesh->GetMesh() != nullptr)
				cMesh->DrawBoundingBox(cMesh->GetLocalAABB(), float3(0.0f, 1.0f, 0.0f));
			else
				KOFI_ERROR(" Renderer: Could not draw local AABB, mesh was nullptr.");
		}
	}
}

void M_Renderer3D::RenderMeshes(C_Camera* camera, GameObject* go)
{
	OPTICK_EVENT();
	//Get needed variables
	C_Material* cMat = go->GetComponent<C_Material>();
	C_Mesh* cMesh = go->GetComponent<C_Mesh>();
	R_Mesh* mesh = cMesh->GetMesh();
	if (mesh)
	{
		//Check textures
		if (cMat)
		{
			if (!cMat->active)
			{
				glDisable(GL_TEXTURE_2D);
			}
			else
			{
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, cMat->texture->GetTextureId());
			}

			if (engine->GetSceneManager()->GetCurrentScene()->GetShadowCaster())
			{
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, depthMapTexture);
			}

			// Set Shaders
			uint shader = cMat->GetMaterial()->shaderProgramID;
			if (shader != 0)
			{
				glEnable(GL_BLEND);
				glEnable(GL_STENCIL_TEST);
				glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

				glClear(GL_STENCIL_BUFFER_BIT);

				if (mesh->GetRenderOutline())
				{
					glStencilFunc(GL_ALWAYS, 1, 0xFF);
					glStencilMask(0xFF);
				}
				else
				{
					glStencilMask(0x00);
				}

				glUseProgram(shader);

				// Passing Shader Uniforms
				GLint model_matrix = glGetUniformLocation(shader, "model_matrix");
				glUniformMatrix4fv(model_matrix, 1, GL_FALSE, cMesh->owner->GetTransform()->GetGlobalTransform().Transposed().ptr());
				GLint view_location = glGetUniformLocation(shader, "view");
				glUniformMatrix4fv(view_location, 1, GL_FALSE, camera->GetViewMatrix().Transposed().ptr());
				GLint projection_location = glGetUniformLocation(shader, "projection");
				glUniformMatrix4fv(projection_location, 1, GL_FALSE, camera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());

				if (mesh->IsAnimated())
					StepAnimatedMesh(go, mesh, shader);

				GLint refractTexCoord = glGetUniformLocation(shader, "refractTexCoord");
				glUniformMatrix4fv(refractTexCoord, 1, GL_FALSE, camera->GetViewMatrix().Transposed().ptr());

				float2 resolution = float2(1080.0f, 720.0f);
				glUniform2fv(glGetUniformLocation(shader, "resolution"), 1, resolution.ptr());

				this->timeWaterShader += 0.02f;
				glUniform1f(glGetUniformLocation(shader, "time"), this->timeWaterShader);

				// Pass all varibale uniforms from the material to the shaderProgram
				for (Uniform* uniform : cMat->GetMaterial()->uniforms)
				{
					switch (uniform->type)
					{
					case GL_INT:
					{
						glUniform1d(glGetUniformLocation(shader, uniform->name.c_str()), ((UniformT<int>*)uniform)->value);
					}
					break;
					case GL_FLOAT:
					{
						glUniform1f(glGetUniformLocation(shader, uniform->name.c_str()), ((UniformT<float>*)uniform)->value);
					}
					break;
					case GL_BOOL:
					{
						glUniform1d(glGetUniformLocation(shader, uniform->name.c_str()), ((UniformT<bool>*)uniform)->value);
					}
					break;
					case GL_FLOAT_VEC2:
					{
						UniformT<float2>* uf2 = (UniformT<float2>*)uniform;
						glUniform2fv(glGetUniformLocation(shader, uniform->name.c_str()), 1, uf2->value.ptr());
					}
					break;
					case GL_FLOAT_VEC3:
					{
						UniformT<float3>* uf3 = (UniformT<float3>*)uniform;
						glUniform3fv(glGetUniformLocation(shader, uniform->name.c_str()), 1, uf3->value.ptr());
					}
					break;
					case GL_FLOAT_VEC4:
					{
						UniformT<float4>* uf4 = (UniformT<float4>*)uniform;
						glUniform4fv(glGetUniformLocation(shader, uniform->name.c_str()), 1, uf4->value.ptr());
					}
					break;
					default:
						break;
					}
				}
				LightUniforms(shader, go);
				
				GameObject* light = engine->GetSceneManager()->GetCurrentScene()->GetShadowCaster();
				if (light)
				{
					C_LightSource* lightSource = light->GetComponent<C_LightSource>();
					if (lightSource != nullptr) {
						DirectionalLight* dirLight = (DirectionalLight*)light->GetComponent<C_LightSource>()->GetLightSource();
						GLint lightSpaceMatrix = glGetUniformLocation(shader, "lightSpaceMatrix");
						glUniformMatrix4fv(lightSpaceMatrix, 1, GL_FALSE, dirLight->lightSpaceMatrix.Transposed().ptr());
					}
				}

				GLint ourTexture = glGetUniformLocation(shader, "ourTexture");
				glUniform1i(ourTexture, 1); 

				GLint depthMap = glGetUniformLocation(shader, "shadowMap");
				glUniform1i(depthMap, 3);
				
			
				//Draw Mesh
				mesh->Draw();

				glUseProgram(0);

				if (mesh->GetRenderOutline())
					RenderOutline(mesh, camera, go);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, 0);
				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, 0);

				
			}
		}
	}
}


void M_Renderer3D::RenderSkyBox(C_Camera* camera, SkyBox& skybox)
{
	glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content

	uint shader = skybox.material->shaderProgramID;

	float4x4 mat = float4x4::identity;

	if (shader != 0)
	{
		glUseProgram(shader);

		// Passing Shader Uniforms
		glUniform1d(glGetUniformLocation(shader, "skybox"), 0);

		float4x4 view = float4x4::identity;
		view.Set3x3Part(camera->GetViewMatrix().Float3x3Part());

		GLint view_location = glGetUniformLocation(shader, "view");
		glUniformMatrix4fv(view_location, 1, GL_FALSE, view.Transposed().ptr());


		float4x4 proj = float4x4::identity;
		proj = camera->GetProjectionMatrix();
		GLint projection_location = glGetUniformLocation(shader, "projection");

		glUniformMatrix4fv(projection_location, 1, GL_FALSE, proj.Transposed().ptr());
		
		skybox.DrawSkyBox();
		glUseProgram(0); // Always Last!

	}
	glDepthFunc(GL_LESS);  // change depth function so depth test passes when values are equal to depth buffer's content

}

void M_Renderer3D::LightUniforms(uint shader, GameObject* go)
{
	OPTICK_EVENT();

	if (engine->GetSceneManager()->GetCurrentScene()->lights.size() > 0)
	{
		// ---- directional lights ----
		std::vector<GameObject*> directionalLights = engine->GetSceneManager()->GetCurrentScene()->GetLights(SourceType::DIRECTIONAL);
		if (directionalLights.size() > 0)
		{
			//TODO: is it worth it to allocate this array and update only whan dirty?
			int i = 0;
			for (auto light : directionalLights)
			{
				//current iteration to string
				std::string number = std::to_string(i);
				//get corresponding directional light
				C_LightSource* c_light = light->GetComponent<C_LightSource>();

				if (c_light == nullptr)
					continue;

				auto lightSource = (DirectionalLight*)c_light->GetLightSource();

				//fill the first variable of the DirLight struct: vec3 color
				GLint lightColor = glGetUniformLocation(shader, ("dirLights[" + number + "].color").c_str());
				glUniform3f(lightColor, lightSource->color.x, lightSource->color.y, lightSource->color.z);
				//second variable: vec3 direction
				GLint lightDir = glGetUniformLocation(shader, ("dirLights[" + number + "].direction").c_str());
				glUniform3f(lightDir, lightSource->direction.x, lightSource->direction.y, lightSource->direction.z);
				//third variable: float ambient
				GLint ambientValue = glGetUniformLocation(shader, ("dirLights[" + number + "].ambient").c_str());
				glUniform1f(ambientValue, lightSource->ambient);
				//forth variable: float diffuse
				GLint diffuseValue = glGetUniformLocation(shader, ("dirLights[" + number + "].diffuse").c_str());
				glUniform1f(diffuseValue, lightSource->diffuse);
				i++;

			}

			GLint numDirLights = glGetUniformLocation(shader, "numOfDirectionalLights");
			glUniform1i(numDirLights, i);
		}
		else
		{
			GLint numDirLights = glGetUniformLocation(shader, "numOfDirectionalLights");
			glUniform1i(numDirLights, 0);
		}

		// ---- point lights ----
		std::vector<GameObject*> pointLights = engine->GetSceneManager()->GetCurrentScene()->GetLights(SourceType::POINT);
		if (pointLights.size() > 0)
		{
			int i = 0;
			for (auto light : pointLights)
			{
				//current iteration to string
				std::string number = std::to_string(i);

				//get corresponding point light
				C_LightSource* c_light = light->GetComponent<C_LightSource>();

				if (c_light == nullptr)
					continue;
				// --- basic light parameters ---
				auto lightSource = (PointLight*)c_light->GetLightSource();

				//fill the first variable of the PointLight struct: vec3 color
				GLint lightColor = glGetUniformLocation(shader, ("pointLights[" + number + "].color").c_str());
				glUniform3f(lightColor, lightSource->color.x, lightSource->color.y, lightSource->color.z);
				//second variable: vec3 position
				GLint lightPos = glGetUniformLocation(shader, ("pointLights[" + number + "].position").c_str());
				glUniform3f(lightPos, lightSource->position.x, lightSource->position.y, lightSource->position.z);
				//float ambient always 0
				GLint ambientValue = glGetUniformLocation(shader, ("pointLights[" + number + "].ambient").c_str());
				glUniform1f(ambientValue, 0.0f);
				//forth variable: float diffuse
				GLint diffuseValue = glGetUniformLocation(shader, ("pointLights[" + number + "].diffuse").c_str());
				glUniform1f(diffuseValue, lightSource->diffuse);

				// --- light attenuation paramenters ---

				//fifth variable: float constant
				GLint constantValue = glGetUniformLocation(shader, ("pointLights[" + number + "].constant").c_str());
				glUniform1f(constantValue, lightSource->constant);
				//sixth variable: float linear
				GLint linearValue = glGetUniformLocation(shader, ("pointLights[" + number + "].linear").c_str());
				glUniform1f(linearValue, lightSource->linear);
				//seventh variable: float quadratic
				GLint quadraticValue = glGetUniformLocation(shader, ("pointLights[" + number + "].quadratic").c_str());
				glUniform1f(quadraticValue, lightSource->quadratic);
				i++;
			}

			GLint numPointLights = glGetUniformLocation(shader, "numOfPointLights");
			glUniform1i(numPointLights, i);
		}
		else
		{
			GLint numPointLights = glGetUniformLocation(shader, "numOfPointLights");
			glUniform1i(numPointLights, 0);
		}
		// ---- focal lights ----
		std::vector<GameObject*> focalLights = engine->GetSceneManager()->GetCurrentScene()->GetLights(SourceType::FOCAL);
		if (focalLights.size() > 0)
		{
			char* location = (char*)malloc(sizeof(char) * 128);
			memset(location, 0, 128);

			int i = 0;
			for (auto light : focalLights)
			{
				//current iteration to string
				std::string number = std::to_string(i);

				std::string prefix = "focalLights[" + number + "].\0";
				strcpy(location, prefix.c_str());
				char* n = location;
				while (*n != '\0') {
					n++;
				}

				n--;

				//get corresponding point light
				C_LightSource* c_light = light->GetComponent<C_LightSource>();

				if (c_light == nullptr)
					continue;
				// --- basic light parameters ---
				auto lightSource = (FocalLight*)c_light->GetLightSource();

				if (!lightSource->GOInRange(go)) continue;

				// -- basic light parameters --
				//fill the first variable of the focalLights struct: vec3 color

				strcpy(n, ".color\0");
				GLint lightColor = glGetUniformLocation(shader, location);
				glUniform3f(lightColor, lightSource->color.x, lightSource->color.y, lightSource->color.z);
				//vec3 position
				strcpy(n, ".position\0");
				GLint lightPos = glGetUniformLocation(shader, location);
				glUniform3f(lightPos, lightSource->position.x, lightSource->position.y, lightSource->position.z);
				//float ambient always 0
				strcpy(n, ".ambient\0");
				GLint ambientValue = glGetUniformLocation(shader, location);
				glUniform1f(ambientValue, 0.0f);
				//float diffuse
				strcpy(n, ".diffuse\0");
				GLint diffuseValue = glGetUniformLocation(shader, location);
				glUniform1f(diffuseValue, lightSource->diffuse);

				// -- light cone parameters --
				//float cutOffAngle
				strcpy(n, ".cutOffAngle\0");
				GLint cutOffValue = glGetUniformLocation(shader, location);
				glUniform1f(cutOffValue, lightSource->cutOffAngle);
				//float cutOffAngle
				strcpy(n, ".range\0");
				GLint range = glGetUniformLocation(shader, location);
				glUniform1f(range, lightSource->range);
				//float3 lightDirection
				strcpy(n, ".direction\0");
				GLint lightDirection = glGetUniformLocation(shader, location);
				glUniform3f(lightDirection, lightSource->lightDirection.x, lightSource->lightDirection.y, lightSource->lightDirection.z);

				// -- light attenuation paramenters --
				//fifth variable: float constant
				strcpy(n, ".constant\0");
				GLint constantValue = glGetUniformLocation(shader, location);
				glUniform1f(constantValue, lightSource->constant);
				//sixth variable: float linear
				strcpy(n, ".linear\0");
				GLint linearValue = glGetUniformLocation(shader, location);
				glUniform1f(linearValue, lightSource->linear);
				//seventh variable: float quadratic
				strcpy(n, ".quadratic\0");
				GLint quadraticValue = glGetUniformLocation(shader, location);
				glUniform1f(quadraticValue, lightSource->quadratic);
				i++;
			}

			free(location);

			GLint numFocalLights = glGetUniformLocation(shader, "numOfFocalLights");
			glUniform1i(numFocalLights, i);
		}
		else
		{
			GLint numFocalLights = glGetUniformLocation(shader, "numOfFocalLights");
			glUniform1i(numFocalLights, 0);
		}
	}
	else
	{
		GLint numDirLights = glGetUniformLocation(shader, "numOfDirectionalLights");
		glUniform1i(numDirLights, 0);

		GLint numPointLights = glGetUniformLocation(shader, "numOfPointLights");
		glUniform1i(numPointLights, 0);

		GLint numFocalLights = glGetUniformLocation(shader, "numOfFocalLights");
		glUniform1i(numFocalLights, 0);
	}
}

void M_Renderer3D::RenderUI(GameObject* go)
{
	C_RenderedUI* cRenderedUI = go->GetComponent<C_RenderedUI>();
	cRenderedUI->Draw();
}

void M_Renderer3D::StepAnimatedMesh(GameObject* go, R_Mesh* mesh, uint shader)
{
	OPTICK_EVENT();

	C_Animator* cAnimator = go->GetParent()->GetComponent<C_Animator>();
	if (cAnimator != nullptr && cAnimator->GetMeshesInfo().size() > 0)
	{
		AnimatorClip* animatorClip = cAnimator->GetSelectedClip();
		if (animatorClip != nullptr)
		{
			if (animatorClip->GetFinishedBool() && animatorClip->GetLoopBool())
				animatorClip->SetFinishedBool(false);

			transformsAnim.clear();
			transformsAnim.shrink_to_fit();
			if (!animatorClip->GetFinishedBool())
			{
				float animationTimeSec = cAnimator->GetAnimTime();
				cAnimator->GetBoneTransforms(animationTimeSec, transformsAnim, go);
			}
			else
			{
				if (cAnimator->GetLastBoneTransforms(mesh).size() > 0)
					transformsAnim = cAnimator->GetLastBoneTransforms(mesh);
				else
					cAnimator->GetBoneTransforms(0, transformsAnim, go);
			}

			GLint finalBonesMatrices = glGetUniformLocation(shader, "finalBonesMatrices");
			glUniformMatrix4fv(finalBonesMatrices, transformsAnim.size(), GL_FALSE, transformsAnim.begin()->ptr());
			GLint isAnimated = glGetUniformLocation(shader, "isAnimated");
			glUniform1i(isAnimated, true);
		}
	}
}

// Method to receive and manage events
void M_Renderer3D::OnNotify(const Event& event)
{
	// Manage events
}

bool M_Renderer3D::GetVsync() const
{
	return vsync;
}

void M_Renderer3D::SetVsync(bool vsync)
{
	if (this->vsync != vsync)
	{
		this->vsync = vsync;
		if (SDL_GL_SetSwapInterval(vsync ? 1 : 0) < 0) {
			CONSOLE_LOG("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
			appLog->AddLog("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
		}

		SDL_GL_GetSwapInterval() ? appLog->AddLog("Vsync Started\n") : appLog->AddLog("Vsync Stopped\n");;
	}
}

void M_Renderer3D::OnResize()
{
	glViewport(0, 0, engine->GetWindow()->GetWidth(), engine->GetWindow()->GetHeight());
	if (engine->GetCamera3D()->currentCamera)
	{
		engine->GetCamera3D()->currentCamera->SetAspectRatio(engine->GetWindow()->GetWidth() / engine->GetWindow()->GetHeight());
	}
	else
	{
		KOFI_ERROR(" Renderer 3D: Could not recalculate the aspect ratio! Error: Current Camera was nullptr.");
	}
	RecalculateProjectionMatrix();
}

void M_Renderer3D::DrawCylinder(float4x4 transform)
{
}

void M_Renderer3D::DrawCone(float3 position, float3 forward, float3 up, float angle, int length)
{
	glColor3f(0.0f, 1.0f, 1.0f);
	glLineWidth(6.0f);

	Quat rot;

	rot.SetFromAxisAngle(up, angle / 2 * DEGTORAD);
	float3 av = rot * forward;

	rot.SetFromAxisAngle(up, -angle / 2 * DEGTORAD);
	float3 bv = rot * forward;

	float3 a = position + av * length;
	float3 b = position + bv * length;

	glBegin(GL_LINES);
	glVertex3f(position.x, position.y, position.z);
	glVertex3f(a.x, a.y, a.z);
	glVertex3f(a.x, a.y, a.z);
	glVertex3f(b.x, b.y, b.z);
	glVertex3f(b.x, b.y, b.z);
	glVertex3f(position.x, position.y, position.z);
	glEnd();

	glLineWidth(1.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
}

void M_Renderer3D::DrawCircle(float3 position, float radius, float3 color, float stroke)
{
	GLfloat x = 0.0;
	GLfloat y = 0.0;
	GLfloat height = 1.0;
	GLfloat angle = 0.0;
	GLfloat angle_stepsize = 0.1;

	/** Draw the circle on top of cylinder */
	glColor3f(color.x / 255.0f, color.y / 255.0f, color.z / 255.0f);
	glLineWidth(stroke);
	glBegin(GL_POLYGON);
	angle = 0.0;
	while (angle < 2 * M_PI) {
		x = radius * cos(angle);
		y = radius * sin(angle);
		glVertex3f(x, y, height);
		angle = angle + angle_stepsize;
	}
	glVertex3f(radius, 0.0, height);
	glEnd();

	// Actually, this does not render a circle, instead, renders a cylinder xd didnt know how to do the circle one
	//float halfLength = 5;
	//int slices = 10;
	//for (int i = 0; i < slices; i++) {
	//	float theta = ((float)i) * 2.0 * M_PI;
	//	float nextTheta = ((float)i + 1) * 2.0 * M_PI;
	//	glBegin(GL_TRIANGLE_STRIP);
	//	/*vertex at middle of end */
	//	glVertex3f(0.0, halfLength, 0.0);
	//	/*vertices at edges of circle*/
	//	glVertex3f(radius * cos(theta), halfLength, radius * sin(theta));
	//	glVertex3f(radius * cos(nextTheta), halfLength, radius * sin(nextTheta));
	//	/* the same vertices at the bottom of the cylinder*/
	//	glVertex3f(radius * cos(nextTheta), -halfLength, radius * sin(nextTheta));
	//	glVertex3f(radius * cos(theta), -halfLength, radius * sin(theta));
	//	glVertex3f(0.0, -halfLength, 0.0);
	//	glEnd();
	//}
}

void M_Renderer3D::DrawLine(float3 a, float3 b)
{
	glColor3f(0.0f, 1.0f, 1.0f);
	glLineWidth(3.0f);

	glBegin(GL_LINES);
	glVertex3f(a.x, a.y, a.z);
	glVertex3f(b.x, b.y, b.z);
	glEnd();

	glLineWidth(1.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
}

// Debug ray for mouse picking
void M_Renderer3D::DrawRay()
{
	glColor3f(0.0f, 1.0f, 1.0f);
	glLineWidth(3.0f);

	glBegin(GL_LINES);
	glVertex3f(ray.a.x, ray.a.y, ray.a.z);
	glVertex3f(ray.b.x, ray.b.y, ray.b.z);
	glEnd();

	glLineWidth(1.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
}

void M_Renderer3D::SetRay(LineSegment ray)
{
	this->ray = ray;
}

LineSegment M_Renderer3D::GetRay()
{
	return ray;
}

void M_Renderer3D::InitFrameBuffers()
{
	show_viewport_window = true;

	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &textureBuffer);
	glBindTexture(GL_TEXTURE_2D, textureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, engine->GetWindow()->GetWidth(), engine->GetWindow()->GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0); //Unbind texturef

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureBuffer, 0);

	//Render Buffers
	glGenRenderbuffers(1, &renderBufferoutput);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferoutput);

	//Bind tex data with render buffers
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, engine->GetWindow()->GetWidth(), engine->GetWindow()->GetHeight());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBufferoutput);
	//After binding tex data, we must unbind renderbuffer and framebuffer not usefull anymore
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glGenFramebuffers(1, &previewFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, previewFrameBuffer);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenTextures(1, &previewTextureBuffer);
	glBindTexture(GL_TEXTURE_2D, previewTextureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, engine->GetWindow()->GetWidth(), engine->GetWindow()->GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0); //Unbind texture

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, previewTextureBuffer, 0);
	//Render Buffers
	glGenRenderbuffers(1, &renderPreviewBufferoutput);
	glBindRenderbuffer(GL_RENDERBUFFER, renderPreviewBufferoutput);

	//Bind tex data with render buffers
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, engine->GetWindow()->GetWidth(), engine->GetWindow()->GetHeight());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderPreviewBufferoutput);

	//After binding tex data, we must unbind renderbuffer and framebuffer not usefull anymore
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void M_Renderer3D::PrepareFrameBuffers()
{
	OPTICK_EVENT();

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void M_Renderer3D::UnbindFrameBuffers()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void M_Renderer3D::ResizeFrameBuffers(int width, int height)
{
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glBindTexture(GL_TEXTURE_2D, textureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferoutput);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void M_Renderer3D::ResizePreviewFrameBuffers(int width, int height)
{
	float lwidth, lheight = 0.0f;
	float aspectRatio = 0.0f;
	aspectRatio = engine->GetEditor()->viewportSize.x / engine->GetEditor()->viewportSize.y;
	if (width < height)
	{
		lwidth = width;
		lheight = width * aspectRatio;
	}
	else
	{

		lheight = height;
		lwidth = height * aspectRatio;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, previewFrameBuffer);
	glBindTexture(GL_TEXTURE_2D, previewTextureBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lwidth, lheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glBindRenderbuffer(GL_RENDERBUFFER, renderPreviewBufferoutput);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, lwidth, lheight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void M_Renderer3D::ReleaseFrameBuffers()
{
	if (textureBuffer != 0) glDeleteTextures(1, &textureBuffer);
	if (previewTextureBuffer != 0) glDeleteTextures(1, &previewTextureBuffer);
	if (frameBuffer != 0) glDeleteFramebuffers(1, &frameBuffer);
	if (previewFrameBuffer != 0) glDeleteFramebuffers(1, &previewFrameBuffer);
	if (renderBufferoutput != 0) glDeleteRenderbuffers(1, &renderBufferoutput);
	if (renderPreviewBufferoutput != 0) glDeleteRenderbuffers(1, &renderPreviewBufferoutput);
}

uint M_Renderer3D::GetTextureBuffer()
{
	return textureBuffer;
}

uint M_Renderer3D::GetPreviewTextureBuffer()
{
	return previewTextureBuffer;
}

void M_Renderer3D::AddParticle(R_Texture& tex, Color color, const float4x4 transform, float distanceToCamera)
{
	ParticleRenderer pRenderer = ParticleRenderer(tex, color, transform);
	particles.insert(std::make_pair(distanceToCamera, pRenderer));
}

void M_Renderer3D::RenderAllParticles()
{
	OPTICK_EVENT();

	for (auto it = particles.rbegin(); it != particles.rend(); ++it) {
		RenderParticle(&it->second);
	}

	particles.clear();
}

void M_Renderer3D::InitDepthMapFramebufferAndTexture()
{
	//First we'll create a framebuffer object for rendering the depth map:
	glGenFramebuffers(1, &depthMapFBO);

	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	//we are only interested in storing the depth component inside the texture
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		//engine->GetWindow()->GetWidth(), engine->GetWindow()->GetHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		depthMapResolution, depthMapResolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	//once the texture is created, attach it to the framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
	//configure it to explicitly not have color data
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void M_Renderer3D::FillShadowMap()
{
	OPTICK_EVENT();
	//skip filling the shadow map if there are no lights in the scene
	//std::vector<GameObject*> directionalLights = engine->GetSceneManager()->GetCurrentScene()->GetLights(SourceType::DIRECTIONAL);
	GameObject* light = engine->GetSceneManager()->GetCurrentScene()->GetShadowCaster();
	if (light)
	{
		C_Material* lightMat = light->GetComponent<C_Material>();
		if (!lightMat)
			return;
		
		for (auto go : gameObejctsToRenderDistanceSphere)
		{
			if (!go->active)
				continue;

			//Get needed variables
			C_Material* cMat = go->GetComponent<C_Material>();
			C_Mesh* cMesh = go->GetComponent<C_Mesh>();

			if (!cMesh || !cMat)
				continue;

			R_Mesh* mesh = cMesh->GetMesh();

			uint shader = lightMat->GetMaterial()->shaderProgramID;
			glUseProgram(shader);

			//fill uniforms
			ShadowMapUniforms(cMesh, shader, light);
			
			if(mesh)
				mesh->Draw();

			glUseProgram(0);

		}
	}
}

void M_Renderer3D::ShadowMapUniforms(C_Mesh* cMesh, uint shader, GameObject* light)
{
	C_LightSource* lightSource = light->GetComponent<C_LightSource>();
	if (!lightSource)
		return;

	DirectionalLight* dirLight = (DirectionalLight*)light->GetComponent<C_LightSource>()->GetLightSource(); 
	if (!dirLight)
		return;

	int i = 0;
	// Passing Shader Uniforms
	GLint model_matrix = glGetUniformLocation(shader, "model_matrix");
	glUniformMatrix4fv(model_matrix, 1, GL_FALSE, cMesh->owner->GetTransform()->GetGlobalTransform().Transposed().ptr());

	GLint lightSpaceMatrix = glGetUniformLocation(shader, "lightSpaceMatrix");
	glUniformMatrix4fv(lightSpaceMatrix, 1, GL_FALSE, dirLight->lightSpaceMatrix.Transposed().ptr());
}

ParticleRenderer::ParticleRenderer()
{
	tex = nullptr;
}

ParticleRenderer::ParticleRenderer(R_Texture& tex, Color color, const float4x4 transform):
tex(&tex),
color(color),
transform(transform)
{

}

void M_Renderer3D::RenderParticle(ParticleRenderer* particle)
{
	GLint saveProgram;

	glGetIntegerv(GL_CURRENT_PROGRAM, &saveProgram);
	glUseProgram(0);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_TEXTURE_2D);
	//glColor3f(1.0f, 1.0f, 1.0f);

	if (particle->tex->GetTextureId() != TEXTUREID_DEFAULT)
	{
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, particle->tex->GetTextureId());
	}
	
	glColor4f(particle->color.r, particle->color.g, particle->color.b, particle->color.a);

	glPushMatrix();
	glMultMatrixf(particle->transform.Transposed().ptr());	// model
	//Drawing to tris in direct mode
	glBegin(GL_TRIANGLES);

	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(.5f, -.5f, .0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-.5f, .5f, .0f);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-.5f, -.5f, .0f);

	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(.5f, -.5f, .0f);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(.5f, .5f, .0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-.5f, .5f, .0f);

	glEnd();

	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_BLEND);
	glUseProgram(saveProgram);
}

OcclusionQuery::OcclusionQuery()
{
	glGenQueries(1, &queryID);
	CONSOLE_LOG("Created occlusion query with ID");
}

OcclusionQuery::~OcclusionQuery()
{
}

void OcclusionQuery::BeginQuery() const
{
	glBeginQuery(GL_SAMPLES_PASSED, queryID);

}

void OcclusionQuery::EndQuery()
{
	glEndQuery(GL_SAMPLES_PASSED);
	glGetQueryObjectiv(queryID, GL_QUERY_RESULT, &samplesPassed);

}

GLint OcclusionQuery::GetNumSamplesPassed() const
{
	return samplesPassed;
}

GLint OcclusionQuery::GetResultAvilable() const
{
	GLint available;
	glGetQueryObjectiv(queryID,
		GL_QUERY_RESULT_AVAILABLE,
		&available);
	return available;
}

bool OcclusionQuery::AnySamplesPassed() const
{
	return samplesPassed > 0;
}

bool M_Renderer3D::GOComp::operator()(const GameObject* lhs, const GameObject* rhs) const
{
	float3 cameraPosition = lhs->GetEngine()->GetCamera3D()->gameCamera->owner->GetTransform()->GetPosition();
	return cameraPosition.DistanceSq(lhs->GetTransform()->GetPosition()) < cameraPosition.DistanceSq(rhs->GetTransform()->GetPosition());
}

void M_Renderer3D::RenderOutline(R_Mesh* rMesh, C_Camera* camera, GameObject* go)
{
	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);

	if (outlineShader != nullptr)
	{
		uint32 shaderProgram = outlineShader->shaderProgramID;
		if (shaderProgram != 0)
		{
			glUseProgram(shaderProgram);

			// Passing Shader Uniforms
			GLint model_matrix = glGetUniformLocation(shaderProgram, "model_matrix");
			glUniformMatrix4fv(model_matrix, 1, GL_FALSE, go->GetTransform()->GetGlobalTransform().Transposed().ptr());
			GLint view_location = glGetUniformLocation(shaderProgram, "view");
			glUniformMatrix4fv(view_location, 1, GL_FALSE, camera->GetViewMatrix().Transposed().ptr());
			GLint projection_location = glGetUniformLocation(shaderProgram, "projection");
			glUniformMatrix4fv(projection_location, 1, GL_FALSE, camera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());

			// Shader outline
			GLuint uinformLoc = glGetUniformLocation(shaderProgram, "outlineThickness");
			glUniform1f(uinformLoc, rMesh->GetOutlineThickness());
			GLint color = glGetUniformLocation(shaderProgram, "outlineColor");
			glUniform4f(color, rMesh->GetOutlineColor().x, rMesh->GetOutlineColor().y, rMesh->GetOutlineColor().z, rMesh->GetOutlineColor().w);

			// Animations
			GLuint isAnimated = glGetUniformLocation(shaderProgram, "activeAnimation");
			glUniform1f(isAnimated, rMesh->IsAnimated());
			if (rMesh->IsAnimated())
			{
				if (!transformsAnim.empty())
				{
					GLint finalBonesMatrices = glGetUniformLocation(shaderProgram, "finalBonesMatrices");
					glUniformMatrix4fv(finalBonesMatrices, transformsAnim.size(), GL_FALSE, transformsAnim.begin()->ptr());
					GLint isAnimated = glGetUniformLocation(shaderProgram, "isAnimated");
					glUniform1i(isAnimated, true);
				}
			}
		}
	}

	//if (rMesh->IsAnimated() == false)
	//{
		glBindVertexArray(rMesh->VAO);
		glDrawElements(GL_TRIANGLES, rMesh->indicesSizeBytes / sizeof(uint), GL_UNSIGNED_INT, nullptr);
	//}
	//else
	//{
	//	glBindVertexArray(rMesh->VAO);
	//	glDrawElements(GL_TRIANGLES, rMesh->indicesSizeBytes / sizeof(uint), GL_UNSIGNED_INT, nullptr);
	//}

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	glBindVertexArray(0);				// Unbind Vertex Array
	glBindTexture(GL_TEXTURE_2D, 0);	// Unbind Texture

	glUseProgram(0);
}