#ifndef RECENT_APPS_TEST_APP_H
#define RECENT_APPS_TEST_APP_H

#include <Application.h>

class RecentAppsTestApp : public BApplication {
public:
	RecentAppsTestApp(const char *sig) : BApplication(sig) {}
	
	virtual void ReadyToRun() {
		Quit();
	}	
};

// App indices, sigs, and filenames:
//  0: Qualifying: BEOS:APP_FLAGS == (B_SINGLE_LAUNCH || B_MULTIPLE_LAUNCH || B_EXCLUSIVE_LAUNCH) 
//  1: Non-qualifying: BEOS:APP_FLAGS &== B_BACKGROUND_APP || B_ARGV_ONLY
//  2: Empty: No BEOS:APP_FLAGS attribute
//  3: Control: (same as qualifying, but with a different sig to make sure qualifying apps work correctly)
enum RecentAppsTestAppId {
	kQualifyingApp = 0,
	kNonQualifyingApp,
	kEmptyApp,
	kControlApp,
};
extern const char *kRecentAppsTestAppSigs[];
extern const char *kRecentAppsTestAppFilenames[];


#endif
