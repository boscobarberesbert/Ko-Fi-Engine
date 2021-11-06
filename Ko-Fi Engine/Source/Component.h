#pragma once
class GameObject;
#include "imgui.h"
enum class COMPONENT_TYPE
{
	COMPONENT_TRANSFORM = 0,
	COMPONENT_MESH,
	COMPONENT_MATERIAL
};
class Component {
public:

	Component(COMPONENT_TYPE type): type(type),active(true) {}
	~Component(){}

	virtual void Enable() { active = true; }
	virtual bool Start() { return true; }
	virtual bool PreUpdate() { return true; }
	virtual bool Update() {return true;}
	virtual bool PostUpdate() {return true;}
	virtual bool InspectorDraw() {return true;}
	virtual void Disable() { active = false; }
public:
	COMPONENT_TYPE type;
private:
	bool active;
	GameObject* owner;

};