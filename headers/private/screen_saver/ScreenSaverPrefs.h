#ifndef SCREEN_SAVER_PREFS_H
#define SCREEN_SAVER_PREFS_H
#include "Message.h"
#include "Path.h"
#include <string.h>

enum arrowDirection {NONE=-1,UPLEFT,UPRIGHT,DOWNRIGHT,DOWNLEFT,CENTER};

class ScreenSaverPrefs 
{
public:
	ScreenSaverPrefs(void);
	bool LoadSettings(void);

	BRect WindowFrame(void) {return getRect("windowframe");}
	int WindowTab(void) {return getInt("windowtab");}
	int TimeFlags(void) {return getInt("timeflags");}
	int BlankTime(void) {return getInt("timefade");}
	int StandbyTime(void) {return getInt("timestandby");}
	int SuspendTime(void) {return getInt("timesuspend");}
	int OffTime(void) {return getInt("timeoff");}
	arrowDirection GetBlankCorner(void) {return (arrowDirection)(getInt("cornernow"));}
	arrowDirection GetNeverBlankCorner(void) {return (arrowDirection)(getInt("cornerNever"));}
	bool LockEnable(void) {return getBool("lockenable");}
	int PasswordTime(void) {return getInt("lockdelay");}
	const char *Password(void);
	bool IsNetworkPassword(void) {return (0==strcmp("custom",getString("lockmethod")));}
	const char *ModuleName(void) {
		const char *name=getString("modulename");
		return ((name)?name:"Blackness");
		}
	BMessage *GetState(const char *name);
	BMessage *GetSettings(void) {return &currentMessage;}  

	void SetWindowFrame(const BRect &fr) {setRect("windowframe",fr);}
	void SetWindowTab(int tab) {setInt("windowtab",tab);}
	void SetTimeFlags(int tf) {setInt("timeflags",tf);}
	void SetBlankTime(int bt) {setInt("timefade",bt);}
	void SetStandbyTime(int time) {setInt("timestandby",time);}
	void SetSuspendTime(int time) {setInt("timesuspend",time);}
	void SetOffTime(int intime) {setInt("timeoff",intime);}
	void SetBlankCorner(arrowDirection in) {setInt("cornernow",(int)in);}
	void SetNeverBlankCorner(arrowDirection in){setInt("cornernever",(int)in);}
	void SetLockEnable(bool en) {setBool("lockenable",en);}
	void SetPasswordTime(int pt) {setInt("lockdelay",pt);}
	void SetPassword(char *pw){setString("lockpassword",pw);} // Can not set network password from here.
	void SetModuleName(const char *mn) {setString("modulename",mn);}
	void SetState(const char *name,BMessage *in);
	void SaveSettings (void);
private:
	bool parseSettings(BMessage *newSSMessage);

	bool getBool(const char *name) {bool b;currentMessage.FindBool(name,&b);return b;}
	int32 getInt(const char *name){int32 b;currentMessage.FindInt32(name,&b);return b;}
	const char *getString(const char *name){const char *b;currentMessage.FindString(name,&b);return b;}
	BRect getRect(const char *name){BRect b;currentMessage.FindRect(name,&b);return b;}

	void setBool(const char *name,bool value)  {
	    if (B_NAME_NOT_FOUND == currentMessage.ReplaceBool(name,value))
        	currentMessage.AddBool(name,value); 
	}

	void setInt(const char *name,int value) {
	    if (B_NAME_NOT_FOUND == currentMessage.ReplaceInt32(name,value))
        	currentMessage.AddInt32(name,value); 
	}

	void setString(const char *name,const char *value) {
	    if (B_NAME_NOT_FOUND == currentMessage.ReplaceString(name,value))
        	currentMessage.AddString(name,value); 
	}

	void setRect(const char *name,BRect value) {
	    if (B_NAME_NOT_FOUND == currentMessage.ReplaceRect(name,value))
        	currentMessage.AddRect(name,value); 
	}

	BMessage currentMessage,stateMsg;
	BPath ssPath,networkPath;
	char password[512];
};


#endif //SCREEN_SAVER_PREFS_H
