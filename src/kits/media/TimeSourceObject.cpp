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
#include <string.h>
#include <MediaRoster.h>
#include "MediaMisc.h"
#include "TimeSourceObject.h"
#include "TimeSourceObjectManager.h"

TimeSourceObject::TimeSourceObject(const media_node &node)
 :	BMediaNode("some timesource object", node.node, node.kind),
	BTimeSource(node.node)
{
	TRACE("TimeSourceObject::TimeSourceObject enter, id = %ld\n", node.node);
	if (fControlPort > 0)
		delete_port(fControlPort);
		
	// we use the control port of the real time source object.
	// this way, all messages are send to the real time source,
	// and this shadow object won't receive any.
	fControlPort = node.port;
	ASSERT(fNodeID == node.node);
	ASSERT(fKinds == node.kind);

	if (node.node == NODE_SYSTEM_TIMESOURCE_ID) {
		strcpy(fName, "System Clock");
		fIsRealtime = true;
	} else {
		live_node_info lni;
		if (B_OK == BMediaRoster::Roster()->GetLiveNodeInfo(node, &lni)) {
			strcpy(fName, lni.name);
		} else {
			sprintf(fName, "timesource %ld", node.node);
		}
	}

	AddNodeKind(NODE_KIND_SHADOW_TIMESOURCE);
	AddNodeKind(NODE_KIND_NO_REFCOUNTING);
	
	TRACE("TimeSourceObject::TimeSourceObject leave, node id %ld\n", fNodeID);
}

/* virtual */ status_t
TimeSourceObject::TimeSourceOp(
				const time_source_op_info & op,
				void * _reserved)
{
	// we don't get anything here
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
//	if (fIsRealtime) {
//		ERROR("TimeSourceObject::DeleteHook: system time source clone delete hook called\n");
//		return B_ERROR;
//	}
	printf("TimeSourceObject::DeleteHook enter\n");
	_TimeSourceObjectManager->ObjectDeleted(this);
	status = BTimeSource::DeleteHook(node);
	printf("TimeSourceObject::DeleteHook leave\n");
	return status;
}

