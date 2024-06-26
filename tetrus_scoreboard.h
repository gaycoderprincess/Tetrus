#ifndef TETRUS_SCOREBOARD_H
#define TETRUS_SCOREBOARD_H

namespace Scoreboard {
	struct tScore {
		struct tSaveable {
			int score;
			int lines;
			int randomSeed;
			int gameMode;
			char playerName[24];
			size_t additionalDataSize = 0;
		} saveData;
		uint8_t* additionalData = nullptr;
		bool highlighted = false;

		tScore();
		bool HasReplay() const;
	};
	extern std::vector<tScore> aScores;
	extern std::string fileName;
	extern int randomizerType;
	extern int startingLevelId;
	extern bool isEndGameScoreboard;

	bool ScoreCompFunction(tScore a, tScore b);
	void ClearHighlight();
	void Load();
	void Save();
	void AddScore(int score, int lines, int randomSeed, const char* playerName, CReplay* replay);
	void Draw(bool showEmpty);
	void SetFile(const char* path);
	void SetFile(int startingLevel, int randomizer, bool nesLevels);
};

void ShowEndgameScoreboard();
#endif