#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_audio.h"
#include "tetrus_board.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_menu.h"
#include "tetrus_scoreboard.h"
#include "tetrus_net.h"

bool bOnlinePauseMenu = false;

float fMenuSize;
float fMenuTop;
float fForceMenuWidth = 0.0;
int nDrawPiecePreviews = -1;

bool bBlockMenuBack = false;
bool bBlockMenuSelect = false;
bool bIsInInputMenu = false;

struct tMenuState {
	int nSelectedOption[8] = { 0 };
	int nNumOptions[8] = { 0 };
	int nLevel = 0;
	int nTargetLevel = 0;
};
tMenuState mainMenuState;
tMenuState onlinePauseMenuState;
tMenuState* pMenu = &mainMenuState;

enum eMenuFlags {
	MENU_FLAG_SHOW_ABOVE = 1 << 0,
	MENU_FLAG_EDIT_VALUE = 1 << 1,
	MENU_FLAG_BLOCK_ABOVE = 1 << 2,
};

class tmpMenuClass {
public:
	bool isSelected = false;
	bool isHovered = false;
	uint32_t flags = 0;

	tmpMenuClass() {
		pMenu->nTargetLevel++;
	}
	~tmpMenuClass() {
		pMenu->nTargetLevel--;
	}
	explicit operator int() const {
		if (flags & MENU_FLAG_EDIT_VALUE && isHovered) {
			int editCount = 0;
			if (GetMenuLeft()) editCount--;
			if (GetMenuRight()) editCount++;
			if (editCount != 0) PlayGameSound(SOUND_MENUMOVE);
			if (isSelected) editCount++;
			return editCount;
		}
		return isSelected;
	}
	explicit operator bool() const {
		return (int)*this != 0;
	}
};

struct tMenuOption {
	std::string str;
	int id;
	bool selected;
	int level;
	uint32_t flags;

	[[nodiscard]] bool ShouldDraw() const {
		if (str.empty()) return false;
		if (level != pMenu->nLevel && !(flags & MENU_FLAG_SHOW_ABOVE)) return false;
		if (level > pMenu->nLevel) return false;
		return true;
	}

	[[nodiscard]] double GetTextWidth() const {
		return GetStringWidth(fMenuSize, str.c_str());
	}

	void Draw() const {
		if (!ShouldDraw()) return;

		tNyaStringData string;
		string.x = 0.5;
		string.y = fMenuTop + fMenuSize * id;
		string.size = fMenuSize;
		string.XCenterAlign = true;
		NyaDrawing::CNyaRGBA32 col = {255, 255, 255, 255};
		if (selected) col = {245, 171, 185, 255};
		string.SetColor(&col);
		if (selected) DrawString(string, ("< " + str + " >").c_str());
		else DrawString(string, str.c_str());
	}
};
std::vector<tMenuOption> aMenuOptionsToDraw;

tmpMenuClass DrawMenuOption(const char* str, uint32_t flags = 0) {
	bool bSelected = pMenu->nSelectedOption[pMenu->nTargetLevel] == pMenu->nNumOptions[pMenu->nTargetLevel];

	tMenuOption option;
	option.str = str;
	option.id = pMenu->nNumOptions[pMenu->nTargetLevel];
	option.selected = bSelected;
	option.level = pMenu->nTargetLevel;
	option.flags = flags;
	aMenuOptionsToDraw.push_back(option);

	pMenu->nNumOptions[pMenu->nTargetLevel]++;
	if (flags & MENU_FLAG_SHOW_ABOVE) {
		for (int i = pMenu->nTargetLevel + 1; i < 8; i++) {
			pMenu->nNumOptions[i]++;
		}
	}

	auto retValue = tmpMenuClass();
	retValue.flags = flags;
	retValue.isHovered = bSelected && pMenu->nLevel >= pMenu->nTargetLevel - 1;
	retValue.isSelected = bSelected && pMenu->nLevel >= pMenu->nTargetLevel;
	if (retValue.isSelected && (flags & MENU_FLAG_BLOCK_ABOVE || flags & MENU_FLAG_EDIT_VALUE)) pMenu->nLevel--;
	return retValue;
}

bool KeybindCheck(CController::tControl* out) {
	for (int i = 0; i < NUM_NYA_PAD_KEYS; i++) {
		if (i == NYA_PAD_KEY_START) continue;
		for (int j = 0; j < XUSER_MAX_COUNT; j++) {
			if (IsPadKeyJustPressed(i, j)) {
				out->keyId = i;
				out->controller = j;
				return true;
			}
		}
	}
	for (int i = 0; i < 255; i++) {
		if (i == VK_SHIFT) continue;
		if (i == VK_CONTROL) continue;
		if (i == VK_MENU) continue;
		if (i == VK_RETURN) continue;
		if (i == VK_ESCAPE) continue;
		if (IsKeyJustPressed(i)) {
			out->controller = -1;
			out->keyId = i;
			return true;
		}
	}
	return false;
}

void ProcessColorPaletteMenu() {
	for (int i = 0; i < 10; i++) {
		if (gGameConfig.prideFlags) {
			gGameConfig.prideFlagColors[i] += (int)DrawMenuOption(std::format("Level {} - {}", GetLevelName(i), tGameConfig::GetPrideFlagTypeName(gGameConfig.prideFlagColors[i])).c_str(), MENU_FLAG_EDIT_VALUE);
			if (gGameConfig.prideFlagColors[i] < 0) gGameConfig.prideFlagColors[i] = tGameConfig::NUM_PRIDE_FLAG_TYPES - 1;
			if (gGameConfig.prideFlagColors[i] >= tGameConfig::NUM_PRIDE_FLAG_TYPES) gGameConfig.prideFlagColors[i] = 0;
		}
		else if (auto tmp = DrawMenuOption(std::format("Level {}", GetLevelName(i)).c_str())) {
			fForceMenuWidth = 0.45 * GetAspectRatioInv();
			nDrawPiecePreviews = i;

			auto &colors = gGameConfig.pieceColors[i];
			for (int j = 0; j < 2; j++) {
				colors.paletteId[j] += (int)DrawMenuOption(" ", MENU_FLAG_EDIT_VALUE);
				if (colors.paletteId[j] < 0) colors.paletteId[j] = 0x3F;
				if (colors.paletteId[j] > 0x3F) colors.paletteId[j] = 0;
			}
			if (DrawMenuOption("Default", MENU_FLAG_BLOCK_ABOVE)) {
				colors = tGameConfig::GetDefaultColorSetup(i);
			}
		}
	}
}

void ProcessPlayerSettingsMenu(int i) {
	if (DrawMenuOption(std::format("Ghost Piece - {}", gGameConfig.dropPreviewOn[i] ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.dropPreviewOn[i] = !gGameConfig.dropPreviewOn[i];

		if (NyaNet::IsConnected() && !aPlayers.empty()) aPlayers[0]->dropPreviewOn = gGameConfig.dropPreviewOn[0];
	}
	if (auto tmp = DrawMenuOption(std::format("Name - {}", ((std::string)gGameConfig.playerName[i] + (bIsInInputMenu ? "..." : ""))).c_str(), MENU_FLAG_BLOCK_ABOVE)) {
		bIsInInputMenu = true;
	}
	if (auto tmp = DrawMenuOption("Controls")) {
		auto player = &aPlayerControls[i];

		for (int j = 0; j < CController::NUM_CONTROLS; j++) {
			if (auto tmp = DrawMenuOption((CController::GetControlName(j) + (std::string) " - " +
										   (pMenu->nSelectedOption[pMenu->nTargetLevel] == pMenu->nNumOptions[pMenu->nTargetLevel] && pMenu->nLevel > pMenu->nTargetLevel ? "..." : player->GetKeyName(j))).c_str(), true)) {
				bBlockMenuBack = true;
				bBlockMenuSelect = true;

				auto control = &player->aControls[j];
				if (IsKeyJustPressed(VK_ESCAPE)) {
					control->controller = -1;
					control->keyId = 0;
					pMenu->nLevel--;
				}
				else {
					if (KeybindCheck(control)) {
						pMenu->nLevel--;
					}
				}
			}
		}
		player->rumblePad += (int)DrawMenuOption(std::format("Rumble - {}", player->rumblePad >= 0 ? ("Pad " + std::to_string(player->rumblePad + 1)) : "Off").c_str(), MENU_FLAG_EDIT_VALUE | MENU_FLAG_SHOW_ABOVE);
		if (player->rumblePad < -1) player->rumblePad = XUSER_MAX_COUNT - 1;
		if (player->rumblePad >= XUSER_MAX_COUNT) player->rumblePad = -1;
		if (DrawMenuOption("Back", MENU_FLAG_SHOW_ABOVE)) {
			pMenu->nLevel -= 2;
		}
	}

	if (bIsInInputMenu) {
		std::string name = gGameConfig.playerName[i];
		if (name.length() < 23) name += sKeyboardInput;
		if (IsKeyJustPressed(VK_BACK) && !name.empty()) name.pop_back();
		strcpy_s(gGameConfig.playerName[i], name.c_str());
		if (GetMenuAcceptInput()) {
			if (name == "transrights") {
				gGameConfig.prideFlagsOptionVisible = true;
				PlayGameSound(SOUND_TETRIS);
			}
		}
	}
}

void ProcessPlayerSettingsMenu() {
	if (NyaNet::IsConnected()) {
		ProcessPlayerSettingsMenu(0);
	}
	else for (int i = 0; i < 4; i++) {
		if (auto tmp = DrawMenuOption(std::format("Player {}", i + 1).c_str())) {
			ProcessPlayerSettingsMenu(i);
			if (DrawMenuOption("Back")) {
				pMenu->nLevel -= 2;
			}
		}
	}
}

void ProcessOptionsMenu() {
	if (DrawMenuOption(std::format("Sound Effects - {}", gGameConfig.sfxOn ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.sfxOn = !gGameConfig.sfxOn;
	}
	if (DrawMenuOption(std::format("Overlapping Sounds - {}", gGameConfig.overlappingSounds ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.overlappingSounds = !gGameConfig.overlappingSounds;
	}
	if (DrawMenuOption(std::format("V-Sync - {}", gGameConfig.vsync ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.vsync = !gGameConfig.vsync;
	}
	if (!NyaNet::IsConnected() && DrawMenuOption(std::format("Same RNG for All Players - {}", gGameConfig.randSameForAllPlayers ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.randSameForAllPlayers = !gGameConfig.randSameForAllPlayers;
	}
	if (auto tmp = DrawMenuOption("Visual Settings")) {
		gGameConfig.blockTexture += (int)DrawMenuOption(std::format("Block Sprite - {}", gGameConfig.GetBlockTextureName()).c_str(), MENU_FLAG_EDIT_VALUE);
		if (gGameConfig.blockTexture < 0) gGameConfig.blockTexture = tGameConfig::NUM_SPRITE_TYPES - 1;
		if (gGameConfig.blockTexture >= tGameConfig::NUM_SPRITE_TYPES) gGameConfig.blockTexture = tGameConfig::SPRITE_STANDARD;
		if (DrawMenuOption(std::format("Board Borders - {}", gGameConfig.boardBorders ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
			gGameConfig.boardBorders = !gGameConfig.boardBorders;
		}
		if (gGameConfig.prideFlagsOptionVisible && !bOnlinePauseMenu && DrawMenuOption(std::format("Pride Flag Colors - {}", gGameConfig.prideFlags ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
			gGameConfig.prideFlags = !gGameConfig.prideFlags;
		}
		if (auto tmp = DrawMenuOption("Color Palette")) {
			ProcessColorPaletteMenu();
			if (DrawMenuOption("Back")) {
				pMenu->nLevel -= 2;
			}
		}
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("Player Settings")) {
		ProcessPlayerSettingsMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
}

void ProcessScoresMenu() {
	for (int i = 0; i < tGameConfig::NUM_STARTING_LEVELS; i++) {
		if (auto tmp = DrawMenuOption(tGameConfig::GetStartingLevelName(i))) {
			for (int j = 0; j < tGameConfig::NUM_RANDOMIZER_TYPES; j++) {
				if (auto tmp = DrawMenuOption(tGameConfig::GetRandomizerTypeName(j), i == 0 ? MENU_FLAG_BLOCK_ABOVE : 0)) {
					if (i != 0) {
						for (int k = 0; k < 2; k++) {
							if (auto tmp = DrawMenuOption(k ? "NES First Level" : "10 Clear First Level", MENU_FLAG_BLOCK_ABOVE)) {
								gGameState = STATE_SCOREBOARD_VIEW;
								Scoreboard::ClearHighlight();
								Scoreboard::SetFile(i, j, k);
							}
						}
						if (DrawMenuOption("Back")) {
							pMenu->nLevel -= 2;
						}
					}
					else {
						gGameState = STATE_SCOREBOARD_VIEW;
						Scoreboard::ClearHighlight();
						Scoreboard::SetFile(i, j, false);
					}
				}
			}
			if (DrawMenuOption("Back")) {
				pMenu->nLevel -= 2;
			}
		}
	}
}

void ShowGameSettings() {
	if (NyaNet::IsConnected() && NetGetNumConnectedPlayers() < 2) {
		DrawMenuOption("Waiting for players...", MENU_FLAG_BLOCK_ABOVE);
	}
	else if (DrawMenuOption("Start")) {
		if (NyaNet::IsServer()) {
			CNyaNetPacket<tStartGamePacket> packet;
			packet.data.rngForced = gGameConfig.randSameForAllPlayers;
			packet.data.rng = time(nullptr);
			packet.Send(true);
		}
		else {
			PlayGameSound(SOUND_START);
			gGameState = STATE_PLAYING;
			gForcedRNGValue = -1;
			ResetAllPlayers();
		}
		pMenu->nLevel--;
	}
	gGameConfig.startingLevel += (int)DrawMenuOption(std::format("Starting Level - {}", gGameConfig.GetStartingLevelName()).c_str(), MENU_FLAG_EDIT_VALUE);
	if (gGameConfig.startingLevel < 0) gGameConfig.startingLevel = tGameConfig::NUM_STARTING_LEVELS - 1;
	if (gGameConfig.startingLevel >= tGameConfig::NUM_STARTING_LEVELS) gGameConfig.startingLevel = 0;
	if (NyaNet::IsConnected()) {
		gGameConfig.gameType = tGameConfig::TYPE_1PLAYER;
	}
	else {
		gGameConfig.gameType += (int) DrawMenuOption(std::format("Mode - {}", gGameConfig.GetGameTypeName()).c_str(),
													 MENU_FLAG_EDIT_VALUE);
		if (gGameConfig.gameType < 0) gGameConfig.gameType = tGameConfig::NUM_GAMETYPES - 1;
		if (gGameConfig.gameType >= tGameConfig::NUM_GAMETYPES) gGameConfig.gameType = tGameConfig::TYPE_1PLAYER;
	}
	gGameConfig.randomizerType += (int)DrawMenuOption(std::format("Randomizer - {}", gGameConfig.GetRandomizerTypeName()).c_str(), MENU_FLAG_EDIT_VALUE);
	if (gGameConfig.randomizerType < 0) gGameConfig.randomizerType = tGameConfig::NUM_RANDOMIZER_TYPES - 1;
	if (gGameConfig.randomizerType >= tGameConfig::NUM_RANDOMIZER_TYPES) gGameConfig.randomizerType = tGameConfig::RANDOMIZER_NES;
	if (DrawMenuOption(std::format("NES First Level Behavior - {}", gGameConfig.nesInitialLevelClear ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.nesInitialLevelClear = !gGameConfig.nesInitialLevelClear;
	}
	if (NyaNet::IsConnected() && DrawMenuOption(std::format("Same RNG for All Players - {}", gGameConfig.randSameForAllPlayers ? "On" : "Off").c_str(), MENU_FLAG_EDIT_VALUE)) {
		gGameConfig.randSameForAllPlayers = !gGameConfig.randSameForAllPlayers;
	}
}

void ShowPlayerListMenu() {
	if (auto tmp = DrawMenuOption(("Player List (" + std::to_string(NetGetNumConnectedPlayers()) + "/" + std::to_string(NyaNet::nMaxPossiblePlayers) + ")").c_str())) {
		for (auto& ply : aNetPlayers) {
			if (!ply.connected) continue;
			DrawMenuOption(ply.GetName(), MENU_FLAG_BLOCK_ABOVE);
		}
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
}

void ProcessNetClientMenu() {
	if (NetGetNumConnectedPlayers() <= 0) {
		DrawMenuOption("Connecting...", MENU_FLAG_BLOCK_ABOVE);
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
		return;
	}

	DrawMenuOption(aNetPlayers[0].slowPlayerData.inGame ? "Match in progress..." : "Waiting for host...", MENU_FLAG_BLOCK_ABOVE);
	DrawMenuOption(std::format("Starting Level - {}", gGameConfig.GetStartingLevelName()).c_str(), MENU_FLAG_BLOCK_ABOVE);
	DrawMenuOption(std::format("Randomizer - {}", gGameConfig.GetRandomizerTypeName()).c_str(), MENU_FLAG_BLOCK_ABOVE);
	DrawMenuOption(std::format("NES First Level Behavior - {}", gGameConfig.nesInitialLevelClear ? "On" : "Off").c_str(), MENU_FLAG_BLOCK_ABOVE);
	DrawMenuOption(std::format("Same RNG for All Players - {}", gNetGameSettings.randSameForAllPlayers ? "On" : "Off").c_str(), MENU_FLAG_BLOCK_ABOVE);
	ShowPlayerListMenu();
	if (auto tmp = DrawMenuOption("Options")) {
		ProcessOptionsMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (DrawMenuOption("Back")) {
		pMenu->nLevel -= 2;
	}
}

void ProcessMenuHead(bool mainMenu) {
	pMenu = mainMenu ? &mainMenuState : &onlinePauseMenuState;

	if (mainMenu) {
		DrawBackgroundImage(0, 1);

		static auto logo = LoadTexture("logo.png");

		float fLogoSizeY = 0.33 * 0.34375;
		float fLogoSizeX = 0.33 * GetAspectRatioInv();
		float fLogoPosY = 0.2;

		float fLogoBGSizeY = fLogoSizeY + 0.01;
		float fLogoBGSizeX = fLogoSizeX + 0.01 * GetAspectRatioInv();

		DrawRectangle(0.5 - fLogoBGSizeX, 0.5 + fLogoBGSizeX, fLogoPosY - fLogoBGSizeY, fLogoPosY + fLogoBGSizeY,
					  {0, 0, 0, 127}, 0.01);
		DrawRectangle(0.5 - fLogoSizeX, 0.5 + fLogoSizeX, fLogoPosY - fLogoSizeY, fLogoPosY + fLogoSizeY,
					  {255, 255, 255, 255}, 0, logo);
	}
	else {
		DrawRectangle(0, 1, 0, 1, {0,0,0,200});
	}

	aMenuOptionsToDraw.clear();
	pMenu->nTargetLevel = 0;
	memset(pMenu->nNumOptions, 0, sizeof(pMenu->nNumOptions));
	bBlockMenuBack = false;
	bBlockMenuSelect = false;
	fForceMenuWidth = 0.0;
	nDrawPiecePreviews = -1;
}

void ProcessMenuTail(bool mainMenu) {
	float menuCenter = mainMenu ? 0.7 : 0.6;

	fMenuSize = 1 / ((double)pMenu->nNumOptions[pMenu->nLevel] * 2);
	if (fMenuSize > 0.075) fMenuSize = 0.075;
	fMenuTop = menuCenter - (fMenuSize * pMenu->nNumOptions[pMenu->nLevel] * 0.5);

	if (mainMenu) {
		float menuMaxWidth = 0;
		if (fForceMenuWidth != 0.0) menuMaxWidth = fForceMenuWidth;
		else {
			for (auto &option: aMenuOptionsToDraw) {
				if (!option.ShouldDraw()) continue;

				auto width = option.GetTextWidth();
				if (width > menuMaxWidth) menuMaxWidth = width;
			}
		}
		float menuBgWidth = menuMaxWidth * 0.5 + 0.075 * GetAspectRatioInv();
		float ySpacing = 0.05;
		float ySize = (fMenuSize * (pMenu->nNumOptions[pMenu->nLevel] - 1)) + ySpacing;
		DrawRectangle(0.5 - menuBgWidth, 0.5 + menuBgWidth, fMenuTop - ySpacing, fMenuTop + ySize, {0,0,0,127}, 0.01);
	}
	if (nDrawPiecePreviews != -1) {
		auto &colors = gGameConfig.pieceColors[nDrawPiecePreviews];

		float fBlockSize = 0.04;
		float fBlockOffset = fBlockSize * 0.5;
		DrawBlock(0.5 - (fBlockOffset * GetAspectRatioInv()), fMenuTop - fBlockOffset, fBlockSize, NESPaletteToRGB(colors.paletteId[0]), NESPaletteToRGB(colors.backgroundPaletteId[1]), false);
		DrawBlock(0.5 - (fBlockOffset * GetAspectRatioInv()), fMenuTop + fMenuSize - fBlockOffset, fBlockSize, NESPaletteToRGB(colors.paletteId[1]), NESPaletteToRGB(colors.backgroundPaletteId[1]), false);

		tNyaStringData string;
		string.x = 0.5 + 0.175 * GetAspectRatioInv();
		string.y = fMenuTop;
		string.size = fMenuSize * 0.75;
		string.XCenterAlign = true;
		NyaDrawing::CNyaRGBA32 colBase = {255,255,255,255};
		NyaDrawing::CNyaRGBA32 colSelected = {245,171,185,255};
		string.SetColor(pMenu->nSelectedOption[pMenu->nTargetLevel] == 0 ? &colSelected : &colBase);
		DrawString(string, std::format("({}/{})", colors.paletteId[0] + 1, 64).c_str());
		string.y += fMenuSize;
		string.SetColor(pMenu->nSelectedOption[pMenu->nTargetLevel] == 1 ? &colSelected : &colBase);
		DrawString(string, std::format("({}/{})", colors.paletteId[1] + 1, 64).c_str());
	}
	for (auto& option : aMenuOptionsToDraw) {
		option.Draw();
	}

	if (bIsInInputMenu) {
		if (GetMenuAcceptInput()) bIsInInputMenu = false;
	}
	else {
		if (GetMenuUp()) pMenu->nSelectedOption[pMenu->nLevel]--;
		if (GetMenuDown()) pMenu->nSelectedOption[pMenu->nLevel]++;
		if (!bBlockMenuBack && GetMenuBack()) {
			if (pMenu->nLevel > 0) {
				PlayGameSound(SOUND_ROTATE);
				pMenu->nLevel--;
			}
			else if (!mainMenu) {
				bOnlinePauseMenu = false;
				pMenu->nSelectedOption[0] = 0;
			}
		}
		if (!bBlockMenuSelect && GetMenuSelect()) {
			if (bOnlinePauseMenu || pMenu->nLevel != 1 || pMenu->nSelectedOption[0] != 0 || pMenu->nSelectedOption[1] != 0) PlayGameSound(SOUND_LOCK);
			pMenu->nLevel++;
		}

		if (GetMenuUp() || GetMenuDown()) PlayGameSound(SOUND_MENUMOVE);
	}

	if (pMenu->nLevel < 0) pMenu->nLevel = 0;
	for (int i = 0; i <= pMenu->nLevel; i++) {
		if (pMenu->nSelectedOption[i] < 0) pMenu->nSelectedOption[i] = pMenu->nNumOptions[i] - 1;
		if (pMenu->nSelectedOption[i] >= pMenu->nNumOptions[i]) pMenu->nSelectedOption[i] = 0;
	}
	for (int i = pMenu->nLevel + 1; i < 8; i++) {
		pMenu->nSelectedOption[i] = 0;
	}

	if (mainMenu) {
		tNyaStringData string;
		string.x = 1 - (0.01 * GetAspectRatioInv());
		string.y = 1.025;
		string.size = 0.05;
		string.XRightAlign = true;
		string.YBottomAlign = true;
		string.SetColor(255, 255, 255, 255);
		DrawString(string, "v2.20");
	}
}

void ProcessMultiplayerMenu() {
	if (auto tmp = DrawMenuOption("Host Game")) {
		static std::string portString = gGameConfig.port;

		if (auto tmp = DrawMenuOption(std::format("Port - {}", (portString + (bIsInInputMenu ? "..." : ""))).c_str(), MENU_FLAG_BLOCK_ABOVE)) {
			bIsInInputMenu = true;
		}

		if (bIsInInputMenu) {
			if (portString.length() < 63) portString += sKeyboardInput;
			if (IsKeyJustPressed(VK_BACK) && !portString.empty()) portString.pop_back();
		}

		if (auto tmp = DrawMenuOption("Host")) {
			if (NyaNet::IsServer()) {
				ShowGameSettings();
				ShowPlayerListMenu();
				if (auto tmp = DrawMenuOption("Options")) {
					ProcessOptionsMenu();
					if (DrawMenuOption("Back")) {
						pMenu->nLevel -= 2;
					}
				}
				if (DrawMenuOption("Back")) {
					pMenu->nLevel -= 2;
				}
			}
			else {
				strcpy_s(gGameConfig.port, 64, portString.c_str());
				try {
					nNetGamePort = 0;
					nNetGamePort = std::stoi(portString);
					if (!NetCreateGame()) pMenu->nLevel--;
				}
				catch (const std::invalid_argument& e) {
					pMenu->nLevel--;
				}
				catch (const std::out_of_range& e) {
					pMenu->nLevel--;
				}
			}

		}

		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("Join Game")) {
		static std::string ipAddrString = gGameConfig.ipAddress;
		static std::string portString = gGameConfig.port;
		static int inputMenuId = 0;

		if (auto tmp = DrawMenuOption(std::format("IP Address - {}", (ipAddrString + (bIsInInputMenu && inputMenuId == 0 ? "..." : ""))).c_str(), MENU_FLAG_BLOCK_ABOVE)) {
			bIsInInputMenu = true;
			inputMenuId = 0;
		}
		if (auto tmp = DrawMenuOption(std::format("Port - {}", (portString + (bIsInInputMenu && inputMenuId == 1 ? "..." : ""))).c_str(), MENU_FLAG_BLOCK_ABOVE)) {
			bIsInInputMenu = true;
			inputMenuId = 1;
		}

		if (bIsInInputMenu) {
			auto& str = inputMenuId == 0 ? ipAddrString : portString;
			if (str.length() < 63) str += sKeyboardInput;
			if (IsKeyJustPressed(VK_BACK) && !str.empty()) str.pop_back();
		}

		if (auto tmp = DrawMenuOption("Connect")) {
			if (NyaNet::IsClient()) {
				ProcessNetClientMenu();
			}
			else {
				ENetAddress address;
				if (!enet_address_set_host_ip(&address, ipAddrString.c_str())) {
					strcpy_s(gGameConfig.ipAddress, 64, ipAddrString.c_str());
					strcpy_s(gGameConfig.port, 64, portString.c_str());
					try {
						nNetGameAddress = address.host;
						nNetGamePort = 0;
						nNetGamePort = std::stoi(portString);
						NetJoinGame();
					}
					catch (const std::invalid_argument& e) {
						pMenu->nLevel--;
					}
					catch (const std::out_of_range& e) {
						pMenu->nLevel--;
					}
				}
			}

			if (!NyaNet::IsConnected()) pMenu->nLevel--;
		}

		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
}

void ProcessAboutMenu() {
	DrawMenuOption("Programming - gaycoderprincess", MENU_FLAG_BLOCK_ABOVE);
	DrawMenuOption("Graphics Design - teddyator", MENU_FLAG_BLOCK_ABOVE);
	DrawMenuOption("References Used:", MENU_FLAG_BLOCK_ABOVE);
	if (DrawMenuOption("meatfighter.com/nintendotetrisai", MENU_FLAG_BLOCK_ABOVE)) {
		ShellExecuteA(nullptr, nullptr, "https://meatfighter.com/nintendotetrisai", nullptr, nullptr , SW_SHOW );
	}
}

void ProcessMainMenu() {
	ProcessMenuHead(true);

	// yay hacks!~
	if (pMenu->nLevel < 3) DisconnectNet();

	if (auto tmp = DrawMenuOption("Single Player")) {
		ShowGameSettings();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("Multiplayer")) {
		ProcessMultiplayerMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("Options")) {
		ProcessOptionsMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("Scores")) {
		ProcessScoresMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("About")) {
		ProcessAboutMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (DrawMenuOption("Quit")) {
		ProgramOnExit();
		exit(0);
	}

	ProcessMenuTail(true);
}

void ExitOnlinePauseMenu() {
	bOnlinePauseMenu = false;
	pMenu->nLevel = 0;
	pMenu->nSelectedOption[0] = 0;
}

void ProcessOnlinePauseMenu() {
	ProcessMenuHead(false);

	if (auto tmp = DrawMenuOption("Options")) {
		ProcessOptionsMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (auto tmp = DrawMenuOption("About")) {
		ProcessAboutMenu();
		if (DrawMenuOption("Back")) {
			pMenu->nLevel -= 2;
		}
	}
	if (NyaNet::IsServer()) {
		if (DrawMenuOption("Quit to Menu")) {
			if (NyaNet::IsServer()) {
				CNyaNetPacket<tEndGamePacket> packet;
				packet.Send(true);
			} else {
				ShowEndgameScoreboard();
			}
			ExitOnlinePauseMenu();
		}
	}
	if (NyaNet::IsClient()) {
		if (DrawMenuOption("Disconnect")) {
			DisconnectNet();
			ExitOnlinePauseMenu();
		}
	}
	if (DrawMenuOption("Back to Game")) {
		ExitOnlinePauseMenu();
	}

	ProcessMenuTail(false);
}
