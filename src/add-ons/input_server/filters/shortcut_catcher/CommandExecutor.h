/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 */


#ifndef CommandExecutor_h
#define CommandExecutor_h

#include <Looper.h>
#include <Message.h>
#include <OS.h>

// This thread receives BMessages telling it what
// to launch, and launches them.
class CommandExecutor : public BLooper {
public:
							CommandExecutor();
							~CommandExecutor();

	virtual	void			MessageReceived(BMessage* msg);

private:
			bool			GetNextWord(char** setBeginWord, char** setEndWord)
								const;
};

#endif
