/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef SCREEN_SAVER_PREFS_H
#define SCREEN_SAVER_PREFS_H
#include <Message.h>
#include <Path.h>
#include <String.h>

enum arrowDirection {
	NONE = -1,
	UPLEFT,
	UPRIGHT,
	DOWNRIGHT,
	DOWNLEFT,
	CENTER
};

class ScreenSaverPrefs 
{
public:
	ScreenSaverPrefs();
	bool LoadSettings();
	void Defaults();

	BRect WindowFrame() {return fWindowFrame;}
	int32 WindowTab() {return fWindowTab;}
	int32 TimeFlags() {return fTimeFlags;}
	bigtime_t BlankTime() {return fBlankTime;}
	bigtime_t StandbyTime() {return fStandByTime;}
	bigtime_t SuspendTime() {return fSuspendTime;}
	bigtime_t OffTime() {return fOffTime;}
	arrowDirection GetBlankCorner() {return fBlankCorner;}
	arrowDirection GetNeverBlankCorner() {return fNeverBlankCorner;}
	bool LockEnable() {return fLockEnabled;}
	bigtime_t PasswordTime() {return fPasswordTime;}
	const char *Password() { return fPassword.String(); }
	bool IsNetworkPassword() {return fIsNetworkPassword;}
	const char *ModuleName() {return fModuleName.String();}
	BMessage *GetState(const char *name);

	void SetWindowFrame(const BRect &fr) { fWindowFrame = fr;}
	void SetWindowTab(int32 tab) { fWindowTab = tab;}
	void SetTimeFlags(int32 tf) {fTimeFlags = tf;}
	void SetBlankTime(bigtime_t bt) {fBlankTime = bt;}
	void SetStandbyTime(bigtime_t time) {fStandByTime = time;}
	void SetSuspendTime(bigtime_t time) {fSuspendTime = time;}
	void SetOffTime(bigtime_t intime) {fOffTime = intime;}
	void SetBlankCorner(arrowDirection in) {fBlankCorner = in;}
	void SetNeverBlankCorner(arrowDirection in) {fNeverBlankCorner = in;}
	void SetLockEnable(bool en) {fLockEnabled = en;}
	void SetPasswordTime(bigtime_t pt) {fPasswordTime = pt;}
	void SetPassword(char *pw) {fPassword = pw;} // Can not set network password from here.
	void SetModuleName(const char *mn) {fModuleName = mn;}
	void SetState(const char *name,BMessage *in);
	void SaveSettings();
	BMessage * GetSettings();
private:
	BRect fWindowFrame;
	int32 fWindowTab;
	int32 fTimeFlags;
	bigtime_t fBlankTime;
	bigtime_t fStandByTime;
	bigtime_t fSuspendTime;
	bigtime_t fOffTime;
	arrowDirection fBlankCorner;
	arrowDirection fNeverBlankCorner;
	bool fLockEnabled;
	bigtime_t fPasswordTime;
	BString fPassword;
	BString fModuleName;
	BString fLockMethod;

	bool fIsNetworkPassword;

	BMessage stateMsg, currentMessage;
	BPath ssPath,networkPath;
};


#endif //SCREEN_SAVER_PREFS_H
