/*
 * Copyright 2011-2012, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef DEFAULT_NOTIFIER_H
#define DEFAULT_NOTIFIER_H


#include <Notification.h>
#include <String.h>

#include "MailProtocol.h"

#include "ErrorLogWindow.h"
#include "StatusWindow.h"


class DefaultNotifier : public BMailNotifier {
public:
								DefaultNotifier(const char* accountName,
									bool inbound, ErrorLogWindow* errorWindow,
									uint32 showMode);
								~DefaultNotifier();

			BMailNotifier*		Clone();

			void				ShowError(const char* error);
			void				ShowMessage(const char* message);

			void				SetTotalItems(uint32 items);
			void				SetTotalItemsSize(uint64 size);
			void				ReportProgress(uint32 messages, uint64 bytes,
									const char* message = NULL);
			void				ResetProgress(const char* message = NULL);

private:
			void				_NotifyIfAllowed(int timeout = 0);

private:
			BString				fAccountName;
			bool				fIsInbound;
			ErrorLogWindow*		fErrorWindow;
			BNotification		fNotification;
			uint32&				fShowMode;

			uint32				fTotalItems;
			uint32				fItemsDone;
			uint64				fTotalSize;
			uint64				fSizeDone;
};


#endif // DEFAULT_NOTIFIER_H
