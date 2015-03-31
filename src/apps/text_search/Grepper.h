/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GREPPER_H
#define GREPPER_H

#include <Messenger.h>
#include <String.h>

class FileIterator;
class Model;

// Executes "grep" in a background thread.
class Grepper {
public:
								Grepper(BString pattern, const Model* model,
									const BHandler* target,
									FileIterator* iterator);
	virtual						~Grepper();

			bool				IsValid() const;

			void				Start();
			void				Cancel();

private:
	// Spawns the real grepper thread.
	static	int32				_SpawnThread(void* cookie);

	// The thread function that does the actual grepping.
			int32				_GrepperThread();

	// Counts the number of linebreaks from startPos to endPos.
			int32				_CountLines(BString& str, int32 startPos,
											int32 endPos);
	// Gets the complete line that "pos" is on.
			BString				_GetLine(BString& str, int32 pos);
	private:
	// The search pattern.
			BString				fPattern;

	// The settings from the model.
			BMessenger			fTarget;
			bool				fEscapeText : 1;
			bool				fCaseSensitive : 1;
			uint32				fEncoding;

	// The supplier of files to grep
			FileIterator*		fIterator;

	// Our thread's ID.
			thread_id			fThreadId;

	// Whether our thread must quit.
	volatile bool				fMustQuit;
};

#endif // GREPPER_H
