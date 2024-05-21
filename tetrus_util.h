int GetRandom(int max);
double NESFramesToSeconds(double time);
NyaDrawing::CNyaRGBA32 NESPaletteToRGB(int paletteId);
const char* GetLevelName(int level);
const char* GetLineClearCountName(int count);
void DrawBackgroundImage(float left, float right);
extern CNyaTimer gGameTimer;