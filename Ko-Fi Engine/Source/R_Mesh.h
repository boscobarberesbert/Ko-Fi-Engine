#ifndef __MESH_H__
#define __MESH_H__

#include "MathGeoLib/Geometry/AABB.h"
#include "MathGeoLib/Math/float4x4.h"
#include "MathGeoLib/Math/float3.h"
#include "MathGeoLib/Math/Quat.h"
#include "MathGeoLib/Math/TransformOps.h"

#include "Assimp.h"
#include "Resource.h"
#include "Globals.h"
#include "Log.h"

#include "par_shapes.h"

#define POSITION_LOCATION 0
#define TEX_COORD_LOCATION 1
#define NORMAL_LOCATION 2
#define BONE_LOCATION 3
#define WEIGHT_LOCATION 4

#define MAX_NUM_BONES_PER_VERTEX 4

class GameObject;
class aiBone;
class R_Animation;
struct Channel;

enum class Shape
{
	NONE,
	CUBE,
	SPHERE,
	CYLINDER,
	TORUS,
	PLANE,
	CONE
};

struct VertexBoneData
{
	uint boneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
	float weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };
	VertexBoneData() {}
	void AddBoneData(uint boneID, float weight)
	{
		for (uint i = 0; i < ARRAY_SIZE_IN_ELEMENTS(boneIDs); i++)
		{
			if (weights[i] == 0.0)
			{
				boneIDs[i] = boneID;
				weights[i] = weight;
				CONSOLE_LOG("bone %d weight %f index %i\n",boneID,weight,i);
				return;
			}
		}
		assert(0);
	}
};

struct BoneInfo
{
	float4x4 offsetMatrix;
	float4x4 finalTransformation;

	BoneInfo()
	{
		offsetMatrix = float4x4::zero;
		finalTransformation = float4x4::zero;
	}
	BoneInfo(const float4x4& offset)
	{
		offsetMatrix = offset;
		finalTransformation = float4x4::zero;
	}
};

class R_Mesh : public Resource
{
public:
	R_Mesh(Shape shape = Shape::NONE);
	~R_Mesh() override;

	// Mesh Functions
	void SetUpMeshBuffers();

	void Draw();
	void DebugDraw();

	inline void SetVertexNormals(bool vertex) { drawVertexNormals = vertex; }
	inline bool GetVertexNormals() const { return drawVertexNormals; }

	inline void SetFaceNormals(bool faces) { drawFaceNormals = faces; }
	inline bool GetFaceNormals() const { return drawFaceNormals; }

	//void GetBoneTransforms(float timeInSeconds, std::vector<float4x4>& transforms, GameObject* gameObject);
	//void ReadNodeHeirarchy(float animationTimeTicks, const GameObject* rootNode, const float4x4& parentTransform);

	void inline SetIsAnimated(bool isAnimated) { this->isAnimated = isAnimated; }
	bool inline IsAnimated() const { return isAnimated; }
	//void inline SetRootNode(const GameObject* rootNode) { this->rootNode = rootNode; }
	//inline const GameObject* GetRootNode() { return rootNode; }
	//inline const R_Animation* GetAnimation() const { return animation; }
	//void inline SetAnimation(R_Animation* animation) { this->animation = animation; }

	//const Channel* FindNodeAnim(std::string nodeName);
	//inline const std::vector<float4x4> GetLastBoneTransforms() const { return transformsAnim; }

	//uint FindPosition(float AnimationTimeTicks, const Channel* pNodeAnim);
	//void CalcInterpolatedPosition(float3& Out, float AnimationTimeTicks, const Channel* pNodeAnim);

	//uint FindRotation(float AnimationTimeTicks, const Channel* pNodeAnim);
	//void CalcInterpolatedRotation(Quat& Out, float AnimationTimeTicks, const Channel* pNodeAnim);

	//uint FindScaling(float AnimationTimeTicks, const Channel* pNodeAnim);
	//void CalcInterpolatedScaling(float3& Out, float AnimationTimeTicks, const Channel* pNodeAnim);

	//float4x4 InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ);
	//float4x4 InitRotateTransform(const aiQuaternion& quat);
	//float4x4 InitTranslationTransform(float x, float y, float z);

	//float4x4 aiMatrix3x32Float4x4(aiMatrix3x3 assimpMatrix);
	float* GetTransformedVertices(float4x4 transform);

	static R_Mesh* MeshUnion(std::vector<R_Mesh*> meshes, std::vector<float4x4> transformations);

	//float4x4 GetMatrixFromQuat(Quat quat);

	inline void SetRenderOutline(const bool& setTo) { renderOutline = setTo; };
	inline bool GetRenderOutline() { return renderOutline; };

	inline void SetOutlineThickness(const float& newThickness) { outlineThickness = newThickness; };
	inline float GetOutlineThickness() { return outlineThickness; };

	inline void SetOutlineColor(const float4& newColor) { outlineColor = newColor; };
	inline float4 GetOutlineColor() { return outlineColor; };

	// Size in Bytes
	unsigned indicesSizeBytes = 0;
	unsigned verticesSizeBytes = 0;
	unsigned normalsSizeBytes = 0;
	unsigned texCoordSizeBytes = 0;

	// Indices
	uint idIndex = 0; // index in VRAM
	uint* indices = nullptr;

	// Vertices
	uint idVertex = 0; // unique vertex in VRAM
	float* vertices = nullptr;

	// Normals
	uint idNormal = 0;
	float* normals = nullptr;

	// Texture coordinates
	uint idTexCoord = 0;
	float* texCoords = nullptr;

	// Bones
	uint idBones = 0;
	std::vector<VertexBoneData> bones;
	std::vector<BoneInfo> boneInfo;
	std::map<std::string, uint> boneNameToIndexMap;

	unsigned int VAO = 0;

	// AABB
	AABB localAABB;

	Shape meshType;

private:
	// Debug functions for drawing
	void DrawVertexNormals() const;
	void DrawFaceNormals() const;

	void PrimitiveMesh(par_shapes_mesh* primitiveMesh);

private:
	// Debug bools
	bool drawVertexNormals = false;
	bool drawFaceNormals = false;

	bool isAnimated = false;

	bool renderOutline = false;
	float outlineThickness = 50.0f;
	float4 outlineColor = float4(0.43f, 0.89f, 1.f, 0.8f);

	// Bone transforms
	//const GameObject* rootNode = nullptr;
	//R_Animation* animation = nullptr;
	//std::vector<float4x4> transformsAnim;
};

#endif // !__MESH_H__