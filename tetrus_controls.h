#ifndef TETRUS_CONTROLS_H
#define TETRUS_CONTROLS_H

bool GetMenuPause();
bool GetMenuBack(bool countBackspace = true);
bool GetMenuSelect();
bool GetMenuUp();
bool GetMenuDown();
bool GetMenuLeft();
bool GetMenuRight();
bool GetMenuQuit();
bool GetMenuGameOverQuit();
bool GetMenuAcceptInput();

class CController {
public:
	enum ePlayerControl {
		CONTROL_LEFT,
		CONTROL_RIGHT,
		CONTROL_DOWN,
		CONTROL_ROTATE,
		CONTROL_ROTATE_BACK,
		CONTROL_LEFT_ALT,
		CONTROL_RIGHT_ALT,
		CONTROL_DOWN_ALT,
		CONTROL_ROTATE_ALT,
		CONTROL_ROTATE_BACK_ALT,
		NUM_CONTROLS
	};

	constexpr static inline const char* gPlayerControlNames[] = {
		"Move Left",
		"Move Right",
		"Move Down",
		"Rotate CW",
		"Rotate CCW",
		"Alt Move Left",
		"Alt Move Right",
		"Alt Move Down",
		"Alt Rotate CW",
		"Alt Rotate CCW",
	};

	struct tControl {
		int controller = -1;
		int keyId = 0;

		[[nodiscard]] bool IsPressed() const;
		[[nodiscard]] bool IsJustPressed() const;
		[[nodiscard]] bool IsBound() const;
		[[nodiscard]] const char* GetKeyName() const;
	} aControls[NUM_CONTROLS];
	int rumblePad;

	constexpr static const int gP1Defaults[NUM_CONTROLS] = {
			'A',
			'D',
			'S',
			'E',
			'Q',
			-1,
			-1,
			-1,
			-1,
			-1
	};

	constexpr static const int gPadDefaults[NUM_CONTROLS] = {
			NYA_PAD_KEY_DPAD_LEFT,
			NYA_PAD_KEY_DPAD_RIGHT,
			NYA_PAD_KEY_DPAD_DOWN,
			NYA_PAD_KEY_B,
			NYA_PAD_KEY_A,
			-1,
			-1,
			-1,
			-1,
			-1
	};

	CController(int playerId);
	const char* GetKeyName(int id);
	static const char* GetControlName(int id);
};
extern CController aPlayerControls[4];

void AddRumble(int padId, int strength, double time);
void ProcessRumble();
#endif