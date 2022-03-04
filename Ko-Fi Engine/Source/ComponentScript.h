#pragma once
#ifndef __COMPONENT_SCRIPT_H__
#define __COMPONENT_SCRIPT_H__

#include "Component.h"

#include "Scripting.h"

class GameObject;
class ComponentTransform;

class ComponentScript : public Component
{
public:
	ComponentScript(GameObject* gameObject);
	~ComponentScript();

	bool Start() override;
	bool Update(float dt) override;
	bool PostUpdate(float dt) override;
	bool CleanUp() override;
	bool InspectorDraw(PanelChooser* chooser); // (OnGui)
	bool LoadScript(std::string scriptPath);
	void SetRunning(const bool& setTo);

public:
	sol::load_result script; // Check if it can be private
	Scripting* handler = nullptr;

private:
	std::string fileName;
	std::string fullName;
	int numScript;
	bool isRunning = false; // Temp
	bool scriptLoaded = false;

};

#endif // __COMPONENT_SCRIPT_H__