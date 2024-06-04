#ifndef TETRUS_CONFIG_H
#define TETRUS_CONFIG_H

enum eGameState {
	STATE_MAIN_MENU,
	STATE_PLAYING,
	STATE_PAUSED,
	STATE_SCOREBOARD_VIEW,
	STATE_REPLAY_VIEW,
	STATE_REPLAY_PAUSED
};
extern int gGameState;

// struct to make it easier to save/load :3
struct tGameConfig {
	enum eGameType {
		TYPE_1PLAYER,
		TYPE_2PLAYER_COOP,
		TYPE_2PLAYER_VS,
		TYPE_4PLAYER_VS,
		TYPE_3PLAYER_2V1COOPVS,
		TYPE_4PLAYER_COOPVS,
		NUM_GAMETYPES
	};

	enum eRandomizerType {
		RANDOMIZER_NES,
		RANDOMIZER_TRUE_RANDOM,
		RANDOMIZER_MODERN,
		NUM_RANDOMIZER_TYPES
	};

	enum eBlockSpriteType {
		SPRITE_STANDARD,
		SPRITE_CLASSIC,
		SPRITE_BEZEL,
		NUM_SPRITE_TYPES
	};

	enum ePrideFlagType {
		PRIDE_FLAG_ENBY,
		PRIDE_FLAG_LESBIAN,
		PRIDE_FLAG_TRANS,
		PRIDE_FLAG_BI,
		PRIDE_FLAG_RAINBOW,
		NUM_PRIDE_FLAG_TYPES
	};

	constexpr static const int startingLevelIds[5] = {
			0, 9, 18, 19, 29
	};
	static const int NUM_STARTING_LEVELS = sizeof(startingLevelIds) / sizeof(startingLevelIds[0]);

	int startingLevel;
	int randomizerType;
	int blockTexture;
	int gameType;
	bool sfxOn;
	bool dropPreviewOn[4];
	struct tColorSetup {
		char backgroundPaletteId[2];
		char paletteId[2];
	} pieceColors[10];
	bool prideFlags;
	bool prideFlagsOptionVisible;
	char playerName[4][24];
	bool boardBorders;
	bool vsync;
	bool randSameForAllPlayers;
	char prideFlagColors[10];
	bool overlappingSounds;
	bool nesInitialLevelClear;
	char ipAddress[64];
	char port[64];

	tGameConfig();
	static tColorSetup GetDefaultColorSetup(int level);
	[[nodiscard]] bool IsCoopMode() const;
	[[nodiscard]] int GetStartingLevel() const;
	[[nodiscard]] static const char* GetStartingLevelName(int level);
	[[nodiscard]] const char* GetStartingLevelName() const;
	[[nodiscard]] const char* GetBlockTextureName() const;
	[[nodiscard]] static const char* GetRandomizerTypeName(int randomizer);
	[[nodiscard]] const char* GetRandomizerTypeName() const;
	[[nodiscard]] const char* GetGameTypeName() const;
	[[nodiscard]] static const char* GetPrideFlagTypeName(int type);
	[[nodiscard]] static std::vector<NyaDrawing::CNyaRGBA32> GetPrideFlagColor(int type);
};
extern tGameConfig gGameConfig;
#endif