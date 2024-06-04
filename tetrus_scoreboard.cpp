#include <fstream>

#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_board.h"
#include "tetrus_replay.h"
#include "tetrus_scoreboard.h"

namespace Scoreboard {
	std::vector<tScore> aScores;
	std::string fileName;
	int randomizerType;
	int startingLevelId;
	int selectedReplay;
	bool nesInitialLevelClear;
	bool isEndGameScoreboard;

	tScore::tScore() {
		memset(this, 0, sizeof(*this));
	}

	bool tScore::HasReplay() const {
		if (!additionalData) return false;
		if (saveData.additionalDataSize <= 0) return false;
		if (saveData.additionalDataSize % sizeof(CReplay::tEvent) != 0) return false;
		auto events = (CReplay::tEvent*)additionalData;
		int count = saveData.additionalDataSize / sizeof(CReplay::tEvent);
		for (int i = 0; i < count; i++) {
			if (!CReplay::IsEventHandled(events[i].type)) return false;
		}
		return true;
	}

	bool ScoreCompFunction(tScore a, tScore b) {
		if (a.saveData.score == b.saveData.score) {
			return a.saveData.lines > b.saveData.lines;
		}
		return a.saveData.score > b.saveData.score;
	}

	void Load() {
		std::ifstream file;
		file.open(sProgramDir + "/" + fileName + ".dat", std::ios::binary);
		if (!file.is_open()) return;

		char header[4];
		file.read(header, sizeof(header));
		if (memcmp(header, "nya~", sizeof(header)) != 0) return;

		int count = 0;
		file.read((char *) &count, sizeof(count));

		aScores.clear();

		for (int i = 0; i < count; i++) {
			tScore score;
			file.read((char *) &score.saveData, sizeof(score.saveData));
			if (score.saveData.additionalDataSize > 0) {
				score.additionalData = new uint8_t[score.saveData.additionalDataSize];
				file.read((char *) score.additionalData, score.saveData.additionalDataSize);
			}
			if (score.saveData.playerName[0] && !score.saveData.playerName[23]) {
				aScores.push_back(score);
			}
		}
	}

	void Save() {
		std::ofstream file;
		file.open(sProgramDir + "/" + fileName + ".dat", std::ios::binary);
		if (!file.is_open()) return;

		file.write("nya~", 4);

		int count = aScores.size();
		file.write((char *) &count, sizeof(count));

		for (auto &score: aScores) {
			file.write((char *) &score.saveData, sizeof(score.saveData));
			if (score.HasReplay()) {
				file.write((char *) score.additionalData, score.saveData.additionalDataSize);
			}
		}
	}

	void ClearHighlight() {
		for (auto& score : aScores) {
			score.highlighted = false;
		}
	}

	void AddScore(int score, int lines, int randomSeed, const char *playerName, CReplay* replay) {
		tScore data;
		data.saveData.score = score;
		data.saveData.lines = lines;
		data.saveData.randomSeed = randomSeed;
		data.saveData.gameMode = gGameConfig.gameType;
		strcpy_s(data.saveData.playerName, 24, playerName);
		data.saveData.additionalDataSize = replay->events.size() * sizeof(replay->events[0]);
		data.additionalData = new uint8_t[data.saveData.additionalDataSize];
		memcpy(data.additionalData, &replay->events[0], data.saveData.additionalDataSize);
		data.highlighted = true;
		aScores.push_back(data);

		sort(aScores.begin(), aScores.end(), ScoreCompFunction);

		while (aScores.size() > 10) aScores.pop_back();
	}

	void Draw(bool showEmpty) {
		DrawBackgroundImage(0, 1);

		if (!isEndGameScoreboard) {
			if (GetMenuUp()) selectedReplay--;
			if (GetMenuDown()) selectedReplay++;
			if (selectedReplay < 0) selectedReplay = aScores.size() - 1;
			if (selectedReplay >= aScores.size()) selectedReplay = 0;
		}

		float yPos = 0.2;
		float ySpacing = 0.05;
		float size = 0.05;
		float xLeft = 0.5 - (0.3 * GetAspectRatioInv());
		float xRight = 0.5 + (0.3 * GetAspectRatioInv());

		int i = 0;
		if (aScores.empty() && showEmpty) {
			tNyaStringData string;
			string.size = size;
			string.x = 0.5;
			string.y = 0.5;
			string.XCenterAlign = true;
			auto emptyStr = "The scoreboard is empty.";
			{
				float bgWidth = GetStringWidth(string.size, emptyStr) * 0.5 + 0.01;
				float bgHeight = GetStringHeight(string.size, emptyStr) * 0.5 + 0.005;
				DrawRectangle(string.x - bgWidth, string.x + bgWidth, string.y - bgHeight, string.y + bgHeight,
							  {0, 0, 0, 127},
							  0.01);
			}
			DrawString(string, emptyStr);
		}
		else {
			float bgMin = yPos - GetStringHeight(size, "null") * 0.5 - 0.0075;
			float bgMax = yPos + ySpacing * (aScores.size() - 1) + GetStringHeight(size, "null") * 0.5 + 0.0075;
			DrawRectangle(xLeft - 0.01 * GetAspectRatioInv(), xRight + 0.01 * GetAspectRatioInv(), bgMin, bgMax, {0, 0, 0, 127}, 0.01);

			for (auto &score: aScores) {
				if (!isEndGameScoreboard && i == selectedReplay && GetMenuSelect() && score.HasReplay()) {
					gGameConfig.gameType = tGameConfig::TYPE_1PLAYER;
					gGameConfig.randomizerType = randomizerType;
					gGameConfig.startingLevel = startingLevelId;
					gGameConfig.nesInitialLevelClear = nesInitialLevelClear;
					ResetAllPlayers();
					aPlayers[0]->SetPieceRandomizer(score.saveData.randomSeed);
					aPlayers[0]->ReInit(false);
					StartReplay(score.additionalData, score.saveData.additionalDataSize, aPlayers[0]);
					return;
				}

				int col = score.highlighted || (!isEndGameScoreboard && i == selectedReplay) ? 255 : 127;

				tNyaStringData string;
				string.size = size;
				string.x = xLeft;
				string.y = yPos + ySpacing * i++;
				string.SetColor(col, col, col, 255);
				DrawString(string, score.saveData.playerName);
				string.x = xRight;
				string.XRightAlign = true;
				DrawString(string, (std::to_string(score.saveData.score) + " (" + std::to_string(score.saveData.lines) +
									")").c_str());
			}
		}
	}

	void SetFile(const char* path) {
		if (fileName != path) {
			if (!fileName.empty() && !aScores.empty()) Save();
			for (auto& score : aScores) {
				if (score.additionalData) delete[] score.additionalData;
			}
			aScores.clear();
			selectedReplay = 0;
			fileName = path;
			Load();
		}
	}

	void SetFile(int startingLevel, int randomizer, bool nesLevels) {
		startingLevelId = startingLevel;
		randomizerType = randomizer;
		nesInitialLevelClear = nesLevels;
		isEndGameScoreboard = false;

		std::string path = "score_" + std::to_string(tGameConfig::startingLevelIds[startingLevel]);
		switch (randomizer) {
			case tGameConfig::RANDOMIZER_NES:
				path += "_nes";
				break;
			case tGameConfig::RANDOMIZER_TRUE_RANDOM:
				path += "_truerandom";
				break;
			case tGameConfig::RANDOMIZER_MODERN:
				path += "_modern";
				break;
			default:
				break;
		}
		if (nesInitialLevelClear && startingLevel > 0) path += "_neslvl";
		SetFile(path.c_str());
	}
}

void ShowEndgameScoreboard() {
	gGameState = STATE_SCOREBOARD_VIEW;
	Scoreboard::ClearHighlight();
	Scoreboard::SetFile(gGameConfig.startingLevel, gGameConfig.randomizerType, gGameConfig.nesInitialLevelClear);
	for (auto player: aPlayers) {
		if (player->board->linesCleared == 0) continue;
		Scoreboard::AddScore(player->board->score, player->board->linesCleared, player->GetPieceRandomizerSeed(), gGameConfig.playerName[player->playerId], player->recordingReplay);
	}
	Scoreboard::Save();
	Scoreboard::isEndGameScoreboard = true;
}