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


DecisionProvider::~DecisionProvider()
{
}


Context::Context(DecisionProvider& decisionProvider)
	:
	fDecisionProvider(decisionProvider),
	fJobStateListener(NULL)
{
	BPath tempPath;
	if (find_directory(B_COMMON_TEMP_DIRECTORY, &tempPath) != B_OK)
		tempPath.SetTo("/tmp");
	BDirectory tempDirectory(tempPath.Path());

	BString contextName = BString("pkgkit-context-") << find_thread(NULL);
	BDirectory baseDirectory;
	tempDirectory.CreateDirectory(contextName.String(), &baseDirectory);
	fTempfileManager.SetBaseDirectory(baseDirectory);
}


Context::~Context()
{
}


TempfileManager&
Context::GetTempfileManager() const
{
	return fTempfileManager;
}


JobStateListener*
Context::GetJobStateListener() const
{
	return fJobStateListener;
}


void
Context::SetJobStateListener(JobStateListener* listener)
{
	fJobStateListener = listener;
}


DecisionProvider&
Context::GetDecisionProvider() const
{
	return fDecisionProvider;
}


}	// namespace Package

}	// namespace Haiku
