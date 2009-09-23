/*
 * Copyright 2001-2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _HANDLER_H
#define _HANDLER_H


#include <Archivable.h>


class BLooper;
class BMessageFilter;
class BMessage;
class BMessenger;
class BList;

#define B_OBSERVE_WHAT_CHANGE "be:observe_change_what"
#define B_OBSERVE_ORIGINAL_WHAT "be:observe_orig_what"
const uint32 B_OBSERVER_OBSERVE_ALL = 0xffffffff;

namespace BPrivate {
	class ObserverList;
}

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
	virtual BHandler*		ResolveSpecifier(BMessage* msg, int32 index,
								BMessage* specifier, int32 form,
								const char* property);
	virtual status_t		GetSupportedSuites(BMessage* data);

	// Observer calls, inter-looper and inter-team
			status_t		StartWatching(BMessenger target, uint32 what);
			status_t		StartWatchingAll(BMessenger target);
			status_t		StopWatching(BMessenger target, uint32 what);
			status_t		StopWatchingAll(BMessenger target);

	// Observer calls for observing targets in the local team
			status_t		StartWatching(BHandler* observer, uint32 what);
			status_t		StartWatchingAll(BHandler* observer);
			status_t		StopWatching(BHandler* observer, uint32 what);
			status_t		StopWatchingAll(BHandler* observer);


	// Reserved
	virtual status_t		Perform(perform_code d, void* arg);

	// Notifier calls
	virtual	void 			SendNotices(uint32 what, const BMessage* notice = NULL);
			bool			IsWatched() const;

private:
	typedef BArchivable		_inherited;
	friend inline int32		_get_object_token_(const BHandler* );
	friend class BLooper;
	friend class BMessageFilter;

	virtual	void			_ReservedHandler2();
	virtual	void			_ReservedHandler3();
	virtual	void			_ReservedHandler4();

			void			_InitData(const char* name);
			BPrivate::ObserverList* _ObserverList();

							BHandler(const BHandler&);
			BHandler&		operator=(const BHandler&);
			void			SetLooper(BLooper* looper);

			int32			fToken;
			char*			fName;
			BLooper*		fLooper;
			BHandler*		fNextHandler;
			BList*			fFilters;
			BPrivate::ObserverList*	fObserverList;
			uint32			_reserved[3];
};

#endif	// _HANDLER_H
