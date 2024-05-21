#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_audio.h"
#include "tetrus_board.h"

void DrawBlock(float x, float y, float scale, NyaDrawing::CNyaRGBA32 rgb, NyaDrawing::CNyaRGBA32 textureRgb, bool useAltTexture) {
	if (rgb.a <= 0) return;

	float scaleY = scale * 0.5;
	float scaleX = scaleY * GetAspectRatioInv();
	static ID3D11ShaderResourceView* textures[] = { LoadTexture("block1.png"), LoadTexture("block2.png"), LoadTexture("block3.png") };
	auto texture = textures[gGameConfig.blockTexture];
	if (gGameConfig.blockTexture == tGameConfig::SPRITE_CLASSIC && useAltTexture) {
		static auto altTexture = LoadTexture("block2_alt.png");
		if (altTexture) texture = altTexture;
	}
	DrawRectangle(x, x + scaleX * 2, y, y + scaleY * 2, rgb);
	DrawRectangle(x, x + scaleX * 2, y, y + scaleY * 2, {textureRgb.r, textureRgb.g, textureRgb.b, rgb.a}, 0, texture);
}

CBoard::CBoard(int sizeX, int sizeY) {
	extentsX = sizeX;
	extentsY = sizeY;
	posX = 0.5;
	posY = 0.5;
	blockScale = 0.035;
	state = STATE_IDLE;
	lineClearTimer = 0;
	linesCleared = 0;
	lineClearCombo = 0;
	level = gGameConfig.GetStartingLevel();
	score = 0;
	data = new tPlacedPiece[extentsX * extentsY];
	isCoopBoard = sizeX == gBoardSizeX * 2;
	areStatsOnLeft = false;
	areStatsOnBottom = false;
	justUpdated = false;
}

CBoard::tPlacedPiece* CBoard::GetPieceAt(int x, int y) {
	if (x < 0 || x >= extentsX) return nullptr;
	if (y < 0 || y >= extentsY) return nullptr;

	return &data[(x * extentsY) + y];
}

void CBoard::ReInit() {
	memset(data, 0, sizeof(tPlacedPiece) * extentsX * extentsY);
	state = STATE_IDLE;
	lineClearTimer = 0;
	linesCleared = 0;
	lineClearCombo = 0;
	level = gGameConfig.GetStartingLevel();
	score = 0;
}

void CBoard::ReInitSettings() {
	areStatsOnLeft = false;
	areStatsOnBottom = false;
}

float CBoard::BoardToScreenX(float x) const {
	float l = posX - (blockScale * (extentsX * 0.5) * GetAspectRatioInv());
	float r = posX + (blockScale * (extentsX * 0.5) * GetAspectRatioInv());
	return std::lerp(l, r, x / (double)extentsX);
}

float CBoard::BoardToScreenY(float x) const {
	float t = posY - (blockScale * (extentsY * 0.5));
	float b = posY + (blockScale * (extentsY * 0.5));
	return std::lerp(t, b, x / (double)extentsY);
}

bool CBoard::IsLineFull(int y) {
	int numFilled = 0;
	for (int i = 0; i < extentsX; i++) {
		if (GetPieceAt(i, y)->exists > 0) numFilled++;
	}
	return numFilled >= extentsX;
}

bool CBoard::IsLineEmpty(int y) {
	for (int i = 0; i < extentsX; i++) {
		if (GetPieceAt(i, y)->exists > 0) return false;
	}
	return true;
}

bool CBoard::AreLinesEmpty(int yStart, int yEnd) {
	for (int i = yStart; i < yEnd; i++) {
		if (!IsLineEmpty(i)) return false;
	}
	return true;
}

void CBoard::DrawBackground() const {
	if (gGameConfig.boardBorders) {
		float borderSize = 0.33;
		DrawRectangle(BoardToScreenX(-borderSize), BoardToScreenX(extentsX + borderSize), BoardToScreenY(-borderSize), BoardToScreenY(extentsY + borderSize), {255, 255, 255,255});
	}

	DrawRectangle(BoardToScreenX(0), BoardToScreenX(extentsX), BoardToScreenY(0), BoardToScreenY(extentsY), {0,0,0,255});
}

void CBoard::Draw() {
	for (int x = 0; x < extentsX; x++) {
		for (int y = 0; y < extentsY; y++) {
			if (state == STATE_LINE_CLEARING && IsLineFull(y)) {
				int distFromMiddle = abs((extentsX * 0.5) - x);
				if (distFromMiddle <= lineClearTimer / NESFramesToSeconds(4)) continue;
			}

			auto piece = GetPieceAt(x, y);
			if (!piece->exists) continue;
			DrawBlock(BoardToScreenX(x), BoardToScreenY(y), blockScale, GetColor(piece->colorId), GetTextureColor(), piece->altTexture);
		}
	}

	// coop separator
	if (!gGameConfig.boardBorders && isCoopBoard) {
		DrawBackgroundImage(BoardToScreenX(extentsX * 0.5 - 0.1), BoardToScreenX(extentsX * 0.5 + 0.1));
	}
	if (gGameConfig.boardBorders && isCoopBoard) {
		DrawRectangle(BoardToScreenX(extentsX * 0.5 - 0.1), BoardToScreenX(extentsX * 0.5 + 0.1), BoardToScreenY(0), BoardToScreenY(extentsY), {255,255,255,255});
	}
}

int CBoard::LineClearCheck() {
	if (!justUpdated) return 0;

	int numCleared = 0;

	// mark lines
	for (int y = 0; y < extentsY; y++) {
		if (IsLineFull(y)) numCleared++;
	}

	return numCleared;
}

void CBoard::MoveLine(int yFrom, int yTo) {
	if (yFrom < 0) return;
	for (int i = 0; i < extentsX; i++) {
		*GetPieceAt(i, yTo) = *GetPieceAt(i, yFrom);
	}
}

void CBoard::ClearLine(int y) {
	for (int i = 0; i < extentsX; i++) {
		GetPieceAt(i, y)->exists = false;
		GetPieceAt(i, 0)->exists = false; // always clear the topmost one
	}

	for (int i = y; i >= 0; i--) {
		MoveLine(i - 1, i);
	}

	linesCleared++;
	lineClearCombo++;

	if (linesCleared % 10 == 0) level++;
}

void CBoard::ClearFullLines() {
	for (int y = 0; y < extentsY; y++) {
		if (IsLineFull(y)) {
			ClearLine(y);
			return ClearFullLines();
		}
	}

	int scoreAdd = 0;
	switch (lineClearCombo) {
		case 1:
			scoreAdd = 40;
			break;
		case 2:
			scoreAdd = 100;
			break;
		case 3:
			scoreAdd = 300;
			break;
		case 4:
			scoreAdd = 1200;
			break;
		default:
			break;
	}
	score += (level + 1) * scoreAdd;
	lineClearCombo = 0;
}

NyaDrawing::CNyaRGBA32 CBoard::GetColor(int id) const {
	if (gGameConfig.prideFlags) {
		auto colors = tGameConfig::GetPrideFlagColor(gGameConfig.prideFlagColors[level % 10]);
		return colors[id % colors.size()];
	}

	int wrappedLevel = level;
	if (wrappedLevel < 138) wrappedLevel %= sizeof(gGameConfig.pieceColors) / sizeof(gGameConfig.pieceColors[0]);
	return NESPaletteToRGB(gGameConfig.pieceColors[wrappedLevel].paletteId[id]);
}

NyaDrawing::CNyaRGBA32 CBoard::GetTextureColor() const {
	if (gGameConfig.prideFlags) return {255,255,255,255};

	int wrappedLevel = level;
	if (wrappedLevel < 138) wrappedLevel %= sizeof(gGameConfig.pieceColors) / sizeof(gGameConfig.pieceColors[0]);
	return NESPaletteToRGB(gGameConfig.pieceColors[wrappedLevel].backgroundPaletteId[1]);
}

bool CBoard::AllowPlayerControl() const {
	return state != STATE_LINE_CLEARING && gGameState == STATE_PLAYING;
}

void CBoard::Process() {
	if (gGameState != STATE_PAUSED && gGameState != STATE_REPLAY_PAUSED) {
		if (state == STATE_LINE_CLEARING) {
			lineClearTimer += gGameTimer.fDeltaTime;
			if (isCoopBoard) lineClearTimer += gGameTimer.fDeltaTime; // clear 2x faster in coop since the board is wider
			if (lineClearTimer >= (extentsX * 0.5 + 1) * NESFramesToSeconds(4)) {
				state = STATE_IDLE;
				lineClearTimer = 0;
				ClearFullLines();
			}
		} else {
			if (int num = LineClearCheck()) {
				state = STATE_LINE_CLEARING;
				PlayGameSound(num >= 4 ? SOUND_TETRIS : SOUND_LINE);
			}
		}
	}

	Draw();

	tNyaStringData string;
	if (areStatsOnBottom) {
		string.x = BoardToScreenX(2.5);
		string.y = BoardToScreenY(extentsY + 1);
	}
	else {
		string.x = areStatsOnLeft ? BoardToScreenX(-5) : BoardToScreenX(extentsX + 5);
		string.y = BoardToScreenY(extentsY * 0.5 - 2);
	}
	string.size = 0.05;
	string.XCenterAlign = true;
	auto levelString = std::format("Level {}", GetLevelName(level));
	auto scoreString = std::to_string(score);
	{
		float bgWidth = std::max(GetStringWidth(string.size, scoreString.c_str()), GetStringWidth(string.size, levelString.c_str())) * 0.5 + 0.01 * GetAspectRatioInv();
		float bgMin = GetStringHeight(string.size, levelString.c_str()) * 0.5 + 0.0025;
		float bgMax = GetStringHeight(string.size, levelString.c_str()) * 0.5 + 0.04 + 0.0025;
		DrawRectangle(string.x - bgWidth, string.x + bgWidth, string.y - bgMin, string.y + bgMax, {0, 0, 0, 127},
					  0.01);
	}
	DrawString(string, levelString.c_str());
	string.y += 0.04;
	DrawString(string, scoreString.c_str());
	string.x = BoardToScreenX(extentsX * 0.5);
	string.y = BoardToScreenY(gGameConfig.boardBorders ? -1.33 : -1);
	auto linesString = std::format("Lines - {}", GetLineClearCountName(linesCleared));
	{
		float bgWidth = GetStringWidth(string.size, linesString.c_str()) * 0.5 + 0.01 * GetAspectRatioInv();
		float bgHeight = GetStringHeight(string.size, linesString.c_str()) * 0.5 + 0.005;
		DrawRectangle(string.x - bgWidth, string.x + bgWidth, string.y - bgHeight, string.y + bgHeight, {0, 0, 0, 127},
					  0.01);
	}
	DrawString(string, linesString.c_str());

	if (gGameState == STATE_REPLAY_VIEW || gGameState == STATE_REPLAY_PAUSED) {
		string.x = BoardToScreenX(extentsX * 0.5);
		string.y = BoardToScreenY(extentsY + 2);
		DrawString(string, "Q - Quit to Main Menu");
	}

	justUpdated = false;
}
CBoard gPlayer1Board(gBoardSizeX, gBoardSizeY);
CBoard gPlayer2Board(gBoardSizeX, gBoardSizeY);
CBoard gPlayer3Board(gBoardSizeX, gBoardSizeY);
CBoard gPlayer4Board(gBoardSizeX, gBoardSizeY);
CBoard gCoopTeam1Board(gBoardSizeX * 2, gBoardSizeY);
CBoard gCoopTeam2Board(gBoardSizeX * 2, gBoardSizeY);