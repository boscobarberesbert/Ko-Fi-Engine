#include "Emitter.h"
#include "ParticleModule.h"

Emitter::Emitter(const char* newName)
{
	name = newName;
	maxParticles = MAX_PARTICLES;
	texture = new R_Texture();
	texture->SetAssetPath("Assets/Textures/Particles/light.png");
	Importer::GetInstance()->textureImporter->Import(texture->GetAssetPath(), texture, true);
	modules.push_back(CreateModule<EmitterDefault>());
	//modules.push_back(CreateModule<EmitterMovement>());
	//modules.push_back(CreateModule<EmitterSize>());
	modules.push_back(CreateModule<ParticleBillboarding>());
	checkerTexture = false;
}

Emitter::~Emitter()
{
	Cleanup();
}

bool Emitter::Update(float dt)
{
	return true;
}

bool Emitter::Cleanup()
{
	for (auto it : modules)
	{
		RELEASE(it);
	}
	modules.clear();
	modules.shrink_to_fit();

	name.clear();
	name.shrink_to_fit();

	return true;
}

void Emitter::AddModule(ParticleModule* m)
{
	modules.push_back(m);
}

void Emitter::DeleteModule(ParticleModule* m)
{
	for (std::vector<ParticleModule*>::iterator it = modules.begin(); it < modules.end(); ++it)
	{
		if ((*it) == m)
		{
			RELEASE(*it);
			modules.erase(it);
			modules.shrink_to_fit();
		}
	}
}

void Emitter::AddModuleByType(ParticleModuleType type)
{
	// Check if it is repeated
	for (ParticleModule* module : modules)
	{
		if (module->type == type)
		{
			LOG_BOTH("Modules cannot be duplicated!");
			return;
		}
	}

	switch (type)
	{
	case ParticleModuleType::DEFAULT:
	{
		modules.push_back(CreateModule<EmitterDefault>());
		break;
	}
	case ParticleModuleType::MOVEMENT:
	{
		modules.push_back(CreateModule<EmitterMovement>());
		break;
	}
	case ParticleModuleType::COLOR:
	{
		modules.push_back(CreateModule<EmitterColor>());
		break;
	}
	case ParticleModuleType::SIZE:
	{
		modules.push_back(CreateModule<EmitterSize>());
		break;
	}
	case ParticleModuleType::BILLBOARDING:
	{
		modules.push_back(CreateModule<ParticleBillboarding>());
		break;
	}
	}

	//emitterinstances refresh 
}
