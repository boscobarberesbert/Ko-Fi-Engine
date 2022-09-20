#pragma once

#include "Component.h"

#include "MathGeoLib/Math/float3.h"
#include "MathGeoLib/Geometry/Frustum.h"
#include "MathGeoLib/Math/MathFunc.h"

class GameObject;
class C_Transform;
using Json = nlohmann::json;

class C_Camera : public Component
{
public:

	// Camera Type
	enum CameraType {
		KOFI_PERSPECTIVE,
		KOFI_ORTHOGRAPHIC,
	};

	// Constructors
	C_Camera(GameObject* gameObject);
	virtual ~C_Camera();

	// Game Loop
	bool Start() override;
	bool PreUpdate() override;
	bool Update(float dt) override;
	bool PostUpdate(float dt) override;
	bool CleanUp() override;
	bool InspectorDraw(PanelChooser* chooser); // (OnGui)

	// Serialization
	void Save(Json& json) const override;
	void Load(Json& json) override;

	// Getters
	// OrthoGraphic Planes Have the Same SIZE!
	float GetNearPlaneHeight() const { return cameraFrustum.NearPlaneHeight(); }
	// OrthoGraphic Planes Have the Same SIZE!
	float GetNearPlaneWidth() const { return cameraFrustum.NearPlaneWidth(); }

	inline float GetAspectRatio() const { return cameraFrustum.AspectRatio(); }
	inline float3 GetReference() const { return reference; }

	inline bool IsEngineCamera() const { return isEngineCamera; }
	inline bool GetIsMainCamera() const { return isMainCamera; }

	inline Frustum GetCameraFrustum() const { return cameraFrustum; }
	inline float3 GetRight() const { return cameraFrustum.WorldRight(); }
	inline float3 GetFront() const { return cameraFrustum.Front(); }
	inline float3 GetUp() const { return cameraFrustum.Up(); }
	inline float3 GetPosition() const { return cameraFrustum.Pos(); }

	// CAUTION! --> The Value is on DEG
	// Vertical FOV is LOCKED, You can't set it.
	inline float GetVerticalFov() const { return RadToDeg(cameraFrustum.VerticalFov()); }
	// CAUTION! --> The Value is on DEG
	inline float GetHorizontalFov() const { return RadToDeg(cameraFrustum.HorizontalFov()); }

	inline float GetNearPlaneDistance() const { return cameraFrustum.NearPlaneDistance(); }
	inline float GetFarPlaneDistance() const { return cameraFrustum.FarPlaneDistance(); }
	inline bool GetIsFrustumActive() const { return isFrustumCullingActive; }
	inline bool GetIsSphereCullingActive() const { return isSphereCullingActive; }

	inline float GetOrthoWidth() const { return orthoWidth; }
	inline float GetOrthoHeight() const { return orthoHeight; }

	float4x4 GetViewMatrix() const;
	float4x4 GetWorldMatrix() const;
	float4x4 GetProjectionMatrix() const;

	// Setters
	void SetAspectRatio(const float& aspectRatio);

	inline void SetReference(float3 newReference) { reference = newReference; }

	inline void SetIsEngineCamera(bool value) { isEngineCamera = value; }
	inline void SetIsMainCamera(bool value) { isMainCamera = value; }
	
	inline void SetFrontAndUp(float3 newFront, float3 newUp) { this->cameraFrustum.SetFrontUp(newFront.Normalized(),newUp.Normalized()); }
	void SetPosition(float3 newPos);

	// IMPORTANT!! Horizontal Fov Must Be In DEG, not in radians!
	inline void SetHorizontalFov(float horizontalFov) { this->cameraFrustum.SetHorizontalFovAndAspectRatio(DegToRad(horizontalFov) , cameraFrustum.AspectRatio()); }
	inline void SetNearPlaneDistance(float nearPlaneDistance) { this->cameraFrustum.SetViewPlaneDistances(nearPlaneDistance,cameraFrustum.FarPlaneDistance());}
	inline void SetFarPlaneDistance(float farPlaneDistance) { this->cameraFrustum.SetViewPlaneDistances(cameraFrustum.NearPlaneDistance(),farPlaneDistance);}
	inline void SetViewPlaneDistances(float nearPlaneDistance,float farPlaneDistance) { this->cameraFrustum.SetViewPlaneDistances(nearPlaneDistance,farPlaneDistance);}

	inline void SetIsFrustumActive(bool value) { isFrustumCullingActive = value; }
	inline void SetIsSphereCullingActive(bool value) { isSphereCullingActive = value; }
	// Camera Functions
	void LookAt(const float3 point);
	void LookAt2(float3 front, float3 up);

	void SetProjectionType(const CameraType &type);
	//only call when projection type is ortho
	inline void SetOrthoWidth(float width) { orthoWidth = width; }
	inline void SetOrthoHeight(float height) { orthoHeight = height; }

	void ApplyOrthoWidthAndHeight();
private:
	// Frustum Culling
	void SphereCulling();
	void FrustumCulling();

public:
	void DrawSphereCulling() const;
	void DrawFrustum() const;
	void ApplyCullings();
	bool ClipsWithBBox(const AABB& refBox) const;
	void SetSCullingRadius(float radius);
private:
	// Properties
	float3 reference;
	Frustum cameraFrustum;
	float4x4 tempTransform;//Don't use this, only for updating transform internally on camera.
private:
	bool isSphereCullingActive = false;
	bool isFrustumCullingActive = false;

	bool isMainCamera = false;
	bool isEngineCamera = false;

	CameraType cameraType = KOFI_PERSPECTIVE;

	// DON'T USE, USE GETFOV INSTEAD
	float hFov, vFov = 0.0f;
	float orthoWidth = 1000.0f;
	float orthoHeight = 1000.0f;
	int sCullingRadius = 500.0f;
	
};