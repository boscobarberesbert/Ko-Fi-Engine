#include "M_UI.h"
#include "glew.h"
#include "MathGeoLib/Math/float2.h"

#include "Engine.h"
#include "GameObject.h"
#include "M_Editor.h"
#include "ImGuiAppLog.h"
#include "M_Camera3D.h"
#include "M_SceneManager.h"
#include "Scene.h"
#include "M_Window.h"
#include "M_FileSystem.h"
#include "R_Texture.h"


#include "C_Mesh.h"
#include "C_Transform2D.h"
#include "C_Camera.h"

#include "SDL.h"

#include <queue>
#include <string>
#include <vector>

#include "optick.h"

MyPlane::MyPlane(GameObject* _owner) {
	vertices.push_back({ 0, 0, 0 });
	vertices.push_back({ 0, 1, 0 });
	vertices.push_back({ 1, 0, 0 });
	vertices.push_back({ 1, 1, 0 });

	texCoords.push_back({ 0, 1 });
	texCoords.push_back({ 0, 0 });
	texCoords.push_back({ 1, 1 });
	texCoords.push_back({ 1, 0 });

	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);
	indices.push_back(2);
	indices.push_back(1);

	indices.push_back(2);
	indices.push_back(1);
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);

	glGenBuffers(1, &vertexBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float3) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &indexBufferId);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * indices.size(), &indices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &textureBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, textureBufferId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float2) * texCoords.size(), &texCoords[0], GL_STATIC_DRAW);

	owner = _owner;
}

MyPlane::~MyPlane()
{
	indices.clear();
	indices.shrink_to_fit();
	texCoords.clear();
	texCoords.shrink_to_fit();
	vertices.clear();
	vertices.shrink_to_fit();
	owner = nullptr;
}

void MyPlane::DrawPlane2D(R_Texture* texture, SDL_Color color)
{
	if (texture->GetTextureWidth() != -1 && texture->GetTextureHeight() != -1)
		DrawPlane2D(texture->GetTextureId(), color);
	else
		DrawPlane2D((unsigned int)0, color);
}

void MyPlane::DrawPlane2D(unsigned int texture, SDL_Color color) {
	OPTICK_EVENT();

	C_Transform2D* cTransform = owner->GetComponent<C_Transform2D>();

	float2 normalizedPosition = cTransform->GetNormalizedPosition();
	float2 normalizedSize = cTransform->GetNormalizedSize();

	float3 position3d = { normalizedPosition.x, normalizedPosition.y, -1};
	float3 rotation3d = cTransform->GetRotation();
	Quat quaternion3d = Quat::FromEulerXYZ(rotation3d.x, rotation3d.y, rotation3d.z);
	float3 size3d = { normalizedSize.x, normalizedSize.y, 1 };

	math::float4x4 transform = float4x4::FromTRS(position3d, quaternion3d, size3d);

	glEnableClientState(GL_VERTEX_ARRAY);

	if (texture != 0) {
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, textureBufferId);
		glTexCoordPointer(2, GL_FLOAT, 0, NULL);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferId);
	glVertexPointer(3, GL_FLOAT, 0, NULL);

	if (texture != 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferId);

	glPushMatrix();
	glMultMatrixf(transform.Transposed().ptr());
	glColor3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, NULL);
	glPopMatrix();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (texture != 0) {
		glBindBuffer(GL_TEXTURE_COORD_ARRAY, 0);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	glDisableClientState(GL_VERTEX_ARRAY);
}

M_UI::M_UI(KoFiEngine* engine) : Module()
{
	name = "UI";
	this->engine = engine;
	Uint32 flags = SDL_RENDERER_ACCELERATED;
	renderer = SDL_CreateRenderer(engine->GetWindow()->window, -1, flags);
	TTF_Init();
	rubik = TTF_OpenFont("Assets/Fonts/Rubik_Mono_One/RubikMonoOne-Regular.ttf", 60);
}

M_UI::~M_UI()
{
}

bool M_UI::Awake(Json configModule)
{
	bool ret = true;

	ret = LoadConfiguration(configModule);

	return ret;
}

bool M_UI::Start()
{
	return true;
}

bool M_UI::PreUpdate(float dt)
{
	return true;
}

bool M_UI::Update(float dt) {

	return true;
}

bool M_UI::PostUpdate(float dt)
{

	return true;
}

bool M_UI::CleanUp()
{
	TTF_CloseFont(rubik);
	return true;
}

void M_UI::OnNotify(const Event& event)
{
}

bool M_UI::SaveConfiguration(Json& configModule) const
{
	return true;
}

bool M_UI::LoadConfiguration(Json& configModule)
{
	return true;
}

bool M_UI::InspectorDraw()
{
	return true;
}

void M_UI::PrepareUIRender()
{
	right = engine->GetCamera3D()->currentCamera->GetRight();
	up = engine->GetCamera3D()->currentCamera->GetUp();
	front = engine->GetCamera3D()->currentCamera->GetFront();
	position = engine->GetCamera3D()->currentCamera->GetPosition();

	float2 offset = { engine->GetEditor()->lastViewportSize.x / 2, engine->GetEditor()->lastViewportSize.y / 2 };
	engine->GetCamera3D()->currentCamera->SetPosition({ offset.x, offset.y, 0 });
	engine->GetCamera3D()->currentCamera->LookAt({ offset.x, offset.y, -1 });

	engine->GetCamera3D()->currentCamera->SetProjectionType(C_Camera::CameraType::KOFI_ORTHOGRAPHIC);
	
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(engine->GetCamera3D()->currentCamera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(engine->GetCamera3D()->currentCamera->GetViewMatrix().Transposed().ptr());

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);

	glDisable(GL_DEPTH_TEST);

	glDisable(GL_LIGHTING);
}

void M_UI::EndUIRender()
{
	glEnable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);

	glDisable(GL_BLEND);

	glPopMatrix();

	engine->GetCamera3D()->currentCamera->SetPosition({ position.x, position.y, position.z });
	engine->GetCamera3D()->currentCamera->SetFrontAndUp(front, up);
	
	engine->GetCamera3D()->currentCamera->SetProjectionType(C_Camera::CameraType::KOFI_PERSPECTIVE);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(engine->GetCamera3D()->currentCamera->GetCameraFrustum().ProjectionMatrix().Transposed().ptr());
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(engine->GetCamera3D()->currentCamera->GetViewMatrix().Transposed().ptr());
}

float2 M_UI::GetUINormalizedMousePosition()
{
	float mouseX = engine->GetEditor()->mouseScenePosition.x;
	float mouseY = engine->GetEditor()->mouseScenePosition.y;

	return { mouseX, mouseY };
}

void M_UI::OnGui()
{
}

/*void ModuleUI::OnLoad(const JSONReader& reader)
{
}

void ModuleUI::OnSave(JSONWriter& writer) const
{
}*/