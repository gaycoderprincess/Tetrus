#include <fstream>

#include "nya_dx11_appbase.h"

#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_board.h"
#include "tetrus_piece.h"
#include "tetrus_saveload.h"
#include "tetrus_player.h"
#include "tetrus_replay.h"
#include "tetrus_scoreboard.h"

bool LoadControlConfig() {
	std::ifstream file;
	file.open(sProgramDir + "/controls.dat", std::ios::binary);
	if (!file.is_open()) return false;

	char header[4];
	file.read(header, sizeof(header));
	if (memcmp(header, "nya~", sizeof(header)) != 0) return false;

	file.read((char*)aPlayerControls, sizeof(aPlayerControls));

	return true;
}

bool LoadGameConfig() {
	std::ifstream file;
	file.open(sProgramDir + "/config.dat", std::ios::binary);
	if (!file.is_open()) return false;

	char header[4];
	file.read(header, sizeof(header));
	if (memcmp(header, "nya~", sizeof(header)) != 0) return false;

	file.read((char*)&gGameConfig, sizeof(gGameConfig));

	return true;
}

bool SaveControlConfig() {
	std::ofstream file;
	file.open(sProgramDir + "/controls.dat", std::ios::binary);
	if (!file.is_open()) return false;

	file.write("nya~", 4);
	file.write((char*)aPlayerControls, sizeof(aPlayerControls));

	return true;
}

bool SaveGameConfig() {
	std::ofstream file;
	file.open(sProgramDir + "/config.dat", std::ios::binary);
	if (!file.is_open()) return false;

	file.write("nya~", 4);
	file.write((char*)&gGameConfig, sizeof(gGameConfig));

	return true;
}

bool LoadPieceTypes() {
	std::ifstream file;
	file.open(sProgramDir + "/pieces.dat", std::ios::binary);
	if (!file.is_open()) return false;

	char header[4];
	file.read(header, sizeof(header));
	if (memcmp(header, "nya~", sizeof(header)) != 0) return false;

	int numPieceTypes = 0;
	file.read((char*)&numPieceTypes, sizeof(numPieceTypes));
	for (int i = 0; i < numPieceTypes; i++) {
		tPieceType type;
		int numRotations = 0;
		file.read((char*)&numRotations, sizeof(numRotations));

		for (int j = 0; j < numRotations; j++) {
			tPieceType::tRotation data;
			file.read((char*)&data, sizeof(data));
			if (!data.IsEmpty()) type.rotations.push_back(data);
		}

		if (!type.rotations.empty()) aPieceTypes.push_back(type);
	}

	return !aPieceTypes.empty();
}