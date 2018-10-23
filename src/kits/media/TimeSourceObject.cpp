/*
 * Copyright 2002 Marcus Overhagen. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */


/*!	The object returned by BMediaRoster's
	MakeTimeSourceFor(const media_node& forNode);
*/


#include "TimeSourceObject.h"

#include <stdio.h>
#include <string.h>

#include <MediaRoster.h>
#include <OS.h>

#include <MediaMisc.h>
#include <MediaDebug.h>

#include "TimeSourceObjectManager.h"


TimeSourceObject::TimeSourceObject(const media_node& node)
	:
	BMediaNode("some timesource object", node.node, node.kind),
	BTimeSource(node.node)
{
	TRACE("TimeSourceObject::TimeSourceObject enter, id = %"
		B_PRId32 "\n", node.node);

	if (fControlPort > 0)
		delete_port(fControlPort);

	// We use the control port of the real time source object.
	// this way, all messages are send to the real time source,
	// and this shadow object won't receive any.
	fControlPort = node.port;

	ASSERT(fNodeID == node.node);
	ASSERT(fKinds == node.kind);

	if (node.node == NODE_SYSTEM_TIMESOURCE_ID) {
		strcpy(fName, "System clock");
		fIsRealtime = true;
	} else {
		live_node_info liveNodeInfo;
		if (BMediaRoster::Roster()->GetLiveNodeInfo(node, &liveNodeInfo)
				== B_OK)
			strlcpy(fName, liveNodeInfo.name, B_MEDIA_NAME_LENGTH);
		else {
			snprintf(fName, B_MEDIA_NAME_LENGTH, "timesource %" B_PRId32,
				node.node);
		}
	}

	AddNodeKind(NODE_KIND_SHADOW_TIMESOURCE);
	AddNodeKind(NODE_KIND_NO_REFCOUNTING);

	TRACE("TimeSourceObject::TimeSourceObject leave, node id %" B_PRId32 "\n",
		fNodeID);
}


status_t
TimeSourceObject::TimeSourceOp(const time_source_op_info& op, void* _reserved)
{
	// we don't get anything here
	return B_OK;
}


BMediaAddOn*
TimeSourceObject::AddOn(int32* _id) const
{
	if (_id != NULL)
		*_id = 0;

	return NULL;
}


status_t
TimeSourceObject::DeleteHook(BMediaNode* node)
{
//	if (fIsRealtime) {
//		ERROR("TimeSourceObject::DeleteHook: system time source clone delete hook called\n");
//		return B_ERROR;
//	}
	PRINT(1, "TimeSourceObject::DeleteHook enter\n");
	gTimeSourceObjectManager->ObjectDeleted(this);
	status_t status = BTimeSource::DeleteHook(node);
	PRINT(1, "TimeSourceObject::DeleteHook leave\n");
	return status;
}

