#include <windows.h>

#include "nya_dx11_appbase.h"

#include "tetrus_controls.h"

bool GetMenuPause() {
	if (IsKeyJustPressed(VK_ESCAPE)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_START)) return true;
	return false;
}

bool GetMenuBack(bool countBackspace) {
	if (IsKeyJustPressed(VK_ESCAPE)) return true;
	if (countBackspace && IsKeyJustPressed(VK_BACK)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_B)) return true;
	return false;
}

bool GetMenuSelect() {
	if (IsKeyJustPressed(VK_RETURN)) return true;
	if (IsKeyJustPressed(VK_SPACE)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_A)) return true;
	return false;
}

bool GetMenuUp() {
	if (IsKeyJustPressed(VK_UP)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_DPAD_UP)) return true;
	return false;
}

bool GetMenuDown() {
	if (IsKeyJustPressed(VK_DOWN)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_DPAD_DOWN)) return true;
	return false;
}

bool GetMenuLeft() {
	if (IsKeyJustPressed(VK_LEFT)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_DPAD_LEFT)) return true;
	return false;
}

bool GetMenuRight() {
	if (IsKeyJustPressed(VK_RIGHT)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_DPAD_RIGHT)) return true;
	return false;
}

bool GetMenuQuit() {
	if (IsKeyJustPressed('Q')) return true;
	return false;
}

bool GetMenuGameOverQuit() {
	if (IsKeyJustPressed(VK_RETURN)) return true;
	if (IsKeyJustPressed(VK_ESCAPE)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_START)) return true;
	return false;
}

bool GetMenuAcceptName() {
	if (IsKeyJustPressed(VK_RETURN)) return true;
	if (IsKeyJustPressed(VK_ESCAPE)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_A)) return true;
	if (IsPadKeyJustPressed(NYA_PAD_KEY_B)) return true;
	return false;
}

bool CController::tControl::IsPressed() const {
	if (keyId <= 0) return false;

	if (controller == -1) return IsKeyPressed(keyId);
	return IsPadKeyPressed(keyId, controller);
}

bool CController::tControl::IsJustPressed() const {
	if (keyId <= 0) return false;

	if (controller == -1) return IsKeyJustPressed(keyId);
	return IsPadKeyJustPressed(keyId, controller);
}

bool CController::tControl::IsBound() const {
	if (keyId <= 0) return false;
	if (controller < -1) return false;
	if (controller >= XUSER_MAX_COUNT) return false;
	if (controller != -1 && keyId >= NUM_NYA_PAD_KEYS) return false;
	return true;
}

const char* CController::tControl::GetKeyName() const {
	if (!IsBound()) return "Unbound";
	if (controller != -1) {
		static std::string padKey;
		padKey = nyaPadKeyNames[keyId];
		padKey += " (Pad " + std::to_string(controller + 1) + ")";
		return padKey.c_str();
	}
	static char str[256];
	memset(str, 0, sizeof(str));
	GetKeyNameTextA(MapVirtualKeyA(keyId, MAPVK_VK_TO_VSC) << 16, str, sizeof(str));
	if (!str[0]) return "Unknown";
	return str;
}

CController::CController(int playerId) {
	memset(aControls, 0, sizeof(aControls));

	if (playerId == 0) {
		for (int i = 0; i < NUM_CONTROLS; i++) {
			aControls[i].controller = -1;
			aControls[i].keyId = gP1Defaults[i];
		}
	}
	else {
		for (int i = 0; i < NUM_CONTROLS; i++) {
			if (gPadDefaults[i] == -1) continue;
			aControls[i].controller = playerId - 1;
			aControls[i].keyId = gPadDefaults[i];
		}
	}
}

const char* CController::GetKeyName(int id) {
	if (id < 0 || id >= NUM_CONTROLS) return "invalid";
	return aControls[id].GetKeyName();
}

const char* CController::GetControlName(int id) {
	switch (id) {
		case CONTROL_LEFT:
			return "Move Left";
		case CONTROL_RIGHT:
			return "Move Right";
		case CONTROL_DOWN:
			return "Move Down";
		case CONTROL_ROTATE:
			return "Rotate CW";
		case CONTROL_ROTATE_BACK:
			return "Rotate CCW";
		case CONTROL_LEFT_ALT:
			return "Alt Move Left";
		case CONTROL_RIGHT_ALT:
			return "Alt Move Right";
		case CONTROL_DOWN_ALT:
			return "Alt Move Down";
		case CONTROL_ROTATE_ALT:
			return "Alt Rotate CW";
		case CONTROL_ROTATE_BACK_ALT:
			return "Alt Rotate CCW";
		default:
			return "Invalid";
	}
}

CController aPlayerControls[4] = {CController(0), CController(1), CController(2), CController(3)};