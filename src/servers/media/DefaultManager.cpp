/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>
#include <MediaNode.h>
#include "DefaultManager.h"
#include "debug.h"

/* no locking used in this file, we assume that the caller (NodeManager) does it.
 */

DefaultManager::DefaultManager()
 :	fSystemTimeSource(-1)
{
}

DefaultManager::~DefaultManager()
{
}

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
		case VIDEO_INPUT:
		case AUDIO_INPUT:
		case VIDEO_OUTPUT:
		case AUDIO_MIXER:
		case AUDIO_OUTPUT:
		case AUDIO_OUTPUT_EX:
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

status_t
DefaultManager::Rescan()
{
	return B_OK;
}

void
DefaultManager::Dump()
{
}

void
DefaultManager::CleanupTeam(team_id team)
{
}
