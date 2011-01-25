/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <new>

#include <package/Context.h>
#include <package/TempfileManager.h>

#include <Directory.h>
#include <FindDirectory.h>
#include <OS.h>
#include <Path.h>


namespace BPackageKit {


using BPrivate::TempfileManager;


BDecisionProvider::~BDecisionProvider()
{
}


BContext::BContext(BDecisionProvider& decisionProvider,
	BJobStateListener& jobStateListener)
	:
	fDecisionProvider(decisionProvider),
	fJobStateListener(jobStateListener),
	fTempfileManager(NULL)
{
	fInitStatus = _Initialize();
}


BContext::~BContext()
{
	delete fTempfileManager;
}


status_t
BContext::InitCheck() const
{
	return fInitStatus;
}


status_t
BContext::GetNewTempfile(const BString& baseName, BEntry* entry) const
{
	if (entry == NULL)
		return B_BAD_VALUE;
	if (fTempfileManager == NULL)
		return B_NO_INIT;
	*entry = fTempfileManager->Create(baseName);
	return entry->InitCheck();
}


BJobStateListener&
BContext::JobStateListener() const
{
	return fJobStateListener;
}


BDecisionProvider&
BContext::DecisionProvider() const
{
	return fDecisionProvider;
}


status_t
BContext::_Initialize()
{
	fTempfileManager = new (std::nothrow) TempfileManager();
	if (fTempfileManager == NULL)
		return B_NO_MEMORY;

	BPath tempPath;
	status_t result = find_directory(B_COMMON_TEMP_DIRECTORY, &tempPath, true);
	if (result != B_OK)
		return result;
	BDirectory tempDirectory(tempPath.Path());
	if ((result = tempDirectory.InitCheck()) != B_OK)
		return result;

	BString contextName = BString("pkgkit-context-") << find_thread(NULL);
	BDirectory baseDirectory;
	result = tempDirectory.CreateDirectory(contextName.String(),
		&baseDirectory);
	if (result != B_OK)
		return result;

	fTempfileManager->SetBaseDirectory(baseDirectory);

	return B_OK;
}


}	// namespace BPackageKit
