#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_board.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_net.h"

uint32_t nNetGameAddress = 0;
uint16_t nNetGamePort = 9803;
int nSpectatePlayer = 0;
CNetGameSettings gNetGameSettings;
CNetPlayer aNetPlayers[NyaNet::nMaxPossiblePlayers];
CNetPlayer gLocalNetPlayer;

CNetPlayer::CNetPlayer() {
	Reset();
}

void CNetPlayer::Reset() {
	handle = nullptr;
	connected = false;
	memset(playerName, 0, sizeof(playerName));
	memset(&playerData, 0, sizeof(playerData));
	memset(&slowPlayerData, 0, sizeof(slowPlayerData));
}

const char* CNetPlayer::GetName() {
	if (playerName[0]) return playerName;
	return "???";
}

void CNetPlayer::SetAsLocalHost() {
	Reset();
	connected = true;
	strcpy_s(playerName, 24, gGameConfig.playerName[0]);
}

CNetPlayer* GetNetPlayerFromHandle(NyaNet::NyaNetHandle handle) {
	for (auto& ply : aNetPlayers) {
		if (handle == ply.handle) return &ply;
	}
	return nullptr;
}

CNetPlayer* GetNewNetPlayer() {
	// first go through all empty slots, then go through all slots that were occupied by players who left
	// should be less unstable with rapid rejoining this way methinks
	for (auto& ply : aNetPlayers) {
		if (!ply.handle && !ply.connected) return &ply;
	}
	for (auto& ply : aNetPlayers) {
		if (ply.handle && !ply.connected) return &ply;
	}
	return nullptr;
}

void ResetNetPlayers() {
	for (int i = 0; i < NyaNet::nMaxPossiblePlayers; i++) {
		aNetPlayers[i].Reset();
		aNetPlayers[i].playerId = i;
	}
}

void DisconnectNet() {
	if (NyaNet::IsConnected()) {
		NyaNet::Disconnect();
	}
	ResetNetPlayers();
}

void NetStatusUpdateCallback(bool connected, NyaNet::NyaNetHandle handle) {
	if (NyaNet::IsClient()) {
		// finished connecting to server
		if (connected) {
			return;
		}
		// disconnected from server
		else {
			DisconnectNet();
			return;
		}
	}
	else {
		if (connected) {
			auto ply = GetNewNetPlayer();
			if (!ply) return;

			ply->Reset();
			ply->handle = handle;
			ply->connected = true;
		}
		else {
			auto ply = GetNetPlayerFromHandle(handle);
			if (!ply) return;

			ply->handle = nullptr;
			ply->connected = false;
		}
	}
}

template<typename T>
bool PacketHandler(void* data, size_t len, NyaNet::NyaNetHandle handle) {
	auto packet = (CNyaNetPacket<T>*)data;
	if (packet->type == T::nType) {
		if (!packet->IsValid(len)) return true;

		if (NyaNet::IsServer()) {
			auto ply = GetNetPlayerFromHandle(handle);
			if (!ply) return true;
			packet->playerId = ply->playerId;
		}
		if (packet->IsRepeat()) return true;

		if (NyaNet::IsServer()) packet->Send(1);
		else packet->Parse();
		return true;
	}
	return false;
}

void NetPacketCallback(void* data, size_t len, NyaNet::NyaNetHandle handle) {
	if (PacketHandler<tServerDataPacket>(data, len, handle)) return;
	if (PacketHandler<tClientDataPacket>(data, len, handle)) return;
	if (PacketHandler<tClientSlowDataPacket>(data, len, handle)) return;
	if (PacketHandler<tPlayerListUpdatePacket>(data, len, handle)) return;
	if (PacketHandler<tStartGamePacket>(data, len, handle)) return;
	if (PacketHandler<tEndGamePacket>(data, len, handle)) return;

	std::string name = "unknown";
	if (auto ply = GetNetPlayerFromHandle(handle)) {
		name = ply->GetName();
	}
	LogString("Unhandled packet " + std::to_string(*(int*)data) + " of size " + std::to_string(len) + " from " + name);
}

bool NetCreateGame() {
	if (NyaNet::IsServer()) return true;
	if (!nNetGamePort) return false;

	NyaNet::Disconnect();
	ResetNetPlayers();
	if (!NyaNet::Host(nNetGamePort, NetStatusUpdateCallback, NetPacketCallback)) {
		DisconnectNet();
		return false;
	}
	aNetPlayers[0].SetAsLocalHost();
	return true;
}

bool NetJoinGame() {
	if (NyaNet::IsClient()) return true;
	if (!nNetGamePort) return false;

	NyaNet::Disconnect();
	ResetNetPlayers();
	if (!NyaNet::Connect(nNetGameAddress, nNetGamePort, NetStatusUpdateCallback, NetPacketCallback)) {
		DisconnectNet();
		return false;
	}
	return true;
}

void CNetGameSettings::Fill() {
	startingLevel = gGameConfig.startingLevel;
	randomizerType = gGameConfig.randomizerType;
	nesInitialLevelClear = gGameConfig.nesInitialLevelClear;
	randSameForAllPlayers = gGameConfig.randSameForAllPlayers;
}

void CNetGameSettings::Read() const {
	gGameConfig.startingLevel = startingLevel;
	gGameConfig.randomizerType = randomizerType;
	gGameConfig.nesInitialLevelClear = nesInitialLevelClear;
	gGameConfig.gameType = tGameConfig::TYPE_1PLAYER;
}

void CNetPlayerData::Fill() {
	pieceType = aPlayers[0]->pieceType;
	nextPieceType = aPlayers[0]->nextPieceType;
	pieceRotation = aPlayers[0]->pieceRotation;
	pieceX = aPlayers[0]->posX;
	pieceY = aPlayers[0]->posY;
	colorId = aPlayers[0]->colorId;
	colorIdLast = aPlayers[0]->colorIdLast;
	altTexture = aPlayers[0]->altTexture;
	altTextureLast = aPlayers[0]->altTextureLast;
}

void CNetSlowPlayerData::Fill() {
	score = aBoards[0]->score;
	lines = aBoards[0]->linesCleared;
	level = aBoards[0]->level;
	prideFlags = gGameConfig.prideFlags;
	currentPrideFlag = gGameConfig.prideFlagColors[aBoards[0]->level % 10];
	currentColorSetup = *aBoards[0]->GetColorSetup();
	dropPreviewOn = gGameConfig.dropPreviewOn[0];
	alive = !aPlayers[0]->gameOver;
	inGame = gGameState == STATE_PLAYING;
	memcpy(board, aBoards[0]->data, sizeof(board));
}

void ProcessNet() {
	if (gGameState != STATE_PLAYING || !NyaNet::IsConnected()) bOnlinePauseMenu = false;

	if (!NyaNet::IsConnected()) return;

	NyaNet::Process();

	if (NyaNet::IsServer()) {
		gNetGameSettings.Fill();
	}
	else {
		gNetGameSettings.Read();
	}

	if (!aBoards.empty() && !aPlayers.empty()) {
		gLocalNetPlayer.playerData.Fill();
		gLocalNetPlayer.slowPlayerData.Fill();
	}

	{
		const double syncInterval = 1.0 / 4.0;
		static double syncTimer = 0;
		syncTimer += gGameTimer.fDeltaTime;
		if (syncTimer > syncInterval) {
			if (NyaNet::IsServer()) {
				CNyaNetPacket<tServerDataPacket> packet;
				packet.Send(false);
			}
			CNyaNetPacket<tClientSlowDataPacket> packet;
			packet.Send(NyaNet::IsServer());
			syncTimer -= syncInterval;
		}
	}

	{
		const double syncInterval = 1.0 / 30.0;
		static double syncTimer = 0;
		syncTimer += gGameTimer.fDeltaTime;
		if (syncTimer > syncInterval) {
			CNyaNetPacket<tClientDataPacket> packet;
			packet.Send(NyaNet::IsServer());
			syncTimer -= syncInterval;
		}
	}

	if (NyaNet::IsServer()) {
		const double syncInterval = 1.0 / 0.5;
		static double syncTimer = 0;
		syncTimer += gGameTimer.fDeltaTime;
		if (syncTimer > syncInterval) {
			CNyaNetPacket<tPlayerListUpdatePacket> packet;
			packet.Send(false);
			syncTimer -= syncInterval;
		}
	}
}

bool IsPlayerValidForSpectate(int playerId) {
	if (playerId < 0) return false;
	if (playerId >= NyaNet::nMaxPossiblePlayers) return false;

	auto spectatePlayer = &aNetPlayers[nSpectatePlayer];
	if (!spectatePlayer->connected) return false;
	if (!spectatePlayer->slowPlayerData.inGame) return false;
	return true;
}

void PickNextSpectatePlayer(int add) {
	int counter = 0;
	do {
		nSpectatePlayer += add;
		if (nSpectatePlayer < 0) nSpectatePlayer = NyaNet::nMaxPossiblePlayers - 1;
		if (nSpectatePlayer >= NyaNet::nMaxPossiblePlayers) nSpectatePlayer = 0;
		counter++;
	} while (!IsPlayerValidForSpectate(nSpectatePlayer) && counter < NyaNet::nMaxPossiblePlayers * 2);
}

int NetGetNumConnectedPlayers() {
	int count = 0;
	for (auto& ply : aNetPlayers) {
		if (!ply.connected) continue;
		if (!ply.playerName[0]) continue;
		count++;
	}
	return count;
}

void ProcessNetSpectate() {
	auto spectatePlayer = &aNetPlayers[nSpectatePlayer];
	if (GetMenuLeft()) PickNextSpectatePlayer(-1);
	if (GetMenuRight() || !IsPlayerValidForSpectate(nSpectatePlayer)) PickNextSpectatePlayer(1);

	if (!IsPlayerValidForSpectate(nSpectatePlayer)) return;

	auto local = GetMPSpectatePlayer();
	auto localBoard = &gMPSpectateBoard;
	auto remote = &spectatePlayer->playerData;
	auto remoteSlow = &spectatePlayer->slowPlayerData;

	local->isRemotePlayer = true;
	localBoard->isRemoteBoard = true;
	strcpy_s(local->playerName, 24, spectatePlayer->GetName());

	local->pieceType = remote->pieceType;
	local->nextPieceType = remote->nextPieceType;
	local->pieceRotation = remote->pieceRotation;
	local->posX = remote->pieceX;
	local->posY = remote->pieceY;
	local->colorId = remote->colorId;
	local->colorIdLast = remote->colorIdLast;
	local->altTexture = remote->altTexture;
	local->altTextureLast = remote->altTextureLast;
	local->dropPreviewOn = remoteSlow->dropPreviewOn;
	localBoard->score = remoteSlow->score;
	localBoard->linesCleared = remoteSlow->lines;
	localBoard->level = remoteSlow->level;
	localBoard->prideFlags = remoteSlow->prideFlags;
	localBoard->remoteBoardPrideFlag = remoteSlow->currentPrideFlag;
	localBoard->remoteBoardColorSetup = remoteSlow->currentColorSetup;
	local->gameOver = !remoteSlow->alive;
	memcpy(localBoard->data, remoteSlow->board, sizeof(remoteSlow->board));

	local->Process();
	localBoard->Process();
}

struct tScoreboardEntry {
	const char* playerName;
	uint32_t level;
	uint32_t score;
	uint32_t lines;
	bool alive;
};

bool ScoreboardCompFunction(tScoreboardEntry a, tScoreboardEntry b) {
	if (a.score == b.score) {
		return a.lines > b.lines;
	}
	return a.score > b.score;
}

void ProcessNetScoreboards() {
	std::vector<tScoreboardEntry> scoreboard;

	for (auto& ply : aNetPlayers) {
		if (!ply.connected) continue;
		if (!ply.slowPlayerData.inGame) continue;

		scoreboard.push_back({ply.GetName(), ply.slowPlayerData.level, ply.slowPlayerData.score, ply.slowPlayerData.lines, ply.slowPlayerData.alive});
	}

	if (scoreboard.empty()) return;

	sort(scoreboard.begin(), scoreboard.end(), ScoreboardCompFunction);

	float maxWidth = 0;

	const float top = 0.1;
	const float bottom = 0.9;
	float size = 0.03;
	while (top + (scoreboard.size() * size) > bottom) {
		size *= 0.9;
	}

	tNyaStringData string;
	string.x = 0.1 * GetAspectRatioInv();
	string.y = top;
	string.size = size;
	string.topLevel = true;

	for (auto& ply : scoreboard) {
		auto str = ply.playerName + (std::string)" - " + std::to_string(ply.score) + " (" + std::to_string(ply.lines) + ", Lv" + GetLevelName(ply.level) + ")";
		auto width = GetStringWidth(string.size, str.c_str());
		if (width > maxWidth) maxWidth = width;
		if (ply.alive) string.SetColor(255, 255, 255, 255);
		else string.SetColor(255, 0, 0, 255);
		DrawString(string, str.c_str());
		string.y += size;
	}

	float bgSpacing = 0.025 * GetAspectRatioInv();
	float bgWidth = maxWidth + bgSpacing;
	float ySpacing = 0.025;
	DrawRectangle(string.x - bgSpacing, string.x + bgWidth, top - ySpacing, string.y - 0.03 + ySpacing, {0,0,0,127}, 0.01);
}