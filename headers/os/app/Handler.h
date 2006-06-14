/*
 * Copyright 2001-2006, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _HANDLER_H
#define _HANDLER_H


#include <BeBuild.h>
#include <Archivable.h>


class BLooper;
class BMessageFilter;
class BMessage;
class BList;
class _ObserverList;

#define B_OBSERVE_WHAT_CHANGE "be:observe_change_what"
#define B_OBSERVE_ORIGINAL_WHAT "be:observe_orig_what"
const uint32 B_OBSERVER_OBSERVE_ALL = 0xffffffff;


class BHandler : public BArchivable {
public:
							BHandler(const char* name = NULL);
	virtual					~BHandler();

	// Archiving
							BHandler(BMessage* data);
	static	BArchivable*	Instantiate(BMessage* data);
	virtual	status_t		Archive(BMessage* data, bool deep = true) const;

	// BHandler guts.
	virtual	void			MessageReceived(BMessage* message);
			BLooper*		Looper() const;
			void			SetName(const char* name);
			const char*		Name() const;
	virtual	void			SetNextHandler(BHandler* handler);
			BHandler*		NextHandler() const;

	// Message filtering
	virtual	void			AddFilter(BMessageFilter* filter);
	virtual	bool			RemoveFilter(BMessageFilter* filter);
	virtual	void			SetFilterList(BList* filters);
			BList*			FilterList();

			bool			LockLooper();
			status_t		LockLooperWithTimeout(bigtime_t timeout);
			void			UnlockLooper();

	// Scripting
	virtual BHandler*		ResolveSpecifier(BMessage* msg,
											 int32 index,
											 BMessage* specifier,
											 int32 form,
											 const char* property);
	virtual status_t		GetSupportedSuites(BMessage* data);

	// Observer calls, inter-looper and inter-team
			status_t		StartWatching(BMessenger, uint32 what);
			status_t		StartWatchingAll(BMessenger);
			status_t		StopWatching(BMessenger, uint32 what);
			status_t		StopWatchingAll(BMessenger);

	// Observer calls for observing targets in the same BLooper
			status_t		StartWatching(BHandler* , uint32 what);
			status_t		StartWatchingAll(BHandler* );
			status_t		StopWatching(BHandler* , uint32 what);
			status_t		StopWatchingAll(BHandler* );


	// Reserved
	virtual status_t		Perform(perform_code d, void* arg);

	// Notifier calls
	virtual	void 			SendNotices(uint32 what, const BMessage*  = 0);
			bool			IsWatched() const;

private:
	typedef BArchivable		_inherited;
	friend inline int32		_get_object_token_(const BHandler* );
	friend class BLooper;
	friend class BMessageFilter;

	virtual	void			_ReservedHandler2();
	virtual	void			_ReservedHandler3();
	virtual	void			_ReservedHandler4();

			void			InitData(const char* name);

							BHandler(const BHandler&);
			BHandler&		operator=(const BHandler&);
			void			SetLooper(BLooper* loop);

			int32			fToken;
			char*			fName;
			BLooper*		fLooper;
			BHandler*		fNextHandler;
			BList*			fFilters;
			_ObserverList*	fObserverList;
			uint32			_reserved[3];
};

#endif	// _HANDLER_H
