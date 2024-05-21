#include "nya_dx11_appbase.h"

#include "tetrus_config.h"
#include "tetrus_audio.h"

struct tSoundCacheEntry {
	NyaAudio::NyaSound sound;
	int soundId;
};
std::vector<tSoundCacheEntry> aSoundCache;

bool IsSoundImmuneFromOverlapCheck(int soundId) {
	if (soundId == SOUND_LINE) return true;
	if (soundId == SOUND_LOCK) return true;
	if (soundId == SOUND_TETRIS) return true;
	if (soundId == SOUND_PAUSE) return true;
	return false;
}

bool EraseMatchingFromSoundCache(int soundId) {
	int i = 0;
	for (auto& data : aSoundCache) {
		if ((!gGameConfig.overlappingSounds && !IsSoundImmuneFromOverlapCheck(data.soundId)) || data.soundId == soundId) {
			if (!NyaAudio::IsFinishedPlaying(data.sound)) NyaAudio::Stop(data.sound);
			auto tmp = data;
			NyaAudio::Delete(&tmp.sound);
			aSoundCache.erase(aSoundCache.begin() + i);
			return true;
		}
		i++;
	}
	return false;
}

const char* soundFilenames[] = {
		"move.mp3",
		"rotate.mp3",
		"lock.mp3",
		"line.mp3",
		"tetris.mp3",
		"menumove.mp3",
		"start.mp3",
		"pause.mp3",
		"gameover.mp3",
};

void PlayGameSound(int soundId) {
	if (!gGameConfig.sfxOn) return;

	auto file = NyaAudio::LoadFile(soundFilenames[soundId]);
	if (!file) return;
	NyaAudio::Play(file);

	while (EraseMatchingFromSoundCache(soundId)) {}
	aSoundCache.push_back({file, soundId});
}

void ProcessSoundCache() {
	int i = 0;
	for (auto& data : aSoundCache) {
		if (NyaAudio::IsFinishedPlaying(data.sound)) {
			auto tmp = data.sound;
			NyaAudio::Delete(&tmp);
			aSoundCache.erase(aSoundCache.begin() + i);
			return ProcessSoundCache();
		}
		i++;
	}
}