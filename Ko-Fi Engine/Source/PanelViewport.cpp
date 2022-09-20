// Modules
#include "PanelViewport.h"
//#include "ViewportFrameBuffer.h"
// Modules
#include "Engine.h"
#include "M_Camera3D.h"
#include "M_SceneManager.h"
#include "M_Editor.h"
#include "M_Input.h"
#include "M_Window.h"
#include "M_FileSystem.h"
#include "M_Renderer3D.h"
#include "M_ResourceManager.h"
//Importer
#include "I_Texture.h"
//Resources
#include "R_Texture.h"

// GameObject
#include "GameObject.h"
#include "C_Material.h"
#include "C_Camera.h"

#include "Importer.h"
#include "R_Texture.h"

#include "Log.h"
#include "FSDefs.h"
// Tools
#include <imgui.h>
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "imgui_internal.h"

#include "optick.h"

PanelViewport::PanelViewport(M_Editor* editor, KoFiEngine* engine)
{
	this->editor = editor;
	this->engine = engine;
	panelName = "Viewport";
}

PanelViewport::~PanelViewport()
{
}

bool PanelViewport::Start()
{
	speedCameraIcon = new R_Texture();
	litIcon = new R_Texture();
	gizmoMoveIcon = new R_Texture();
	gizmoRotateIcon = new R_Texture();
	gizmoScaleIcon = new R_Texture();

	std::string name = ICONS_DIR + std::string("video.png");
	Importer::GetInstance()->textureImporter->Import(name.c_str(), speedCameraIcon, true);
	name = ICONS_DIR + std::string("lit_mode.png");
	Importer::GetInstance()->textureImporter->Import(name.c_str(), litIcon, true);
	name = ICONS_DIR + std::string("translate.png");
	Importer::GetInstance()->textureImporter->Import(name.c_str(), gizmoMoveIcon, true);
	name = ICONS_DIR + std::string("3d-rotate.png");
	Importer::GetInstance()->textureImporter->Import(name.c_str(), gizmoRotateIcon, true);
	name = ICONS_DIR + std::string("maximize.png");
	Importer::GetInstance()->textureImporter->Import(name.c_str(), gizmoScaleIcon, true);
	return true;
}

bool PanelViewport::Update()
{
	OPTICK_EVENT();
#ifdef KOFI_GAME
#ifdef IMGUI_HAS_VIEWPORT
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
#else 
	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
#endif
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
#ifdef KOFI_GAME
	if (ImGui::Begin("Scene", &editor->panelsState.showViewportWindow, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoNav))
#else
	if (ImGui::Begin("Scene", &editor->panelsState.showViewportWindow, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize))
#endif KOFI_GAME

	{
		editor->scenePanelOrigin = ImGui::GetWindowPos();
		editor->scenePanelOrigin.x += ImGui::GetWindowContentRegionMin().x;
		editor->scenePanelOrigin.y += ImGui::GetWindowContentRegionMin().y;

		int winX, winY;
		engine->GetWindow()->GetPosition(winX, winY);

		editor->mouseScenePosition.x = engine->GetInput()->GetMouseX() - editor->scenePanelOrigin.x;
		editor->mouseScenePosition.y = engine->GetInput()->GetMouseY() - editor->scenePanelOrigin.y;


		ImVec2 viewportSize = ImGui::GetCurrentWindow()->Size;

		if (viewportSize.x != editor->lastViewportSize.x || viewportSize.y != editor->lastViewportSize.y)
		{
			editor->lastViewportSize = viewportSize;
			engine->GetCamera3D()->currentCamera->SetAspectRatio(viewportSize.x / viewportSize.y);
			engine->GetRenderer()->ResizeFrameBuffers(viewportSize.x, viewportSize.y);
		}
		editor->viewportSize = viewportSize;
		ImGui::Image((ImTextureID)engine->GetRenderer()->GetTextureBuffer(), viewportSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSETS_ITEM");
			if (payload != nullptr)
			{
				if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly))
				{
					std::string path = (const char*)payload->Data;
					path = engine->GetResourceManager()->GetValidPath(path.c_str());
					Resource* resource = engine->GetResourceManager()->GetResourceFromLibrary(path.c_str());
					if (resource != nullptr)
						engine->GetSceneManager()->LoadResourceToScene(resource);
					else
					{
						if (path.find(".json") != std::string::npos)
							Importer::GetInstance()->sceneImporter->LoadScene(engine->GetSceneManager()->GetCurrentScene(), engine->GetFileSystem()->GetNameFromPath(path).c_str());
						else
							dragDropPopup = true;
					}
					// OLD STUF
					//if (path.find(".fbx") != std::string::npos || path.find(".md5mesh") != std::string::npos)
					//{
					//	Importer::GetInstance()->sceneImporter->Import(path.c_str());
					//}
					//else if (path.find(".jpg") != std::string::npos || path.find(".png") != std::string::npos)
					//{
					//	for (int i = 0; i < engine->GetEditor()->panelGameObjectInfo.selectedGameObjects.size(); i++)
					//	{
					//		if (engine->GetEditor()->panelGameObjectInfo.selectedGameObjects[i] != 0)
					//		{
					//			GameObject* go = engine->GetSceneManager()->GetCurrentScene()->GetGameObject(engine->GetEditor()->panelGameObjectInfo.selectedGameObjects[i]);

					//			if (go->GetComponent<C_Material>())
					//			{
					//				R_Texture* texture = new R_Texture();
					//				Importer::GetInstance()->textureImporter->Import(path.c_str(), texture);

					//				go->GetComponent<C_Material>()->texture = texture;
					//				//cMaterial->textures.push_back(texture);
					//			}
					//		}
					//	}
					//	// Apply texture
					//	
					//}
				}
			}
			ImGui::EndDragDropTarget();
		}
#ifndef KOFI_GAME
		DrawViewportBar();
#endif // KOFI_GAME

		if (ImGui::IsMouseClicked(1)) ImGui::SetWindowFocus();
		isFocused = ImGui::IsWindowFocused() && ImGui::IsWindowHovered();
	}
	ImGui::End();
#ifdef KOFI_GAME
	ImGui::PopStyleVar(2);
#else
	ImGui::PopStyleVar();
#endif // KOFI_GAME


	return true;
}

bool PanelViewport::IsWindowFocused()
{
	return isFocused;
}

void PanelViewport::SetIsFocused(bool isFocused)
{
	this->isFocused = isFocused;
}

void PanelViewport::DrawViewportBar()
{
	ImGui::SetItemAllowOverlap();
	ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth()/2 - (5*24),ImGui::GetWindowContentRegionMin().y+8));
	ImGui::BeginGroup();
	//Overlay Items
	if (ImGui::ImageButton((ImTextureID)litIcon->GetTextureId(), ImVec2(24, 24)))
	{
		ImGui::OpenPopup("Lit Popup");
	}
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)gizmoMoveIcon->GetTextureId(), ImVec2(24, 24)))
	{
		editor->engine->GetSceneManager()->SetGizmoOperation(ImGuizmo::OPERATION::TRANSLATE);
	}
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)gizmoRotateIcon->GetTextureId(), ImVec2(24, 24)))
	{
		editor->engine->GetSceneManager()->SetGizmoOperation(ImGuizmo::OPERATION::ROTATE);
	}
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)gizmoScaleIcon->GetTextureId(), ImVec2(24, 24)))
	{
		editor->engine->GetSceneManager()->SetGizmoOperation(ImGuizmo::OPERATION::SCALE);
	}
	ImGui::SameLine();
	if (ImGui::ImageButton((ImTextureID)speedCameraIcon->GetTextureId(),ImVec2(24,24)))
	{
		ImGui::OpenPopup("Camera Speed Popup");
	}
//Popups
	if (ImGui::BeginPopup("Lit Popup"))
	{
		if (ImGui::Selectable("Wireframe"))
		{

			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		}
		if (ImGui::Selectable("Lit"))
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopup("Camera Speed Popup"))
	{
		int newSpeedMultiplier = editor->engine->GetCamera3D()->GetSpeedMultiplier();
		if (ImGui::SliderInt("##Camera Speed", &newSpeedMultiplier, 1.0f, 5.0f))
		{
			editor->engine->GetCamera3D()->ChangeSpeed(newSpeedMultiplier);
		}
		ImGui::EndPopup();
	}
	if (dragDropPopup)
	{
		ImGui::OpenPopup("Unsupported Extension");
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Unsupported Extension", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("The current file extension is unsupported.\nYour asset is not imported yet, maybe you should try refreshing?\n\n");

			if (ImGui::Button("OK", ImVec2(120, 0)))
			{
				dragDropPopup = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::SetItemDefaultFocus();
			ImGui::EndPopup();
		}
	}


	
	
	ImGui::EndGroup();
}
