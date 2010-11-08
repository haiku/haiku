/*
 * Copyright 2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef PRINT_ADD_ON_SERVER_H
#define PRINT_ADD_ON_SERVER_H

#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <Message.h>
#include <SupportDefs.h>

#include <PrintAddOnServerProtocol.h>


class PrintAddOnServerApplication : public BApplication
{
public:
					PrintAddOnServerApplication(const char* signature);
			void	MessageReceived(BMessage* message);

private:
			void		AddPrinter(BMessage* message);
			status_t	AddPrinter(const char* driver,
							const char* spoolFolderName);

			void		ConfigPage(BMessage* message);
			status_t	ConfigPage(const char* driver,
							BDirectory* spoolFolder,
							BMessage* settings);

			void		ConfigJob(BMessage* message);
			status_t	ConfigJob(const char* driver,
							BDirectory* spoolFolder,
							BMessage* settings);

			void		DefaultSettings(BMessage* message);
			status_t	DefaultSettings(const char* driver,
							BDirectory* spoolFolder,
							BMessage* settings);

			void		TakeJob(BMessage* message);
			status_t	TakeJob(const char* driver,
							const char* spoolFile,
							BDirectory* spoolFolder);

			void		SendReply(BMessage* message, status_t status);
			void		SendReply(BMessage* message, BMessage* reply);
};

#endif
