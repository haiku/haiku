/*
 * Copyright 2001-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef LOOPER_LIST_H
#define LOOPER_LIST_H


#include <vector>

#include <Locker.h>
#include <OS.h>
#include <SupportDefs.h>


class BList;
class BLooper;


namespace BPrivate {


class BLooperList {
public:
								BLooperList();

			bool				Lock();
			void				Unlock();
			bool				IsLocked();

			void				AddLooper(BLooper* l);
			bool				IsLooperValid(const BLooper* l);
			bool				RemoveLooper(BLooper* l);
			void				GetLooperList(BList* list);
			int32				CountLoopers();
			BLooper*			LooperAt(int32 index);
			BLooper*			LooperForThread(thread_id tid);
			BLooper*			LooperForName(const char* name);
			BLooper*			LooperForPort(port_id port);

			void				InitAfterFork();

private:
	struct LooperData {
		LooperData();
		LooperData(BLooper* looper);
		LooperData(const LooperData& rhs);
		LooperData& operator=(const LooperData& rhs);

		BLooper*	looper;
	};
	struct FindLooperPred {
		FindLooperPred(const BLooper* loop) : looper(loop) {}
		bool operator()(LooperData& Data);
		const BLooper* looper;
	};
	struct FindThreadPred {
		FindThreadPred(thread_id tid) : thread(tid) {}
		bool operator()(LooperData& Data);
		thread_id thread;
	};
	struct FindNamePred {
		FindNamePred(const char* n) : name(n) {}
		bool operator()(LooperData& Data);
		const char* name;
	};
	struct FindPortPred {
		FindPortPred(port_id pid) : port(pid) {}
		bool operator()(LooperData& Data);
		port_id port;
	};

	static	bool				EmptySlotPred(LooperData& Data);
			void				AssertLocked();

private:
			BLocker				fLock;
			std::vector<LooperData>	fData;
};


extern BLooperList gLooperList;


}	// namespace BPrivate


#endif	// LOOPER_LIST_H
