//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		MessageRunnerManager.h
//	Author:			Ingo Weinhold (bonefish@users.sf.net)
//	Description:	Manages the registrar side "shadows" of BMessageRunners.
//------------------------------------------------------------------------------

#ifndef MESSAGE_RUNNER_MANAGER_H
#define MESSAGE_RUNNER_MANAGER_H

#include <List.h>
#include <Locker.h>

class BMessage;
class EventQueue;

class MessageRunnerManager {
public:
	MessageRunnerManager(EventQueue *eventQueue);
	virtual ~MessageRunnerManager();

	void HandleRegisterRunner(BMessage *request);
	void HandleUnregisterRunner(BMessage *request);
	void HandleSetRunnerParams(BMessage *request);
	void HandleGetRunnerInfo(BMessage *request);

	bool Lock();
	void Unlock();

private:
	class RunnerEvent;
	struct RunnerInfo;
	friend class RunnerEvent;

private:
	bool _AddInfo(RunnerInfo *info);
	bool _RemoveInfo(RunnerInfo *info);
	RunnerInfo *_RemoveInfo(int32 index);
	RunnerInfo *_RemoveInfoWithToken(int32 token);
	bool _DeleteInfo(RunnerInfo *info, bool eventRemoved);

	int32 _CountInfos() const;

	RunnerInfo *_InfoAt(int32 index) const;
	RunnerInfo *_InfoForToken(int32 token) const;

	bool _HasInfo(RunnerInfo *info) const;

	int32 _IndexOf(RunnerInfo *info) const;
	int32 _IndexOfToken(int32 token) const;

	bool _DoEvent(RunnerInfo *info);
	bool _ScheduleEvent(RunnerInfo *info);

	int32 _NextToken();

private:
	BList		fRunnerInfos;
	BLocker		fLock;
	EventQueue	*fEventQueue;
	int32		fNextToken;
};

#endif	// MESSAGE_RUNNER_MANAGER_H
