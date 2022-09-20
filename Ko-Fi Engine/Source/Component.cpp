#include "Component.h"
#include "GameObject.h"

Component::Component(GameObject* parent) : owner(parent)
{
	type = ComponentType::NONE;
	typeIndex = typeid(*this);

	if (parent)
		parent->PushBackComponent(this);
}

Component::~Component()
{
}

const char* Component::GetNameByComponentType(ComponentType type)
{
	return componentTypeUtils::ComponentTypeToString(type);
}

bool Component::DrawDeleteButton(GameObject* owner, Component* component)
{
	bool ret = false;

	bool showPopup = false;

	// Make the button appear at the same line of the collapsing header, positioned on the right minus 15 pixels
	ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 15);

	// label is used for differentiating imgui buttons logic
	std::string label = "X##";
	label += component->GetNameByComponentType(component->type);

	// titlePopup is used for the title that the pop up window has
	std::string titlePopup = "Do you want to delete ";																
	titlePopup += component->GetNameByComponentType(component->type);									
	titlePopup += " component?";

	// ImGui button
	if (ImGui::Button(label.c_str()))
		showPopup = true;

	// Whenever showPopup bool is true, the popups keeps open
	if (showPopup) ImGui::OpenPopup(titlePopup.c_str());
	
	// PopUp
	if (ImGui::BeginPopupModal(titlePopup.c_str()))
	{
		// Delete button
		if (ImGui::Button("DELETE"))
		{
			owner->DeleteComponent(component);
			showPopup = false;
			ImGui::CloseCurrentPopup();
			ret = true;
		}

		// No delete button
		if (ImGui::Button("CANCEL"))
		{
			showPopup = false;
			ImGui::CloseCurrentPopup();
			ret = false;
		}

		ImGui::EndPopup();
	}

	return ret;
}

const char* componentTypeUtils::ComponentTypeToString(ComponentType e)
{
	
		const std::map<ComponentType, const char*> componentTypeStrings{
			{ ComponentType::MESH, "MESH" },
			{ ComponentType::MATERIAL, "MATERIAL" },
			{ ComponentType::PARTICLE, "PARTICLE" },
			{ ComponentType::CAMERA, "CAMERA" },
			{ ComponentType::BOX_COLLIDER, "BOX_COLLIDER" },
			{ ComponentType::CAPSULE_COLLIDER, "CAPSULE_COLLIDER" },
			{ ComponentType::SPHERE_COLLIDER, "SPHERE_COLLIDER" },
			{ ComponentType::SCRIPT, "SCRIPT" },
			{ ComponentType::RIGID_BODY, "RIGID BODY" },
			{ ComponentType::AUDIO_SOURCE, "AUDIO_SOURCE" },
			{ ComponentType::AUDIO_SWITCH, "AUDIO_SWITCH" },
			{ ComponentType::ANIMATOR, "ANIMATOR" },
			{ ComponentType::CANVAS, "CANVAS" },
			{ ComponentType::IMAGE, "IMAGE" },
			{ ComponentType::BUTTON, "BUTTON" },
			{ ComponentType::TEXT, "TEXT" },
			{ ComponentType::WALKABLE, "WALKABLE" },
			{ ComponentType::FOLLOW_PATH, "FOLLOW_PATH" },
			{ ComponentType::LIGHT_SOURCE, "LIGHT_SOURCE" },
			{ ComponentType::TRANSFORM, "TRANSFORM" },
			{ ComponentType::TRANSFORM2D, "TRANSFORM2D" },
			{ ComponentType::INFO, "INFO" },
		};
		auto   it = componentTypeStrings.find(e);
		return it == componentTypeStrings.end() ? "Out of range" : it->second;
	
}