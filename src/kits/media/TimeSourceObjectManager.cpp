/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 *
 ***********************************************************************/

#include <OS.h>
#include <stdio.h>
#include <MediaRoster.h>
#include <Autolock.h>
#include "TimeSourceObjectManager.h"
#include "TimeSourceObject.h"
#include "debug.h"


static BPrivate::media::TimeSourceObjectManager manager;
BPrivate::media::TimeSourceObjectManager *_TimeSourceObjectManager = &manager;

namespace BPrivate {
namespace media {

TimeSourceObjectManager::TimeSourceObjectManager()
 :	fSystemTimeSource(0),
 	fSystemTimeSourceID(0)
{
	CALLED();
	fLock = new BLocker("timesource object manager locker");
	fMap = new Map<media_node_id,BTimeSource *>;
}


TimeSourceObjectManager::~TimeSourceObjectManager()
{
	CALLED();
	delete fLock;

	// force unloading all currently loaded 
	BTimeSource **pts;
	for (fMap->Rewind(); fMap->GetNext(&pts); ) {
		FATAL("Forcing release of TimeSource id %ld...\n", (*pts)->ID());
		int debugcnt = 0;
		while ((*pts)->Release() != NULL)
			debugcnt++;
		FATAL("Forcing release of TimeSource done, released %d times\n", debugcnt);
	}
	
	delete fMap;
}

BTimeSource *
TimeSourceObjectManager::GetTimeSource(const media_node &node)
{
	CALLED();
	BAutolock lock(fLock);

//	printf("TimeSourceObjectManager::GetTimeSource, node id %ld\n", node.node);
	
	if (fSystemTimeSource == 0) {
		media_node clone;
		status_t rv;
		// XXX this clone is never released
		rv = BMediaRoster::Roster()->GetSystemTimeSource(&clone); 
		if (rv != B_OK) {
			FATAL("TimeSourceObjectManager::GetTimeSource, GetSystemTimeSource failed\n");
			return NULL;
		}
		fSystemTimeSource = new SystemTimeSourceObject(clone);
		fSystemTimeSourceID = fSystemTimeSource->ID();
	}
	
	if (node.node == fSystemTimeSourceID)
		return dynamic_cast<BTimeSource *>(fSystemTimeSource->Acquire());
	
	BTimeSource **pts;
	if (fMap->Get(node.node, &pts))
		return dynamic_cast<BTimeSource *>((*pts)->Acquire());
		
	media_node clone;
	status_t rv;
	
	rv = BMediaRoster::Roster()->GetNodeFor(node.node, &clone);
	if (rv != B_OK) {
		FATAL("TimeSourceObjectManager::GetTimeSource, GetNodeFor %ld failed\n", node.node);
		return NULL;
	}
		
	BTimeSource *ts;
	ts = new TimeSourceObject(clone);
	fMap->Insert(clone.node, ts);
	return ts;
}

void
TimeSourceObjectManager::ObjectDeleted(BTimeSource *timesource)
{
	CALLED();
	BAutolock lock(fLock);

//	printf("TimeSourceObjectManager::ObjectDeleted, node id %ld\n", timesource->ID());
	
	bool b;
	b = fMap->Remove(timesource->ID());
	if (!b) {
		FATAL("TimeSourceObjectManager::ObjectDeleted, Remove failed\n");
	}
	
	status_t rv;
	rv = BMediaRoster::Roster()->ReleaseNode(timesource->Node());
	if (rv != B_OK) {
		FATAL("TimeSourceObjectManager::ObjectDeleted, ReleaseNode failed\n");
	}
}

}; // namespace media
}; // namespace BPrivate
