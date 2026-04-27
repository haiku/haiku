/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <package/CleanUpAdminDirectoryRequest.h>

#include <Directory.h>
#include <Entry.h>
#include <JobQueue.h>
#include <Path.h>
#include <RemoveEngine.h>
#include <StringList.h>
#include <StringFormat.h>

#include <StringForSize.h>

#include <package/PackagesDirectoryDefs.h>


namespace BPackageKit {


using namespace BPrivate;


class CleanUpAdminDirectoryJob : public BJob {
	typedef	BJob				inherited;

public:
	CleanUpAdminDirectoryJob(
		const BContext& context, const BString& title,
		const BInstallationLocationInfo& location,
		time_t cleanupBefore, int32 minStatesToKeep);
	virtual ~CleanUpAdminDirectoryJob();

protected:
	virtual	status_t Execute();

private:
	friend class CleanUpAdminDirectoryRequest;

	status_t _GetOldStateDirectories(BStringList& directories);

private:
	const BInstallationLocationInfo& fLocationInfo;
	time_t fCleanupBefore;
	int32 fMinimumStatesToKeep;
};


CleanUpAdminDirectoryJob::CleanUpAdminDirectoryJob(const BContext& context,
	const BString& title, const BInstallationLocationInfo& location,
	time_t cleanupBefore, int32 minStatesToKeep)
	:
	inherited(context, title),
	fLocationInfo(location),
	fCleanupBefore(cleanupBefore),
	fMinimumStatesToKeep(minStatesToKeep)
{
}


CleanUpAdminDirectoryJob::~CleanUpAdminDirectoryJob()
{
}


status_t
CleanUpAdminDirectoryJob::Execute()
{
	BStringList oldStatesAndTransactions;
	status_t status = _GetOldStateDirectories(oldStatesAndTransactions);
	if (status != B_OK)
		return status;
	if (oldStatesAndTransactions.IsEmpty())
		return B_OK;

	size_t totalSize = 0;
	for (int32 i = 0; i < oldStatesAndTransactions.CountStrings(); i++) {
		BDirectory dir(oldStatesAndTransactions.StringAt(i));
		BEntry entry;
		while (dir.GetNextEntry(&entry, true) == B_OK) {
			off_t entrySize;
			if (entry.GetSize(&entrySize) == B_OK)
				totalSize += entrySize;
		}
	}

	static BStringFormat format("{0, plural,"
		"one{Clean up # old state (%size)?} other{Clean up # old states (%size)?}}");

	BString question;
	format.Format(question, oldStatesAndTransactions.CountStrings());

	char buffer[128];
	question.ReplaceAll("%size", string_for_size(totalSize, buffer, sizeof(buffer)));

	bool yes = fContext.DecisionProvider().YesNoDecisionNeeded("", question,
		"yes", "no", "yes");
	if (!yes)
		return B_CANCELED;

	for (int32 i = 0; i < oldStatesAndTransactions.CountStrings(); i++) {
		BString directory = oldStatesAndTransactions.StringAt(i);
		status_t status = BRemoveEngine().RemoveEntry(BRemoveEngine::Entry(
			directory));

		if (status != B_OK) {
			SetErrorString(BString().SetToFormat("Failed to remove %s: %s\n",
				directory.String(), strerror(status)));
			return status;
		}
	}

	return B_OK;
}


status_t
CleanUpAdminDirectoryJob::_GetOldStateDirectories(BStringList& directories)
{
	BDirectory packages(&fLocationInfo.PackagesDirectoryRef());
	BDirectory administrative(&packages, PACKAGES_DIRECTORY_ADMIN_DIRECTORY);
	if (administrative.InitCheck() != B_OK)
		return administrative.InitCheck();

	BStringList states, transactions;
	BEntry entry;
	int32 skippedStates = 0;
	while (administrative.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsDirectory())
			continue;

		time_t mtime;
		if (entry.GetModificationTime(&mtime) != B_OK)
			continue;

		BStringList* list = NULL;
		BString name = entry.Name();
		if (name.StartsWith("transaction-")) {
			if (mtime >= fCleanupBefore)
				continue;

			list = &transactions;
		} else if (name.StartsWith("state_")) {
			if (mtime >= fCleanupBefore) {
				skippedStates++;
				continue;
			}
			if (!fLocationInfo.ActiveStateName().IsEmpty()
					&& name >= fLocationInfo.ActiveStateName()) {
				skippedStates++;
				continue;
			}

			list = &states;
		} else {
			continue;
		}

		list->Add(BPath(&administrative, name).Path());
	}

	states.Sort();
	while (!states.IsEmpty() && skippedStates < fMinimumStatesToKeep) {
		states.Remove(states.CountStrings() - 1);
		skippedStates++;
	}

	directories.MakeEmpty();
	directories.Add(states);
	directories.Add(transactions);
	return B_OK;
}


CleanUpAdminDirectoryRequest::CleanUpAdminDirectoryRequest(const BContext& context,
	const BInstallationLocationInfo& location,
	time_t cleanupBefore, int32 minStatesToKeep)
	:
	inherited(context),
	fLocationInfo(location),
	fCleanupBefore(cleanupBefore),
	fMinimumStatesToKeep(minStatesToKeep)
{
}


CleanUpAdminDirectoryRequest::~CleanUpAdminDirectoryRequest()
{
}


status_t
CleanUpAdminDirectoryRequest::GetOldStatesCount(size_t& count)
{
	status_t status = InitCheck();
	if (status != B_OK)
		return B_NO_INIT;

	CleanUpAdminDirectoryJob temp(fContext, "",
		fLocationInfo, fCleanupBefore, fMinimumStatesToKeep);

	BStringList dirs;
	status = temp._GetOldStateDirectories(dirs);
	if (status != B_OK)
		return status;

	count = dirs.CountStrings();
	return B_OK;
}


status_t
CleanUpAdminDirectoryRequest::CreateInitialJobs()
{
	status_t result = InitCheck();
	if (result != B_OK)
		return B_NO_INIT;

	CleanUpAdminDirectoryJob* cleanUpJob
		= new (std::nothrow) CleanUpAdminDirectoryJob(fContext,
			"", fLocationInfo, fCleanupBefore, fMinimumStatesToKeep);
	if (cleanUpJob == NULL)
		return B_NO_MEMORY;
	if ((result = QueueJob(cleanUpJob)) != B_OK) {
		delete cleanUpJob;
		return result;
	}

	return B_OK;
}


}	// namespace BPackageKit
