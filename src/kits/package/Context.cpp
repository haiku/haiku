/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/Context.h>

#include <Directory.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>


namespace Haiku {

namespace Package {


Context::Context()
	:
	fDefaultJobStateListener(NULL)
{
	BPath tempPath;
	if (find_directory(B_COMMON_TEMP_DIRECTORY, &tempPath) != B_OK)
		tempPath.SetTo("/tmp");
	BDirectory tempDirectory(tempPath.Path());

	BString contextName = BString("pkgkit-context-") << find_thread(NULL);
	BDirectory baseDirectory;
	tempDirectory.CreateDirectory(contextName.String(), &baseDirectory);
	fTempEntryManager.SetBaseDirectory(baseDirectory);
}


Context::~Context()
{
}


TempEntryManager&
Context::GetTempEntryManager() const
{
	return fTempEntryManager;
}


JobStateListener*
Context::DefaultJobStateListener() const
{
	return fDefaultJobStateListener;
}


void
Context::SetDefaultJobStateListener(JobStateListener* listener)
{
	fDefaultJobStateListener = listener;
}


}	// namespace Package

}	// namespace Haiku
