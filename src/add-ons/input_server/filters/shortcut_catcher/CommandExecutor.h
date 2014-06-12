/*
 * Copyright 1999-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */
#ifndef _COMMAND_EXECUTOR_H
#define _COMMAND_EXECUTOR_H


#include <Looper.h>
#include <Message.h>
#include <OS.h>


// This thread receives BMessages telling it what
// to launch, and launches them.

class CommandExecutor : public BLooper {
public:
								CommandExecutor();
	virtual						~CommandExecutor();

	virtual	void				MessageReceived(BMessage* message);

private:
			bool				GetNextWord(char** setBeginWord,
									char** setEndWord) const;
};


#endif	// _COMMAND_EXECUTOR_H
