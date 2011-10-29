/*
 * Copyright 2011, Haiku Inc. All Rights Reserved.
 * Copyright 2011 Clemens Zeidler. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_FOLDERS_H
#define IMAP_FOLDERS_H


#include <vector>

#include "IMAPHandler.h"
#include "IMAPProtocol.h"


class FolderInfo {
public:
	FolderInfo()
		:
		subscribed(false)
	{
	}

			BString				folder;
			bool				subscribed;
};


typedef std::vector<FolderInfo> FolderList;


class IMAPFolders : public IMAPProtocol {
public:
								IMAPFolders();
								IMAPFolders(IMAPProtocol& connection);

			status_t			GetFolders(FolderList& folders);
			status_t			SubscribeFolder(const char* folder);
			status_t			UnsubscribeFolder(const char* folder);

			status_t			GetQuota(uint64& used, uint64& total);
private:
			status_t			_GetAllFolders(StringList& folders);
			status_t			_GetSubscribedFolders(StringList& folders);

			CapabilityHandler	fCapabilityHandler;
};


#endif // IMAP_FOLDERS_H
