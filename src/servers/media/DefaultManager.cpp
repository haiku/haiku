/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <string.h>
#include "DefaultManager.h"
#include "debug.h"

/* no locking used in this file, we assume that the caller (NodeManager) does it.
 */

DefaultManager::DefaultManager()
 :	fSystemTimeSource(-1),
 	fDefaultVideoOut(-1),
 	fDefaultVideoIn(-1)
{
}

DefaultManager::~DefaultManager()
{
}

// this is called by the media_server *before* any add-ons have been loaded
status_t
DefaultManager::LoadState()
{
	return B_OK;
}

status_t
DefaultManager::SaveState()
{
	return B_OK;
}

status_t
DefaultManager::Set(node_type type, const media_node *node, const dormant_node_info *info, const media_input *input)
{
	switch (type) {
		case VIDEO_INPUT:
		case AUDIO_INPUT:
		case VIDEO_OUTPUT:
		case AUDIO_MIXER:
		case AUDIO_OUTPUT:
		case AUDIO_OUTPUT_EX:
		case TIME_SOURCE:
			return B_ERROR;
		
		case SYSTEM_TIME_SOURCE: //called by the media_server's ServerApp::StartSystemTimeSource()
		{
			ASSERT(fSystemTimeSource == -1);
			ASSERT(node != 0);
			fSystemTimeSource = node->node;
			return B_OK;
		}
		
		default:
		{
			FATAL("DefaultManager::Set Error: called with unknown type %d\n", type);
			return B_ERROR;
		}
	}
}

status_t
DefaultManager::Get(media_node_id *nodeid, char *input_name, int32 *input_id, node_type type)
{
	switch (type) {
		case VIDEO_INPUT: 		// output: nodeid
			*nodeid = fDefaultVideoOut;
			return *nodeid > 0 ? B_OK : B_ERROR;

		case AUDIO_INPUT: 		// output: nodeid
			*nodeid = -999;
			return B_ERROR;
			
		case VIDEO_OUTPUT: 		// output: nodeid
			*nodeid = fDefaultVideoIn;
			return *nodeid > 0 ? B_OK : B_ERROR;

		case AUDIO_MIXER:		// output: nodeid
		case AUDIO_OUTPUT:		// output: nodeid
		case AUDIO_OUTPUT_EX:	// output: nodeid, input_name, input_id
			*nodeid = -999;
			return B_ERROR;

		case TIME_SOURCE:
		case SYSTEM_TIME_SOURCE:
			*nodeid = fSystemTimeSource;
			return B_OK;

		default:
		{
			FATAL("DefaultManager::Get Error: called with unknown type %d\n", type);
			return B_ERROR;
		}
	}
}

// this is called by the media_server *after* the initial add-on loading has been done
status_t
DefaultManager::Rescan()
{
	thread_id	fThreadId = spawn_thread(rescan_thread, "rescan defaults", 8, this);
	resume_thread(fThreadId);
	return B_OK;
}

int32
DefaultManager::rescan_thread(void *arg)
{
	reinterpret_cast<DefaultManager *>(arg)->RescanThread();
	return 0;
}

void
DefaultManager::RescanThread()
{
	dormant_node_info info;
	media_format output;
	media_format input;
	int32 count;
	status_t rv;
	
	printf("DefaultManager::RescanThread() enter\n");
	
	memset(&output, 0, sizeof(output));
	output.type = B_MEDIA_RAW_VIDEO;
	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, NULL, &output);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find default video output node\n");
		fDefaultVideoOut = -1;
	} else {
		media_node node;
		rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node);
		if (rv != B_OK) {
			printf("Couldn't instantiate default video output node\n");
			fDefaultVideoOut = -1;
		} else {
			printf("Default video output created!\n");
			fDefaultVideoOut = node.node;
		}
	}


	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_VIDEO;
	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, &input);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find default video input node\n");
		fDefaultVideoIn = -1;
	} else {
		media_node node;
		rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node);
		if (rv != B_OK) {
			printf("Couldn't instantiate default video input node\n");
			fDefaultVideoIn = -1;
		} else {
			printf("Default video input created!\n");
			fDefaultVideoIn = node.node;
		}
	}


	printf("DefaultManager::RescanThread() leave\n");
}


void
DefaultManager::Dump()
{
}

void
DefaultManager::CleanupTeam(team_id team)
{
}
