#ifndef __TPREFS_H__
#define __TPREFS_H__

//Volume II, Issue 35; September 2, 1998 (Eric Shepherd)
/*
Usage
  TPreferences prefs("PrefsSample_prefs");   // Preferences
  if (prefs.InitCheck() != B_OK) {
    prefs.SetInt64("last_used", real_time_clock());
    prefs.SetInt32("use_count", 0);
  }
  
Then we call PrintToStream() to print out the contents of the TPreferences object:

prefs.PrintToStream();

Finally, we update the preferences:

      int32 count;
      if (prefs.FindInt32("use_count", &count) != B_OK) {
        count = 0;
      }
      prefs.SetInt64("last_used", real_time_clock());
      prefs.SetInt32("use_count", ++count);
  
*/
#include <Message.h>
#include <Path.h>
#include <FindDirectory.h>

class TPreferences : public BMessage {
	public:
					TPreferences(directory_which directory, const char* filename);
		virtual  	~TPreferences();
		

	    status_t    InitCheck(void);
		status_t	SetBool(const char *name, bool b);
    	status_t    SetInt8(const char *name, int8 i);
    	status_t    SetInt16(const char *name, int16 i);
    	status_t    SetInt32(const char *name, int32 i);
    	status_t    SetInt64(const char *name, int64 i);
    	status_t    SetFloat(const char *name, float f);
	    status_t    SetDouble(const char *name, double d);
    	status_t    SetString(const char *name,
                          const char *string);
    	status_t    SetPoint(const char *name, BPoint p);
    	status_t    SetRect(const char *name, BRect r);
    	status_t    SetMessage(const char *name,
                           const BMessage *message);
    	status_t    SetFlat(const char *name,
                        const BFlattenable *obj);
	private:
    	BPath      fPath;
    	status_t    fStatus;
};

inline
status_t TPreferences::InitCheck(void)
{
	return fStatus;
}

#if __POWERPC__ 
#pragma export off
#endif 

#endif  // __TPREFS_H__
