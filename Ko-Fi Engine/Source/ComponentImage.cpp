#include "ComponentImage.h"

#include "ComponentMesh.h"
#include "GameObject.h"
#include "ComponentTransform2D.h"
#include "ComponentMaterial.h"
#include "ComponentTransform.h"
#include "Camera3D.h"
#include "Engine.h"
#include "Editor.h"
#include "Window.h"

#include "PanelChooser.h"
#include "SceneIntro.h"

#include "par_shapes.h"
#include "Log.h"
#include "UI.h"

#include "MathGeoLib/Math/Quat.h"
#include "ImGuiAppLog.h"
#include "SDL_image.h"

#include "SDL.h"

#include "glew.h"
#include <vector>

ComponentImage::ComponentImage(GameObject* parent) : Component(parent)
{
	type = ComponentType::IMAGE;
	glGenFramebuffers(1, &fboId);
}

ComponentImage::~ComponentImage()
{

}

bool ComponentImage::Update(float dt)
{
	return true;
}

bool ComponentImage::PostUpdate(float dt)
{
	ComponentTransform2D* cTransform = this->owner->GetComponent<ComponentTransform2D>();

	SDL_Rect rect;

	rect.x = 0;
	rect.y = 0;
	rect.w = openGLTexture.GetTextureWidth();
	rect.h = openGLTexture.GetTextureHeight();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fboId);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, openGLTexture.GetTextureId(), 0);

	float2 normalizedPosition = cTransform->GetNormalizedPosition();
	float2 normalizedSize = cTransform->GetNormalizedSize();

	float2 lowerLeft = { normalizedPosition.x, normalizedPosition.y };
	float2 upperRight = { lowerLeft.x + normalizedSize.x * mask.x, lowerLeft.y + normalizedSize.y * mask.y };
	glBlitFramebuffer(0, 0, rect.w * mask.x, rect.h * mask.y, lowerLeft.x, lowerLeft.y, upperRight.x, upperRight.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	return true;
}

bool ComponentImage::InspectorDraw(PanelChooser* panelChooser)
{
	if (ImGui::CollapsingHeader("Image")) {
		// Texture display
		ImGui::Text("Texture: ");
		ImGui::SameLine();
		if (openGLTexture.GetTextureId() == 0) // Supposedly there is no textureId = 0 in textures array
		{
			ImGui::Text("None");
		}
		else
		{
			ImGui::Text(openGLTexture.GetTexturePath());
			ImGui::Image((ImTextureID)openGLTexture.GetTextureId(), ImVec2(128, 128), ImVec2(0, 1), ImVec2(1, 0));
		}

		if (panelChooser->IsReadyToClose("AddTextureImage")) {
			if (panelChooser->OnChooserClosed() != nullptr) {
				std::string path = panelChooser->OnChooserClosed();
				SetTexture(path.c_str());
			}
		}

		if (ImGui::Button("Set Texture")) {
			panelChooser->OpenPanel("AddTextureImage", "png");
		}

		ImGui::DragFloat2("Mask", &mask[0], 0.005f, 0.0f, 1.0f);
	}

	return true;
}

/*SDL_Texture* ComponentImage::LoadTexture(const char* path)
{
	SDL_Texture* texture = NULL;
	SDL_Surface* surface = IMG_Load(path);

	if (surface == NULL)
	{
		appLog->AddLog("Could not load surface with path: %s. IMG_Load: %s", path, IMG_GetError());
	}
	else
	{
		texture = LoadSurface(surface);
		SDL_FreeSurface(surface);
	}

	return texture;
}*/

/*SDL_Texture* const ComponentImage::LoadSurface(SDL_Surface* surface)
{
	SDL_Texture* texture = SDL_CreateTextureFromSurface(owner->GetEngine()->GetUI()->renderer, surface);

	if (texture == NULL)
	{
		appLog->AddLog("Unable to create texture from surface! SDL Error: %s\n", SDL_GetError());
	}
	else
	{
		owner->GetEngine()->GetUI()->loadedTextures.push_back(texture);
	}

	return texture;
}*/

void ComponentImage::SetTexture(const char* path)
{
	FreeTextures();
	Importer::GetInstance()->textureImporter->Import(path, &openGLTexture);
	//SDLTexture = LoadTexture(path);
}

void ComponentImage::FreeTextures()
{
	//if (SDLTexture != nullptr)
	//	SDL_DestroyTexture(SDLTexture);
	if (openGLTexture.GetTextureId() != 0) {
		GLuint id = openGLTexture.GetTextureId();
		glDeleteTextures(1, &id);
	}
}

/*void ComponentImage::OnLoad(const JSONReader& reader)
{
	// Loading texture name
	if (reader.HasMember("Texture name"))
	{
		const rapidjson::Value& itemTextureName = reader["Texture name"];
		texture.name = itemTextureName.GetString();
		texture = App->textures->Load(texture.name);
	}

	// Loading texture size
	if (reader.HasMember("Size"))
	{
		const rapidjson::Value& itemSize = reader["Size"];
		int width_ = 0;
		int height_ = 0;
		int i = 0;
		for (rapidjson::Value::ConstValueIterator it = itemSize.Begin(); it != itemSize.End(); ++it)
		{
			if (i == 0) width_ = it->GetInt();
			else if (i == 1) height_ = it->GetInt();
			i++;
		}
		texture.width = width_;
		texture.height = height_;
	}

	// Loading texture Id
	if (reader.HasMember("Texture ID"))
	{
		const rapidjson::Value& itemTextureId = reader["Texture ID"];
		//texture.id = itemTextureId.GetInt();
	}

	// Loading image color
	if (reader.HasMember("Image color"))
	{
		const rapidjson::Value& itemSize = reader["Image color"];
		float x_ = 0.000f, y_ = 0.000f, z_ = 0.000f, w_ = 0.000f;
		int i = 0;
		for (rapidjson::Value::ConstValueIterator it = itemSize.Begin(); it != itemSize.End(); ++it)
		{
			if (i == 0) x_ = it->GetDouble();
			else if (i == 1) y_ = it->GetDouble();
			else if (i == 2) z_ = it->GetDouble();
			else if (i == 3) w_ = it->GetDouble();
			i++;
		}
		imageColor = { x_, y_, z_, w_ };
	}
}

void ComponentImage::OnSave(JSONWriter& writer) const
{
	writer.String("Image");
	writer.StartObject();

	// Saving texture name
	writer.String("Texture name");
	writer.String(texture.name.c_str());

	// Saving texture size
	writer.String("Size");
	writer.StartArray();
	writer.Int(texture.width);
	writer.Int(texture.height);
	writer.EndArray();

	// Saving texture id
	writer.String("Texture ID");
	writer.Int(texture.id);

	// Saving image color
	writer.String("Image color");
	writer.StartArray();
	writer.Double(imageColor.x);
	writer.Double(imageColor.y);
	writer.Double(imageColor.z);
	writer.Double(imageColor.w);
	writer.EndArray();

	writer.EndObject();
}*/