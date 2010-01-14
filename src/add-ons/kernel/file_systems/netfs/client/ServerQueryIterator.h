// ServerQueryIterator.h

#ifndef NET_FS_SERVER_QUERY_ITERATOR_H
#define NET_FS_SERVER_QUERY_ITERATOR_H

#include "EntryInfo.h"
#include "NodeInfo.h"
#include "QueryIterator.h"
#include "HashString.h"

class ServerQueryIterator : public QueryIterator {
public:
								ServerQueryIterator(Volume* volume);
	virtual						~ServerQueryIterator();

			void				SetRemoteCookie(int32 cookie);
			int32				GetRemoteCookie() const;

			status_t			SetEntry(const int32* shareVolumeIDs,
									int32 shareVolumeCount,
									const NodeInfo& dirInfo,
									const EntryInfo& entryInfo);
			void				UnsetEntry();

			const int32*		GetShareVolumeIDs() const;
			int32				CountShareVolumes() const;
			const NodeInfo&		GetDirectoryInfo() const;
			const EntryInfo&	GetEntryInfo() const;

			bool				HasNextShareVolumeID() const;
			int32				NextShareVolumeID();

private:
			enum { IN_OBJECT_ID_COUNT = 4 };

			int32				fRemoteCookie;
			int32*				fShareVolumeIDs;
			int32				fShareVolumeCount;
			int32				fShareVolumeIndex;
			NodeInfo			fDirectoryInfo;
			EntryInfo			fEntryInfo;
			HashString			fEntryName;
			int32				fInObjectIDs[IN_OBJECT_ID_COUNT];
};

#endif	// NET_FS_SERVER_QUERY_ITERATOR_H
