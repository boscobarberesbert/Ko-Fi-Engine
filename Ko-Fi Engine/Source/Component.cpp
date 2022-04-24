#include "Component.h"
#include "GameObject.h"

Component::Component(GameObject* parent) : owner(parent)
{
	type = ComponentType::NONE;

	if (parent)
	{
		parent->PushBackComponent(this);

	}
}

const char* Component::GetNameByComponentType(ComponentType type)
{
	switch (type)
	{
	case ComponentType::NONE:
		return "NONE";
	case ComponentType::MESH:
		return "MESH";
	case ComponentType::MATERIAL:
		return "MATERIAL";
	case ComponentType::CAMERA:
		return "CAMERA";
	case ComponentType::COLLIDER2:
		return "COLLIDER";
	case ComponentType::SCRIPT:
		return "SCRIPT";
	case ComponentType::RIGID_BODY2:
		return "RIGID_BODY";
	case ComponentType::TRANSFORM2D:
		return "TRANSFORM2D";
	case ComponentType::CANVAS:
		return "CANVAS";
	case ComponentType::IMAGE:
		return "IMAGE";
	case ComponentType::BUTTON:
		return "BUTTON";
	case ComponentType::TEXT:
		return "TEXT";
	case ComponentType::TRANSFORM:
		return "TRANSFORM";
	case ComponentType::INFO:
		return "INFO";
	default:
		return "ERROR, goto Component.cpp line 13";
	}
	return "ERROR, no component type!";
}

void Component::DrawDeleteButton(GameObject* owner, Component* component)
{
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
	{
		showPopup = true;
	}

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
		}

		// No delete button
		if (ImGui::Button("CANCEL"))																
		{
			showPopup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

const char* componentTypeUtils::ComponentTypeToString(ComponentType e)
{
	
		const std::map<ComponentType, const char*> componentTypeStrings{
			{ ComponentType::MESH, "MESH" },
			{ ComponentType::MATERIAL, "MATERIAL" },
			{ ComponentType::PARTICLE, "PARTICLE" },
			{ ComponentType::CAMERA, "CAMERA" },
			{ ComponentType::COLLIDER2, "COLLIDER" },
			{ ComponentType::SCRIPT, "SCRIPT" },
			{ ComponentType::RIGID_BODY2, "RIGID BODY" },
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

		};
		auto   it = componentTypeStrings.find(e);
		return it == componentTypeStrings.end() ? "Out of range" : it->second;
	
}