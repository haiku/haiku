/*
 * Copyright 2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef PRINT_ADD_ON_SERVER_H
#define PRINT_ADD_ON_SERVER_H


#include <Directory.h>
#include <Entry.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <String.h>
#include <SupportDefs.h>


class PrintAddOnServer
{
public:
						PrintAddOnServer(const char* driver);
	virtual				~PrintAddOnServer();

			status_t	AddPrinter(const char* spoolFolderName);
			status_t	ConfigPage(BDirectory* spoolFolder,
							BMessage* settings);
			status_t	ConfigJob(BDirectory* spoolFolder,
							BMessage* settings);
			status_t	DefaultSettings(BDirectory* spoolFolder,
							BMessage* settings);
			status_t	TakeJob(const char* spoolFile,
							BDirectory* spoolFolder);

	static	status_t	FindPathToDriver(const char* driver, BPath* path);

private:
			const char*	Driver() const;

			status_t	Launch(BMessenger& messenger);
			bool		IsLaunched();
			void		Quit();

			void		AddDirectory(BMessage& message, const char* name,
							BDirectory* directory);
			void		AddEntryRef(BMessage& message, const char* name,
							const entry_ref* entryRef);
			status_t	SendRequest(BMessage& request, BMessage& reply);
			status_t	GetResult(BMessage& reply);
			status_t	GetResultAndUpdateSettings(BMessage& reply,
							BMessage* settings);

	BString		fDriver;
	status_t	fLaunchStatus;
	BMessenger	fMessenger;
};



#endif
