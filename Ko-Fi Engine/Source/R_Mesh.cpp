#define PAR_SHAPES_IMPLEMENTATION
#include "R_Mesh.h"
#include "Globals.h"

// Modules
#include "Engine.h"
#include "M_Renderer3D.h"

// GameObject
#include "GameObject.h"
#include "C_Material.h"
#include "C_Transform.h"
#include "C_Animator.h"
#include "C_Image.h"
#include "C_Button.h"
#include "C_Canvas.h"
#include "C_Text.h"

// Resources
#include "R_Texture.h"
#include "R_Animation.h"

// Helpers
#include "AnimatorClip.h"
#include "Channel.h"

#include "Importer.h"

#include "glew.h"
#include <gl/GL.h>
#include <gl/GLU.h>
#include <iostream>

#include "MathGeoLib/Math/float4.h"
#include "MathGeoLib/Math/float4x4.h"

#include "optick.h"


R_Mesh::R_Mesh(Shape shape) : Resource(ResourceType::MESH)
{
	verticesSizeBytes = 0;
	normalsSizeBytes = 0;
	texCoordSizeBytes = 0;
	indicesSizeBytes = 0;
	VAO = 0;
	isAnimated = false;
	meshType = shape;

	switch (shape)
	{
	case Shape::CUBE:
		PrimitiveMesh(par_shapes_create_cube());
		break;
	case Shape::CYLINDER:
		PrimitiveMesh(par_shapes_create_cylinder(20, 20));
		break;
	case Shape::SPHERE:
		PrimitiveMesh(par_shapes_create_parametric_sphere(20, 20));
		break;
	case Shape::TORUS:
		PrimitiveMesh(par_shapes_create_torus(20, 20, 0.2));
		break;
	case Shape::PLANE:
		PrimitiveMesh(par_shapes_create_plane(5, 5));
		break;
	case Shape::CONE:
		PrimitiveMesh(par_shapes_create_cone(20, 20));
		break;
	default:
		break;
	}
}

R_Mesh::~R_Mesh()
{
	glDeleteVertexArrays(1, &VAO);

	// Indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIndex);
	glDeleteBuffers(1, &idIndex);
	RELEASE_MALLOC(indices);

	// Vertices
	glBindBuffer(GL_ARRAY_BUFFER, idVertex);
	glDeleteBuffers(1, &idVertex);
	RELEASE_MALLOC(vertices);

	// Normals
	glBindBuffer(GL_ARRAY_BUFFER, idNormal);
	glDeleteBuffers(1, &idNormal);
	RELEASE_MALLOC(normals);

	// Texture coords
	glBindBuffer(GL_ARRAY_BUFFER, idTexCoord);
	glDeleteBuffers(1, &idTexCoord);
	RELEASE_MALLOC(texCoords);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	bones.clear();
	bones.shrink_to_fit();
	boneInfo.clear();
	boneInfo.shrink_to_fit();
	boneNameToIndexMap.clear();
}

void R_Mesh::SetUpMeshBuffers()
{
	// Vertex Array Object (VAO)
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Indices
	glGenBuffers(1, &idIndex);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idIndex);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSizeBytes, indices, GL_DYNAMIC_DRAW);

	// Vertices
	glGenBuffers(1, &idVertex);
	glBindBuffer(GL_ARRAY_BUFFER, idVertex);
	glBufferData(GL_ARRAY_BUFFER, verticesSizeBytes, vertices, GL_DYNAMIC_DRAW);
	// Add vertex position attribute to the vertex array object (VAO)
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(POSITION_LOCATION);

	// Normals
	glGenBuffers(1, &idNormal);
	glBindBuffer(GL_ARRAY_BUFFER, idNormal);
	glBufferData(GL_ARRAY_BUFFER, normalsSizeBytes, normals, GL_DYNAMIC_DRAW);
	// Add normals attribute to the vertex array object (VAO)
	glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(NORMAL_LOCATION);

	// Texture coords
	if (texCoords)
	{
		glGenBuffers(1, &idTexCoord);
		glBindBuffer(GL_ARRAY_BUFFER, idTexCoord);
		glBufferData(GL_ARRAY_BUFFER, texCoordSizeBytes, texCoords, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(TEX_COORD_LOCATION);
	}
	if (bones.size() > 0)
	{
		glGenBuffers(1, &idBones);
		glBindBuffer(GL_ARRAY_BUFFER, idBones);
		glBufferData(GL_ARRAY_BUFFER, sizeof(bones[0]) * bones.size(), &bones[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(BONE_LOCATION);
		glVertexAttribPointer(BONE_LOCATION, MAX_NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (void*)0);
		glEnableVertexAttribArray(WEIGHT_LOCATION);
		glVertexAttribPointer(WEIGHT_LOCATION, MAX_NUM_BONES_PER_VERTEX, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), (const GLvoid*)(MAX_NUM_BONES_PER_VERTEX * sizeof(int32_t)));
	}
	
	// Unbind any vertex array we have binded before.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void R_Mesh::Draw()
{
	OPTICK_EVENT();

	glBindVertexArray(VAO);

	glDrawElements(GL_TRIANGLES, indicesSizeBytes / sizeof(uint), GL_UNSIGNED_INT, NULL);

	//DebugDraw();

	glBindVertexArray(0);

	// Unbind Texture
	glBindTexture(GL_TEXTURE_2D, 0);
}

void R_Mesh::DebugDraw()
{
	// Debug draw
	if (drawVertexNormals)
		DrawVertexNormals();

	if (drawFaceNormals)
		DrawFaceNormals();
}

float* R_Mesh::GetTransformedVertices(float4x4 transform)
{
	float* ret = (float*)malloc(verticesSizeBytes); //DONE

	int nvf = verticesSizeBytes / sizeof(float);
	for (int i = 0; i < nvf; i += 3) {
		float3 v = { vertices[i], vertices[i + 1], vertices[i + 2] };
		float4 e = { v.x, v.y, v.z, 1 };
		float4 t = transform * e;
		float3 homogenized = { t.x / t.w, t.y / t.w, t.z / t.w };
		ret[i] = homogenized.x;
		ret[i + 1] = homogenized.y;
		ret[i + 2] = homogenized.z;
	}


	return ret;
}

R_Mesh* R_Mesh::MeshUnion(std::vector<R_Mesh*> meshes, std::vector<float4x4> transformations)
{
	R_Mesh* ret = new R_Mesh();

	int verticesSizeBytes = 0;
	int indicesSizeBytes = 0;
	for (auto m : meshes) {
		verticesSizeBytes += m->verticesSizeBytes;
		indicesSizeBytes += m->indicesSizeBytes;
	}

	ret->vertices = (float*)malloc(verticesSizeBytes);
	ret->indices = (unsigned int*)malloc(indicesSizeBytes);
	ret->verticesSizeBytes = verticesSizeBytes;
	ret->indicesSizeBytes = indicesSizeBytes;

	int indicesOffset = 0;
	std::vector<float> outputVertices;
	std::vector<unsigned int> outputIndices;
	for (int i = 0; i < meshes.size(); i++) {
		R_Mesh* m = meshes[i];
		float4x4 t = transformations[i];

		float* vertices = m->GetTransformedVertices(t);

		for (int i = 0; i < m->verticesSizeBytes / sizeof(float); i++) {
			outputVertices.push_back(vertices[i]);
		}

		//memcpy(ret->vertices + verticesBytesOffset, vertices, m->verticesSizeBytes);
		//verticesBytesOffset += m->verticesSizeBytes;

		for (int i = 0; i < m->indicesSizeBytes / sizeof(unsigned int); i++) {
			outputIndices.push_back(m->indices[i] + indicesOffset);
		}

		indicesOffset += m->verticesSizeBytes / sizeof(float3);

		free(vertices); //DONE
	}

	for (int i = 0; i < outputVertices.size(); i++) {
		ret->vertices[i] = outputVertices[i];
	}

	for (int i = 0; i < outputIndices.size(); i++) {
		ret->indices[i] = outputIndices[i];
	}

	return ret;
}

void R_Mesh::DrawVertexNormals() const
{
	if (idNormal == -1 || normals == nullptr)
		return;

	float normal_lenght = 0.5f;

	// Vertices normals
	glBegin(GL_LINES);
	for (size_t i = 0, c = 0; i < verticesSizeBytes / sizeof(float); i += 3, c += 4)
	{
		glColor3f(1.0f, 0.0f, 0.0f);
		//glColor4f(colors[c], colors[c + 1], colors[c + 2], colors[c + 3]);
		glVertex3f(vertices[i], vertices[i + 1], vertices[i + 2]);

		glVertex3f(vertices[i] + (normals[i] * normal_lenght),
			vertices[i + 1] + (normals[i + 1] * normal_lenght),
			vertices[i + 2] + (normals[i + 2]) * normal_lenght);
	}

	glColor3f(1.0f, 1.0f, 1.0f);
	glEnd();
}

void R_Mesh::DrawFaceNormals() const
{
	if (idNormal == -1 || normals == nullptr)
		return;

	// Face normals
	glBegin(GL_LINES);
	for (size_t i = 0; i < verticesSizeBytes / sizeof(float); i += 3)
	{
		glColor3f(0.0f, 1.0f, 0.0f);
		float vx = (vertices[i] + vertices[i + 3] + vertices[i + 6]) / 3;
		float vy = (vertices[i + 1] + vertices[i + 4] + vertices[i + 7]) / 3;
		float vz = (vertices[i + 2] + vertices[i + 5] + vertices[i + 8]) / 3;

		float nx = (normals[i] + normals[i + 3] + normals[i + 6]) / 3;
		float ny = (normals[i + 1] + normals[i + 4] + normals[i + 7]) / 3;
		float nz = (normals[i + 2] + normals[i + 5] + normals[i + 8]) / 3;

		glVertex3f(vx, vy, vz);

		glVertex3f(vx + (normals[i] * 0.5f),
			vy + (normals[i + 1] * 0.5f),
			vz + (normals[i + 2]) * 0.5f);
	}

	glColor3f(1.0f, 1.0f, 1.0f);

	glEnd();
}

void R_Mesh::PrimitiveMesh(par_shapes_mesh* primitiveMesh)
{

	//verticesSizeBytes = sizeof(float) * primitiveMesh->npoints * 3;
	//normalsSizeBytes = verticesSizeBytes;
	//texCoordSizeBytes = sizeof(float) * primitiveMesh->npoints * 2;
	//indicesSizeBytes = sizeof(uint) * primitiveMesh->ntriangles * 3;

	//vertices = (float*)malloc(verticesSizeBytes);
	//normals = (float*)malloc(normalsSizeBytes);
	//indices = (uint*)malloc(indicesSizeBytes);
	//texCoords = (float*)malloc(texCoordSizeBytes);


	//par_shapes_compute_normals(primitiveMesh);
	////memcpy(vertices, primitiveMesh->points, verticesSizeBytes);
	////for (uint i = 0; i < primitiveMesh->ntriangles; i++)
	////{

	////	indices[i * 3] = primitiveMesh->triangles[i * 3];
	////	
	////}

	////memcpy(normals, primitiveMesh->normals, normalsSizeBytes);
	////memcpy(texCoords, primitiveMesh->tcoords, texCoordSizeBytes);

	//for (size_t i = 0; i < primitiveMesh->npoints; ++i)
	//{
	//	memcpy(&vertices[i], &primitiveMesh->points[i * 3], sizeof(float) * 3);
	//	memcpy(&normals[i], &primitiveMesh->normals[i * 3], sizeof(float) * 3);
	//	if (primitiveMesh->tcoords != nullptr)
	//	{
	//		memcpy(&texCoords[i], &primitiveMesh->tcoords[i * 2], sizeof(float) * 2);
	//	}
	//	else if (meshType == Shape::CUBE)
	//	{
	//		switch (i % 4)
	//		{
	//		case 0:
	//			texCoords[i] = 0.0f;
	//			texCoords[i + 1] = 0.0f;
	//			break;
	//		case 1:
	//			texCoords[i] = 1.0f;
	//			texCoords[i + 1] = 0.0f;
	//			break;
	//		case 2:
	//			texCoords[i] = 1.0f;
	//			texCoords[i + 1] = 1.0f;
	//			break;
	//		case 3:
	//			texCoords[i] = 0.0f;
	//			texCoords[i + 1] = 1.0f;
	//			break;
	//		}
	//	}
	//	else
	//	{
	//		texCoords[i] = 0.0f;
	//		texCoords[i + 1] = 0.0f;
	//	}
	//}

	//for (size_t i = 0; i < (primitiveMesh->ntriangles * 3); ++i)
	//{
	//	indices[i] = primitiveMesh->triangles[i];
	//}

	//memcpy(&normals[0], primitiveMesh->normals, primitiveMesh->npoints);


	//par_shapes_free_mesh(primitiveMesh);
	vertices = (float*)malloc(primitiveMesh->npoints);
	normals = (float*)malloc(primitiveMesh->ntriangles);
	indices = (uint*)malloc(primitiveMesh->ntriangles * 3);
	texCoords = (float*)malloc(primitiveMesh->npoints);

	verticesSizeBytes = sizeof(float) * primitiveMesh->npoints;
	normalsSizeBytes = sizeof(float) * primitiveMesh->ntriangles;
	texCoordSizeBytes = sizeof(uint) * primitiveMesh->ntriangles * 3;
	indicesSizeBytes = sizeof(float) * primitiveMesh->npoints;

	par_shapes_compute_normals(primitiveMesh);
	for (size_t i = 0; i < primitiveMesh->npoints; ++i)
	{
		memcpy(&vertices[i], &primitiveMesh->points[i * 3], sizeof(float) * 3);
		memcpy(&normals[i], &primitiveMesh->normals[i * 3], sizeof(float) * 3);
		if (primitiveMesh->tcoords != nullptr)
		{
			memcpy(&texCoords[i], &primitiveMesh->tcoords[i * 2], sizeof(float) * 2);
		}
		else if (meshType == Shape::CUBE)
		{
			switch (i % 4)
			{
			case 0:
				texCoords[i] = 0.0f;
				texCoords[i + 1] = 0.0f;
				break;
			case 1:
				texCoords[i] = 1.0f;
				texCoords[i + 1] = 0.0f;
				break;
			case 2:
				texCoords[i] = 1.0f;
				texCoords[i + 1] = 1.0f;
				break;
			case 3:
				texCoords[i] = 0.0f;
				texCoords[i + 1] = 1.0f;
				break;
			}
		}
		else
		{
			texCoords[i] = 0.0f;
			texCoords[i + 1] = 0.0f;
		}
	}

	for (size_t i = 0; i < (primitiveMesh->ntriangles * 3); ++i)
	{
		indices[i] = primitiveMesh->triangles[i];
	}

	memcpy(&normals[0], primitiveMesh->normals, primitiveMesh->npoints);

	par_shapes_free_mesh(primitiveMesh);
	SetUpMeshBuffers();
}

//void R_Mesh::GetBoneTransforms(float timeInSeconds, std::vector<float4x4>& transforms, GameObject* gameObject)
//{
//	OPTICK_EVENT();
//
//	if (!gameObject->GetEngine()->GetRenderer()->isFirstPass)
//	{
//		transforms.resize(transformsAnim.size());
//
//		for (uint i = 0; i < transformsAnim.size(); i++)
//		{
//			transforms[i] = transformsAnim[i];
//		}
//
//		return;
//	}
//
//	rootNode = gameObject->GetParent();
//
//	float4x4 identity = float4x4::identity;
//
//	float ticksPerSecond = (float)(animation->GetTicksPerSecond() != 0 ? animation->GetTicksPerSecond() : 25.0f);
//	float timeInTicks = timeInSeconds * ticksPerSecond;
//
//	float startFrame, endFrame, animDur;
//	AnimatorClip* selectedClip = gameObject->GetParent()->GetComponent<C_Animator>()->GetSelectedClip();
//	if (selectedClip != nullptr)
//	{
//		startFrame = selectedClip->GetStartFrame();
//		endFrame = selectedClip->GetEndFrame();
//		animDur = endFrame - startFrame;
//	}
//	else
//	{
//		startFrame = 0.0f;
//		endFrame = 0.0f;
//		animDur = animation->GetDuration();
//	}
//
//	float animationTimeTicks = fmod(timeInTicks, (float)animDur); // This divides the whole animation into segments of animDur.
//
//	// Checking if the animation has finished (the animation time ticks is equal to the duration time ticks).
//	float animationSeconds = fmod(timeInSeconds, (float)selectedClip->GetDurationInSeconds());
//	if ((selectedClip->GetDurationInSeconds() - animationSeconds) <= 0.1f && timeInSeconds != 0.0f)
//	{
//		if (!selectedClip->GetLoopBool())
//			selectedClip->SetFinishedBool(true);
//	}
//
//	ReadNodeHeirarchy(animationTimeTicks + startFrame, gameObject->GetParent(), identity); // We add startFrame as an offset to the duration.
//	transforms.resize(boneInfo.size());
//	transformsAnim.resize(boneInfo.size());
//
//	for (uint i = 0; i < boneInfo.size(); i++)
//	{
//		transforms[i] = boneInfo[i].finalTransformation;
//		transformsAnim[i] = boneInfo[i].finalTransformation;
//	}
//}
//
//void R_Mesh::ReadNodeHeirarchy(float animationTimeTicks, const GameObject* pNode, const float4x4& parentTransform)
//{
//	OPTICK_EVENT();
//
//	std::string nodeName(pNode->GetName());
//
//	float4x4 nodeTransformation(pNode->GetTransform()->GetLocalTransform());
//
//	const Channel* pNodeAnim = FindNodeAnim(nodeName);
//
//	if (pNodeAnim && pNodeAnim->rotationKeyframes.size() && pNodeAnim->scaleKeyframes.size() && pNodeAnim->positionKeyframes.size())
//	{
//		// Interpolate scaling and generate scaling transformation matrix
//		float3 scaling;
//		CalcInterpolatedScaling(scaling, animationTimeTicks, pNodeAnim);
//		float4x4 scalingM = InitScaleTransform(scaling.x, scaling.y, scaling.z);
//
//		// Interpolate rotation and generate rotation transformation matrix
//		Quat rotationQ;
//		CalcInterpolatedRotation(rotationQ, animationTimeTicks, pNodeAnim);
//		float4x4 rotationM = GetMatrixFromQuat(rotationQ).Transposed();
//
//		// Interpolate translation and generate translation transformation matrix
//		float3 translation;
//		CalcInterpolatedPosition(translation, animationTimeTicks, pNodeAnim);
//		float4x4 translationM = InitTranslationTransform(translation.x, translation.y, translation.z);
//
//		// Combine the above transformations
//		nodeTransformation = translationM * rotationM * scalingM;
//	}
//
//	float4x4 globalTransformation = parentTransform * nodeTransformation;
//
//	if (boneNameToIndexMap.contains(nodeName))
//	{
//		uint boneIndex = boneNameToIndexMap[nodeName];
//		float4x4 globalInversedTransform = rootNode->GetTransform()->GetGlobalTransform().Inverted();
//		float4x4 delta = globalInversedTransform * globalTransformation * boneInfo[boneIndex].offsetMatrix;
//		boneInfo[boneIndex].finalTransformation = delta.Transposed();
//	}
//
//	for (uint i = 0; i < pNode->GetChildren().size(); i++)
//	{
//		ReadNodeHeirarchy(animationTimeTicks, pNode->GetChildren().at(i), globalTransformation);
//	}
//}
//
//const Channel* R_Mesh::FindNodeAnim(std::string nodeName)
//{
//	OPTICK_EVENT();
//	
//	return &animation->channels[nodeName];
//}
//
//uint R_Mesh::FindPosition(float AnimationTimeTicks, const Channel* pNodeAnim)
//{
//	for (uint i = 0; i < pNodeAnim->positionKeyframes.size() - 1; i++) {
//		float t = (float)pNodeAnim->positionKeyframes.at(i + 1).time;
//		if (AnimationTimeTicks < t) {
//			return i;
//		}
//	}
//
//	return 0;
//}
//
//void R_Mesh::CalcInterpolatedPosition(float3& Out, float AnimationTimeTicks, const Channel* pNodeAnim)
//{
//	OPTICK_EVENT();
//
//	// we need at least two values to interpolate...
//	if (pNodeAnim->positionKeyframes.size() == 1)
//	{
//		Out = pNodeAnim->positionKeyframes.at(0).value;
//		return;
//	}
//
//	uint PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
//	uint NextPositionIndex = PositionIndex + 1;
//	assert(NextPositionIndex < pNodeAnim->positionKeyframes.size());
//	float t1 = (float)pNodeAnim->positionKeyframes.at(PositionIndex).time;
//	float t2 = (float)pNodeAnim->positionKeyframes.at(NextPositionIndex).time;
//	float DeltaTime = t2 - t1;
//	float Factor = (AnimationTimeTicks - t1) / DeltaTime;
//	assert(Factor >= 0.0f && Factor <= 1.0f);
//	const float3& Start = pNodeAnim->positionKeyframes.at(PositionIndex).value;
//	const float3& End = pNodeAnim->positionKeyframes.at(NextPositionIndex).value;
//	float3 Delta = End - Start;
//	Out = Start + Factor * Delta;
//}
//
//uint R_Mesh::FindRotation(float AnimationTimeTicks, const Channel* pNodeAnim)
//{
//	assert(pNodeAnim->rotationKeyframes.size() > 0);
//
//	for (uint i = 0; i < pNodeAnim->rotationKeyframes.size() - 1; i++) {
//		float t = (float)pNodeAnim->rotationKeyframes.at(i + 1).time;
//		if (AnimationTimeTicks < t) {
//			return i;
//		}
//	}
//
//	return 0;
//}
//
//void R_Mesh::CalcInterpolatedRotation(Quat& Out, float AnimationTimeTicks, const Channel* pNodeAnim)
//{
//	OPTICK_EVENT();
//
//	// we need at least two values to interpolate...
//	if (pNodeAnim->rotationKeyframes.size() == 1) {
//		Out = pNodeAnim->rotationKeyframes.at(0).value;
//		return;
//	}
//
//	uint RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
//	uint NextRotationIndex = RotationIndex + 1;
//	assert(NextRotationIndex < pNodeAnim->rotationKeyframes.size());
//	float t1 = (float)pNodeAnim->rotationKeyframes.at(RotationIndex).time;
//	float t2 = (float)pNodeAnim->rotationKeyframes.at(NextRotationIndex).time;
//	float DeltaTime = t2 - t1;
//	float Factor = (AnimationTimeTicks - t1) / DeltaTime;
//	assert(Factor >= 0.0f && Factor <= 1.0f);
//	const Quat& StartRotationQ = pNodeAnim->rotationKeyframes.at(RotationIndex).value;
//	const Quat& EndRotationQ = pNodeAnim->rotationKeyframes.at(NextRotationIndex).value;
//	StartRotationQ.Slerp(EndRotationQ, Factor);
//	Out = StartRotationQ;
//	Out.Normalize();
//}
//
//
//uint R_Mesh::FindScaling(float AnimationTimeTicks, const Channel* pNodeAnim)
//{
//	assert(pNodeAnim->scaleKeyframes.size() > 0);
//
//	for (uint i = 0; i < pNodeAnim->scaleKeyframes.size() - 1; i++) {
//		float t = (float)pNodeAnim->scaleKeyframes.at(i + 1).time;
//		if (AnimationTimeTicks < t) {
//			return i;
//		}
//	}
//
//	return 0;
//}
//
//void R_Mesh::CalcInterpolatedScaling(float3& Out, float AnimationTimeTicks, const Channel* pNodeAnim)
//{
//	OPTICK_EVENT();
//
//	// we need at least two values to interpolate...
//	if (pNodeAnim->scaleKeyframes.size() == 1) {
//		Out = pNodeAnim->scaleKeyframes.at(0).value;
//		return;
//	}
//
//	uint ScalingIndex = FindScaling(AnimationTimeTicks, pNodeAnim);
//	uint NextScalingIndex = ScalingIndex + 1;
//	assert(NextScalingIndex < pNodeAnim->scaleKeyframes.size());
//	float t1 = (float)pNodeAnim->scaleKeyframes.at(ScalingIndex).time;
//	float t2 = (float)pNodeAnim->scaleKeyframes.at(NextScalingIndex).time;
//	float DeltaTime = t2 - t1;
//	float Factor = (AnimationTimeTicks - (float)t1) / DeltaTime;
//	assert(Factor >= 0.0f && Factor <= 1.0f);
//	const float3& Start = pNodeAnim->scaleKeyframes.at(ScalingIndex).value;
//	const float3& End = pNodeAnim->scaleKeyframes.at(NextScalingIndex).value;
//	float3 Delta = End - Start;
//	Out = Start + Factor * Delta;
//}
//
//float4x4 R_Mesh::InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ)
//{
//	float4x4 m;
//
//	m[0][0] = ScaleX; m[0][1] = 0.0f;   m[0][2] = 0.0f;   m[0][3] = 0.0f;
//	m[1][0] = 0.0f;   m[1][1] = ScaleY; m[1][2] = 0.0f;   m[1][3] = 0.0f;
//	m[2][0] = 0.0f;   m[2][1] = 0.0f;   m[2][2] = ScaleZ; m[2][3] = 0.0f;
//	m[3][0] = 0.0f;   m[3][1] = 0.0f;   m[3][2] = 0.0f;   m[3][3] = 1.0f;
//
//	return m;
//}
//
//float4x4 R_Mesh::InitRotateTransform(const aiQuaternion& quat)
//{
//	float4x4 m;
//
//	float yy2 = 2.0f * quat.y * quat.y;
//	float xy2 = 2.0f * quat.x * quat.y;
//	float xz2 = 2.0f * quat.x * quat.z;
//	float yz2 = 2.0f * quat.y * quat.z;
//	float zz2 = 2.0f * quat.z * quat.z;
//	float wz2 = 2.0f * quat.w * quat.z;
//	float wy2 = 2.0f * quat.w * quat.y;
//	float wx2 = 2.0f * quat.w * quat.x;
//	float xx2 = 2.0f * quat.x * quat.x;
//	m[0][0] = -yy2 - zz2 + 1.0f;
//	m[0][1] = xy2 + wz2;
//	m[0][2] = xz2 - wy2;
//	m[0][3] = 0;
//	m[1][0] = xy2 - wz2;
//	m[1][1] = -xx2 - zz2 + 1.0f;
//	m[1][2] = yz2 + wx2;
//	m[1][3] = 0;
//	m[2][0] = xz2 + wy2;
//	m[2][1] = yz2 - wx2;
//	m[2][2] = -xx2 - yy2 + 1.0f;
//	m[2][3] = 0.0f;
//	m[3][0] = m[3][1] = m[3][2] = 0;
//	m[3][3] = 1.0f;
//
//	return m;
//}
//
//float4x4 R_Mesh::InitTranslationTransform(float x, float y, float z)
//{
//	float4x4 m;
//
//	m[0][0] = 1.0f; m[0][1] = 0.0f; m[0][2] = 0.0f; m[0][3] = x;
//	m[1][0] = 0.0f; m[1][1] = 1.0f; m[1][2] = 0.0f; m[1][3] = y;
//	m[2][0] = 0.0f; m[2][1] = 0.0f; m[2][2] = 1.0f; m[2][3] = z;
//	m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;
//
//	return m;
//}
//
//float4x4 R_Mesh::aiMatrix3x32Float4x4(aiMatrix3x3 assimpMatrix)
//{
//	float4x4 m;
//
//	m[0][0] = assimpMatrix.a1; m[0][1] = assimpMatrix.a2; m[0][2] = assimpMatrix.a3; m[0][3] = 0.0f;
//	m[1][0] = assimpMatrix.b1; m[1][1] = assimpMatrix.b2; m[1][2] = assimpMatrix.b3; m[1][3] = 0.0f;
//	m[2][0] = assimpMatrix.c1; m[2][1] = assimpMatrix.c2; m[2][2] = assimpMatrix.c3; m[2][3] = 0.0f;
//	m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;
//
//	return m;
//}
//
//float4x4 R_Mesh::GetMatrixFromQuat(Quat quat)
//{
//	float4x4 m;
//
//	float yy2 = 2.0f * quat.y * quat.y;
//	float xy2 = 2.0f * quat.x * quat.y;
//	float xz2 = 2.0f * quat.x * quat.z;
//	float yz2 = 2.0f * quat.y * quat.z;
//	float zz2 = 2.0f * quat.z * quat.z;
//	float wz2 = 2.0f * quat.w * quat.z;
//	float wy2 = 2.0f * quat.w * quat.y;
//	float wx2 = 2.0f * quat.w * quat.x;
//	float xx2 = 2.0f * quat.x * quat.x;
//	m[0][0] = -yy2 - zz2 + 1.0f;
//	m[0][1] = xy2 + wz2;
//	m[0][2] = xz2 - wy2;
//	m[0][3] = 0;
//	m[1][0] = xy2 - wz2;
//	m[1][1] = -xx2 - zz2 + 1.0f;
//	m[1][2] = yz2 + wx2;
//	m[1][3] = 0;
//	m[2][0] = xz2 + wy2;
//	m[2][1] = yz2 - wx2;
//	m[2][2] = -xx2 - yy2 + 1.0f;
//	m[2][3] = 0.0f;
//	m[3][0] = m[3][1] = m[3][2] = 0;
//	m[3][3] = 1.0f;
//
//	return m;
//}