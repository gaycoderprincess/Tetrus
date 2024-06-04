#ifndef TETRUS_NET_H
#define TETRUS_NET_H

#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_audio.h"
#include "tetrus_board.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_saveload.h"
#include "tetrus_menu.h"
#include "tetrus_replay.h"
#include "tetrus_scoreboard.h"
#include "tetrus_net.h"

extern uint32_t nNetGameAddress;
extern int nSpectatePlayer;

void DisconnectNet();
void NetStatusUpdateCallback(bool connected, NyaNet::NyaNetHandle handle);
void NetPacketCallback(void* data, size_t len, NyaNet::NyaNetHandle handle);
bool NetCreateGame();
bool NetJoinGame();
int NetGetNumConnectedPlayers();
void ProcessNet();
void ProcessNetScoreboards();
void ProcessNetSpectate();

class CNetPlayerData {
public:
	uint8_t pieceType : 4;
	uint8_t nextPieceType : 4;
	uint8_t pieceRotation : 4;
	unsigned int colorId : 4;
	unsigned int colorIdLast : 4;
	bool altTexture : 1;
	bool altTextureLast : 1;
	int8_t pieceX;
	int8_t pieceY;

	void Fill();
};

class CNetSlowPlayerData {
public:
	uint32_t score;
	uint32_t lines;
	uint32_t level;
	int8_t currentPrideFlag;
	tGameConfig::tColorSetup currentColorSetup;
	bool alive : 1;
	bool inGame : 1;
	bool prideFlags : 1;
	bool dropPreviewOn : 1;
	CBoard::tPlacedPiece board[gBoardSizeX * gBoardSizeY];

	void Fill();
};

class CNetPlayer {
public:
	NyaNet::NyaNetHandle handle;
	bool connected;
	char playerName[24];
	int playerId = -1;
	CNetPlayerData playerData;
	CNetSlowPlayerData slowPlayerData;

	CNetPlayer();
	const char* GetName();
	void SetAsLocalHost();
	void Reset();
};
extern CNetPlayer aNetPlayers[NyaNet::nMaxPossiblePlayers];
extern CNetPlayer gLocalNetPlayer;

CNetPlayer* GetNetPlayerFromHandle(NyaNet::NyaNetHandle handle);

class CNetGameSettings {
public:
	uint8_t startingLevel;
	uint8_t randomizerType : 4;
	bool nesInitialLevelClear : 1;
	bool randSameForAllPlayers : 1;

	void Fill();
	void Read() const;
};
extern CNetGameSettings gNetGameSettings;

enum eNyaPacketType {
	NYA_PACKETTYPE_SERVER_DATA = 1,
	NYA_PACKETTYPE_CLIENT_DATA,
	NYA_PACKETTYPE_CLIENT_SLOW_DATA,
	NYA_PACKETTYPE_UPDATE_PLAYER_LIST,
	NYA_PACKETTYPE_START_GAME,
	NYA_PACKETTYPE_END_GAME,
	NUM_NYA_PACKET_TYPES
};

template<typename T>
class CNyaNetPacket {
public:
	int type = T::nType;
	uint64_t counter = 0;
	int playerId = 0;
	T data;

	// counters to make sure old packets are ditched
	// this is per type! template statics are neat :3
	static inline uint64_t nSentPacketCounter = 1;
	static inline bool bRemoteSentPacketCountersInitialized = false;
	static inline uint64_t nRemoteSentPacketCounters[NyaNet::nMaxPossiblePlayers] = { 0 };
	static inline NyaNet::NyaNetHandle nRemoteSentPacketHandles[NyaNet::nMaxPossiblePlayers] = { nullptr };

	CNyaNetPacket() {
		if (!bRemoteSentPacketCountersInitialized) {
			memset(nRemoteSentPacketCounters, 0, sizeof(nRemoteSentPacketCounters));
			memset(nRemoteSentPacketHandles, 0, sizeof(nRemoteSentPacketHandles));
			bRemoteSentPacketCountersInitialized = true;
		}

		counter = nSentPacketCounter++;
	}

	bool IsValid(uint32_t size) {
		if (type <= 0 || type >= NUM_NYA_PACKET_TYPES) return false;
		if (size != sizeof(*this)) return false;
		if (!NyaNet::IsServer() && (playerId < 0 || playerId >= NyaNet::nMaxPossiblePlayers)) return false;
		return data.IsValid();
	}

	bool IsRepeat() {
		if (!bRemoteSentPacketCountersInitialized) {
			memset(nRemoteSentPacketCounters, 0, sizeof(nRemoteSentPacketCounters));
			memset(nRemoteSentPacketHandles, 0, sizeof(nRemoteSentPacketHandles));
			bRemoteSentPacketCountersInitialized = true;
		}

		if (!data.bReliable) {
			if (nRemoteSentPacketHandles[playerId] != aNetPlayers[playerId].handle) {
				nRemoteSentPacketHandles[playerId] = aNetPlayers[playerId].handle;
				nRemoteSentPacketCounters[playerId] = 0;
			}

			if (nRemoteSentPacketCounters[playerId] == counter) return true;
			nRemoteSentPacketCounters[playerId] = counter;
		}
		return false;
	}

	void Parse() {
		return data.Parse(playerId);
	}

	void Send(bool parse) {
		if (parse) Parse();
		NyaNet::QueuePacket(nullptr, this, sizeof(*this), data.bReliable);
	}

	void SendToPlayer(NyaNet::NyaNetHandle handle) {
		NyaNet::QueuePacket(handle, this, sizeof(*this), data.bReliable);
	}
};

struct tServerDataPacket {
	CNetGameSettings gameSettings;

	static inline bool bReliable = false;
	static inline int nType = NYA_PACKETTYPE_SERVER_DATA;

	tServerDataPacket() {
		gameSettings = gNetGameSettings;
	}

	bool IsValid() {
		return true;
	}

	void Parse(int playerId) const {
		gNetGameSettings = gameSettings;
	}
};

struct tClientDataPacket {
	CNetPlayerData clientData;

	static inline bool bReliable = false;
	static inline int nType = NYA_PACKETTYPE_CLIENT_DATA;

	tClientDataPacket() {
		clientData = gLocalNetPlayer.playerData;
	}

	bool IsValid() {
		return true;
	}

	void Parse(int playerId) const {
		if (playerId < 0) return;
		aNetPlayers[playerId].playerData = clientData;
	}
};

struct tClientSlowDataPacket {
	char playerName[24];
	CNetSlowPlayerData clientData;

	static inline bool bReliable = false;
	static inline int nType = NYA_PACKETTYPE_CLIENT_SLOW_DATA;

	tClientSlowDataPacket() {
		memset(playerName, 0, sizeof(playerName));
		strcpy_s(playerName, 24, gGameConfig.playerName[0]);
		clientData = gLocalNetPlayer.slowPlayerData;
	}

	bool IsValid() {
		if (!playerName[0]) return false;
		if (playerName[23]) return false;
		return true;
	}

	void Parse(int playerId) const {
		if (playerId < 0) return;
		strcpy_s(aNetPlayers[playerId].playerName, 24, playerName);
		aNetPlayers[playerId].slowPlayerData = clientData;
	}
};

struct tPlayerListUpdatePacket {
	struct {
		NyaNet::NyaNetHandle handle;
		bool connected;
	} players[NyaNet::nMaxPossiblePlayers];

	static inline bool bReliable = false;
	static inline int nType = NYA_PACKETTYPE_UPDATE_PLAYER_LIST;

	tPlayerListUpdatePacket() {
		memset(players, 0, sizeof(players));
		for (int i = 0; i < NyaNet::nMaxPossiblePlayers; i++) {
			players[i].handle = aNetPlayers[i].handle;
			players[i].connected = aNetPlayers[i].connected;
		}
	}

	bool IsValid() {
		return true;
	}

	void Parse(int playerId) const {
		for (int i = 0; i < NyaNet::nMaxPossiblePlayers; i++) {
			aNetPlayers[i].handle = players[i].handle;
			aNetPlayers[i].connected = players[i].connected;
		}
	}
};

struct tStartGamePacket {
	bool rngForced = false;
	int rng = 0;

	static inline bool bReliable = true;
	static inline int nType = NYA_PACKETTYPE_START_GAME;

	bool IsValid() {
		return true;
	}

	void Parse(int playerId) const {
		if (playerId != 0) return;

		PlayGameSound(SOUND_START);
		gGameState = STATE_PLAYING;
		gForcedRNGValue = rngForced ? rng : -1;
		nSpectatePlayer = 0;
		ResetAllPlayers();
	}
};

struct tEndGamePacket {
	static inline bool bReliable = true;
	static inline int nType = NYA_PACKETTYPE_END_GAME;

	bool IsValid() {
		return true;
	}

	void Parse(int playerId) const {
		if (playerId != 0) return;

		ShowEndgameScoreboard();
	}
};
#endif