/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef IMAP_FOLDER_CONFIG_H
#define IMAP_FOLDER_CONFIG_H


#include <Button.h>
#include <ListView.h>
#include <StringView.h>
#include <Window.h>

#include <MailSettings.h>

#include "IMAPFolders.h"


class FolderConfigWindow : public BWindow {
public:
								FolderConfigWindow(BRect rect,
									const BMessage& settings);

			void				MessageReceived(BMessage* message);

private:
			void				_LoadFolders();
			void				_ApplyChanges();

			IMAPFolders			fIMAPFolders;
			BListView*			fFolderListView;
			BButton*			fApplyButton;
	const	BMessage			fSettings;
			FolderList			fFolderList;

			BStringView*		fQuotaView;
};


#endif //IMAP_FOLDER_CONFIG_H
