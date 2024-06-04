#ifndef TETRUS_REPLAY_H
#define TETRUS_REPLAY_H

enum eReplayEvent {
	MIN_REPLAY_EVENT = 0,
	REPLAY_EVENT_MOVE_LEFT = MIN_REPLAY_EVENT,
	REPLAY_EVENT_MOVE_RIGHT,
	REPLAY_EVENT_MOVE_DOWN,
	REPLAY_EVENT_ROTATE,
	REPLAY_EVENT_ROTATE_BACK,
	REPLAY_EVENT_DROP,
	REPLAY_EVENT_PAUSE,
	REPLAY_EVENT_UNPAUSE,
	NUM_REPLAY_EVENTS
};

class CReplay {
public:
	struct tEvent {
		eReplayEvent type;
		double time;
	};
	std::vector<tEvent> events;
	CPlayer* player;

	double timer;
	int nextEvent;

	CReplay(CPlayer* player);
	void PlayEvent(eReplayEvent type) const;
	void AddEvent(eReplayEvent type);
	static bool IsEventHandled(eReplayEvent type);
	void Playback();
};
extern CReplay gPlaybackReplay;

void StartReplay(void* data, size_t dataSize, CPlayer* player);
void ProcessReplay();
#endif