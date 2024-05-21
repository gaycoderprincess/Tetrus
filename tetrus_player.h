#include <random>

class CReplay;

class CPlayer {
public:
	int posX = 0;
	int posY = 0;
	int pieceType = 0;
	int pieceRotation = 0;
	int nextPieceType = 0;
	bool* pieceAlreadyPicked;
	int* pieceCounter;
	double gravityTimer = 0;
	double dropWaitTimer = 0;
	double dasLeftTimer = 0;
	double dasRightTimer = 0;
	double dasLeftMovingTimer = 0;
	double dasRightMovingTimer = 0;
	unsigned int colorId : 4;
	unsigned int colorIdLast : 4;
	bool altTexture : 1;
	bool altTextureLast : 1;
	bool downReleaseRequired;
	bool gameOver;
	bool initialGravity;
	CBoard* board;
	int playerId;
	CController* pad;
	int pieceRNGSeed;
	std::mt19937 pieceRNG;
	CReplay* recordingReplay;
	bool drawCurrentAsPreviewNextFrame;

	CPlayer(int playerId, CBoard* board);
	[[nodiscard]] double GetGravity() const;
	[[nodiscard]] tPieceType::tRotation* GetRotatedPiece() const;
	[[nodiscard]] tPieceType::tRotation* GetPreviewPiece() const;
	[[nodiscard]] tPieceType* GetPieceType() const;
	[[nodiscard]] tPieceType* GetPreviewPieceType() const;
	[[nodiscard]] int GetNumRotations() const;
	[[nodiscard]] bool IsRightSideCoopPlayer() const;
	[[nodiscard]] int GetBoardXMin() const;
	[[nodiscard]] int GetBoardXMax() const;
	[[nodiscard]] bool IsPieceWithinExtents(int x, int y) const;
	[[nodiscard]] bool IsPiecePositionValid(int x, int y) const;
	[[nodiscard]] int GetPieceRandomizerSeed() const;
	int GetRandomPieceID(int max);
	void SetPieceRandomizer(int seed);
	void ResetPieceRandomizer();
	int GenerateNextPieceType();
	void NextPiece(bool checkGameOver = true);
	void DrawDropPreview() const;
	void DrawStatistics() const;
	void Draw() const;
	void Move(int x);
	bool MoveDown();
	void Rotate(int num);
	void Place();
	void ProcessDAS();
	void ProcessControls();
	void ReInit(bool reInitRNG = true);
	void Process();
};

extern std::vector<CPlayer*> aPlayers;
extern std::vector<CBoard*> aBoards;

void ResetAllPlayers();