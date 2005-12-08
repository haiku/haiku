/*
 * Copyright 2005, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Class used to manage global access to on-screen debugging info
 * in the RootLayer class.
 *
 */

//#include "RootLayer.h"

#include "DebugInfoManager.h"

// init globals
DebugInfoManager*
DebugInfoManager::sDefaultInstance = NULL;

#if ON_SCREEN_DEBUGGING_INFO
char* gDebugString = new char[2048]; 
#endif

// destructor
DebugInfoManager::~DebugInfoManager()
{
}

// Default
DebugInfoManager*
DebugInfoManager::Default()
{
	if (!sDefaultInstance)	
		sDefaultInstance = new DebugInfoManager();
	return sDefaultInstance;
}

// AddInfo
void
DebugInfoManager::AddInfo(const char* string)
{
#if ON_SCREEN_DEBUGGING_INFO
	if (fRootLayer) {
		fRootLayer->AddDebugInfo(string);
	}
#endif // ON_SCREEN_DEBUGGING_INFO
}

// constructor
DebugInfoManager::DebugInfoManager()
#if ON_SCREEN_DEBUGGING_INFO
	: fRootLayer(NULL)
{
	gDebugString[0] = 0;
}
#else
{
}
#endif // ON_SCREEN_DEBUGGING_INFO

#if ON_SCREEN_DEBUGGING_INFO
// SetRootLayer
void
DebugInfoManager::SetRootLayer(RootLayer* layer)
{
	fRootLayer = layer;
}
#endif // ON_SCREEN_DEBUGGING_INFO

