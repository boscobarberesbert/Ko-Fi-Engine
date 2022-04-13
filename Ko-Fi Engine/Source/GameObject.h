#ifndef __GAMEOBJECT_H__
#define __GAMEOBJECT_H__

#include "R_Mesh.h"
#include <vector>

#include "Component.h"

class KoFiEngine;
//class C_Mesh;
class C_Transform;
class C_Mesh;
class C_Info;
class C_Script;
class C_Collider;
class C_Animator;
class C_LightSource;

enum class Tag 
{
	TAG_UNTAGGED,
	TAG_PLAYER,
	TAG_ENEMY,
	TAG_WALL,
	TAG_PROJECTILE
};

class GameObject
{
public:
	GameObject(int id, KoFiEngine* engine, const char* name = nullptr, bool is3D = true);
	GameObject();
	//GameObject(const char* path, int id, const char* name = nullptr);
	~GameObject();

	bool Start();
	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate(float dt);
	bool CleanUp();

	bool OnPlay();
	bool OnPause();
	bool OnStop();
	bool OnResume();
	bool OnTick();

	void Enable();
	void Disable();

	template<class T> T* GetComponent()
	{
		T* component = nullptr;
		for (Component* c : components)
		{
			component = dynamic_cast<T*>(c);
			if (component)
				break;
		}
		return component;
	}
	
	// New way
	void DeleteComponent(Component* component);
	void PushBackComponent(Component* component) { components.push_back(component); }
	Component* AddComponentByType(ComponentType componentType);
	void AttachChild(GameObject* child);
	void RemoveChild(GameObject* child);

	void PropagateTransform();

	// Old way
	void SetName(const char* name);
	const char* GetName() const;

	std::vector<GameObject*> GetChildren() const;
	void SetChild(GameObject* child);
	GameObject* GetParent() const;

	C_Transform* GetTransform() const;
	std::vector<Component*> GetComponents() const;
	AABB BoundingAABB();
	void SetUID(uint uid);
	uint GetUID() const;

	void SetParentUID(uint uid);
	uint GetParentUID() const;

	bool HasChildrenWithUID(uint uid);
	bool HasParentWithUID(uint uid);

	KoFiEngine* GetEngine() const;
	void SetEngine(KoFiEngine* engine);
	
	bool PrefabSaveJson();
	bool PrefabSave(Json& jsonFile);
	bool LoadPrefabJson(const char* path, bool exists);
	bool LoadPrefab(Json& jsonFile);
	bool UpdatePrefab(Json& jsonFile);

	bool IsSelected();
	void LoadSceneFromName(std::string name);
	void SetChangeScene(bool changeSceneLua, std::string sceneNameLua);
private:
	std::string SetObjectNumberedName(const char* _name);

public:
	template<class T> T* CreateComponent()
	{
		T* newComponent = new T(this);
		return newComponent;
	}

public:
	bool active = true;
	int numScripts = 0;
	bool is3D = true;
	bool isPrefab = false;
	bool changeScene = false;
	std::string sceneName;
	std::string prefabPath;
	Tag tag;

	std::vector<GameObject*> children;
private:
	std::string name;
	std::vector<Component*> components;
	GameObject* parent = nullptr;
	uint uid;
	uint parentUid;

	KoFiEngine* engine = nullptr;
	C_Transform* transform = nullptr;
};

#endif // !__GAMEOBJECT_H__