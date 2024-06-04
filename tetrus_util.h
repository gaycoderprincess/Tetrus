#ifndef TETRUS_UTIL_H
#define TETRUS_UTIL_H

int GetRandom(int max);
double NESFramesToSeconds(double time);
NyaDrawing::CNyaRGBA32 NESPaletteToRGB(int paletteId);
const char* GetLevelName(int level);
const char* GetLineClearCountName(int count);
void DrawBackgroundImage(float left, float right);
void LogString(const std::string& str);

extern CNyaTimer gGameTimer;
#endif