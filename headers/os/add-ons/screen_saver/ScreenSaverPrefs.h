#ifndef SCREEN_SAVER_PREFS_H
#define SCREEN_SAVER_PREFS_H
#include "Message.h"
#include <string.h>

enum arrowDirection {NONE=-1,UPLEFT,UPRIGHT,DOWNRIGHT,DOWNLEFT};

class ScreenSaverPrefs 
{
public:
	ScreenSaverPrefs(void);
	bool LoadSettings(void);

	BRect WindowFrame(void) {return windowFrame;}
	int WindowTab(void) {return windowTab;}
	int TimeFlags(void) {return timeFlags;}
	int BlankTime(void) {return blankTime;}
	int StandbyTime(void) {return standbyTime;}
	int SuspendTime(void) {return suspendTime;}
	int OffTime(void) {return offTime;}
	arrowDirection GetBlankCorner(void) {return blank;}
	arrowDirection GetNeverBlankCorner(void) {return neverBlank;}
	bool LockEnable(void) {return lockenable;}
	int PasswordTime(void) {return passwordTime;}
	char *Password(void) {return password;}
	bool IsNetworkPassword(void) {return isNetworkPWD;}
	char *ModuleName(void) {return moduleName;}
	BMessage *GetState(void) {return &stateMsg;}

	void SetWindowFrame(const BRect &fr) {windowFrame=fr;}
	void SetWindowTab(int tab) {windowTab=tab;}
	void SetTimeFlags(int tf) {timeFlags=tf;}
	void SetBlankTime(int bt) {blankTime=bt;}
	void SetStandbyTime(int time) {standbyTime=time;}
	void SetSuspendTime(int time) {suspendTime=time;}
	void SetOffTime(int intime) {offTime=intime;}
	void SetBlankCorner(arrowDirection in) {blank=in;}
	void SetNeverBlankCorner(arrowDirection in){neverBlank=in;}
	void SetLockEnable(bool en) {lockenable=en;}
	void SetPasswordTime(int pt) {passwordTime=pt;}
	void SetPassword(char *pw){strncpy(password,pw,B_PATH_NAME_LENGTH-1);}
	void SetNetworkPassword(bool np) {isNetworkPWD=np;}
	void SetModuleName(const char *mn) {strncpy(moduleName,mn,B_PATH_NAME_LENGTH-1);}
	void SetState(BMessage *);
	void SaveSettings (void);
private:
	bool parseSettings(BMessage *newSSMessage);
	BRect windowFrame;
	int windowTab;
	int timeFlags;
	int blankTime; // AKA timefade
	int standbyTime; // AKA timestandby
	int suspendTime; // AKA timesuspend
	int offTime; // AKA timeoff
	arrowDirection blank; // AKA cornernow
	arrowDirection neverBlank; // AKA cornernever
	bool lockenable; 
	int passwordTime; // AKA lockdelay
	char password[B_PATH_NAME_LENGTH]; // aka lockpassword
	bool isNetworkPWD;

	char moduleName[B_PATH_NAME_LENGTH];
	BMessage stateMsg;
};


#endif //SCREEN_SAVER_PREFS_H
