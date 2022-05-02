#include "Globals.h"
#include "M_Camera3D.h"
#include "Engine.h"
#include "M_Input.h"
#include "M_SceneManager.h"
#include "M_Editor.h"
#include "GameObject.h"
#include "C_Transform.h"
#include "C_Mesh.h"
#include "C_Script.h"
#include "M_Renderer3D.h"
#include "PanelViewport.h"
#include "Scripting.h"
#include "GameObject.h"

#include "SDL.h"
#include "Log.h"
#include "ImGuiAppLog.h"
#include "MathGeoLib/Geometry/LineSegment.h"
#include "MathGeoLib/Geometry/Triangle.h"
#include "MathGeoLib/Math/float4.h"

#include "optick.h"

M_Camera3D::M_Camera3D(KoFiEngine* engine) : Module()
{
	name = "Camera";
	this->engine = engine;
	engineCameraObject = new GameObject(0, engine, "");
	engineCamera = new C_Camera(engineCameraObject, true);
	engineCamera->SetIsEngineCamera(true);

	currentCamera = engineCamera;

	currentCamera->SetUp(float3(-0.000605105015f, 0.804563046f, 0.593866348f));
	currentCamera->SetFront(float3(-0.000820143265f, -0.593866587f, 0.804562271f));

	currentCamera->SetPosition(float3(-3.65002513f, 153.790665f, -131.349442f));
	currentCamera->SetReference(float3(0.0f, 0.0f, 0.0f));

	currentCamera->LookAt(float3::zero);
}

M_Camera3D::~M_Camera3D()
{
}

bool M_Camera3D::Awake(Json configModule)
{
	bool ret = true;

	ret = LoadConfiguration(configModule);

	return ret;
}

bool M_Camera3D::Start()
{
	CONSOLE_LOG("Setting up the camera");
	appLog->AddLog("Setting up the camera\n");

	bool ret = true;

	return ret;
}

// -----------------------------------------------------------------
bool M_Camera3D::Update(float dt)
{
	OPTICK_EVENT();

	if (engine->GetEditor()->GetPanel<PanelViewport>())
		if (!engine->GetEditor()->GetPanel<PanelViewport>()->IsWindowFocused())
			return true;

	if (currentCamera->GetIsEngineCamera())
	{
		CheckMouseMotion(dt);
	}


	return true;
}

// -----------------------------------------------------------------
bool M_Camera3D::CleanUp()
{
	CONSOLE_LOG("Cleaning camera");
	appLog->AddLog("Cleaning camera\n");
	//RELEASE(currentCamera);
	//RELEASE(engineCamera);
	//RELEASE(gameCamera);
	return true;
}

// Method to receive and manage events
void M_Camera3D::OnNotify(const Event& event)
{
	// Manage events
}

void M_Camera3D::OnGui()
{
	if (ImGui::CollapsingHeader("Module Camera"))
	{
		float newVerticalFov = currentCamera->GetVerticalFov();
		if (ImGui::DragFloat("Vertical fov", &newVerticalFov))
		{
			currentCamera->SetVerticalFov(newVerticalFov);
		}
		float2 planeDistances = {currentCamera->GetNearPlaneDistance(),currentCamera->GetFarPlaneDistance()};
		if (ImGui::DragFloat("Near plane distance", &(planeDistances[0])))
		{
			currentCamera->SetViewPlaneDistances(planeDistances.x, planeDistances.y);
		}

	}
}

void M_Camera3D::CheckInput(float dt)
{
	float3 newPos(0, 0, 0);
	float speed = currentCamera->GetCameraSpeed() * dt;

	if (engine->GetInput()->GetKey(SDL_SCANCODE_LSHIFT) == KEY_REPEAT) speed *= 5.0f;

	if (engine->GetInput()->GetKey(SDL_SCANCODE_Q) == KEY_REPEAT) newPos.y -= speed;
	if (engine->GetInput()->GetKey(SDL_SCANCODE_E) == KEY_REPEAT) newPos.y += speed;

	// Focus --> NEEDS TO BE FIXED... SOME (MESH) FUNCTIONS DEPEND ON A PRIMITIVE LIBRARY WE STILL DON'T HAVE IMPLEMENTED.
	if (engine->GetInput()->GetKey(SDL_SCANCODE_F) == KEY_DOWN)
	{
		// TO DO: Manage current object selection by the game object itself! Not by its index...
		if (/*engine->GetEditor()->gameobjectSelected != nullptr <-- Should be this way*/
			engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID != -1)
		{
			// If we change the previous TO DO, this will be no longer needed...
			GameObject* gameObjectSelected =
				engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID);

			if (C_Mesh* mesh = /*App->editor->gameobjectSelected->GetComponent<C_Mesh>()*/
				gameObjectSelected->GetComponent<C_Mesh>())
			{
				const float3 meshCenter = mesh->GetCenterPointInWorldCoords(); // FIX THIS FUNCTION
				currentCamera->LookAt(meshCenter);
				const float meshRadius = mesh->GetSphereRadius(); // FIX THIS FUNCTION
				const float currentDistance = meshCenter.Distance(currentCamera->GetPosition());
				const float desiredDistance = (meshRadius * 2) / atan(currentCamera->GetCameraFrustum().HorizontalFov());
				currentCamera->SetPosition(currentCamera->GetPosition() + currentCamera->GetFront() * (currentDistance - desiredDistance));
			}
			else
			{
				C_Transform* transform = gameObjectSelected->GetTransform();
				if (transform != nullptr)
					currentCamera->LookAt(gameObjectSelected->GetTransform()->GetPosition());
			}
		}
	}

	vec3 spot(0, 0, 0); // Spot where the current selected game object is located.
	if (engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID != -1)
	{
		C_Transform* transform = engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID)->GetTransform();
		if (transform != nullptr) {
			spot.x = (engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID)->GetTransform())->GetPosition().x;
			spot.y = (engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID)->GetTransform())->GetPosition().y;
			spot.z = (engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID)->GetTransform())->GetPosition().z;
		}
	}
	if (engine->GetInput()->GetKey(SDL_SCANCODE_F) == KEY_DOWN)
	{
		currentCamera->LookAt(float3(5, 5, 5));
	}

	if (engine->GetInput()->GetKey(SDL_SCANCODE_W) == KEY_REPEAT) newPos += currentCamera->GetFront() * speed;
	if (engine->GetInput()->GetKey(SDL_SCANCODE_S) == KEY_REPEAT) newPos -= currentCamera->GetFront() * speed;

	

	if (engine->GetInput()->GetKey(SDL_SCANCODE_A) == KEY_REPEAT) newPos -= currentCamera->GetRight() * speed;
	if (engine->GetInput()->GetKey(SDL_SCANCODE_D) == KEY_REPEAT) newPos += currentCamera->GetRight() * speed;

	//Ruedecita del raton
	if (engine->GetInput()->GetMouseZ() > 0) newPos += currentCamera->GetFront() * speed * 2;
	if (engine->GetInput()->GetMouseZ() < 0) newPos -= currentCamera->GetFront() * speed * 2;

	currentCamera->SetPosition(currentCamera->GetPosition() + newPos); // MODULE CAMERA REVISION CHECKPOINT --> CHECK AND FIX ERRORS FIRST!
}

void M_Camera3D::CheckMouseMotion(float dt)
{

	// Mouse motion ----------------

	bool hasRotated = false;

	if (engine->GetInput()->GetMouseButton(SDL_BUTTON_RIGHT) == KEY_REPEAT)
	{
		CheckInput(dt);
		int dx = -engine->GetInput()->GetMouseXMotion();
		int dy = -engine->GetInput()->GetMouseYMotion();

		if (engine->GetInput()->GetKey(SDL_SCANCODE_LALT) == KEY_REPEAT) {
			if (/*engine->GetEditor()->gameobjectSelected != nullptr*/
				engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID != -1)
			{
				const float newDeltaX = (float)dx * currentCamera->GetCameraSensitivity();
				const float newDeltaY = (float)dy * currentCamera->GetCameraSensitivity();

				currentCamera->SetReference(engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()
					->panelGameObjectInfo.selectedGameObjectID)->GetComponent<C_Transform>()->GetPosition()); /*engine->GetEditor()->gameobjectSelected->transform->GetPosition()*/
	
				Quat orbitMat = Quat::RotateY(newDeltaX * .1f);

				if (abs(currentCamera->GetUp().y) < 0.3f) // Avoid gimball lock on up & down apex
				{
					if (currentCamera->GetPosition().y > currentCamera->GetReference().y && newDeltaY < 0.f)
						orbitMat = orbitMat * math::Quat::RotateAxisAngle(currentCamera->GetRight(), newDeltaY * .1f);
					if (currentCamera->GetPosition().y < currentCamera->GetReference().y && newDeltaY > 0.f)
						orbitMat = orbitMat * math::Quat::RotateAxisAngle(currentCamera->GetRight(), newDeltaY * .1f);
				}
				else
				{
					orbitMat = orbitMat * math::Quat::RotateAxisAngle(currentCamera->GetRight(), newDeltaY * .1f);
				}

				currentCamera->SetPosition(orbitMat * (currentCamera->GetPosition() - currentCamera->GetReference()) + currentCamera->GetReference());

				currentCamera->LookAt(currentCamera->GetReference());
			}
		}
		else
		{
			if (dx != 0)
			{
				const float newDeltaX = (float)dx * currentCamera->GetCameraSensitivity();
				float deltaX = newDeltaX + 0.95f * (currentCamera->GetLastDeltaX() - newDeltaX); //lerp for smooth rotation acceleration to avoid jittering
				currentCamera->SetLastDeltaX(deltaX);
				Quat rotateY = Quat::RotateY(currentCamera->GetUp().y >= 0.f ? deltaX * .1f : -deltaX * .1f);
				currentCamera->SetUp(rotateY * currentCamera->GetUp());
				currentCamera->SetFront(rotateY * currentCamera->GetFront());
				hasRotated = true;
			}

			if (dy != 0)
			{
				const float newDeltaY = (float)dy * currentCamera->GetCameraSensitivity();
				float deltaY = newDeltaY + 0.95f * (currentCamera->GetLastDeltaY() - newDeltaY); //lerp for smooth rotation acceleration to avoid jittering
				currentCamera->SetLastDeltaY(deltaY);
				Quat rotateX = Quat::RotateAxisAngle(currentCamera->GetRight(), -deltaY * .1f);
				currentCamera->SetUp(rotateX * currentCamera->GetUp());
				currentCamera->SetFront(rotateX * currentCamera->GetFront());
				hasRotated = true;
			}
		}
	}
	if (!hasRotated)
	{
		currentCamera->SetLastDeltaX(0.0f);
		currentCamera->SetLastDeltaY(0.0f);
	}

}

void M_Camera3D::OnPlay()
{
	if (gameCamera == nullptr)
	{
		GameObject* go = engine->GetSceneManager()->GetCurrentScene()->CreateEmptyGameObject("MainCamera");
		C_Camera* cCamera = go->CreateComponent<C_Camera>();
		cCamera->SetIsMainCamera(true);
		gameCamera = cCamera;

	}
		
	currentCamera = gameCamera;
}

void M_Camera3D::OnStop()
{
	currentCamera = engineCamera;
}

bool M_Camera3D::SaveConfiguration(Json& configModule) const
{
	return true;
}

bool M_Camera3D::LoadConfiguration(Json& configModule)
{
	return true;
}

bool M_Camera3D::InspectorDraw()
{
	if (ImGui::CollapsingHeader("Engine Camera##"))
	{
		int newSpeedMultiplier = engineCamera->GetSpeedMultiplier();
		if (ImGui::SliderInt("Camera Speed", &newSpeedMultiplier, 1.0f, 5.0f))
		{
			engineCamera->ChangeSpeed(newSpeedMultiplier);
		}
	}
	return true;
}

void M_Camera3D::SetGameCamera(C_Camera* gameCamera)
{
	if (this->gameCamera != nullptr && this->gameCamera != gameCamera)this->gameCamera->SetIsMainCamera(false);

	this->gameCamera = gameCamera;
	if (engine->GetSceneManager()->GetGameState() == GameState::PLAYING)
	{
		this->currentCamera = this->gameCamera;
	}
}

void M_Camera3D::OnClick(SDL_Event event)
{
	if (event.button.type != SDL_MOUSEBUTTONDOWN || (event.button.button != SDL_BUTTON_LEFT && event.button.button != SDL_BUTTON_RIGHT)) return;
	if (engine->GetEditor()->GetPanel<PanelViewport>())
		if (!engine->GetEditor()->GetPanel<PanelViewport>()->IsWindowFocused())
			return;

	if (event.button.button == SDL_BUTTON_LEFT && engine->GetSceneManager()->GetGameState() != GameState::PLAYING && !ImGuizmo::IsOver())
	{
		GameObject* hit = engine->GetCamera3D()->MousePicking();
		if (hit != nullptr)
		{
			CONSOLE_LOG("%s", hit->GetName());
			if (engine->GetEditor()->contr == true && engine->GetSceneManager()->GetGameState() != GameState::PLAYING && !ImGuizmo::IsOver())
			{
				auto it = std::find (engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.begin(), engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.end(), hit->GetUID());
				if (it != engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.end()) return;

				engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.push_back(hit->GetUID());
			}
			else
			{
				engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.clear();
				engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.shrink_to_fit();
				engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.push_back(hit->GetUID());
			}
			//engine->GetEditor()->panelGameObjectInfo.selectedGameObjectID = hit->GetUID();
		}
		else {
			engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.clear();
			engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.shrink_to_fit();
		}
	}
	else if (engine->GetSceneManager()->GetGameState() == GameState::PLAYING)
	{
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			engine->GetCamera3D()->MousePicking();
		}
		else if (event.button.button == SDL_BUTTON_RIGHT)
		{
			engine->GetCamera3D()->MousePicking(true);
		}
	}
}

//void M_Camera3D::OnSave(JSONWriter& writer) const
//{
//	writer.String("camera");
//	writer.StartObject();
//	SAVE_JSON_FLOAT(verticalFOV)
//		SAVE_JSON_FLOAT(nearPlaneDistance)
//		SAVE_JSON_FLOAT(farPlaneDistance)
//		SAVE_JSON_FLOAT(cameraSpeed)
//		SAVE_JSON_FLOAT(cameraSensitivity)
//		writer.EndObject();
//}
//
//void M_Camera3D::OnLoad(const JSONReader& reader)
//{
//	if (reader.HasMember("camera"))
//	{
//		const auto& config = reader["camera"];
//		LOAD_JSON_FLOAT(verticalFOV);
//		LOAD_JSON_FLOAT(nearPlaneDistance);
//		LOAD_JSON_FLOAT(farPlaneDistance);
//		LOAD_JSON_FLOAT(cameraSpeed);
//		LOAD_JSON_FLOAT(cameraSensitivity);
//	}
//	RecalculateProjection();
//}

GameObject* M_Camera3D::MousePicking(const bool& isRightButton)
{
	float normalX = engine->GetEditor()->mouseScenePosition.x / engine->GetEditor()->lastViewportSize.x;
	float normalY = engine->GetEditor()->mouseScenePosition.y / engine->GetEditor()->lastViewportSize.y;

	normalX = (normalX - 0.5f) * 2.0f;
	normalY = -(normalY - 0.5f) * 2.0f;
	/*CONSOLE_LOG("x: %f", normalX);
	CONSOLE_LOG("y: %f", normalY);*/

	LineSegment newRay = currentCamera->GetCameraFrustum().UnProjectLineSegment(normalX, normalY);
	engine->GetSceneManager()->GetCurrentScene()->ray = newRay;

	std::vector<GameObject*> sceneGameObjects = engine->GetSceneManager()->GetCurrentScene()->gameObjectList;
	std::map<float, GameObject*> hitGameObjects;

	// Find all hit GameObjects
	for (size_t i = 0; i < sceneGameObjects.size(); i++)
	{
		C_Mesh* m = sceneGameObjects[i]->GetComponent<C_Mesh>();
		if (m != nullptr)
		{
			bool hit = newRay.Intersects(m->GetGlobalAABB());

			if (hit)
			{
				float dNear;
				float dFar;
				hit = newRay.Intersects(m->GetGlobalAABB(), dNear, dFar);
				hitGameObjects[dNear] = sceneGameObjects[i];
			}
		}
	}

	for (std::map<float, GameObject*>::iterator it = hitGameObjects.begin(); it != hitGameObjects.end(); it++)
	{
		GameObject* gameObject = it->second;

		LineSegment rayLocal = newRay;
		rayLocal.Transform(gameObject->GetComponent<C_Transform>()->GetGlobalTransform().Inverted());

		C_Mesh* cMesh = gameObject->GetComponent<C_Mesh>();

		if (cMesh != nullptr)
		{
			R_Mesh* rMesh = cMesh->GetMesh();

			if (rMesh == nullptr) continue;

			// Convert our float pointer to a std::vector of float3
			std::vector<float3> vertices;
			float* fvertices = rMesh->vertices;
			for (int i = 0; i < rMesh->verticesSizeBytes / sizeof(float); i += 3)
			{
				vertices.push_back(float3(fvertices[i], fvertices[i + 1], fvertices[i + 2]));
			}

			for (size_t i = 0; i < rMesh->indicesSizeBytes / sizeof(unsigned int); i += 3)
			{
				// Create every triangle
				float3 v1;
				v1.x = vertices[rMesh->indices[i]].x;
				v1.y = vertices[rMesh->indices[i]].y;
				v1.z = vertices[rMesh->indices[i]].z;

				float3 v2;
				v2.x = vertices[rMesh->indices[i + 1]].x;
				v2.y = vertices[rMesh->indices[i + 1]].y;
				v2.z = vertices[rMesh->indices[i + 1]].z;

				float3 v3;
				v3.x = vertices[rMesh->indices[i + 2]].x;
				v3.y = vertices[rMesh->indices[i + 2]].y;
				v3.z = vertices[rMesh->indices[i + 2]].z;

				const Triangle triangle(v1, v2, v3);

				float distance;
				if (rayLocal.Intersects(triangle, &distance, &lastMouseClick))
				{
					//float tmp = lastMouseClick.y;
					//lastMouseClick.y = lastMouseClick.z;
					//lastMouseClick.z = tmp;
					lastMouseClick.x *= gameObject->GetTransform()->GetScale().x;
					lastMouseClick.y *= gameObject->GetTransform()->GetScale().y;
					lastMouseClick.z *= gameObject->GetTransform()->GetScale().z;
					lastMouseClick.x += gameObject->GetTransform()->GetPosition().x;
					lastMouseClick.y += gameObject->GetTransform()->GetPosition().y;
					lastMouseClick.z += gameObject->GetTransform()->GetPosition().z;
					return gameObject;
				}
			}
		}
	}

	return nullptr;
}

float2 M_Camera3D::WorldToScreen(float3 position)
{
	float4 clipSpacePos = currentCamera->GetCameraFrustum().ProjectionMatrix() * (currentCamera->GetCameraFrustum().ViewMatrix() * float4(position, 1.0f));
	float3 ndcSpacePos = clipSpacePos.xyz() / clipSpacePos.w;
	float2 partial = ((ndcSpacePos.xy() + float2(1.0f)) / 2.0f);
	float2 viewport = float2(engine->GetEditor()->lastViewportSize.x, engine->GetEditor()->lastViewportSize.y);
	return float2(partial.x * viewport.x, partial.y * viewport.y) + float2(0, 0);
}

float3 M_Camera3D::GetLastMouseClick() const
{
	return lastMouseClick;
}


