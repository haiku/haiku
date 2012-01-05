/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef FOLDER_CONFIG_WINDOW_H
#define FOLDER_CONFIG_WINDOW_H


#include <ListView.h>
#include <StringView.h>
#include <Window.h>

#include <MailSettings.h>

#include "Protocol.h"
#include "Settings.h"


class FolderConfigWindow : public BWindow {
public:
								FolderConfigWindow(BRect rect,
									const BMessage& settings);

			void				MessageReceived(BMessage* message);

private:
			void				_LoadFolders();
			void				_ApplyChanges();

private:
			const Settings		fSettings;
			IMAP::Protocol		fProtocol;
			IMAP::FolderList	fFolderList;

			BStringView*		fQuotaView;
			BListView*			fFolderListView;
};


#endif // FOLDER_CONFIG_WINDOW_H
