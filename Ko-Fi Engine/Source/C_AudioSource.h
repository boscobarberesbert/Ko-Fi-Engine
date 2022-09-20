#ifndef __C_AUDIO_SOURCE_H__
#define __C_AUDIO_SOURCE_H__

#include "C_Audio.h"
#include "R_Track.h"

using Json = nlohmann::json;

class GameObject;

class C_AudioSource : public C_Audio
{
public:
	C_AudioSource(GameObject* parent);
	virtual ~C_AudioSource();

	bool Start() override;
	bool Update(float dt) override;
	
	bool OnSceneSwitch() override;
	bool OnPlay() override;
	bool OnPause() override;
	bool OnStop() override;
	bool OnResume() override;

	bool InspectorDraw(PanelChooser* chooser) override;

	void Save(Json& json) const override;
	void Load(Json& json) override;

	void UpdatePlayState();

	void DrawEditor();

	void PlayTrack();
	void ResumeTrack();
	void StopTrack();
	void PauseTrack();

private:
    R_Track* track = nullptr;
};

#endif // !__C_AUDIO_SOURCE_H__