/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "IMAPFolders.h"


IMAPFolders::IMAPFolders()
{

}


IMAPFolders::IMAPFolders(IMAPProtocol& connection)
	:
	IMAPProtocol(connection)
{
	
}


status_t
IMAPFolders::GetFolders(FolderList& folders)
{
	StringList allFolders;
	status_t status = _GetAllFolders(allFolders);
	if (status != B_OK)
		return status;
	StringList subscibedFolders;
	status = _GetSubscribedFolders(subscibedFolders);
	if (status != B_OK)
		return status;

	for (unsigned int i = 0; i < allFolders.size(); i++) {
		FolderInfo info;
		info.folder = allFolders[i];
		for (unsigned int a = 0; a < subscibedFolders.size(); a++) {
			if (allFolders[i] == subscibedFolders[a]
				|| allFolders[i].ICompare("INBOX") == 0) {
				info.subscribed = true;
				break;
			}
		}
		folders.push_back(info);
	}

	// you could be subscribed to a folder which not exist currently, add them:
	for (unsigned int a = 0; a < subscibedFolders.size(); a++) {
		bool isInlist = false;
		for (unsigned int i = 0; i < allFolders.size(); i++) {
			if (subscibedFolders[a] == allFolders[i]) {
				isInlist = true;
				break;
			}
		}
		if (isInlist)
			continue;

		FolderInfo info;
		info.folder = subscibedFolders[a];
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
