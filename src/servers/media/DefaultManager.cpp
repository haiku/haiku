/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <OS.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <string.h>
#include <String.h>
#include "DefaultManager.h"
#include "debug.h"

/* no locking used in this file, we assume that the caller (NodeManager) does it.
 */

DefaultManager::DefaultManager()
 :	fPhysicalVideoOut(-1),
	fPhysicalVideoIn(-1),
	fPhysicalAudioOut(-1),
	fPhysicalAudioIn(-1),
	fSystemTimeSource(-1),
	fTimeSource(-1),
	fAudioMixer(-1),
	fPhysicalAudioOutInputID(0)
{
	strcpy(fPhysicalAudioOutInputName, "default");
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
DefaultManager::Get(media_node_id *nodeid, char *input_name, int32 *inputid, node_type type)
{
	switch (type) {
		case VIDEO_INPUT: 		// output: nodeid
			if (fPhysicalVideoIn == -1)
				return B_ERROR;
			*nodeid = fPhysicalVideoIn;
			return B_OK;

		case AUDIO_INPUT: 		// output: nodeid
			if (fPhysicalAudioIn == -1)
				return B_ERROR;
			*nodeid = fPhysicalAudioIn;
			return B_OK;
			
		case VIDEO_OUTPUT: 		// output: nodeid
			if (fPhysicalVideoOut == -1)
				return B_ERROR;
			*nodeid = fPhysicalVideoOut;
			return B_OK;

		case AUDIO_OUTPUT:		// output: nodeid
			if (fPhysicalAudioOut == -1)
				return B_ERROR;
			*nodeid = fPhysicalAudioOut;
			return B_OK;

		case AUDIO_OUTPUT_EX:	// output: nodeid, input_name, input_id
			if (fPhysicalAudioOut == -1)
				return B_ERROR;
			*nodeid = fPhysicalAudioOut;
			*inputid = fPhysicalAudioOutInputID;
			strcpy(input_name, fPhysicalAudioOutInputName);
			return B_OK;

		case AUDIO_MIXER:		// output: nodeid
			if (fAudioMixer == -1)
				return B_ERROR;
			*nodeid = fAudioMixer;
			return B_OK;

		case TIME_SOURCE:
			if (fTimeSource != -1)
				*nodeid = fTimeSource;
			else
				*nodeid = fSystemTimeSource;
			return B_OK;
			
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
	printf("DefaultManager::RescanThread() enter\n");

	// We do not search for the system time source,
	// it should already exist
	ASSERT(fSystemTimeSource != -1);
	
	if (fPhysicalVideoOut == -1)
		FindPhysicalVideoOut();
	if (fPhysicalVideoIn == -1)
		FindPhysicalVideoIn();
	if (fPhysicalAudioOut == -1)
		FindPhysicalAudioOut();
	if (fPhysicalAudioIn == -1)
		FindPhysicalAudioIn();
	if (fAudioMixer == -1)
		FindAudioMixer();
	// The normal time source is searched for after the
	// Physical Audio Out has been created.
	if (fTimeSource == -1)
		FindTimeSource();

	printf("DefaultManager::RescanThread() leave\n");
}

void
DefaultManager::FindPhysicalVideoOut()
{
	dormant_node_info info;
	media_format input; /* a physical video output has a logical data input */
	media_node node;
	int32 count;
	status_t rv;

	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_VIDEO;
	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, &input, NULL, NULL, B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find physical video output node\n");
		return;
	}
	rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node, B_FLAVOR_IS_GLOBAL);
	if (rv != B_OK) {
		printf("Couldn't instantiate physical video output node\n");
	} else {
		printf("Default physical video output created!\n");
		fPhysicalVideoOut = node.node;
	}
}

void
DefaultManager::FindPhysicalVideoIn()
{
	dormant_node_info info;
	media_format output; /* a physical video input has a logical data output */
	media_node node;
	int32 count;
	status_t rv;

	memset(&output, 0, sizeof(output));
	output.type = B_MEDIA_RAW_VIDEO;
	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, NULL, &output, NULL, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find physical video input node\n");
		return;
	}
	rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node, B_FLAVOR_IS_GLOBAL);
	if (rv != B_OK) {
		printf("Couldn't instantiate physical video input node\n");
	} else {
		printf("Default physical video input created!\n");
		fPhysicalVideoIn = node.node;
	}
}

void
DefaultManager::FindPhysicalAudioOut()
{
	dormant_node_info info;
	media_format input; /* a physical audio output has a logical data input */
	media_node node;
	int32 count;
	status_t rv;

	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_AUDIO;
	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, &input, NULL, NULL, B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find physical audio output node\n");
		return;
	}
	rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node, B_FLAVOR_IS_GLOBAL);
	if (rv != B_OK) {
		printf("Couldn't instantiate physical audio output node\n");
	} else {
		printf("Default physical audio output created!\n");
		fPhysicalAudioOut = node.node;
	}
}

void
DefaultManager::FindPhysicalAudioIn()
{
	dormant_node_info info;
	media_format output; /* a physical audio input has a logical data output */
	media_node node;
	int32 count;
	status_t rv;

	memset(&output, 0, sizeof(output));
	output.type = B_MEDIA_RAW_AUDIO;
	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, NULL, &output, NULL, B_BUFFER_PRODUCER | B_PHYSICAL_INPUT);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find physical audio input node\n");
		return;
	}
	rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node, B_FLAVOR_IS_GLOBAL);
	if (rv != B_OK) {
		printf("Couldn't instantiate physical audio input node\n");
	} else {
		printf("Default physical audio input created!\n");
		fPhysicalAudioIn = node.node;
	}
}

void
DefaultManager::FindTimeSource()
{
}

void
DefaultManager::FindAudioMixer()
{
	dormant_node_info info;
	media_node node;
	int32 count;
	status_t rv;

	count = 1;
	rv = BMediaRoster::Roster()->GetDormantNodes(&info, &count, NULL, NULL, NULL, B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find audio mixer node\n");
		return;
	}
	rv = BMediaRoster::Roster()->InstantiateDormantNode(info, &node, B_FLAVOR_IS_GLOBAL);
	if (rv != B_OK) {
		printf("Couldn't instantiate audio mixer node\n");
	} else {
		printf("Default audio mixer created!\n");
		fAudioMixer = node.node;
	}
}

void
DefaultManager::Dump()
{
}

void
DefaultManager::CleanupTeam(team_id team)
{
}
