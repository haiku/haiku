/* 
 * Copyright 2002, 2003 Marcus Overhagen, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Application.h>
#include <OS.h>
#include <MediaNode.h>
#include <MediaRoster.h>
#include <TimeSource.h>
#include <string.h>
#include <storage/File.h>
#include <storage/FindDirectory.h>
#include <storage/Path.h>
#include "DefaultManager.h"
#include "DormantNodeManager.h"
#include "NodeManager.h"
#include "debug.h"

/* no locking used in this file, we assume that the caller (NodeManager) does it.
 */


#define MAX_NODE_INFOS 10
#define MAX_INPUT_INFOS 10

const uint32 kMsgHeader = 'sepx';
const uint32 kMsgTypeVideoIn = 0xffffffef;
const uint32 kMsgTypeVideoOut = 0xffffffee;
const uint32 kMsgTypeAudioIn = 0xfffffffe;
const uint32 kMsgTypeAudioOut = 0xffffffff;

const char *kDefaultManagerType 			= "be:_default";
const char *kDefaultManagerAddon 			= "be:_addon_id";
const char *kDefaultManagerFlavorId 		= "be:_internal_id";
const char *kDefaultManagerFlavorName 		= "be:_flavor_name";
const char *kDefaultManagerPath 			= "be:_path";
const char *kDefaultManagerInput 			= "be:_input_id";

const char *kDefaultManagerSettings			= "Media/MDefaultManager";


DefaultManager::DefaultManager()
 :	fMixerConnected(false),
 	fPhysicalVideoOut(-1),
	fPhysicalVideoIn(-1),
	fPhysicalAudioOut(-1),
	fPhysicalAudioIn(-1),
	fSystemTimeSource(-1),
	fTimeSource(-1),
	fAudioMixer(-1),
	fPhysicalAudioOutInputID(0)
{
	strcpy(fPhysicalAudioOutInputName, "default");
	fBeginHeader[0] = 0xab00150b;
	fBeginHeader[1] = 0x18723462;
	fBeginHeader[2] = 0x00000002;
	fEndHeader[0] = 0x7465726d;
	fEndHeader[1] = 0x6d666c67;
	fEndHeader[2] = 0x00000002;
}

DefaultManager::~DefaultManager()
{
}

// this is called by the media_server *before* any add-ons have been loaded
status_t
DefaultManager::LoadState()
{
	CALLED();
	status_t err = B_OK;
	BPath path;
	if((err = find_directory(B_USER_SETTINGS_DIRECTORY, &path))!=B_OK)
		return err;
		
	path.Append(kDefaultManagerSettings);
	
	BFile file(path.Path(), B_READ_ONLY);
		
	uint32 category_count;
    if (file.Read(fBeginHeader, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;
	TRACE("0x%08lx %ld\n", fBeginHeader[0], fBeginHeader[0]);
	TRACE("0x%08lx %ld\n", fBeginHeader[1], fBeginHeader[1]);
	TRACE("0x%08lx %ld\n", fBeginHeader[2], fBeginHeader[2]);
	if (file.Read(&category_count, sizeof(uint32)) < (int32)sizeof(uint32))
		return B_ERROR;
	while (category_count--) {
		BMessage settings;
		uint32 msg_header;
		uint32 default_type;
		if (file.Read(&msg_header, sizeof(uint32)) < (int32)sizeof(uint32))
			return B_ERROR;
		if (file.Read(&default_type, sizeof(uint32)) < (int32)sizeof(uint32))
			return B_ERROR;
		if(settings.Unflatten(&file)==B_OK) {
			settings.PrintToStream();
			fMsgList.AddItem(new BMessage(settings));
		}
	}
	if (file.Read(fEndHeader, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;
	return B_OK;
}

status_t
DefaultManager::SaveState(NodeManager *node_manager)
{
	CALLED();
	status_t err = B_OK;
	BPath path;
	BList list;
	if((err = find_directory(B_USER_SETTINGS_DIRECTORY, &path))!=B_OK)
		return err;
		
	path.Append(kDefaultManagerSettings);
	
	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);
	
	uint32 default_types[] = {kMsgTypeVideoIn, kMsgTypeVideoOut, kMsgTypeAudioIn, kMsgTypeAudioOut};
	uint32 media_node_ids[] = {fPhysicalVideoIn, fPhysicalVideoOut, fPhysicalAudioIn, fPhysicalAudioOut};
	for(uint32 i=0; i<sizeof(default_types)/sizeof(default_types[0]); i++) {
		BMessage *settings = new BMessage();
		settings->AddInt32(kDefaultManagerType, default_types[i]);
		
		// we call the node manager to have more infos about nodes
		dormant_node_info info;
		media_node node;
		entry_ref ref;
		if (node_manager->GetCloneForId(&node, media_node_ids[i], be_app->Team()) != B_OK)
			continue;
		if (node_manager->GetDormantNodeInfo(&info, node) != B_OK)
			continue;
		if (node_manager->DecrementGlobalRefCount(media_node_ids[i], be_app->Team()) != B_OK)
			continue;
		if (node_manager->GetAddonRef(&ref, info.addon)!=B_OK)
			continue;

		BPath path(&ref);
		settings->AddInt32(kDefaultManagerAddon, info.addon);
		settings->AddInt32(kDefaultManagerFlavorId, info.flavor_id);
		settings->AddInt32(kDefaultManagerInput, default_types[i] == kMsgTypeAudioOut ? fPhysicalAudioOutInputID : 0);
		settings->AddString(kDefaultManagerFlavorName, info.name);
		settings->AddString(kDefaultManagerPath, path.Path());
		
		list.AddItem(settings);
		TRACE("message %s added\n", info.name);
	}
	
	if (file.Write(fBeginHeader, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;
	int32 category_count = list.CountItems();
    if (file.Write(&category_count, sizeof(uint32)) < (int32)sizeof(uint32))
		return B_ERROR;
	
	for (int32 i = 0; i < category_count; i++) {
		BMessage *settings = (BMessage *)list.ItemAt(i);
		uint32 default_type;
		if (settings->FindInt32(kDefaultManagerType, (int32*)&default_type) < B_OK)
			return B_ERROR;
		if (file.Write(&kMsgHeader, sizeof(uint32)) < (int32)sizeof(uint32))
			return B_ERROR;
		if (file.Write(&default_type, sizeof(uint32)) < (int32)sizeof(uint32))
			return B_ERROR;
		if(settings->Flatten(&file) < B_OK)
			return B_ERROR;
		delete settings;
	}
	if (file.Write(fEndHeader, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;
		
	return B_OK;
}

status_t
DefaultManager::Set(media_node_id node_id, const char *input_name, int32 input_id, node_type type)
{
	CALLED();
	TRACE("DefaultManager::Set type : %i, node : %li, input : %li\n", type, node_id, input_id);
	switch (type) {
		case VIDEO_INPUT:
			fPhysicalVideoIn = node_id;
			return B_OK;
		case AUDIO_INPUT:
			fPhysicalAudioIn = node_id;
			return B_OK;
		case VIDEO_OUTPUT:
			fPhysicalVideoOut = node_id;
			return B_OK;
		case AUDIO_MIXER:
			return B_ERROR;
		case AUDIO_OUTPUT:
			fPhysicalAudioOut = node_id;
			fPhysicalAudioOutInputID = input_id;
			strcpy(fPhysicalAudioOutInputName, input_name ? input_name : "<null>");
			return B_OK;
		case TIME_SOURCE:
			return B_ERROR;
		
		case SYSTEM_TIME_SOURCE: //called by the media_server's ServerApp::StartSystemTimeSource()
		{
			ASSERT(fSystemTimeSource == -1);
			fSystemTimeSource = node_id;
			return B_OK;
		}
		
		default:
		{
			ERROR("DefaultManager::Set Error: called with unknown type %d\n", type);
			return B_ERROR;
		}
	}
}

status_t
DefaultManager::Get(media_node_id *nodeid, char *input_name, int32 *inputid, node_type type)
{
	CALLED();
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
			ERROR("DefaultManager::Get Error: called with unknown type %d\n", type);
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

	if (fPhysicalVideoOut == -1) {
		FindPhysical(&fPhysicalVideoOut, kMsgTypeVideoOut, false, B_MEDIA_RAW_VIDEO);
		FindPhysical(&fPhysicalVideoOut, kMsgTypeVideoOut, false, B_MEDIA_ENCODED_VIDEO);
	}		
	if (fPhysicalVideoIn == -1) {
		FindPhysical(&fPhysicalVideoIn, kMsgTypeVideoIn, true, B_MEDIA_RAW_VIDEO);
		FindPhysical(&fPhysicalVideoIn, kMsgTypeVideoIn, true, B_MEDIA_ENCODED_VIDEO);
	}
	if (fPhysicalAudioOut == -1)
		FindPhysical(&fPhysicalAudioOut, kMsgTypeAudioOut, false, B_MEDIA_RAW_AUDIO);
	if (fPhysicalAudioIn == -1)
		FindPhysical(&fPhysicalAudioIn, kMsgTypeAudioIn, true, B_MEDIA_RAW_AUDIO);
	if (fAudioMixer == -1)
		FindAudioMixer();

	// The normal time source is searched for after the
	// Physical Audio Out has been created.
	if (fTimeSource == -1)
		FindTimeSource();

	// Connect the mixer and physical audio out (soundcard)
	if (!fMixerConnected && fAudioMixer != -1 && fPhysicalAudioOut != -1) {
		fMixerConnected = B_OK == ConnectMixerToOutput();
		if (!fMixerConnected)
			ERROR("DefaultManager: failed to connect mixer and soundcard\n");
	} else {
		ERROR("DefaultManager: Did not try to connect mixer and soundcard\n");
	}

	printf("DefaultManager::RescanThread() leave\n");
}


void
DefaultManager::FindPhysical(volatile media_node_id *id, uint32 default_type, bool isInput, media_type type)
{
	live_node_info info[MAX_NODE_INFOS];
	media_format format;
	int32 count;
	status_t rv;
	BMessage *msg = NULL;
	BPath msgPath;
	dormant_node_info msgDninfo;
	int32 input_id;
	bool isAudio = type & B_MEDIA_RAW_AUDIO;

	for(int32 i=0; i<fMsgList.CountItems(); i++) {
		msg = (BMessage *)fMsgList.ItemAt(i);
		int32 msgType;
		if(msg->FindInt32(kDefaultManagerType, &msgType)==B_OK && ((uint32)msgType == default_type)) {
			const char *name = NULL;
			const char *path = NULL;
			msg->FindInt32(kDefaultManagerAddon, &msgDninfo.addon);
			msg->FindInt32(kDefaultManagerFlavorId, &msgDninfo.flavor_id);
			msg->FindInt32(kDefaultManagerInput, &input_id);
			msg->FindString(kDefaultManagerFlavorName, &name);
			msg->FindString(kDefaultManagerPath, &path);
			if(name)
				strcpy(msgDninfo.name, name);
			if(path)
				msgPath = BPath(path);
			break;
		}
	}

	memset(&format, 0, sizeof(format));
	format.type = type;
	count = MAX_NODE_INFOS;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info[0], &count, 
		isInput ? NULL : &format, isInput ? &format : NULL, NULL, 
		isInput ? B_BUFFER_PRODUCER | B_PHYSICAL_INPUT : B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (rv != B_OK || count < 1) {
		ERROR("Couldn't find physical %s %s node\n", isAudio ? "audio" : "video", isInput ? "input" : "output");
		return;
	}
	for (int i = 0; i < count; i++)
		TRACE("info[%d].name %s\n", i, info[i].name);
	
	for (int i = 0; i < count; i++) {
		if (isAudio) {
			if (isInput) {
				if (0 == strcmp(info[i].name, "None In")) {
					// we keep the Null audio driver if none else matchs
					*id = info[i].node.node;
					continue;
				}
				if (0 == strcmp(info[i].name, "DV Input")) // skip the Firewire audio driver
					continue;
			} else {
				if (0 == strcmp(info[i].name, "None Out")) {
					// we keep the Null audio driver if none else matchs
					*id = info[i].node.node;
					if(msg)
						fPhysicalAudioOutInputID = input_id;
					continue;
				}
				if (0 == strcmp(info[i].name, "DV Output")) // skip the Firewire audio driver
					continue;
			}
		}	
		if(msg) {	// we have a default info msg
			dormant_node_info dninfo;
			if(BMediaRoster::Roster()->GetDormantNodeFor(info[i].node, &dninfo) != B_OK) {
				ERROR("Couldn't GetDormantNodeFor\n");
				continue;
			}
			if(dninfo.flavor_id!=msgDninfo.flavor_id
				|| strcmp(dninfo.name, msgDninfo.name)!=0) {
				ERROR("Doesn't match flavor or name\n");
				continue;
			}
			BPath path;
			if((_DormantNodeManager->FindAddonPath(&path, dninfo.addon)!=B_OK) 
				|| (path != msgPath)) {
				ERROR("Doesn't match : path\n");
				continue;
			}
		}
		TRACE("Default physical %s %s \"%s\" created!\n", 
			isAudio ? "audio" : "video", isInput ? "input" : "output", info[i].name);
		*id = info[i].node.node;
		if(msg && isAudio && !isInput)
			fPhysicalAudioOutInputID = input_id;
		return;
	}
}


void
DefaultManager::FindTimeSource()
{
	live_node_info info[MAX_NODE_INFOS];
	media_format input; /* a physical audio output has a logical data input (DAC)*/
	int32 count;
	status_t rv;
	
	/* First try to use the current default physical audio out
	 */
	if (fPhysicalAudioOut != -1) {
		media_node clone;
		if (B_OK == BMediaRoster::Roster()->GetNodeFor(fPhysicalAudioOut, &clone)) {
			if (clone.kind & B_TIME_SOURCE) {
				fTimeSource = clone.node;
				BMediaRoster::Roster()->StartTimeSource(clone, system_time() + 1000);
				BMediaRoster::Roster()->ReleaseNode(clone);
				printf("Default DAC timesource created!\n");
				return;
			}
			BMediaRoster::Roster()->ReleaseNode(clone);
		} else {
			printf("Default DAC is not a timesource!\n");
		}
	} else {
		printf("Default DAC node does not exist!\n");
	}
	
	/* Now try to find another physical audio out node
	 */
	memset(&input, 0, sizeof(input));
	input.type = B_MEDIA_RAW_AUDIO;
	count = MAX_NODE_INFOS;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info[0], &count, &input, NULL, NULL, B_TIME_SOURCE | B_PHYSICAL_OUTPUT);
	if (rv == B_OK && count >= 1) {
		for (int i = 0; i < count; i++)
			printf("info[%d].name %s\n", i, info[i].name);
	
		for (int i = 0; i < count; i++) {
			// The BeOS R5 None Out node pretend to be a physical time source, that is pretty dumb
			if (0 == strcmp(info[i].name, "None Out")) // skip the Null audio driver
				continue;
			if (0 != strstr(info[i].name, "DV Output")) // skip the Firewire audio driver
				continue;
			printf("Default DAC timesource \"%s\" created!\n", info[i].name);
			fTimeSource = info[i].node.node;
			BMediaRoster::Roster()->StartTimeSource(info[i].node, system_time() + 1000);
			return;
		}
	} else {
		printf("Couldn't find DAC timesource node\n");
	}	
	
	/* XXX we might use other audio or video clock timesources
	 */
}

void
DefaultManager::FindAudioMixer()
{
	live_node_info info;
	int32 count;
	status_t rv;

	count = 1;
	rv = BMediaRoster::Roster()->GetLiveNodes(&info, &count, NULL, NULL, NULL, B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER);
	if (rv != B_OK || count != 1) {
		printf("Couldn't find audio mixer node\n");
		return;
	}
	fAudioMixer = info.node.node;
	printf("Default audio mixer node created\n");
}

status_t
DefaultManager::ConnectMixerToOutput()
{
	BMediaRoster 		*roster;
	media_node 			timesource;
	media_node 			mixer;
	media_node 			soundcard;
	media_input			inputs[MAX_INPUT_INFOS];
	media_input 		input;
	media_output 		output;
	media_input 		newinput;
	media_output 		newoutput;
	media_format 		format;
	BTimeSource * 		ts;
	bigtime_t 			start_at;
	int32 				count;
	status_t 			rv;
	
	roster = BMediaRoster::Roster();

	rv = roster->GetNodeFor(fPhysicalAudioOut, &soundcard);
	if (rv != B_OK) {
		printf("DefaultManager: failed to find soundcard (physical audio output)\n");
		return B_ERROR;
	}

	rv = roster->GetNodeFor(fAudioMixer, &mixer);
	if (rv != B_OK) {
		roster->ReleaseNode(soundcard);
		printf("DefaultManager: failed to find mixer\n");
		return B_ERROR;
	}

	// we now have the mixer and soundcard nodes,
	// find a free input/output and connect them

	rv = roster->GetFreeOutputsFor(mixer, &output, 1, &count, B_MEDIA_RAW_AUDIO);
	if (rv != B_OK || count != 1) {
		printf("DefaultManager: can't find free mixer output\n");
		rv = B_ERROR;
		goto finish;
	}

	rv = roster->GetFreeInputsFor(soundcard, inputs, MAX_INPUT_INFOS, &count, B_MEDIA_RAW_AUDIO);
	if (rv != B_OK || count < 1) {
		printf("DefaultManager: can't find free soundcard inputs\n");
		rv = B_ERROR;
		goto finish;
	}
	
	for (int32 i = 0; i < count; i++) {
		input = inputs[i];
		if(input.destination.id == fPhysicalAudioOutInputID)
			break;
	}
	
	for (int i = 0; i < 6; i++) {
		switch (i) {
			case 0:
				printf("DefaultManager: Trying connect in native format (1)\n");
				if (B_OK != roster->GetFormatFor(input, &format)) {
					ERROR("DefaultManager: GetFormatFor failed\n");
					continue;
				}
				// XXX BeOS R5 multiaudio node bug workaround
				if (format.u.raw_audio.channel_count == 1) {
					printf("##### WARNING! DefaultManager: ignored mono format\n");
					continue;
				}
				break;

			case 1:
				printf("DefaultManager: Trying connect in format 1\n");
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 44100;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = 0x2;
				break;

			case 2:
				printf("DefaultManager: Trying connect in format 2\n");
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 48000;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = 0x2;
				break;

			case 3:
				printf("DefaultManager: Trying connect in format 3\n");
				memset(&format, 0, sizeof(format));
				format.type = B_MEDIA_RAW_AUDIO;
				break;

			case 4:
				// BeOS R5 multiaudio node bug workaround
				printf("DefaultManager: Trying connect in native format (2)\n");
				if (B_OK != roster->GetFormatFor(input, &format)) {
					ERROR("DefaultManager: GetFormatFor failed\n");
					continue;
				}
				break;

			case 5:
				printf("DefaultManager: Trying connect in format 4\n");
				memset(&format, 0, sizeof(format));
				break;

		}
		rv = roster->Connect(output.source, input.destination, &format, &newoutput, &newinput);
		if (rv == B_OK)
			break;
	}
	if (rv != B_OK) {
		ERROR("DefaultManager: connect failed\n");
		goto finish;
	}

	roster->SetRunModeNode(mixer, BMediaNode::B_INCREASE_LATENCY);
	roster->SetRunModeNode(soundcard, BMediaNode::B_RECORDING);

	roster->GetTimeSource(&timesource);
	roster->SetTimeSourceFor(mixer.node, timesource.node);
	roster->SetTimeSourceFor(soundcard.node, timesource.node);
	roster->PrerollNode(mixer);
	roster->PrerollNode(soundcard);
	
	ts = roster->MakeTimeSourceFor(mixer);
	start_at = ts->Now() + 50000;
	roster->StartNode(mixer, start_at);
	roster->StartNode(soundcard, start_at);
	ts->Release();
	
finish:
	roster->ReleaseNode(mixer);
	roster->ReleaseNode(soundcard);
	roster->ReleaseNode(timesource);
	return rv;
}

void
DefaultManager::Dump()
{
}

void
DefaultManager::CleanupTeam(team_id team)
{
}

