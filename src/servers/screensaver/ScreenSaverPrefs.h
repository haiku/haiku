#ifndef SCREEN_SAVER_PREFS_H
#define SCREEN_SAVER_PREFS_H
#include "Message.h"

class ScreenSaverPrefs 
{
public:
	ScreenSaverPrefs(void);
	bool LoadSettings(void);
	int BlankTime(void) {return blankTime;}
	int CurrentTime(void) {return currentTime;}
	int PasswordTime(void) {return passwordTime;}
	char *ModuleName(void) {return moduleName;}
	char *Password(void) {return password;}
	BMessage *GetState(void) {return &stateMsg;}
	void SetBlankTime(int );
	void SetCurrentTime(int );
	void SetPasswordTime(int );
	void SetModuleName(char *);
	void SetPassword(char *);
	void SetState(BMessage *);
private:
	bool parseSettings(BMessage *newSSMessage);
	int blankTime;
	int currentTime;
	int passwordTime;
	char moduleName[B_PATH_NAME_LENGTH];
	char password[B_PATH_NAME_LENGTH];
	BMessage stateMsg;
};

#endif //SCREEN_SAVER_PREFS_H
