/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "TList.h"
#include "TMap.h"
#include "TStack.h"
#include "DataExchange.h"

#include <Message.h>

class NodeManager;

class DefaultManager
{
public:
	DefaultManager();
	~DefaultManager();

	status_t LoadState();
	status_t SaveState(NodeManager *node_manager);

	status_t Set(media_node_id nodeid, const char *input_name, int32 input_id, node_type type);
	status_t Get(media_node_id *nodeid, char *input_name, int32 *input_id, node_type type);
	status_t Rescan();
	
	void Dump();

	void CleanupTeam(team_id team);

private:
	static int32 rescan_thread(void *arg);
	void RescanThread();

	void FindPhysical(volatile media_node_id *id, uint32 default_type, bool isInput, media_type type);
	void FindAudioMixer();
	void FindTimeSource();
	
	status_t ConnectMixerToOutput();
	
private:
	volatile bool fMixerConnected;
	volatile media_node_id fPhysicalVideoOut;
	volatile media_node_id fPhysicalVideoIn;
	volatile media_node_id fPhysicalAudioOut;
	volatile media_node_id fPhysicalAudioIn;
	volatile media_node_id fSystemTimeSource;
	volatile media_node_id fTimeSource;
	volatile media_node_id fAudioMixer;
	volatile int32 fPhysicalAudioOutInputID;
	char fPhysicalAudioOutInputName[B_MEDIA_NAME_LENGTH];
	
	BList fMsgList;
	
	uint32 fBeginHeader[3];
	uint32 fEndHeader[3];
};
