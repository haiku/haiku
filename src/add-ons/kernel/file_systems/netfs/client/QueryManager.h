// QueryManager.h

#ifndef NET_FS_QUERY_MANAGER_H
#define NET_FS_QUERY_MANAGER_H

#include "Locker.h"
#include "QueryIterator.h"

class Volume;
class VolumeManager;

// QueryManager
class QueryManager {
public:
								QueryManager(VolumeManager* volumeManager);
								~QueryManager();

			status_t			Init();

			status_t			AddIterator(QueryIterator* iterator);

			status_t			AddSubIterator(
									HierarchicalQueryIterator* iterator,
									QueryIterator* subIterator);
private:
			status_t			RemoveSubIterator(
									HierarchicalQueryIterator* iterator,
									QueryIterator* subIterator);
public:
// TODO: Remove.

			QueryIterator*		GetCurrentSubIterator(
									HierarchicalQueryIterator* iterator);
			void				NextSubIterator(
									HierarchicalQueryIterator* iterator,
									QueryIterator* subIterator);
private:
			void				RewindSubIterator(
									HierarchicalQueryIterator* iterator);
public:
// TODO: Remove.

			void				PutIterator(QueryIterator* iterator);

			void				VolumeUnmounting(Volume* volume);

private:
			struct IteratorMap;

			Locker				fLock;
			VolumeManager*		fVolumeManager;
			IteratorMap*		fIterators;
};

// QueryIteratorPutter
class QueryIteratorPutter {
public:
	QueryIteratorPutter(QueryManager* manager, QueryIterator* iterator)
		: fManager(manager),
		  fIterator(iterator)
	{
	}

	~QueryIteratorPutter()
	{
		if (fManager && fIterator)
			fManager->PutIterator(fIterator);
	}

	void Detach()
	{
		fManager = NULL;
		fIterator = NULL;
	}

private:
	QueryManager*	fManager;
	QueryIterator*	fIterator;
};

#endif	// NET_FS_QUERY_MANAGER_H
