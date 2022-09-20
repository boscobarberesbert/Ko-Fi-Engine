#include "C_BoxCollider.h"
#include "Engine.h"
#include "Globals.h"
#include "GameObject.h"
#include "M_Physics.h"
#include "C_RigidBody.h"
#include "C_Mesh.h"

C_BoxCollider::C_BoxCollider(GameObject *parent) : Component(parent)
{
	type = ComponentType::BOX_COLLIDER;
	typeIndex = typeid(*this);
}

C_BoxCollider::~C_BoxCollider()
{
	CleanUp();
}

bool C_BoxCollider::Start()
{
	if (owner->GetComponent<C_Mesh>())
	{
		float3 boundingBoxSize = owner->BoundingAABB().maxPoint - owner->BoundingAABB().minPoint;
		if (boundingBoxSize.x <= 1)
			boundingBoxSize.x = 1;
		if (boundingBoxSize.y <= 1)
			boundingBoxSize.y = 1;
		if (boundingBoxSize.z <= 1)
			boundingBoxSize.z = 1;
		boxShape = owner->GetEngine()->GetPhysics()->GetPhysicsCommon().createBoxShape(reactphysics3d::Vector3(boundingBoxSize.x / 2, boundingBoxSize.y / 2, boundingBoxSize.z / 2));
		reactphysics3d::Transform transform = reactphysics3d::Transform::identity();
		collider = owner->GetComponent<C_RigidBody>()->GetBody()->addCollider(boxShape, transform);
	}
	else
	{
		float3 boundingBoxSize = float3(5, 5, 5);
		boxShape = owner->GetEngine()->GetPhysics()->GetPhysicsCommon().createBoxShape(reactphysics3d::Vector3(boundingBoxSize.x / 2, boundingBoxSize.y / 2, boundingBoxSize.z / 2));
		reactphysics3d::Transform transform = reactphysics3d::Transform::identity();
		collider = owner->GetComponent<C_RigidBody>()->GetBody()->addCollider(boxShape, transform);
	}

	return true;
}

bool C_BoxCollider::Update(float dt)
{
	OPTICK_EVENT();

	if (!owner->GetComponent<C_RigidBody>())
	{
		owner->DeleteComponent(this);
	}
	return true;
}

bool C_BoxCollider::CleanUp()
{
	if (collider)
	{
		if (owner->GetComponent<C_RigidBody>() && owner->GetComponent<C_RigidBody>()->GetBody())
			owner->GetComponent<C_RigidBody>()->GetBody()->removeCollider(collider);
	}
	if (boxShape)
		owner->GetEngine()->GetPhysics()->GetPhysicsCommon().destroyBoxShape(boxShape);

	filter.clear();
	filter.shrink_to_fit();

	return true;
}

bool C_BoxCollider::InspectorDraw(PanelChooser *chooser)
{
	if (ImGui::CollapsingHeader("Box Collider", ImGuiTreeNodeFlags_AllowItemOverlap))
	{
		if (DrawDeleteButton(owner, this))
			return true;

		std::string newFilter = GetFilter();
		if (ImGui::BeginCombo("Set Filter##", newFilter == "" ? "Set Filter" : newFilter.c_str()))
		{
			std::map<unsigned int, std::string> filterMap = owner->GetEngine()->GetPhysics()->GetFiltersMap();
			for (auto iter = filterMap.begin(); iter != filterMap.end(); ++iter)
			{
				if (ImGui::Selectable(iter->second.c_str()))
				{
					SetFilter(iter->second);
					UpdateFilter();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::Text("Current filter:");
		ImGui::SameLine();
		ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", filter.c_str());

		bool newIsTrigger = GetIsTrigger();
		if (ImGui::Checkbox("Is Trigger##", &newIsTrigger))
		{
			SetIsTrigger(newIsTrigger);
			UpdateIsTrigger();
		}

		ImGui::Text("Center");
		ImGui::SameLine();
		float3 newCenter = GetCenter();
		if (ImGui::DragFloat3("##center", &(newCenter[0]), 0.1f))
		{
			SetCenter(newCenter);
			UpdateCenter();
		}

		ImGui::Text("Scale");
		ImGui::SameLine();
		float3 newScaleFactor = GetScaleFactor();
		if (ImGui::DragFloat3("##scale", &(newScaleFactor[0]), 0.1f, 0.33f, 50000.0f))
		{
			SetScaleFactor(newScaleFactor);
			UpdateScaleFactor();
		}
	}
	else
		DrawDeleteButton(owner, this);

	return true;
}

void C_BoxCollider::Save(Json &json) const
{
	json["type"] = (int)type;

	json["filter"] = filter;
	json["is_trigger"] = isTrigger;
	json["scale_factor"] = {scaleFactor.x, scaleFactor.y, scaleFactor.z};
	json["center"] = {center.x, center.y, center.z};
}

void C_BoxCollider::Load(Json &json)
{
	filter = json.at("filter");
	UpdateFilter();

	isTrigger = json.at("is_trigger");
	UpdateIsTrigger();

	std::vector<float> values = json.at("scale_factor").get<std::vector<float>>();
	scaleFactor = float3(values[0], values[1], values[2]);
	values.clear();
	values.shrink_to_fit();;
	UpdateScaleFactor();

	values = json.at("center").get<std::vector<float>>();
	center = float3(values[0], values[1], values[2]);
	values.clear();
	values.shrink_to_fit();;
	UpdateCenter();
}

void C_BoxCollider::UpdateFilter()
{
	std::map<unsigned int, std::string> filterMap = owner->GetEngine()->GetPhysics()->GetFiltersMap();
	bool **filterMatrix = owner->GetEngine()->GetPhysics()->GetFilterMatrix();
	for (auto iter : filterMap)
	{
		if (iter.second == filter)
		{
			collider->setCollisionCategoryBits(iter.first);
			unsigned int mask = 0;
			for (int i = 0; i < filterMap.size(); ++i)
			{
				if (filterMatrix[iter.first - 1][i])
				{
					mask |= i + 1;
				}
			}
			collider->setCollideWithMaskBits(mask);
		}
	}
}

void C_BoxCollider::UpdateScaleFactor()
{
	if (owner->GetComponent<C_Mesh>())
	{
		float3 boundingBoxSize = owner->BoundingAABB().maxPoint - owner->BoundingAABB().minPoint;
		reactphysics3d::Transform oldTransform = collider->getLocalToBodyTransform();
		owner->GetComponent<C_RigidBody>()->GetBody()->removeCollider(collider);
		owner->GetEngine()->GetPhysics()->GetPhysicsCommon().destroyBoxShape(boxShape);
		if (boundingBoxSize.y <= 1)
			boundingBoxSize.y = 1;
		if (boundingBoxSize.x <= 1)
			boundingBoxSize.x = 1;
		if (boundingBoxSize.z <= 1)
			boundingBoxSize.z = 1;
		boxShape = owner->GetEngine()->GetPhysics()->GetPhysicsCommon().createBoxShape(reactphysics3d::Vector3((boundingBoxSize.x / 2) * scaleFactor.x, (boundingBoxSize.y / 2) * scaleFactor.y, (boundingBoxSize.z / 2) * scaleFactor.z));
		collider = owner->GetComponent<C_RigidBody>()->GetBody()->addCollider(boxShape, oldTransform);
	}
	else
	{
		float3 boundingBoxSize = float3(5, 5, 5);
		reactphysics3d::Transform oldTransform = collider->getLocalToBodyTransform();
		owner->GetComponent<C_RigidBody>()->GetBody()->removeCollider(collider);
		owner->GetEngine()->GetPhysics()->GetPhysicsCommon().destroyBoxShape(boxShape);
		boxShape = owner->GetEngine()->GetPhysics()->GetPhysicsCommon().createBoxShape(reactphysics3d::Vector3((boundingBoxSize.x / 2) * scaleFactor.x, (boundingBoxSize.y / 2) * scaleFactor.y, (boundingBoxSize.z / 2) * scaleFactor.z));
		collider = owner->GetComponent<C_RigidBody>()->GetBody()->addCollider(boxShape, oldTransform);
	}

	UpdateFilter();

	UpdateIsTrigger();

	UpdateCenter();
}

void C_BoxCollider::UpdateIsTrigger()
{
	collider->setIsTrigger(isTrigger);
}

void C_BoxCollider::UpdateCenter()
{
	reactphysics3d::Transform newCenterTransform(reactphysics3d::Vector3(center.x, center.y, center.z), collider->getLocalToBodyTransform().getOrientation());
	collider->setLocalToBodyTransform(newCenterTransform);
}
