enum eSoundId {
	SOUND_MOVE,
	SOUND_ROTATE,
	SOUND_LOCK,
	SOUND_LINE,
	SOUND_TETRIS,
	SOUND_MENUMOVE,
	SOUND_START,
	SOUND_PAUSE,
	SOUND_GAMEOVER,
};
void PlayGameSound(int soundId);
void ProcessSoundCache();