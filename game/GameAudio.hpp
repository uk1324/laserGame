#pragma once

#include <engine/Audio/AudioBuffer.hpp>
#include <engine/Audio/AudioSource.hpp>
#include <engine/Audio/AudioFileStream.hpp>
#include <vector>

struct GameAudio {
	GameAudio();

	//void update(const SettingsAudio& settings);

	struct SoundSource {
		AudioSource source;
		f32 volume = 1.0f;
	};

	void update();

	void playSoundEffect(const AudioBuffer& buffer, f32 pitch = 1.0f);
	void play(std::vector<SoundSource>& sources, const AudioBuffer& buffer, f32 volume, f32 pitch = 1.0f);

	void updateSoundEffectVolumes();

	void setSoundEffectVolume(f32 value);

	void stopSoundEffects();
	void pauseSoundEffects();
	void unpauseSoundEffects();

	void setSoundEffectSourceVolume(SoundSource& source, f32 value);

	AudioFileStream musicStream;
	void setMusicVolume(f32 value);

	f32 soundEffectVolume = 1.0f;
	f32 masterVolume = 1.0f;

	//AudioBuffer levelCompleteSound;
	AudioBuffer transitionSwooshSound;
	AudioBuffer targetOnSound;
	AudioBuffer doorOpenSound;
	AudioBuffer errorSound;
	AudioBuffer uiClickSound;

	SoundSource doorOpeningSource;

	std::vector<SoundSource> soundEffectSourcePool;

	std::vector<SoundSource*> soundEffectSources;
};