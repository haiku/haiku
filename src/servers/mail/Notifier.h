/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2011, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef NOTIFIER_H
#define NOTIFIER_H


#include <Notification.h>
#include <String.h>

#include "MailProtocol.h"

#include "ErrorLogWindow.h"
#include "StatusWindow.h"


class DefaultNotifier : public MailNotifier {
public:
								DefaultNotifier(const char* accountName,
									bool inbound, ErrorLogWindow* errorWindow,
									uint32& showMode);
								~DefaultNotifier();

			MailNotifier*		Clone();

			void				ShowError(const char* error);
			void				ShowMessage(const char* message);

			void				SetTotalItems(int32 items);
			void				SetTotalItemsSize(int32 size);
			void				ReportProgress(int bytes, int messages,
									const char* message = NULL);
			void				ResetProgress(const char* message = NULL);

private:
			BString				fAccountName;
			bool				fIsInbound;
			ErrorLogWindow*		fErrorWindow;
			BNotification		fNotification;
			uint32&				fShowMode;

			int					fTotalItems;
			int					fItemsDone;
			int					fTotalSize;
			int					fSizeDone;
};

#endif //NOTIFIER_H
