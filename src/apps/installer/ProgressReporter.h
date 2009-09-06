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

			void				ItemsCopied(uint64 items, off_t bytes,
									const char* itemName,
									const char* targetFolder);

private:
			void				_UpdateProgress(const char* itemName,
									const char* targetFolder);

private:
			off_t				fBytesRead;
			uint64				fItemsCopied;
			bigtime_t			fTimeRead;

			off_t				fBytesWritten;
			bigtime_t			fTimeWritten;

			off_t				fBytesToCopy;
			uint64				fItemsToCopy;

			BMessenger			fMessenger;
			BMessage*			fMessage;
};


#endif // PROGRESS_REPORTER_H
