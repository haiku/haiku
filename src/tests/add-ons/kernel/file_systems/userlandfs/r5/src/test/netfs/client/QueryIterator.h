// QueryIterator.h

#ifndef NET_FS_QUERY_ITERATOR_H
#define NET_FS_QUERY_ITERATOR_H

#include "DLList.h"
#include "Referencable.h"

class HierarchicalQueryIterator;
class Volume;

// QueryIterator
class QueryIterator : public Referencable,
	public DLListLinkImpl<QueryIterator> {
public:
								QueryIterator(Volume* volume);
	virtual						~QueryIterator();

			Volume*				GetVolume() const;

			void				SetParentIterator(
										HierarchicalQueryIterator* parent);
			HierarchicalQueryIterator* GetParentIterator() const;

	virtual	status_t			ReadQuery(struct dirent* buffer,
									size_t bufferSize, int32 count,
									int32* countRead, bool* done);

			struct GetVolumeLink;
			friend struct GetVolumeLink;

private:
			Volume*				fVolume;
			HierarchicalQueryIterator* fParentIterator;
			DLListLink<QueryIterator> fVolumeLink;
};

// HierarchicalQueryIterator
class HierarchicalQueryIterator : public QueryIterator {
public:
								HierarchicalQueryIterator(Volume* volume);
	virtual						~HierarchicalQueryIterator();

			QueryIterator*		GetCurrentSubIterator() const;
			QueryIterator*		NextSubIterator();
			void				RewindSubIterator();
			void				AddSubIterator(QueryIterator* subIterator);
			void				RemoveSubIterator(QueryIterator* subIterator);
			void				RemoveAllSubIterators(
									DLList<QueryIterator>& subIterators);

private:
			DLList<QueryIterator> fSubIterators;
			QueryIterator*		fCurrentSubIterator;
};

// GetVolumeLink
struct QueryIterator::GetVolumeLink {
	DLListLink<QueryIterator>* operator()(QueryIterator* iterator) const
	{
		return &iterator->fVolumeLink;
	}

	const DLListLink<QueryIterator>* operator()(
		const QueryIterator* iterator) const
	{
		return &iterator->fVolumeLink;
	}
};

#endif	// NET_FS_QUERY_ITERATOR_H
