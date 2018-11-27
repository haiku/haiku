/*
 * Copyright 2001-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler, erik@cgsoftware.com
 */
#ifndef _LOOPER_H
#define _LOOPER_H


#include <BeBuild.h>
#include <Handler.h>
#include <List.h>
#include <OS.h>


class BMessage;
class BMessageQueue;
namespace BPrivate {
	class BDirectMessageTarget;
	class BLooperList;
}

// Port (Message Queue) Capacity
#define B_LOOPER_PORT_DEFAULT_CAPACITY	200


class BLooper : public BHandler {
public:
							BLooper(const char* name = NULL,
								int32 priority = B_NORMAL_PRIORITY,
								int32 portCapacity
									= B_LOOPER_PORT_DEFAULT_CAPACITY);
	virtual					~BLooper();

	// Archiving
							BLooper(BMessage* data);
	static	BArchivable*	Instantiate(BMessage* data);
	virtual	status_t		Archive(BMessage* data, bool deep = true) const;

	// Message transmission
			status_t		PostMessage(uint32 command);
			status_t		PostMessage(BMessage* message);
			status_t		PostMessage(uint32 command, BHandler* handler,
								BHandler* replyTo = NULL);
			status_t		PostMessage(BMessage* message, BHandler* handler,
								BHandler* replyTo = NULL);

	virtual	void			DispatchMessage(BMessage* message,
								BHandler* handler);
	virtual	void			MessageReceived(BMessage* message);
			BMessage*		CurrentMessage() const;
			BMessage*		DetachCurrentMessage();
			void			DispatchExternalMessage(BMessage* message,
								BHandler* handler, bool& _detached);
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
			void			Loop();
	virtual	void			Quit();
	virtual	bool			QuitRequested();
			bool			Lock();
			void			Unlock();
			bool			IsLocked() const;
			status_t		LockWithTimeout(bigtime_t timeout);
			thread_id		Thread() const;
			team_id			Team() const;
	static	BLooper*		LooperForThread(thread_id thread);

	// Loop debugging
			thread_id		LockingThread() const;
			int32			CountLocks() const;
			int32			CountLockRequests() const;
			sem_id			Sem() const;

	// Scripting
	virtual BHandler*		ResolveSpecifier(BMessage* message, int32 index,
								BMessage* specifier, int32 what,
								const char* property);
	virtual status_t		GetSupportedSuites(BMessage* data);

	// Message filters (also see BHandler).
	virtual	void			AddCommonFilter(BMessageFilter* filter);
	virtual	bool			RemoveCommonFilter(BMessageFilter* filter);
	virtual	void			SetCommonFilterList(BList* filters);
			BList*			CommonFilterList() const;

	// Private or reserved
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
	friend class ::BPrivate::BLooperList;
	friend port_id _get_looper_port_(const BLooper* );

	virtual	void			_ReservedLooper1();
	virtual	void			_ReservedLooper2();
	virtual	void			_ReservedLooper3();
	virtual	void			_ReservedLooper4();
	virtual	void			_ReservedLooper5();
	virtual	void			_ReservedLooper6();

							BLooper(const BLooper&);
			BLooper&		operator=(const BLooper&);

							BLooper(int32 priority, port_id port,
								const char* name);

			status_t		_PostMessage(BMessage* msg, BHandler* handler,
								BHandler* reply_to);

	static	status_t		_Lock(BLooper* loop, port_id port,
								bigtime_t timeout);
	static	status_t		_LockComplete(BLooper* loop, int32 old,
								thread_id this_tid, sem_id sem,
								bigtime_t timeout);
			void			_InitData(const char* name, int32 priority,
								port_id port, int32 capacity);
			void			AddMessage(BMessage* msg);
			void			_AddMessagePriv(BMessage* msg);
	static	status_t		_task0_(void* arg);

			void*			ReadRawFromPort(int32* code,
								bigtime_t timeout = B_INFINITE_TIMEOUT);
			BMessage*		ReadMessageFromPort(
								bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual	BMessage*		ConvertToMessage(void* raw, int32 code);
	virtual	void			task_looper();
			void			_QuitRequested(BMessage* msg);
			bool			AssertLocked() const;
			BHandler*		_TopLevelFilter(BMessage* msg, BHandler* target);
			BHandler*		_HandlerFilter(BMessage* msg, BHandler* target);
			BHandler*		_ApplyFilters(BList* list, BMessage* msg,
								BHandler* target);
			void			check_lock();
			BHandler*		resolve_specifier(BHandler* target, BMessage* msg);
			void			UnlockFully();

			::BPrivate::BDirectMessageTarget* fDirectTarget;
			BMessage*		fLastMessage;
			port_id			fMsgPort;
			int32			fAtomicCount;
			sem_id			fLockSem;
			int32			fOwnerCount;
			thread_id		fOwner;
			thread_id		fThread;
			addr_t			fCachedStack;
			int32			fInitPriority;
			BHandler*		fPreferred;
			BList			fHandlers;
			BList*			fCommonFilters;
			bool			fTerminating;
			bool			fRunCalled;
			bool			fOwnsPort;
			uint32			_reserved[11];
};

#endif	// _LOOPER_H
