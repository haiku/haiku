/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPFolders.h"


IMAPFolders::IMAPFolders()
{
	fHandlerList.AddItem(&fCapabilityHandler);
}


IMAPFolders::IMAPFolders(IMAPProtocol& connection)
	:
	IMAPProtocol(connection)
{
	fHandlerList.AddItem(&fCapabilityHandler);
}


status_t
IMAPFolders::GetFolders(FolderList& folders)
{
	StringList allFolders;
	status_t status = _GetAllFolders(allFolders);
	if (status != B_OK)
		return status;
	StringList subscribedFolders;
	status = _GetSubscribedFolders(subscribedFolders);
	if (status != B_OK)
		return status;

	for (unsigned int i = 0; i < allFolders.size(); i++) {
		FolderInfo info;
		info.folder = allFolders[i];
		for (unsigned int a = 0; a < subscribedFolders.size(); a++) {
			if (allFolders[i] == subscribedFolders[a]
				|| allFolders[i].ICompare("INBOX") == 0) {
				info.subscribed = true;
				break;
			}
		}
		folders.push_back(info);
	}

	// you could be subscribed to a folder which not exist currently, add them:
	for (unsigned int a = 0; a < subscribedFolders.size(); a++) {
		bool isInlist = false;
		for (unsigned int i = 0; i < allFolders.size(); i++) {
			if (subscribedFolders[a] == allFolders[i]) {
				isInlist = true;
				break;
			}
		}
		if (isInlist)
			continue;

		FolderInfo info;
		info.folder = subscribedFolders[a];
		info.subscribed = true;
		folders.push_back(info);
	}

	return B_OK;
}


status_t
IMAPFolders::SubscribeFolder(const char* folder)
{
	SubscribeCommand command(folder);
	return ProcessCommand(&command);
}


status_t
IMAPFolders::UnsubscribeFolder(const char* folder)
{
	UnsubscribeCommand command(folder);
	return ProcessCommand(&command);
}


status_t
IMAPFolders::GetQuota(uint64& used, uint64& total)
{
	if (fCapabilityHandler.Capabilities() == "")
		ProcessCommand(fCapabilityHandler.Command());
	if (fCapabilityHandler.Capabilities().FindFirst("QUOTA") < 0)
		return B_ERROR;

	GetQuotaCommand quotaCommand;
	status_t status = ProcessCommand(&quotaCommand);
	if (status != B_OK)
		return status;

	used = quotaCommand.UsedStorage();
	total = quotaCommand.TotalStorage();
	return B_OK;
}


status_t
IMAPFolders::_GetAllFolders(StringList& folders)
{
	ListCommand listCommand;
	status_t status = ProcessCommand(&listCommand);
	if (status != B_OK)
		return status;

	folders = listCommand.FolderList();
	return status;
}


status_t
IMAPFolders::_GetSubscribedFolders(StringList& folders)
{
	ListSubscribedCommand listSubscribedCommand;
	status_t status = ProcessCommand(&listSubscribedCommand);
	if (status != B_OK)
		return status;

	folders = listSubscribedCommand.FolderList();
	return status;
}
