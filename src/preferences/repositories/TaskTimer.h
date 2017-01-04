/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */
#ifndef TASKTIMER_H
#define TASKTIMER_H


#include <Alert.h>
#include <Invoker.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <String.h>

#include "RepoRow.h"

class TaskTimer;
class TaskLooper;


typedef struct {
		RepoRow*		rowItem;
		int32			taskType;
		BString			name, taskParam;
		thread_id		threadId;
		TaskLooper*		owner;
		BString			resultName, resultErrorDetails;
		TaskTimer*		fTimer;
} Task;


class TaskTimer : public BLooper {
public:
							TaskTimer(const BMessenger& target, Task* owner);
							~TaskTimer();
	virtual bool			QuitRequested();
	virtual void			MessageReceived(BMessage*);
	void					Start(const char* name);
	void					Stop(const char* name);

private:
	int32					fTimeoutMicroSeconds;
	bool					fTimerIsRunning;
	BString					fRepositoryName;
	BMessenger				fReplyTarget;
	BMessenger				fMessenger;
	BMessageRunner*			fMessageRunner;
	BMessage				fTimeoutMessage;
	BAlert*					fTimeoutAlert;
	BInvoker				fTimeoutAlertInvoker;
	Task*					fOwner;
	int32					_NextAlertStackCount();
};


#endif
