/***********************************************************************
 * Copyright (c) 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the OpenBeOS License.
 *
 * The object returned by BMediaRoster's
 * BTimeSource * MakeTimeSourceFor(const media_node & for_node);
 *
 ***********************************************************************/

#include <OS.h>
#include <stdio.h>
#include <MediaRoster.h>
#include "MediaMisc.h"
#include "TimeSourceObject.h"
#include "TimeSourceObjectManager.h"

TimeSourceObject::TimeSourceObject(const media_node &node)
 :	BMediaNode("some timesource object", node.node, node.kind),
	BTimeSource(node.node)
{
//	printf("TimeSourceObject::TimeSourceObject enter, id = %ld\n", id);
	if (fControlPort > 0)
		delete_port(fControlPort);
	fControlPort = node.port;
	ASSERT(fNodeID == node.node);
	ASSERT(fKinds == node.kind);

	live_node_info lni;
	if (B_OK == BMediaRoster::Roster()->GetLiveNodeInfo(node, &lni)) {
		strcpy(fName, lni.name);
	} else {
		sprintf(fName, "timesource %ld", node.node);
	}

	AddNodeKind(NODE_KIND_SHADOW_TIMESOURCE);
	AddNodeKind(NODE_KIND_NO_REFCOUNTING);
	fControlPort = SHADOW_TIMESOURCE_CONTROL_PORT; // XXX if we don't do this, we get a infinite loop somewhere. This needs to be debugged
	
//	printf("TimeSourceObject::TimeSourceObject leave, node id %ld\n", fNodeID);
}

/* virtual */ status_t
TimeSourceObject::TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved)
{
	return B_OK;
}

/* virtual */ BMediaAddOn* 
TimeSourceObject::AddOn(int32 *internal_id) const
{
	if (internal_id)
		*internal_id = 0;
	return NULL;
}

/* virtual */ status_t
TimeSourceObject::DeleteHook(BMediaNode *node)
{
	status_t status;
//	printf("TimeSourceObject::DeleteHook enter\n");
	_TimeSourceObjectManager->ObjectDeleted(this);
	status = BTimeSource::DeleteHook(node);
//	printf("TimeSourceObject::DeleteHook leave\n");
	return status;
}


SystemTimeSourceObject::SystemTimeSourceObject(const media_node &node)
 : 	BMediaNode("System Clock", node.node, node.kind),
	TimeSourceObject(node)
{
//	printf("SystemTimeSourceObject::SystemTimeSourceObject enter, id = %ld\n", id);
	fIsRealtime = true;
	AddNodeKind(NODE_KIND_SYSTEM_TIMESOURCE);
	AddNodeKind(NODE_KIND_NO_REFCOUNTING);
//	printf("SystemTimeSourceObject::SystemTimeSourceObject leave, node id %ld\n", ID());
}

/* virtual */ status_t
SystemTimeSourceObject::DeleteHook(BMediaNode * node)
{
	FATAL("SystemTimeSourceObject::DeleteHook called\n");
	return B_ERROR;
}
