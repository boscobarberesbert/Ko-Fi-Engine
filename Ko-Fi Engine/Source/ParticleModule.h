#ifndef __PARTICLE_MODULE_H__
#define __PARTICLE_MODULE_H__

#include "EmitterInstance.h"
#include "Particle.h"

class Particle;
class EmitterInstance;

	enum class ParticleModuleType
	{
		NONE = 0,
		DEFAULT,
		MOVEMENT,
		COLOR,
		SIZE
	};

class ParticleModule
{
public:

	ParticleModule();
	~ParticleModule();
	virtual void Spawn(Particle* particle, EmitterInstance* emitter);
	virtual bool Update(float dt, EmitterInstance* emitter);
	float GetPercentage(Particle* p);

public:
	ParticleModuleType type = ParticleModuleType::NONE;
	//Emitter* emitter = nullptr;
	bool disable = false;
};

// MODULES----------------------------------------------------------

class EmitterDefault : public ParticleModule
{
public:
	EmitterDefault();

	void Spawn(Particle* particle, EmitterInstance* emitter);
	bool Update(float dt, EmitterInstance* emitter);

public:
	float timer = 0.0f;
	float spawnTime = 0.5f;
	float initialLifetime = 1.0f;
};

class EmitterMovement : public ParticleModule
{
public:
	EmitterMovement();

	void Spawn(Particle* particle, EmitterInstance* emitter);
	bool Update(float dt, EmitterInstance* emitter);

public:
	float initialIntensity = 1.0f;
	float finalIntensity = 3.0f;

	float3 initialDirection = float3(1.0f, 1.0f, 1.0f);
	float3 finalDirection = float3(-1.0f, -1.0f, -1.0f);

	float3 initialPosition = float3::zero;
	float3 finalPosition = float3::zero;

	float3 initialAcceleration = float3(0.0f, 0.0f, 0.0f);
	float3 finalAcceleration = float3(1.0f, 1.0f, 1.0f);
};

class EmitterColor : public ParticleModule
{
public:
	EmitterColor();

	void Spawn(Particle* particle, EmitterInstance* emitter);
	bool Update(float dt, EmitterInstance* emitter);
	Color ColorLerp(float current);
	//bool EditColor(FadeColor& color, uint pos = 0);

public:
	std::vector<FadeColor> colorOverTime;
};

class EmitterSize : public ParticleModule
{
public:
	EmitterSize();

	void Spawn(Particle* particle, EmitterInstance* emitter);
	bool Update(float dt, EmitterInstance* emitter);

public:
	float3 initialSize = float3(1.0f, 1.0f, 1.0f);
	float3 finalSize = float3(1.5f, 1.5f, 1.5f);
};

#endif // !__PARTICLE_MODULE_H__