//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
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
//	File Name:		Looper.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//					DarkWyrm (bpmagic@columbus.rr.com)
//	Description:	BLooper class spawns a thread that runs a message loop.
//------------------------------------------------------------------------------

#ifndef _LOOPER_H
#define _LOOPER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <Handler.h>
#include <List.h>
#include <OS.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BMessage;
class BMessageQueue;
struct _loop_data_;

// Port (Message Queue) Capacity -----------------------------------------------
#define B_LOOPER_PORT_DEFAULT_CAPACITY	100


// BLooper class ---------------------------------------------------------------
class BLooper : public BHandler {
public:
						BLooper(const char* name = NULL,
								int32 priority = B_NORMAL_PRIORITY,
								int32 port_capacity = B_LOOPER_PORT_DEFAULT_CAPACITY);
virtual					~BLooper();

// Archiving
						BLooper(BMessage* data);
static	BArchivable*	Instantiate(BMessage* data);
virtual	status_t		Archive(BMessage* data, bool deep = true) const;

// Message transmission
		status_t		PostMessage(uint32 command);
		status_t		PostMessage(BMessage* message);
		status_t		PostMessage(uint32 command,
									BHandler* handler,
									BHandler* reply_to = NULL);
		status_t		PostMessage(BMessage* message,
									BHandler* handler,
									BHandler* reply_to = NULL);

virtual	void			DispatchMessage(BMessage* message, BHandler* handler);
virtual	void			MessageReceived(BMessage* msg);
		BMessage*		CurrentMessage() const;
		BMessage*		DetachCurrentMessage();
		BMessageQueue*	MessageQueue() const;
		bool			IsMessageWaiting() const;

// Message handlers
		void			AddHandler(BHandler* handler);
		bool			RemoveHandler(BHandler* handler);
		int32			CountHandlers() const;
		BHandler*		HandlerAt(int32 index) const;
		int32			IndexOf(BHandler* handler) const;

		BHandler*		PreferredHandler() const;
		void			SetPreferredHandler(BHandler* handler);

// Loop control
virtual	thread_id		Run();
virtual	void			Quit();
virtual	bool			QuitRequested();
		bool			Lock();
		void			Unlock();
		bool			IsLocked() const;
		status_t		LockWithTimeout(bigtime_t timeout);
		thread_id		Thread() const;
		team_id			Team() const;
static	BLooper*		LooperForThread(thread_id tid);

// Loop debugging
		thread_id		LockingThread() const;
		int32			CountLocks() const;
		int32			CountLockRequests() const;
		sem_id			Sem() const;

// Scripting
virtual BHandler*		ResolveSpecifier(BMessage* msg,
										int32 index,
										BMessage* specifier,
										int32 form,
										const char* property);
virtual status_t		GetSupportedSuites(BMessage* data);
		
// Message filters (also see BHandler).
virtual	void			AddCommonFilter(BMessageFilter* filter);
virtual	bool			RemoveCommonFilter(BMessageFilter* filter);
virtual	void			SetCommonFilterList(BList* filters);
		BList*			CommonFilterList() const;

// Private or reserved ---------------------------------------------------------
virtual status_t		Perform(perform_code d, void* arg);

protected:
		// called from overridden task_looper
		BMessage*		MessageFromPort(bigtime_t = B_INFINITE_TIMEOUT);

private:
	typedef BHandler _inherited;
	friend class BWindow;
	friend class BApplication;
	friend class BMessenger;
	friend class BView;
	friend class BHandler;
	friend port_id _get_looper_port_(const BLooper* );
	friend status_t _safe_get_server_token_(const BLooper* , int32* );
	friend team_id	_find_cur_team_id_();

virtual	void			_ReservedLooper1();
virtual	void			_ReservedLooper2();
virtual	void			_ReservedLooper3();
virtual	void			_ReservedLooper4();
virtual	void			_ReservedLooper5();
virtual	void			_ReservedLooper6();

						BLooper(const BLooper&);
		BLooper&		operator=(const BLooper&);

						BLooper(int32 priority, port_id port, const char* name);

		status_t		_PostMessage(BMessage* msg,
									 BHandler* handler,
									 BHandler* reply_to);

static	status_t		_Lock(BLooper* loop,
							  port_id port,
							  bigtime_t timeout);
static	status_t		_LockComplete(BLooper* loop,
									  int32 old,
									  thread_id this_tid,
									  sem_id sem,
									  bigtime_t timeout);
		void			InitData();
		void			InitData(const char* name, int32 prio, int32 capacity);
		void			AddMessage(BMessage* msg);
		void			_AddMessagePriv(BMessage* msg);
static	status_t		_task0_(void* arg);

		void*			ReadRawFromPort(int32* code,
										bigtime_t tout = B_INFINITE_TIMEOUT);
		BMessage*		ReadMessageFromPort(bigtime_t tout = B_INFINITE_TIMEOUT);
virtual	BMessage*		ConvertToMessage(void* raw, int32 code);
virtual	void			task_looper();
		void			do_quit_requested(BMessage* msg);
		bool			AssertLocked() const;
		BHandler*		top_level_filter(BMessage* msg, BHandler* t);
		BHandler*		handler_only_filter(BMessage* msg, BHandler* t);
		BHandler*		apply_filters(	BList* list,
										BMessage* msg,
										BHandler* target);
		void			check_lock();
		BHandler*		resolve_specifier(BHandler* target, BMessage* msg);
		void			UnlockFully();

static	uint32			sLooperID;
static	uint32			sLooperListSize;
static	uint32			sLooperCount;
static	_loop_data_*	sLooperList;
static	BLocker			sLooperListLock;
static	team_id			sTeamID;

static	void			AddLooper(BLooper* l);
static	bool			IsLooperValid(const BLooper* l);
static	void			RemoveLooper(BLooper* l);
static	void			GetLooperList(BList* list);
static	BLooper*		LooperForName(const char* name);
static	BLooper*		LooperForPort(port_id port);
		
		uint32			fLooperID;
		BMessageQueue*	fQueue;
		BMessage*		fLastMessage;
		port_id			fMsgPort;
		long			fAtomicCount;
		sem_id			fLockSem;
		long			fOwnerCount;
		thread_id		fOwner;
		thread_id		fTaskID;
		uint32			_unused1;
		int32			fInitPriority;
		BHandler*		fPreferred;
		BList			fHandlers;
		BList*			fCommonFilters;
		bool			fTerminating;
		bool			fRunCalled;
		thread_id		fCachedPid;
		size_t			fCachedStack;
		void*			fMsgBuffer;
		size_t			fMsgBufferSize;
		uint32			_reserved[6];
};
//------------------------------------------------------------------------------

#endif	// _LOOPER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

