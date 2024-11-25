#include "GameAudio.hpp"
#include "AL/al.h"
#include <engine/Audio/AudioErrorHandling.hpp>
//#include <platformer/Assets.hpp>
#include <Put.hpp>

GameAudio::GameAudio()
	: attractingOrbSource(SoundSource{ .source = AudioSource::generate() }) {

	static constexpr auto SOURCE_COUNT = 16;
	for (i32 i = 0; i < SOURCE_COUNT; i++) {
		auto source = AudioSource::tryGenerate();
		if (!source.has_value()) {
			break;
		}
		soundEffectSourcePool.emplace_back(SoundSource{ .source = std::move(*source) });
	}
	for (auto& source : soundEffectSourcePool) {
		soundEffectSources.push_back(&source);
	}

	updateSoundEffectVolumes();
}

//void GameAudio::update(const SettingsAudio& settings) {
//	if (masterVolume != settings.masterVolume) {
//		setMasterVolume(settings.masterVolume);
//	}
//	if (soundEffectVolume != settings.soundEffectVolume) {
//		setSoundEffectVolume(settings.soundEffectVolume);
//	}
//	if (musicVolume != settings.musicVolume) {
//		setMusicVolume(settings.musicVolume);
//	}
//	musicStream.update();
//}

void GameAudio::playSoundEffect(const AudioBuffer& buffer, f32 pitch) {
	play(soundEffectSourcePool, buffer, soundEffectVolume, pitch);
}

void GameAudio::play(std::vector<SoundSource>& sources, const AudioBuffer& buffer, f32 volume, f32 pitch) {
	for (auto& soundSource : sources) {
		auto& source = soundSource.source;

		ALenum state;
		AL_TRY(alGetSourcei(source.handle(), AL_SOURCE_STATE, &state));
		if (state == AL_PLAYING || state == AL_PAUSED) {
			continue;
		}
		{
			source.setBuffer(buffer);
			source.setPitchMultipier(pitch);
			source.setGain(masterVolume * volume);
			source.play();
		}
		return;
	}
	put("run out of sources");

}

void GameAudio::updateSoundEffectVolumes() {
	for (auto& source : soundEffectSources) {
		source->source.setGain(masterVolume * soundEffectVolume * source->volume);
	}
}

void GameAudio::setSoundEffectVolume(f32 value) {
	soundEffectVolume = value;
	updateSoundEffectVolumes();
}

void GameAudio::stopSoundEffects() {
	for (const auto& source : soundEffectSources) {
		source->source.stop();
	}
}

void GameAudio::pauseSoundEffects() {
	for (const auto& source : soundEffectSources) {
		source->source.pause();
	}
}

void GameAudio::unpauseSoundEffects() {
	for (const auto& source : soundEffectSources) {
		if (!source->source.isPaused()) {
			continue;
		}
		source->source.play();
	}
}

void GameAudio::setSoundEffectSourceVolume(SoundSource& source, f32 value) {
	source.volume = value;
	source.source.setGain(masterVolume * soundEffectVolume * value);
}
