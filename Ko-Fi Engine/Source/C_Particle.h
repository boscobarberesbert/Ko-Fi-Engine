#ifndef __C_PARTICLE_H__
#define __C_PARTICLE_H__

#include "Component.h"
#include "EmitterInstance.h"
#include "SceneIntro.h"
#include "Color.h"

class EmitterInstance;
class R_Particle;
class Emitter;
class GameObject;

enum class ParticleModuleType;
//enum class ParticleBillboarding::BillboardingType;

class C_Particle : public Component
{
public:
	C_Particle(GameObject* parent);
	virtual ~C_Particle();

	bool Start() override;
	bool Update(float dt) override;
	bool PostUpdate(float dt) override;
	bool CleanUp() override;

	void Save(Json& json) const override;
	void Load(Json& json) override;

	bool InspectorDraw(PanelChooser* chooser) override;

	void ClearParticles();
	void StopParticleSpawn();
	void ResumeParticleSpawn();
	void ResetTimers();
	void SetLoop(bool v);
	void SetColor(double r, double g, double b, double a);
	void SetAngle(double angle);
	void DeleteModule(Emitter* e,ParticleModuleType t);

private:
	void NewEmitterName(std::string& name, int n = 1);
	const char* ModuleTypeToString(ParticleModuleType e);

public:
	std::vector<std::string> resourcesList;
	std::vector<EmitterInstance*> emitterInstances;
	R_Particle* resource = nullptr;
	int moduleToAdd = 0;
	int resourceToAdd = 0;
	int currentTextureId = 0;
	int billboardingType = 0;
};

#endif // __C_PARTICLE_H__