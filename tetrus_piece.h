#ifndef TETRUS_PIECE_H
#define TETRUS_PIECE_H

class CBoard;

struct tPieceType {
	struct tRotation {
		bool data[5][5];

		tRotation();
		bool IsEmpty();
		void Draw(CBoard* board, float x, float y, int colorId, int alpha, bool capYMin, bool altTexture);
	};

	std::vector<tRotation> rotations;
};
extern std::vector<tPieceType> aPieceTypes;
#endif