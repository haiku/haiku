/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "TList.h"
#include "TMap.h"
#include "TStack.h"
#include "DataExchange.h"

class DefaultManager
{
public:
	DefaultManager();
	~DefaultManager();

	status_t LoadState();
	status_t SaveState();

	status_t Set(node_type type, const media_node *node, const dormant_node_info *info, const media_input *input);
	status_t Get(media_node_id *nodeid, char *input_name, int32 *input_id, node_type type);
	status_t Rescan();
	
	void Dump();

	void CleanupTeam(team_id team);

private:
	static int32 rescan_thread(void *arg);
	void RescanThread();

	void FindPhysicalVideoOut();
	void FindPhysicalVideoIn();
	void FindPhysicalAudioOut();
	void FindPhysicalAudioIn();
	void FindAudioMixer();
	void FindTimeSource();
	
private:
	media_node_id fPhysicalVideoOut;
	media_node_id fPhysicalVideoIn;
	media_node_id fPhysicalAudioOut;
	media_node_id fPhysicalAudioIn;
	media_node_id fSystemTimeSource;
	media_node_id fTimeSource;
	media_node_id fAudioMixer;
	int32 fPhysicalAudioOutInputID;
	char fPhysicalAudioOutInputName[B_MEDIA_NAME_LENGTH];
};
