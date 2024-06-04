#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_audio.h"
#include "tetrus_board.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_replay.h"

int gForcedRNGValue = -1;

CPlayer::CPlayer(int playerId, CBoard* board) {
	this->board = board;
	this->playerId = playerId;
	this->pad = &aPlayerControls[playerId];
	this->recordingReplay = nullptr;
}

double CPlayer::GetGravity() const {
	if (initialGravity) return NESFramesToSeconds(48);

	switch (board->level) {
		case 0:
			return NESFramesToSeconds(48);
		case 1:
			return NESFramesToSeconds(43);
		case 2:
			return NESFramesToSeconds(38);
		case 3:
			return NESFramesToSeconds(33);
		case 4:
			return NESFramesToSeconds(28);
		case 5:
			return NESFramesToSeconds(23);
		case 6:
			return NESFramesToSeconds(18);
		case 7:
			return NESFramesToSeconds(13);
		case 8:
			return NESFramesToSeconds(8);
		case 9:
			return NESFramesToSeconds(6);
		case 10:
		case 11:
		case 12:
			return NESFramesToSeconds(5);
		case 13:
		case 14:
		case 15:
			return NESFramesToSeconds(4);
		case 16:
		case 17:
		case 18:
			return NESFramesToSeconds(3);
		case 19:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
		case 25:
		case 26:
		case 27:
		case 28:
			return NESFramesToSeconds(2);
		default:
			return NESFramesToSeconds(1);
	}
}

tPieceType::tRotation* CPlayer::GetRotatedPiece() const {
	return &GetPieceType()->rotations[pieceRotation];
}

tPieceType::tRotation* CPlayer::GetPreviewPiece() const {
	return &GetPreviewPieceType()->rotations[0];
}

tPieceType* CPlayer::GetPieceType() const {
	return &aPieceTypes[pieceType];
}

tPieceType* CPlayer::GetPreviewPieceType() const {
	return &aPieceTypes[nextPieceType];
}

int CPlayer::GetNumRotations() const {
	return aPieceTypes[pieceType].rotations.size();
}

bool CPlayer::IsRightSideCoopPlayer() const {
	if (!board->isCoopBoard) return false;
	return playerId == 1 || playerId == 3;
};

int CPlayer::GetBoardXMin() const {
	if (!board->isCoopBoard) return 0;
	if (!IsRightSideCoopPlayer()) return 0;
	return board->extentsX * 0.5;
}

int CPlayer::GetBoardXMax() const {
	if (!board->isCoopBoard) return board->extentsX;
	if (IsRightSideCoopPlayer()) return board->extentsX;
	return board->extentsX * 0.5;
}

bool CPlayer::IsPieceWithinExtents(int x, int y) const {
	auto piece = GetRotatedPiece();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			if (piece->data[i][j]) {
				if (x + i < GetBoardXMin()) return false;
				if (x + i >= GetBoardXMax()) return false;
				if (y + j >= board->extentsY) return false;
			}
		}
	}
	return true;
}

bool CPlayer::IsPiecePositionValid(int x, int y) const {
	if (!IsPieceWithinExtents(x, y)) return false;

	auto piece = GetRotatedPiece();
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			if (piece->data[i][j]) {
				if (y + j < 0) continue;

				if (board->GetPieceAt(x + i, y + j)->exists) return false;
			}
		}
	}
	return true;
}

int CPlayer::GetRandomPieceID(int max) {
	std::uniform_int_distribution<int> uni(0, max - 1);
	return uni(pieceRNG);
}

void CPlayer::SetPieceRandomizer(int seed) {
	pieceRNG = std::mt19937(pieceRNGSeed = seed);
}

void CPlayer::ResetPieceRandomizer() {
	auto randSeed = time(nullptr);
	if (!gGameConfig.randSameForAllPlayers) randSeed += playerId;
	SetPieceRandomizer(gForcedRNGValue >= 0 ? gForcedRNGValue : randSeed);
}

int CPlayer::GetPieceRandomizerSeed() const {
	return pieceRNGSeed;
}

int CPlayer::GenerateNextPieceType() {
	if (gGameConfig.randomizerType == tGameConfig::RANDOMIZER_MODERN) {
		std::vector<int> aAvailableTypes;
		for (int i = 0; i < aPieceTypes.size(); i++) {
			if (!pieceAlreadyPicked[i]) aAvailableTypes.push_back(i);
		}
		if (aAvailableTypes.empty()) {
			memset(pieceAlreadyPicked, 0, aPieceTypes.size());
			return GenerateNextPieceType();
		}
		return aAvailableTypes[GetRandomPieceID(aAvailableTypes.size())];
	}
	else {
		int type = GetRandomPieceID(aPieceTypes.size());
		if (gGameConfig.randomizerType == tGameConfig::RANDOMIZER_NES && type == pieceType) type = GetRandomPieceID(aPieceTypes.size());
		return type;
	}
}

void CPlayer::NextPiece(bool checkGameOver) {
	pieceType = nextPieceType;
	nextPieceType = GenerateNextPieceType();
	pieceAlreadyPicked[nextPieceType] = true;
	pieceCounter[pieceType]++;
	pieceRotation = 0;
	gravityTimer = 0;
	downReleaseRequired = true;
	dasLeftTimer = 0;
	dasLeftMovingTimer = 0;
	dasRightTimer = 0;
	dasRightMovingTimer = 0;
	dropWaitTimer = 0;
	colorIdLast = colorId;
	colorId = GetRandom(gGameConfig.prideFlags ? 16 : 2);
	altTextureLast = altTexture;
	altTexture = (gGameConfig.prideFlags || colorId == 1) && GetRandom(2) == 1;
	posX = IsRightSideCoopPlayer() ? (board->extentsX * 0.5) + 3 : 3;
	posY = -2;

	if (checkGameOver && !IsPiecePositionValid(posX, posY)) {
		PlayGameSound(SOUND_GAMEOVER);
		gameOver = true;
	}
}

void CPlayer::DrawDropPreview() const {
	int x = posX;
	int y = posY;
	while (IsPiecePositionValid(x, y)) {
		y++;
	}
	GetRotatedPiece()->Draw(board, x, y - 1, colorId, 127, true, altTexture);
}

void CPlayer::Draw() const {
	if (pieceType < 0 || pieceType >= aPieceTypes.size()) return;
	if (nextPieceType < 0 || nextPieceType >= aPieceTypes.size()) return;

	bool useCurrentPieceAsPreview = (board->state == CBoard::STATE_LINE_CLEARING && !board->isCoopBoard) || drawCurrentAsPreviewNextFrame;
	if (isRemotePlayer) useCurrentPieceAsPreview = false;

	float previewX = board->extentsX + 3;
	float previewY = board->extentsY * 0.5;
	if ((board->isCoopBoard && !IsRightSideCoopPlayer()) || (!board->isCoopBoard && board->areStatsOnLeft)) {
		previewX = -7;
	}
	if (board->areStatsOnBottom) {
		previewX = GetBoardXMax() - 4.5;
		previewY = board->extentsY - 1;
	}

	if (useCurrentPieceAsPreview) {
		GetRotatedPiece()->Draw(board, previewX, previewY, colorIdLast, 255, false, altTextureLast);
	}
	else {
		if (dropPreviewOn) DrawDropPreview();
		GetPreviewPiece()->Draw(board, previewX, previewY, colorId, 255, false, altTexture);
		GetRotatedPiece()->Draw(board, posX, posY, colorId, 255, true, altTexture);
	}
}

void CPlayer::DrawStatistics() const {
	for (int i = 0; i < aPieceTypes.size(); i++) {
		float x = -4;
		float y = board->extentsY * 0.2 + (i * 2.5);
		int color = i % 3 != 2;
		if (gGameConfig.prideFlags) color = i;

		tNyaStringData string;
		string.x = board->BoardToScreenX(x);
		string.y = board->BoardToScreenY(y);
		string.size = 0.05;
		DrawString(string, std::format("{:03}", pieceCounter[i]).c_str());
		aPieceTypes[i].rotations[0].Draw(board, x - 4.5, y - 2.5, color, 255, false, i % 3 == 0);
	}
}

void CPlayer::Move(int x) {
	recordingReplay->AddEvent(x > 0 ? REPLAY_EVENT_MOVE_RIGHT : REPLAY_EVENT_MOVE_LEFT);

	int bakX = posX;
	posX += x;

	if (!IsPiecePositionValid(posX, posY)) {
		posX = bakX;
	}
	else PlayGameSound(SOUND_MOVE);
}

void CPlayer::Rotate(int num) {
	recordingReplay->AddEvent(num > 0 ? REPLAY_EVENT_ROTATE : REPLAY_EVENT_ROTATE_BACK);

	int bak = pieceRotation;
	pieceRotation += num;
	if (pieceRotation < 0) pieceRotation += GetNumRotations();
	if (pieceRotation >= GetNumRotations()) pieceRotation -= GetNumRotations();

	if (!IsPiecePositionValid(posX, posY)) pieceRotation = bak;
	else PlayGameSound(SOUND_ROTATE);
}

void CPlayer::Place() {
	recordingReplay->AddEvent(REPLAY_EVENT_DROP);
	if (gGameState == STATE_REPLAY_VIEW || gGameState == STATE_REPLAY_PAUSED) {
		drawCurrentAsPreviewNextFrame = true;
	}

	auto piece = GetRotatedPiece();

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			if (piece->data[i][j]) {
				if (posX + i < 0 || posY + j < 0) continue;
				auto at = board->GetPieceAt(posX + i, posY + j);
				at->exists = true;
				at->colorId = colorId;
				at->altTexture = altTexture;
				board->justUpdated = true;
			}
		}
	}

	bool lineClearCheck = board->LineClearCheck();
	NextPiece(!lineClearCheck);
	if (!lineClearCheck) PlayGameSound(SOUND_LOCK);
}

void CPlayer::ProcessDAS() {
	if (pad->aControls[CController::CONTROL_LEFT].IsPressed() || pad->aControls[CController::CONTROL_LEFT_ALT].IsPressed()) dasLeftTimer += gGameTimer.fDeltaTime;
	else dasLeftTimer = 0;
	if (pad->aControls[CController::CONTROL_RIGHT].IsPressed() || pad->aControls[CController::CONTROL_RIGHT_ALT].IsPressed()) dasRightTimer += gGameTimer.fDeltaTime;
	else dasRightTimer = 0;

	if (dasLeftTimer >= NESFramesToSeconds(16))
	{
		dasLeftMovingTimer -= gGameTimer.fDeltaTime;
		if (dasLeftMovingTimer < 0)
		{
			dasLeftMovingTimer += NESFramesToSeconds(6);
			Move(-1);
		}
	}
	else dasLeftMovingTimer = 0;

	if (dasRightTimer >= NESFramesToSeconds(16))
	{
		dasRightMovingTimer -= gGameTimer.fDeltaTime;
		if (dasRightMovingTimer < 0)
		{
			dasRightMovingTimer += NESFramesToSeconds(6);
			Move(1);
		}
	}
	else dasRightMovingTimer = 0;
}

void CPlayer::ProcessControls() {
	if (pad->aControls[CController::CONTROL_LEFT].IsJustPressed()) Move(-1);
	if (pad->aControls[CController::CONTROL_LEFT_ALT].IsJustPressed()) Move(-1);
	if (pad->aControls[CController::CONTROL_RIGHT].IsJustPressed()) Move(1);
	if (pad->aControls[CController::CONTROL_RIGHT_ALT].IsJustPressed()) Move(1);
	if (pad->aControls[CController::CONTROL_ROTATE].IsJustPressed()) Rotate(1);
	if (pad->aControls[CController::CONTROL_ROTATE_ALT].IsJustPressed()) Rotate(1);
	if (pad->aControls[CController::CONTROL_ROTATE_BACK].IsJustPressed()) Rotate(-1);
	if (pad->aControls[CController::CONTROL_ROTATE_BACK_ALT].IsJustPressed()) Rotate(-1);

	ProcessDAS();
}

void CPlayer::ReInit(bool reInitRNG) {
	if (recordingReplay) delete recordingReplay;
	recordingReplay = new CReplay(this);
	if (reInitRNG) ResetPieceRandomizer();
	if (!pieceAlreadyPicked) pieceAlreadyPicked = new bool[aPieceTypes.size()];
	memset(pieceAlreadyPicked, 0, aPieceTypes.size());
	if (!pieceCounter) pieceCounter = new int[aPieceTypes.size()];
	pieceType = 0;
	nextPieceType = 0;
	NextPiece();
	memset(pieceCounter, 0, aPieceTypes.size() * sizeof(int));
	NextPiece();
	initialGravity = true;
	gameOver = false;
	drawCurrentAsPreviewNextFrame = false;
	strcpy_s(playerName, 24, gGameConfig.playerName[playerId]);
	dropPreviewOn = gGameConfig.dropPreviewOn[playerId];
}

bool CPlayer::MoveDown() {
	recordingReplay->AddEvent(REPLAY_EVENT_MOVE_DOWN);

	posY++;
	if (!IsPiecePositionValid(posX, posY)) {
		posY--;
		return false;
	}
	return true;
}

void CPlayer::Process() {
	if (aPlayers.size() > 1 || isRemotePlayer) {
		tNyaStringData string;
		string.x = board->BoardToScreenX((GetBoardXMin() + GetBoardXMax()) * 0.5);
		string.y = board->BoardToScreenY(-2.75);
		string.size = 0.05;
		string.XCenterAlign = true;
		DrawString(string, playerName);
	}
	else {
		DrawStatistics();
	}

	if (gameOver) {
		// coop revival mechanic :3
		if (board->isCoopBoard && board->AreLinesEmpty(0, board->extentsY * 0.5)) {
			gameOver = false;
			PlayGameSound(SOUND_TETRIS);
		}
		else {
			tNyaStringData string;
			string.x = board->BoardToScreenX((GetBoardXMin() + GetBoardXMax()) * 0.5);
			string.y = board->BoardToScreenY(board->extentsY * 0.5);
			string.size = 0.066;
			string.XCenterAlign = true;
			string.topLevel = true;
			string.SetColor(255, 0, 0, 255);
			string.SetOutlineColor(0,0,0,255);
			DrawString(string, "GAME OVER");
			return;
		}
	}

	if (gGameState == STATE_REPLAY_VIEW || gGameState == STATE_REPLAY_PAUSED || isRemotePlayer) {
		Draw();
		drawCurrentAsPreviewNextFrame = false;
		return;
	}
	else {
		recordingReplay->timer += gGameTimer.fDeltaTime;
	}

	if (!board->AllowPlayerControl()) {
		if (gGameState == STATE_PLAYING) dropWaitTimer = 0;
		Draw();
		return;
	}

	if (dropWaitTimer > 0) {
		Draw();
		dropWaitTimer -= gGameTimer.fDeltaTime;
		if (dropWaitTimer <= 0) {
			Place();
		}
		return;
	}

	ProcessControls();

	float gravity = GetGravity();

	if (pad->aControls[CController::CONTROL_DOWN].IsPressed() || pad->aControls[CController::CONTROL_DOWN_ALT].IsPressed()) {
		if (!downReleaseRequired) {
			if (gravity > 1.0 / 30.05) gravity = 1.0 / 30.05;
			if (gravityTimer > gravity) gravityTimer = gravity;
		}
	}
	else downReleaseRequired = false;

	gravityTimer += gGameTimer.fDeltaTime;
	while (gravityTimer >= gravity) {
		gravityTimer -= gravity;
		initialGravity = false;
		if (!MoveDown()) {
			dropWaitTimer = NESFramesToSeconds(10);
		}
	}

	Draw();
}
CPlayer gPlayer1 = CPlayer(0, &gPlayer1Board);
CPlayer gPlayer2 = CPlayer(1, &gPlayer2Board);
CPlayer gPlayer3 = CPlayer(2, &gPlayer3Board);
CPlayer gPlayer4 = CPlayer(3, &gPlayer4Board);
CPlayer gCoopPlayer1 = CPlayer(0, &gCoopTeam1Board);
CPlayer gCoopPlayer2 = CPlayer(1, &gCoopTeam1Board);
CPlayer gCoopPlayer3 = CPlayer(2, &gCoopTeam2Board);
CPlayer gCoopPlayer4 = CPlayer(3, &gCoopTeam2Board);
CPlayer gMPSpectatePlayer = CPlayer(0, &gMPSpectateBoard);

CPlayer* GetMPSpectatePlayer() {
	return &gMPSpectatePlayer;
}

std::vector<CPlayer*> aPlayers;
std::vector<CBoard*> aBoards;

void ResetAllPlayers() {
	aPlayers.clear();
	aBoards.clear();

	gPlayer1Board.ReInitSettings();
	gPlayer2Board.ReInitSettings();
	gPlayer3Board.ReInitSettings();
	gPlayer4Board.ReInitSettings();
	gCoopTeam1Board.ReInitSettings();
	gCoopTeam2Board.ReInitSettings();

	switch (gGameConfig.gameType) {
		case tGameConfig::TYPE_1PLAYER:
			gPlayer1Board.posX = 0.5;
			aPlayers.push_back(&gPlayer1);
			break;
		case tGameConfig::TYPE_2PLAYER_COOP:
			gCoopTeam1Board.posX = 0.5;
			aPlayers.push_back(&gCoopPlayer1);
			aPlayers.push_back(&gCoopPlayer2);
			break;
		case tGameConfig::TYPE_2PLAYER_VS:
			gPlayer1Board.posX = 0.33;
			gPlayer1Board.areStatsOnLeft = true;
			gPlayer2Board.posX = 0.66;
			aPlayers.push_back(&gPlayer1);
			aPlayers.push_back(&gPlayer2);
			break;
		case tGameConfig::TYPE_4PLAYER_VS:
			gPlayer1Board.posX = 0.14;
			gPlayer2Board.posX = 0.38;
			gPlayer3Board.posX = 0.62;
			gPlayer4Board.posX = 0.86;
			gPlayer1Board.areStatsOnBottom = true;
			gPlayer2Board.areStatsOnBottom = true;
			gPlayer3Board.areStatsOnBottom = true;
			gPlayer4Board.areStatsOnBottom = true;
			aPlayers.push_back(&gPlayer1);
			aPlayers.push_back(&gPlayer2);
			aPlayers.push_back(&gPlayer3);
			aPlayers.push_back(&gPlayer4);
			break;
		case tGameConfig::TYPE_3PLAYER_2V1COOPVS:
			gCoopTeam1Board.posX = 0.33;
			gPlayer3Board.posX = 0.8;
			gCoopTeam1Board.areStatsOnBottom = true;
			gPlayer3Board.areStatsOnBottom = true;
			aPlayers.push_back(&gCoopPlayer1);
			aPlayers.push_back(&gCoopPlayer2);
			aPlayers.push_back(&gPlayer3);
			break;
		case tGameConfig::TYPE_4PLAYER_COOPVS:
			gCoopTeam1Board.posX = 0.25;
			gCoopTeam1Board.areStatsOnLeft = true;
			gCoopTeam2Board.posX = 0.75;
			gCoopTeam1Board.areStatsOnBottom = true;
			gCoopTeam2Board.areStatsOnBottom = true;
			aPlayers.push_back(&gCoopPlayer1);
			aPlayers.push_back(&gCoopPlayer2);
			aPlayers.push_back(&gCoopPlayer3);
			aPlayers.push_back(&gCoopPlayer4);
			break;
		default:
			break;
	}

	for (auto& ply : aPlayers) {
		if (std::find(aBoards.begin(), aBoards.end(), ply->board) == aBoards.end()) {
			aBoards.push_back(ply->board);
		}
	}

	// init boards then players, the board needs to be empty first before players can spawn!
	for (auto& board : aBoards) {
		board->ReInit();
	}
	for (auto& ply : aPlayers) {
		ply->ReInit();
	}
}