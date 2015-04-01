/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef GREPPER_H
#define GREPPER_H

#include <Messenger.h>

class FileIterator;
class Model;

// Executes "grep" in a background thread.
class Grepper {
public:
								Grepper(const char* pattern, const Model* model,
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

	// Remembers, and possibly escapes, the search pattern.
			void				_SetPattern(const char* source);

	// Prepends all quotes, dollars and backslashes with at backslash
	// to prevent the shell from misinterpreting them.
			bool				_EscapeSpecialChars(char* buffer,
									ssize_t bufferSize);
	private:
	// The (escaped) search pattern.
			char*				fPattern;

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
