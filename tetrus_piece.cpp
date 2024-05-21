#include "nya_dx11_appbase.h"

#include "tetrus_board.h"
#include "tetrus_piece.h"

tPieceType::tRotation::tRotation() {
	memset(data, 0, sizeof(data));
}

bool tPieceType::tRotation::IsEmpty() {
	auto tmp = tRotation();
	return !memcmp(this, &tmp, sizeof(*this));
}

void tPieceType::tRotation::Draw(CBoard* board, float x, float y, int colorId, int alpha, bool capYMin, bool altTexture) {
	auto rgb = board->GetColor(colorId);
	auto textureRgb = board->GetTextureColor();
	rgb.a = alpha;
	textureRgb.a = alpha;
	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			if (y + j < 0 && capYMin) continue;
			if (data[i][j]) DrawBlock(board->BoardToScreenX(x + i), board->BoardToScreenY(y + j), board->blockScale, rgb, textureRgb, altTexture);
		}
	}
}

std::vector<tPieceType> aPieceTypes;