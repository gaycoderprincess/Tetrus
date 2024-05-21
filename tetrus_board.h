const int gBoardSizeX = 10;
const int gBoardSizeY = 20;

void DrawBlock(float x, float y, float scale, NyaDrawing::CNyaRGBA32 rgb, NyaDrawing::CNyaRGBA32 textureRgb, bool useAltTexture);

class CBoard {
public:
	int extentsX;
	int extentsY;
	float blockScale;
	float posX;
	float posY;
	struct tPlacedPiece {
		bool exists : 1;
		bool altTexture : 1;
		unsigned int colorId : 4;
	};
	tPlacedPiece* data;
	enum eState {
		STATE_IDLE,
		STATE_LINE_CLEARING
	} state;
	double lineClearTimer;
	int linesCleared;
	int lineClearCombo;
	int level;
	int score;
	bool isCoopBoard;
	bool areStatsOnLeft;
	bool areStatsOnBottom;
	bool justUpdated;

	CBoard(int sizeX, int sizeY);
	tPlacedPiece* GetPieceAt(int x, int y);
	void ReInit();
	void ReInitSettings();
	[[nodiscard]] float BoardToScreenX(float x) const;
	[[nodiscard]] float BoardToScreenY(float x) const;
	bool IsLineFull(int y);
	bool IsLineEmpty(int y);
	bool AreLinesEmpty(int yStart, int yEnd);
	void DrawBackground() const;
	void Draw();
	int LineClearCheck();
	void MoveLine(int yFrom, int yTo);
	void ClearLine(int y);
	void ClearFullLines();
	[[nodiscard]] bool AllowPlayerControl() const;
	[[nodiscard]] NyaDrawing::CNyaRGBA32 GetColor(int id) const;
	[[nodiscard]] NyaDrawing::CNyaRGBA32 GetTextureColor() const;
	void Process();
};
extern CBoard gPlayer1Board;
extern CBoard gPlayer2Board;
extern CBoard gPlayer3Board;
extern CBoard gPlayer4Board;
extern CBoard gCoopTeam1Board;
extern CBoard gCoopTeam2Board;