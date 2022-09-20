#ifndef __MODEL_H__
#define __MODEL_H__

#include "Globals.h"
#include "json.hpp"
#include "MathGeoLib/Math/float3.h"
#include "MathGeoLib/Math/Quat.h"
#include "Resource.h"

using Json = nlohmann::json;

class ModelNode
{
public:
	ModelNode();
	ModelNode(std::string name, UID uid, UID parentUid, UID mesh, UID texture, std::string texturePath, float3 pos, Quat rot, float3 scale);
	~ModelNode();

public:
	std::string name;
	UID uid;
	UID parentUid;
	UID mesh;
	UID texture;
	std::string texturePath;
	float3 position;
	Quat rotation;
	float3 scale;
};

class R_Model : public Resource
{
public:
	R_Model();
	~R_Model() override;

	bool SaveMeta(Json& json) const override;

public:
	std::vector<ModelNode> nodes;
	UID animation;
	std::string animationName;
};

#endif // !__MODEL_H__