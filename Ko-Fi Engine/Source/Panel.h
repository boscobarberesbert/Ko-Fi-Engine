#pragma once
#include <string>

#include "optick.h"

class Panel
{
public:
	Panel() : active(false) 
	{}

	void Init()
	{
		active = true;
	}
	// Called before render is available
	virtual bool Awake()
	{
		return true;
	}
	// Called before the first frame
	virtual bool Start() { return true; }
	virtual bool PreUpdate() {
		OPTICK_EVENT();
		return true; }
	virtual bool Update() {
		OPTICK_EVENT();
		return true;
	}
	virtual bool PostUpdate(){
		OPTICK_EVENT();
		return true;
	}
	virtual bool CleanUp(){ return true; }
	virtual void Focus(){
	}

public:
	std::string panelName;
	bool active;
};