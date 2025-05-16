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
								Grepper(const char* pattern, const char* glob, const Model* model,
									const BHandler* target,
									FileIterator* iterator);
	virtual						~Grepper();

			bool				IsValid() const;

			void				Start();
			void				Cancel();

private:
	// Spawns the real grepper threads.
	static	int32				_SpawnRunnerThread(void* cookie);
	static	int32				_SpawnWriterThread(void* cookie);

	// The threads functions that does the actual grepping.
			int32				_RunnerThread();
			int32				_WriterThread();

	// Remembers, and possibly escapes, the search pattern.
			void				_SetPattern(const char* source);

	// Prepends all quotes, dollars and backslashes with at backslash
	// to prevent the shell from misinterpreting them.
			bool				_EscapeSpecialChars(char* buffer,
									ssize_t bufferSize);

	private:
	// The (escaped) search pattern.
			char*				fPattern;
			char*				fGlob;

	// The settings from the model.
			BMessenger			fTarget;
			bool				fRegularExpression : 1;
			bool				fCaseSensitive : 1;
			uint32				fEncoding;

	// The supplier of files to grep
			FileIterator*		fIterator;

	// Our thread's ID.
			thread_id			fRunnerThreadId;
	// xargs input pipe
			int					fXargsInput;

	// Whether our thread must quit.
	volatile bool				fMustQuit;
};

#endif // GREPPER_H
