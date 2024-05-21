#include <string>

#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"

int gGameState = STATE_MAIN_MENU;

tGameConfig::tColorSetup tGameConfig::GetDefaultColorSetup(int level) {
	tGameConfig::tColorSetup defaults[10] = {
			{ 0x0F, 0x30, 0x21, 0x12 },
			{ 0x0F, 0x30, 0x29, 0x1A },
			{ 0x0F, 0x30, 0x24, 0x14 },
			{ 0x0F, 0x30, 0x2A, 0x12 },
			{ 0x0F, 0x30, 0x2B, 0x15 },
			{ 0x0F, 0x30, 0x22, 0x2B },
			{ 0x0F, 0x30, 0x00, 0x16 },
			{ 0x0F, 0x30, 0x05, 0x13 },
			{ 0x0F, 0x30, 0x16, 0x12 },
			{ 0x0F, 0x30, 0x27, 0x16 },
	};
	return defaults[level % 10];
}

tGameConfig::tGameConfig() {
	startingLevel = 0;
	randomizerType = RANDOMIZER_NES;
	blockTexture = SPRITE_CLASSIC;
	gameType = TYPE_1PLAYER;
	sfxOn = true;
	memset(dropPreviewOn, 0, sizeof(dropPreviewOn));
	for (int i = 0; i < 10; i++) {
		pieceColors[i] = GetDefaultColorSetup(i);
	}
	prideFlags = false;
	prideFlagsOptionVisible = false;
	memset(playerName, 0, sizeof(playerName));
	for (int i = 0; i < 4; i++) {
		strcpy_s(playerName[i], std::format("Player {}", i + 1).c_str());
	}
	boardBorders = false;
	vsync = true;
	randSameForAllPlayers = false;
	prideFlagColors[0] = PRIDE_FLAG_ENBY;
	prideFlagColors[1] = PRIDE_FLAG_ENBY;
	prideFlagColors[2] = PRIDE_FLAG_ENBY;
	prideFlagColors[3] = PRIDE_FLAG_ENBY;
	prideFlagColors[4] = PRIDE_FLAG_LESBIAN;
	prideFlagColors[5] = PRIDE_FLAG_LESBIAN;
	prideFlagColors[6] = PRIDE_FLAG_LESBIAN;
	prideFlagColors[7] = PRIDE_FLAG_TRANS;
	prideFlagColors[8] = PRIDE_FLAG_TRANS;
	prideFlagColors[9] = PRIDE_FLAG_TRANS;
	overlappingSounds = true;
}

bool tGameConfig::IsCoopMode() const {
	return gameType == TYPE_2PLAYER_COOP || gameType == TYPE_3PLAYER_2V1COOPVS || gameType == TYPE_4PLAYER_COOPVS;
}

int tGameConfig::GetStartingLevel() const {
	return startingLevelIds[startingLevel];
}

const char* tGameConfig::GetStartingLevelName(int level) {
	static std::string name;
	name = "Level ";
	name += GetLevelName(startingLevelIds[level]);
	return name.c_str();
}

const char* tGameConfig::GetStartingLevelName() const {
	return GetStartingLevelName(startingLevel);
}

const char* tGameConfig::GetBlockTextureName() const {
	switch (blockTexture) {
		case SPRITE_STANDARD:
			return "Standard";
		case SPRITE_CLASSIC:
			return "Classic";
		case SPRITE_BEZEL:
			return "Bezel";
		default:
			return "Invalid";
	}
}

const char* tGameConfig::GetRandomizerTypeName(int randomizer) {
	switch (randomizer) {
		case RANDOMIZER_NES:
			return "NES";
		case RANDOMIZER_TRUE_RANDOM:
			return "True Random";
		case RANDOMIZER_MODERN:
			return "Modern";
		default:
			return "Invalid";
	}
}

const char* tGameConfig::GetRandomizerTypeName() const {
	return GetRandomizerTypeName(randomizerType);
}

const char* tGameConfig::GetGameTypeName() const {
	switch (gameType) {
		case TYPE_1PLAYER:
			return "1 Player";
		case TYPE_2PLAYER_COOP:
			return "2 Player Co-op";
		case TYPE_2PLAYER_VS:
			return "2 Player VS";
		case TYPE_4PLAYER_VS:
			return "4 Player VS";
		case TYPE_3PLAYER_2V1COOPVS:
			return "2v1 Co-op VS";
		case TYPE_4PLAYER_COOPVS:
			return "2v2 Co-op VS";
		default:
			return "Invalid";
	}
}

const char* tGameConfig::GetPrideFlagTypeName(int type) {
	switch (type) {
		case PRIDE_FLAG_ENBY:
			return "Non-Binary Flag";
		case PRIDE_FLAG_LESBIAN:
			return "Lesbian Flag";
		case PRIDE_FLAG_TRANS:
			return "Trans Flag";
		case PRIDE_FLAG_BI:
			return "Bisexual Flag";
		case PRIDE_FLAG_RAINBOW:
			return "Rainbow Flag";
		default:
			return "Invalid";
	}
}

std::vector<NyaDrawing::CNyaRGBA32> tGameConfig::GetPrideFlagColor(int type) {
	std::vector<NyaDrawing::CNyaRGBA32> colors;
	switch (type) {
		case PRIDE_FLAG_ENBY:
			colors.push_back({255, 244, 48, 255});
			colors.push_back({215, 215, 215, 255});
			colors.push_back({156, 89, 209, 255});
			colors.push_back({64, 64, 64, 255});
			break;
		case PRIDE_FLAG_LESBIAN:
			colors.push_back({213,45,0, 255});
			colors.push_back({255,154,86, 255});
			colors.push_back({215,215,215, 255});
			colors.push_back({211,98,164, 255});
			colors.push_back({163,2,98, 255});
			break;
		case PRIDE_FLAG_TRANS:
			colors.push_back({91,207,250, 255});
			colors.push_back({245,171,185, 255});
			colors.push_back({215,215,215, 255});
			break;
		case PRIDE_FLAG_BI:
			colors.push_back({214,2,112, 255});
			colors.push_back({153,80,149, 255});
			colors.push_back({0,56,167, 255});
			break;
		case PRIDE_FLAG_RAINBOW:
			colors.push_back({229,0,0, 255});
			colors.push_back({255,141,0, 255});
			colors.push_back({255,238,0, 255});
			colors.push_back({2,129,33, 255});
			colors.push_back({0,76,255, 255});
			colors.push_back({119,0,136, 255});
			break;
		default:
			colors.push_back({0, 0, 0, 255});
			break;
	}
	return colors;
}
tGameConfig gGameConfig;