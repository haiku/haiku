/*
 * Copyright 2003-2006, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SCREEN_SAVER_SETTINGS_H
#define SCREEN_SAVER_SETTINGS_H


#include <Message.h>
#include <Path.h>
#include <String.h>


enum screen_corner {
	NO_CORNER = -1,
	UP_LEFT_CORNER,
	UP_RIGHT_CORNER,
	DOWN_RIGHT_CORNER,
	DOWN_LEFT_CORNER,
	CENTER_CORNER
};

// time flags
enum {
	SAVER_DISABLED = 0x00,
	ENABLE_SAVER = 0x01,
	ENABLE_DPMS_STAND_BY = 0x02,
	ENABLE_DPMS_SUSPEND = 0x04,
	ENABLE_DPMS_OFF = 0x08,

	ENABLE_DPMS_MASK = ENABLE_DPMS_STAND_BY | ENABLE_DPMS_SUSPEND | ENABLE_DPMS_OFF
};

#define SCREEN_BLANKER_SIG "application/x-vnd.Haiku.screenblanker"

class ScreenSaverSettings {
	public:
		ScreenSaverSettings();

		bool Load();
		void Save();
		void Defaults();
		BPath& Path() { return fSettingsPath; }

		BRect WindowFrame() { return fWindowFrame; }
		int32 WindowTab() { return fWindowTab; }
		int32 TimeFlags() { return fTimeFlags; }
		bigtime_t BlankTime() { return fBlankTime; }
		bigtime_t StandByTime() { return fStandByTime; }
		bigtime_t SuspendTime() { return fSuspendTime; }
		bigtime_t OffTime() { return fOffTime; }

		screen_corner BlankCorner() { return fBlankCorner; }
		screen_corner NeverBlankCorner() { return fNeverBlankCorner; }
		bool LockEnable() { return fLockEnabled; }
		bigtime_t PasswordTime() { return fPasswordTime; }
		const char *Password() { return fPassword.String(); }
		const char *LockMethod() { return fLockMethod.String(); }
		bool IsNetworkPassword() { return strcmp(fLockMethod.String(), "custom") != 0; }
		const char *ModuleName() { return fModuleName.String(); }
		status_t GetModuleState(const char *name, BMessage *stateMsg);

		void SetWindowFrame(const BRect &fr) { fWindowFrame = fr; }
		void SetWindowTab(int32 tab) { fWindowTab = tab; }
		void SetTimeFlags(int32 tf) { fTimeFlags = tf; }
		void SetBlankTime(bigtime_t bt) { fBlankTime = bt; }
		void SetStandByTime(bigtime_t time) { fStandByTime = time; }
		void SetSuspendTime(bigtime_t time) { fSuspendTime = time; }
		void SetOffTime(bigtime_t intime) { fOffTime = intime; }
		void SetBlankCorner(screen_corner in) { fBlankCorner = in; }
		void SetNeverBlankCorner(screen_corner in) { fNeverBlankCorner = in; }
		void SetLockEnable(bool en) { fLockEnabled = en; }
		void SetPasswordTime(bigtime_t pt) { fPasswordTime = pt; }
		void SetPassword(const char *pw) { fPassword = pw; }
			// Cannot set network password from here.
		void SetLockMethod(const char *method) { fLockMethod = method; }
		void SetModuleName(const char *mn) { fModuleName = mn; }
		void SetModuleState(const char *name, BMessage *stateMsg);

		BMessage& Message();

	private:
		BRect fWindowFrame;
		int32 fWindowTab;
		int32 fTimeFlags;
		bigtime_t fBlankTime;
		bigtime_t fStandByTime;
		bigtime_t fSuspendTime;
		bigtime_t fOffTime;
		screen_corner fBlankCorner;
		screen_corner fNeverBlankCorner;
		bool fLockEnabled;
		bigtime_t fPasswordTime;
		BString fPassword;
		BString fModuleName;
		BString fLockMethod;

		BMessage fSettings;
		BPath fSettingsPath, fNetworkPath;
};

#endif	// SCREEN_SAVER_SETTINGS_H
