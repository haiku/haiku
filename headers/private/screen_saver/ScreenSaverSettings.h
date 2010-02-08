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
	ENABLE_SAVER = 0x01,
	ENABLE_DPMS_STAND_BY = 0x02,
	ENABLE_DPMS_SUSPEND = 0x04,
	ENABLE_DPMS_OFF = 0x08,

	ENABLE_DPMS_MASK
		= ENABLE_DPMS_STAND_BY | ENABLE_DPMS_SUSPEND | ENABLE_DPMS_OFF
};

#define SCREEN_BLANKER_SIG "application/x-vnd.Haiku.screenblanker"


class ScreenSaverSettings {
public:
							ScreenSaverSettings();

			bool			Load();
			void			Save();
			void			Defaults();
			BPath&			Path() { return fSettingsPath; }

			// General screen saver settings
			int32			TimeFlags() const { return fTimeFlags; }
			bigtime_t		BlankTime() const { return fBlankTime; }
			bigtime_t		StandByTime() const { return fStandByTime; }
			bigtime_t		SuspendTime() const { return fSuspendTime; }
			bigtime_t		OffTime() const { return fOffTime; }

			screen_corner		BlankCorner() const { return fBlankCorner; }
			screen_corner		NeverBlankCorner() const { return fNeverBlankCorner; }
			bool			LockEnable() const { return fLockEnabled; }
			bigtime_t		PasswordTime() const { return fPasswordTime; }
			const char*		Password() { return fPassword.String(); }
			const char*		LockMethod() { return fLockMethod.String(); }
			bool			IsNetworkPassword()
								{ return strcmp(fLockMethod.String(), "custom")
									!= 0; }

			const char*		ModuleName() { return fModuleName.String(); }
			status_t		GetModuleState(const char* name,
								BMessage* stateMessage);

			void			SetTimeFlags(uint32 flags)
								{ fTimeFlags = flags; }
			void			SetBlankTime(bigtime_t time)
								{ fBlankTime = time; }
			void			SetStandByTime(bigtime_t time)
								{ fStandByTime = time; }
			void			SetSuspendTime(bigtime_t time)
								{ fSuspendTime = time; }
			void			SetOffTime(bigtime_t intime)
								{ fOffTime = intime; }
			void			SetBlankCorner(screen_corner in)
								{ fBlankCorner = in; }
			void			SetNeverBlankCorner(screen_corner in)
								{ fNeverBlankCorner = in; }
			void			SetLockEnable(bool enable)
								{ fLockEnabled = enable; }
			void			SetPasswordTime(bigtime_t time)
								{ fPasswordTime = time; }
			void			SetPassword(const char* password)
								{ fPassword = password; }
								// Cannot set network password from here.
			void			SetLockMethod(const char* method)
								{ fLockMethod = method; }
			void			SetModuleName(const char* name)
								{ fModuleName = name; }
			void			SetModuleState(const char* name,
								BMessage* stateMessage);

			// ScreenSaver preferences settings
			BRect			WindowFrame() const { return fWindowFrame; }
			int32			WindowTab() const { return fWindowTab; }

			void			SetWindowFrame(const BRect& frame)
								{ fWindowFrame = frame; }
			void			SetWindowTab(int32 tab)
								{ fWindowTab = tab; }

			BMessage&		Message();

private:
			int32			fTimeFlags;
			bigtime_t		fBlankTime;
			bigtime_t		fStandByTime;
			bigtime_t		fSuspendTime;
			bigtime_t		fOffTime;
			screen_corner	fBlankCorner;
			screen_corner	fNeverBlankCorner;
			bool			fLockEnabled;
			bigtime_t		fPasswordTime;
			BString			fPassword;
			BString			fModuleName;
			BString			fLockMethod;

			BRect			fWindowFrame;
			int32			fWindowTab;

			BMessage		fSettings;
			BPath			fSettingsPath;
};

#endif	// SCREEN_SAVER_SETTINGS_H
