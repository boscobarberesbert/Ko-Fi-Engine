#ifndef __C_CAPSULE_COLLIDER_H__
#define __C_CAPSULE_COLLIDER_H__
#include "Component.h"
#include "reactphysics3d/reactphysics3d.h"
#include "MathGeoLib/Math/float3.h"
#include "MathGeoLib/Math/float2.h"

class C_CapsuleCollider : public Component
{
public:
	//constructors
	C_CapsuleCollider(GameObject* parent);
	~C_CapsuleCollider();

	//Game Loop
	bool Start() override;
	bool Update(float dt) override;
	bool CleanUp() override;
	bool InspectorDraw(PanelChooser* chooser) override; //OnGui

	//Serialization
	void Save(Json& json) const override;
	void Load(Json& json) override;

	//Updates
	void UpdateFilter();
	void UpdateScaleFactor();
	void UpdateIsTrigger();
	void UpdateCenter();

	// Getters & Setters
	inline std::string GetFilter() const { return filter; };
	inline void SetFilter(const std::string newFilter) { filter = newFilter; };

	inline bool GetIsTrigger() const { return isTrigger; }
	inline void SetIsTrigger(const bool newIsTrigger) { isTrigger = newIsTrigger;}

	inline float2 GetScaleFactor() const { return scaleFactor; }
	inline void SetScaleFactor(const float2 newScaleFactor) { scaleFactor = newScaleFactor; }
	inline void SetScaleFactor(const float scaleX, const float scaleY) { scaleFactor = float2(scaleX, scaleY); }

	inline float3 GetCenter() const { return center; }
	inline void SetCenter(const float3 newCenter) { center = newCenter; }
	inline void SetCenter(const float centerX, const float centerY, const float centerZ) { center = float3(centerX, centerY, centerZ); }

private:
	reactphysics3d::CapsuleShape* capsuleShape = nullptr;
	reactphysics3d::Collider* collider = nullptr;

	// Variables serialized
	std::string filter = "";
	bool isTrigger = false;
	float2 scaleFactor = float2(1,1);
	float3 center = { 0,0,0 };

};
#endif // !__C_CAPSULE_COLLIDER_H__
