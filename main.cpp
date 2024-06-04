#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_controls.h"
#include "tetrus_audio.h"
#include "tetrus_board.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_saveload.h"
#include "tetrus_menu.h"
#include "tetrus_replay.h"
#include "tetrus_scoreboard.h"
#include "tetrus_net.h"

void ProcessGame() {
	DrawBackgroundImage(0, 1);

	if (gGameState == STATE_REPLAY_VIEW || gGameState == STATE_REPLAY_PAUSED) {
		ProcessReplay();
	}

	bool anyPlayerAlive = false;

	for (auto& board: aBoards) {
		board->DrawBackground();
	}

	if (NyaNet::IsConnected() && aPlayers[0]->gameOver) {
		ProcessNetSpectate();
	}
	else {
		for (auto& ply: aPlayers) {
			ply->Process();
			if (!ply->gameOver) anyPlayerAlive = true;
		}
		for (auto& board: aBoards) {
			board->Process();
		}
	}

	if (NyaNet::IsConnected()) {
		for (auto& ply : aNetPlayers) {
			if (!ply.connected) continue;

			if (ply.slowPlayerData.alive) anyPlayerAlive = true;
		}
		ProcessNetScoreboards();
	}

	if (gGameState == STATE_PAUSED || gGameState == STATE_REPLAY_PAUSED) {
		DrawRectangle(0, 1, 0, 1, {0,0,0,127});

		tNyaStringData string;
		string.x = 0.5;
		string.y = 0.5;
		if (gGameState == STATE_PAUSED) string.y -= 0.066;
		string.size = 0.066;
		string.XCenterAlign = true;
		string.topLevel = true;
		string.SetColor(255, 255, 255, 255);
		DrawString(string, "PAUSED");

		// only do the Q to quit code if it's not a replay!
		if (gGameState == STATE_PAUSED) {
			string.y += 0.066 * 2;
			DrawString(string, "Q - Quit to Main Menu");

			if (GetMenuQuit()) {
				gGameState = STATE_MAIN_MENU;
				return;
			}
		}
	}

	if (!anyPlayerAlive && GetMenuGameOverQuit() && !NyaNet::IsClient()) {
		if (gGameState == STATE_REPLAY_VIEW) gGameState = STATE_SCOREBOARD_VIEW;
		else if (!gGameConfig.IsCoopMode() && gGameState != STATE_REPLAY_VIEW) {
			if (NyaNet::IsServer()) {
				CNyaNetPacket<tEndGamePacket> packet;
				packet.Send(true);
			}
			else {
				ShowEndgameScoreboard();
			}
		}
		else gGameState = STATE_MAIN_MENU;
	}
	if (!NyaNet::IsConnected() && gGameState != STATE_REPLAY_VIEW && anyPlayerAlive && GetMenuPause()) {
		gGameState = gGameState == STATE_PLAYING ? STATE_PAUSED : STATE_PLAYING;
		for (auto player : aPlayers) {
			player->recordingReplay->AddEvent(gGameState == STATE_PLAYING ? REPLAY_EVENT_UNPAUSE : REPLAY_EVENT_PAUSE);
		}
		if (gGameState == STATE_PAUSED) PlayGameSound(SOUND_PAUSE);
	}

	if ((gGameState == STATE_REPLAY_VIEW || gGameState == STATE_REPLAY_PAUSED) && GetMenuQuit()) {
		gGameState = STATE_SCOREBOARD_VIEW;
		return;
	}
}

void ProcessScoreboard() {
	Scoreboard::Draw(gGameState == STATE_SCOREBOARD_VIEW);

	tNyaStringData string;
	string.XCenterAlign = true;
	string.x = 0.5;
	string.y = 0.9;
	string.size = 0.05;

	if (gGameState == STATE_SCOREBOARD_VIEW) {
		if (!Scoreboard::isEndGameScoreboard) {
			DrawString(string, "Select an entry to view the replay.");
			string.y += string.size;
		}
		DrawString(string, "Press the back button to quit.");

		if (GetMenuBack() || (Scoreboard::isEndGameScoreboard && GetMenuSelect())) {
			gGameState = STATE_MAIN_MENU;
		}
	}
}

void ProgramLoop() {
	gGameTimer.Process();
	bVSync = gGameConfig.vsync;

	switch (gGameState) {
		case STATE_MAIN_MENU:
			ProcessMainMenu();
			break;
		case STATE_PLAYING:
		case STATE_PAUSED:
		case STATE_REPLAY_VIEW:
		case STATE_REPLAY_PAUSED:
			ProcessGame();
			break;
		case STATE_SCOREBOARD_VIEW:
			ProcessScoreboard();
			break;
		default:
			break;
	}

	ProcessNet();
	ProcessSoundCache();

	CommonMain();
}

void ProgramOnExit() {
	SaveGameConfig();
	SaveControlConfig();
}

int main() {
	if (InitAppBase()) {
		NyaAudio::Init(ghWnd);

		LoadGameConfig();
		LoadControlConfig();
		if (!LoadPieceTypes()) return 2;

		while (true) {
			AppBaseLoop();
			Sleep(0);
		}
		return 0;
	}
	return 1;
}
