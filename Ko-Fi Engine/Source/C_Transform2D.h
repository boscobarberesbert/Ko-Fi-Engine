#pragma once

#include "Component.h"
#include "MathGeoLib/Math/float2.h"
#include "MathGeoLib/Math/float3.h"

class KoFiEngine;
class C_Canvas;
class MyPlane;

class C_Transform2D : public Component {
public:
	enum class Anchor {
		TOP_LEFT,
		TOP_CENTER,
		TOP_RIGHT,
		LEFT,
		CENTER,
		RIGHT,
		BOTTOM_LEFT,
		BOTTOM_CENTER,
		BOTTOM_RIGHT,
	};
public:
	C_Transform2D(GameObject* parent);
	virtual ~C_Transform2D();

	void Save(Json& json) const override;
	void Load(Json& json) override;

	bool Update(float dt) override;
	bool CleanUp() override;
	virtual bool InspectorDraw(PanelChooser* chooser) override;

	// Setters
	void SetPosition(const float2& newPosition);
	void SetPositionX(const float& newPositionX);
	void SetPivot(const float2& newPivot);
	void SetRotation(const float3& newRotation);
	void SetSize(const float2& newSize);
	void SetAnchor(const Anchor& newAnchor);
	void SetMask(const float2& newMask);

	// Getters
	inline float2 GetPosition() const { return position; };
	inline float2 GetPivot() const { return pivot; };
	inline float3 GetRotation() const { return rotation; };
	inline float2 GetSize() const { return size; };
	inline Anchor GetAnchor() const { return anchor; };
	inline float2 GetMask() { return mask; };

	virtual float2 GetNormalizedPosition();
	virtual float2 GetNormalizedSize();
	virtual float2 GetNormalizedPivotOffset();
	virtual float2 GetAnchorPosition(Anchor _anchor);

	virtual C_Canvas* GetCanvas();
	virtual float2 GetCanvasLogicalSize();

	bool CheckPointWithinBounds(float2 vec);

	MyPlane* drawablePlane = nullptr;
protected:
	float2 position = { 0, 0 };
	float2 pivot = { 0, 0 };
	float3 rotation = { 0.f,0.f,0.f };
	float2 size = { 0, 0 };

	Anchor anchor = Anchor::CENTER;

private:
	float2 mask = { 1.0f, 1.0f };
};