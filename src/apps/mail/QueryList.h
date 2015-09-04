/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef QUERY_LIST_H
#define QUERY_LIST_H


#include <map>
#include <vector>

#include <Entry.h>
#include <Handler.h>
#include <Locker.h>
#include <ObjectList.h>
#include <Query.h>


struct QueryList;


class QueryListener {
public:
	virtual 				~QueryListener();
	virtual	void				EntryCreated(QueryList& source,
									const entry_ref& ref, ino_t node) = 0;
	virtual	void				EntryRemoved(QueryList& source,
									const node_ref& nodeRef) = 0;
};


typedef std::map<node_ref, entry_ref> RefMap;


class QueryList : public BHandler, public BLocker {
public:
								QueryList();
	virtual						~QueryList();

			status_t			Init(const char* predicate,
									BVolume* volume = NULL);

			void				AddListener(QueryListener* listener);
			void				RemoveListener(QueryListener* listener);

			const RefMap&		Entries() const
									{ return fRefs; }

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_AddEntry(const entry_ref& ref, ino_t node);
			void				_RemoveEntry(const node_ref& nodeRef);
			void				_NotifyEntryCreated(const entry_ref& ref,
									ino_t node);
			void				_NotifyEntryRemoved(const node_ref& nodeRef);
			void				_AddVolume(BVolume& volume,
									const char* predicate);

	static	status_t			_FetchQuery(void* self);
			status_t			_FetchQuery();

private:
	typedef std::vector<thread_id> ThreadVector;
	typedef std::vector<BQuery*> QueryVector;

			bool				fQuit;
			RefMap				fRefs;
			QueryVector			fQueries;
			QueryVector			fQueryQueue;
			ThreadVector		fFetchThreads;
			BObjectList<QueryListener> fListeners;
};


#endif // QUERY_LIST_H
