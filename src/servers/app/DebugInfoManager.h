/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Class used to manage global access to on-screen debugging info
 * in the RootLayer class.
 *
 */

#ifndef DEBUG_INFO_MANAGER_H
#define DEBUG_INFO_MANAGER_H

#include <OS.h>

#if __HAIKU__
# define ON_SCREEN_DEBUGGING_INFO 0
#else
# define ON_SCREEN_DEBUGGING_INFO 0
#endif

#if ON_SCREEN_DEBUGGING_INFO
  extern char* gDebugString;
# define CRITICAL(x) { sprintf(gDebugString, (x)); DebugInfoManager::Default()->AddInfo(gDebugString); }
#else
//# define CRITICAL(x) debugger (x)
# define CRITICAL(x) ;
#endif // ON_SCREEN_DEBUGGING_INFO

class DebugInfoManager {
public:
virtual						~DebugInfoManager();

static	DebugInfoManager	*Default();
		void				AddInfo(const char* string);

private:
							DebugInfoManager();
static	DebugInfoManager	*sDefaultInstance;

#if ON_SCREEN_DEBUGGING_INFO
friend	class RootLayer;
		void				SetRootLayer(RootLayer* layer);

		RootLayer			*fRootLayer;
#endif // ON_SCREEN_DEBUGGING_INFO
};

#endif // DEBUG_INFO_H
