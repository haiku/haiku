#include "RecentAppsTestApp.h"

#ifdef TEST_R5
#define APP_SIG_SUFFIX "-r5"
#else
#define APP_SIG_SUFFIX 
#endif


// Test app signatures
const char *kRecentAppsTestAppSigs[] = {
	"application/x-vnd.obos-recent-apps-test-qualifying" APP_SIG_SUFFIX,
	"application/x-vnd.obos-recent-apps-test-non-qualifying" APP_SIG_SUFFIX,
	"application/x-vnd.obos-recent-apps-test-empty" APP_SIG_SUFFIX,
	"application/x-vnd.obos-recent-apps-test-control" APP_SIG_SUFFIX,
};

const char *kRecentAppsTestAppFilenames[] = {
	"RecentAppsTestQualifyingApp",
	"RecentAppsTestNonQualifyingApp",
	"RecentAppsTestEmptyApp",
	"RecentAppsTestControlApp",
};
	
