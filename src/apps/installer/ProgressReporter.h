/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PROGRESS_REPORTER_H
#define PROGRESS_REPORTER_H


#include <Messenger.h>


class ProgressReporter {
public:
								ProgressReporter(const BMessenger& messenger,
									BMessage* message);
	virtual						~ProgressReporter();

			void				Reset();

			void				AddItems(uint64 count, off_t bytes);

			void				StartTimer();

			void				ItemsWritten(uint64 items, off_t bytes,
									const char* itemName,
									const char* targetFolder);

			// TODO: Perhaps move cancelling here as well...

private:
			void				_UpdateProgress(const char* itemName,
									const char* targetFolder);

private:
			bigtime_t			fStartTime;

			off_t				fBytesToWrite;
			off_t				fBytesWritten;

			uint64				fItemsToWrite;
			uint64				fItemsWritten;

			BMessenger			fMessenger;
			BMessage*			fMessage;
};


#endif // PROGRESS_REPORTER_H
