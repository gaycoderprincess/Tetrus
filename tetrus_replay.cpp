#include "nya_dx11_appbase.h"

#include "tetrus_util.h"
#include "tetrus_config.h"
#include "tetrus_board.h"
#include "tetrus_controls.h"
#include "tetrus_piece.h"
#include "tetrus_player.h"
#include "tetrus_replay.h"

CReplay::CReplay(CPlayer* player) {
	this->player = player;
	this->timer = 0;
	this->nextEvent = 0;
	this->events.reserve(1024); // reserve at least a little bit to avoid too many allocations at the start >w<
}

void CReplay::PlayEvent(eReplayEvent type) const {
	switch (type) {
		case REPLAY_EVENT_MOVE_LEFT:
			player->Move(-1);
			break;
		case REPLAY_EVENT_MOVE_RIGHT:
			player->Move(1);
			break;
		case REPLAY_EVENT_MOVE_DOWN:
			player->MoveDown();
			break;
		case REPLAY_EVENT_ROTATE:
			player->Rotate(1);
			break;
		case REPLAY_EVENT_ROTATE_BACK:
			player->Rotate(-1);
			break;
		case REPLAY_EVENT_DROP:
			player->Place();
			break;
		case REPLAY_EVENT_PAUSE:
			gGameState = STATE_REPLAY_PAUSED;
			break;
		case REPLAY_EVENT_UNPAUSE:
			gGameState = STATE_REPLAY_VIEW;
			break;
		default:
			break;
	}
}

void CReplay::AddEvent(eReplayEvent type) {
	if (gGameState == STATE_REPLAY_VIEW) return;
	events.push_back({type, timer});
}

bool CReplay::IsEventHandled(eReplayEvent type) {
	return type >= MIN_REPLAY_EVENT && type < NUM_REPLAY_EVENTS;
}

void CReplay::Playback() {
	timer += gGameTimer.fDeltaTime;
	while (nextEvent < events.size() && timer >= events[nextEvent].time) PlayEvent(events[nextEvent++].type);
}

CReplay gPlaybackReplay = CReplay(nullptr);

void StartReplay(void* data, size_t dataSize, CPlayer* player) {
	gPlaybackReplay.player = player;
	gPlaybackReplay.events.clear();
	gPlaybackReplay.timer = 0;
	gPlaybackReplay.nextEvent = 0;

	auto events = (CReplay::tEvent*)data;
	int count = dataSize / sizeof(CReplay::tEvent);
	gPlaybackReplay.events.reserve(count);
	for (int i = 0; i < count; i++) {
		gPlaybackReplay.events.push_back(events[i]);
	}
	gGameState = STATE_REPLAY_VIEW;
}

void ProcessReplay() {
	gPlaybackReplay.Playback();
}