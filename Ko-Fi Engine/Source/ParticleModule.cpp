#include "ParticleModule.h"
#include <vector>
#include "MathGeoLib/Algorithm/Random/LCG.h"
#include "MathGeoLib/Math/MathFunc.h"
#include "Camera3D.h"

ParticleModule::ParticleModule()
{
	//if (emitter != nullptr)
	//{
	//	emitter->AddModule(this);
	//}
}

ParticleModule::~ParticleModule()
{}

void ParticleModule::Spawn(Particle* particle, EmitterInstance* emitter)
{}

bool ParticleModule::Update(float dt, EmitterInstance* emitter)
{
	return true;
}

float ParticleModule::GetPercentage(Particle* p)
{
	return (p->lifeTime / p->maxLifetime);
}

// MODULES---------------------------------------------------------------

EmitterDefault::EmitterDefault()
{
	type = ParticleModuleType::DEFAULT;
}

void EmitterDefault::Spawn(Particle* particle, EmitterInstance* emitter)
{
	GameObject* go = emitter->component->owner;

	particle->position = go->GetTransform()->GetGlobalTransform().TranslatePart();

	particle->maxLifetime = initialLifetime;
	particle->lifeTime = 0.0f;
}

bool EmitterDefault::Update(float dt, EmitterInstance* emitter)
{
	if (disable)
	{
		return true;
	}
	timer += dt;
	if (timer >= spawnTime)
	{
		emitter->SpawnParticle();
		timer = 0;
	}
	for (unsigned int i = 0; i < emitter->activeParticles; i++)
	{
		unsigned int particleIndex = emitter->particleIndices[i];
		Particle* particle = &emitter->particles[particleIndex];

		emitter->particles[i].lifeTime += dt;
		particle->distanceToCamera = float3(emitter->component->owner->GetEngine()->GetCamera3D()->
			cameraFrustum.WorldMatrix().TranslatePart() - particle->position).LengthSq();
	}
}

EmitterMovement::EmitterMovement()
{
	type = ParticleModuleType::MOVEMENT;
}

void EmitterMovement::Spawn(Particle* particle, EmitterInstance* emitter)
{
	LCG random;
	particle->direction = particle->direction.Lerp(initialDirection, finalDirection, random.Float());

	float directionX = math::Lerp(initialDirection.x, finalDirection.x, random.Float());
	float directionY = math::Lerp(initialDirection.y, finalDirection.y, random.Float());
	float directionZ = math::Lerp(initialDirection.z, finalDirection.z, random.Float());
	particle->direction = float3(directionX, directionY, directionZ);

	particle->intensity = math::Lerp(initialIntensity,finalIntensity,random.Float());
	particle->velocity = particle->direction * particle->intensity;

	float accelerationX = math::Lerp(initialAcceleration.x, finalAcceleration.x, random.Float());
	float accelerationY = math::Lerp(initialAcceleration.y, finalAcceleration.y, random.Float());
	float accelerationZ = math::Lerp(initialAcceleration.z, finalAcceleration.z, random.Float());
	particle->acceleration = float3(accelerationX, accelerationY, accelerationZ);
}

bool EmitterMovement::Update(float dt, EmitterInstance* emitter)
{
	if (disable)
	{
		return true;
	}
	for (unsigned int i = 0; i < emitter->activeParticles; i++)
	{
		unsigned int particleIndex = emitter->particleIndices[i];
		Particle* particle = &emitter->particles[particleIndex];

		particle->velocity += particle->acceleration;
		particle->position += particle->velocity * dt;
	}
}

EmitterColor::EmitterColor()
{
	type = ParticleModuleType::COLOR;
	colorOverTime.push_back(FadeColor());
}

void EmitterColor::Spawn(Particle* particle, EmitterInstance* emitter)
{
	particle->CurrentColor = ColorLerp(GetPercentage(particle));
}

bool EmitterColor::Update(float dt, EmitterInstance* emitter)
{
	if (disable)
	{
		return true;
	}
	for (unsigned int i = 0; i < emitter->activeParticles; i++)
	{
		unsigned int particleIndex = emitter->particleIndices[i];
		Particle* particle = &emitter->particles[particleIndex];

		particle->CurrentColor = ColorLerp(GetPercentage(&emitter->particles[i]));
	}
}

Color EmitterColor::ColorLerp(float current)
{
	for (int i = 0; i < colorOverTime.size(); ++i)
	{
		if (colorOverTime.at(i).pos == current)
		{
			return colorOverTime.at(i).color;
		}
		else if (i == 0 && colorOverTime.at(i).pos > current)
		{
			return colorOverTime.at(i).color;
		}
		else if (i == colorOverTime.size() && colorOverTime.at(i).pos < current)
		{
			return colorOverTime.at(i).color;
		}
		else if (colorOverTime.at(i).pos > current && colorOverTime.at(i-1).pos < current)
		{
			float r = math::Lerp(colorOverTime.at(i - 1).color.r, colorOverTime.at(i).color.r, current);
			float g = math::Lerp(colorOverTime.at(i - 1).color.r, colorOverTime.at(i).color.r, current);
			float b = math::Lerp(colorOverTime.at(i - 1).color.r, colorOverTime.at(i).color.r, current);
			float a = math::Lerp(colorOverTime.at(i - 1).color.r, colorOverTime.at(i).color.r, current);
			return Color(r, g, b, a);
		}
	}
	return Color(1.0f,1.0f,1.0f,1.0f);
}

//bool EmitterColor::EditColor(FadeColor& color, uint pos)
//{
//	bool ret = true;
//	ImVec4 vecColor = ImVec4(color.color.r, color.color.g, color.color.b, color.color.a);
//	//if (ImGui::ColorButton("Color", vecColor, ImGuiColorEditFlags_None, ImVec2(100, 20)));
//
//
//	//ImGui::SameLine();
//	//ImGui::TextUnformatted("Color");
//	if (pos > 0)
//	{
//		std::string colorStr = "Remove Color ";
//		colorStr.append(std::to_string(pos));
//		if (ImGui::Button(colorStr.c_str(), ImVec2(125, 25))) ret = false;
//	}
//
//	ImGui::ColorEdit4("Color", &color.color.a, ImGuiColorEditFlags_AlphaBar);
//
//	return ret;
//}

EmitterSize::EmitterSize()
{
	type = ParticleModuleType::SIZE;
}

void EmitterSize::Spawn(Particle* particle, EmitterInstance* emitter)
{
	LCG random;
	particle->scale = particle->scale.Lerp(initialSize, finalSize, random.Float());
}

bool EmitterSize::Update(float dt, EmitterInstance* emitter)
{
	if (disable)
	{
		return true;
	}
	for (unsigned int i = 0; i < emitter->activeParticles; i++)
	{
		unsigned int particleIndex = emitter->particleIndices[i];
		Particle* particle = &emitter->particles[particleIndex];

		float p = GetPercentage(particle);
		particle->scale = particle->scale.Lerp(initialSize, finalSize, p);
	}
}