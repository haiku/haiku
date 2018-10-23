/*
 * Copyright 2002, 2003 Marcus Overhagen, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "DefaultManager.h"

#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <MediaNode.h>
#include <OS.h>
#include <Path.h>
#include <TimeSource.h>
#include <string.h>

#include "MediaDebug.h"
#include "DormantNodeManager.h"
#include "media_server.h"
#include "NodeManager.h"


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

const char *kDefaultManagerSettingsDirectory			= "Media";
const char *kDefaultManagerSettingsFile				= "MDefaultManager";


DefaultManager::DefaultManager()
	:
	fMixerConnected(false),
 	fPhysicalVideoOut(-1),
	fPhysicalVideoIn(-1),
	fPhysicalAudioOut(-1),
	fPhysicalAudioIn(-1),
	fSystemTimeSource(-1),
	fTimeSource(-1),
	fAudioMixer(-1),
	fPhysicalAudioOutInputID(0),
	fRescanThread(-1),
	fRescanRequested(0),
	fRescanLock("rescan default manager"),
	fRoster(NULL)
{
	strcpy(fPhysicalAudioOutInputName, "default");
	fBeginHeader[0] = 0xab00150b;
	fBeginHeader[1] = 0x18723462;
	fBeginHeader[2] = 0x00000002;
	fEndHeader[0] = 0x7465726d;
	fEndHeader[1] = 0x6d666c67;
	fEndHeader[2] = 0x00000002;

	fRoster = BMediaRoster::Roster();
	if (fRoster == NULL)
		TRACE("DefaultManager: The roster is NULL\n");
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
	if ((err = find_directory(B_USER_SETTINGS_DIRECTORY, &path)) != B_OK)
		return err;

	path.Append(kDefaultManagerSettingsDirectory);
	path.Append(kDefaultManagerSettingsFile);

	BFile file(path.Path(), B_READ_ONLY);

	uint32 categoryCount;
	ssize_t size = sizeof(uint32) * 3;
	if (file.Read(fBeginHeader, size) < size)
		return B_ERROR;
	TRACE("0x%08lx %ld\n", fBeginHeader[0], fBeginHeader[0]);
	TRACE("0x%08lx %ld\n", fBeginHeader[1], fBeginHeader[1]);
	TRACE("0x%08lx %ld\n", fBeginHeader[2], fBeginHeader[2]);
	size = sizeof(uint32);
	if (file.Read(&categoryCount, size) < size) {
		fprintf(stderr,
			"DefaultManager::LoadState() failed to read categoryCount\n");
		return B_ERROR;
	}
	TRACE("DefaultManager::LoadState() categoryCount %ld\n", categoryCount);
	while (categoryCount--) {
		BMessage settings;
		uint32 msg_header;
		uint32 default_type;
		if (file.Read(&msg_header, size) < size) {
			fprintf(stderr,
				"DefaultManager::LoadState() failed to read msg_header\n");
			return B_ERROR;
		}
		if (file.Read(&default_type, size) < size) {
			fprintf(stderr,
				"DefaultManager::LoadState() failed to read default_type\n");
			return B_ERROR;
		}
		if (settings.Unflatten(&file) == B_OK)
			fMsgList.AddItem(new BMessage(settings));
		else
			fprintf(stderr, "DefaultManager::LoadState() failed to unflatten\n");
	}
	size = sizeof(uint32) * 3;
	if (file.Read(fEndHeader,size) < size) {
		fprintf(stderr,
			"DefaultManager::LoadState() failed to read fEndHeader\n");
		return B_ERROR;
	}

	TRACE("LoadState returns B_OK\n");
	return B_OK;
}


status_t
DefaultManager::SaveState(NodeManager *node_manager)
{
	CALLED();
	status_t err = B_OK;
	BPath path;
	BList list;
	if ((err = find_directory(B_USER_SETTINGS_DIRECTORY, &path, true)) != B_OK)
		return err;
	path.Append(kDefaultManagerSettingsDirectory);
	if ((err = create_directory(path.Path(), 0755)) != B_OK)
		return err;
	path.Append(kDefaultManagerSettingsFile);

	uint32 default_types[] = {kMsgTypeVideoIn, kMsgTypeVideoOut,
		kMsgTypeAudioIn, kMsgTypeAudioOut};
	media_node_id media_node_ids[] = {fPhysicalVideoIn, fPhysicalVideoOut,
		fPhysicalAudioIn, fPhysicalAudioOut};
	for (uint32 i = 0; i < sizeof(default_types) / sizeof(default_types[0]);
		i++) {

		// we call the node manager to have more infos about nodes
		dormant_node_info info;
		media_node node;
		entry_ref ref;
		if (node_manager->GetCloneForID(media_node_ids[i], be_app->Team(),
				&node) != B_OK
			|| node_manager->GetDormantNodeInfo(node, &info) != B_OK
			|| node_manager->ReleaseNodeReference(media_node_ids[i],
				be_app->Team()) != B_OK
			|| node_manager->GetAddOnRef(info.addon, &ref) != B_OK) {
			if (media_node_ids[i] != -1) {
				// failed to get node info thus just return
				return B_ERROR;
			}
			continue;
		}

		BMessage *settings = new BMessage();
		settings->AddInt32(kDefaultManagerType, default_types[i]);
		BPath path(&ref);
		settings->AddInt32(kDefaultManagerAddon, info.addon);
		settings->AddInt32(kDefaultManagerFlavorId, info.flavor_id);
		settings->AddInt32(kDefaultManagerInput,
			default_types[i] == kMsgTypeAudioOut ? fPhysicalAudioOutInputID : 0);
		settings->AddString(kDefaultManagerFlavorName, info.name);
		settings->AddString(kDefaultManagerPath, path.Path());

		list.AddItem(settings);
		TRACE("message %s added\n", info.name);
	}

	BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);

	if (file.Write(fBeginHeader, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;
	int32 categoryCount = list.CountItems();
	if (file.Write(&categoryCount, sizeof(uint32)) < (int32)sizeof(uint32))
		return B_ERROR;

	for (int32 i = 0; i < categoryCount; i++) {
		BMessage *settings = (BMessage *)list.ItemAt(i);
		uint32 default_type;
		if (settings->FindInt32(kDefaultManagerType,
			(int32*)&default_type) < B_OK)
			return B_ERROR;
		if (file.Write(&kMsgHeader, sizeof(uint32)) < (int32)sizeof(uint32))
			return B_ERROR;
		if (file.Write(&default_type, sizeof(uint32)) < (int32)sizeof(uint32))
			return B_ERROR;
		if (settings->Flatten(&file) < B_OK)
			return B_ERROR;
		delete settings;
	}
	if (file.Write(fEndHeader, sizeof(uint32)*3) < (int32)sizeof(uint32)*3)
		return B_ERROR;

	return B_OK;
}


status_t
DefaultManager::Set(media_node_id node_id, const char *input_name,
	int32 input_id, node_type type)
{
	CALLED();
	TRACE("DefaultManager::Set type : %i, node : %li, input : %li\n", type,
		node_id, input_id);
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
			strcpy(fPhysicalAudioOutInputName,
				input_name ? input_name : "<null>");
			return B_OK;
		case TIME_SOURCE:
			return B_ERROR;

		// called by the media_server's ServerApp::StartSystemTimeSource()
		case SYSTEM_TIME_SOURCE:
		{
			ASSERT(fSystemTimeSource == -1);
			fSystemTimeSource = node_id;
			return B_OK;
		}

		default:
		{
			ERROR("DefaultManager::Set Error: called with unknown type %d\n",
				type);
			return B_ERROR;
		}
	}
}


status_t
DefaultManager::Get(media_node_id *nodeid, char *input_name, int32 *inputid,
	node_type type)
{
	CALLED();
	switch (type) {
		case VIDEO_INPUT: 		// output: nodeid
			if (fPhysicalVideoIn == -1)
				return B_NAME_NOT_FOUND;
			*nodeid = fPhysicalVideoIn;
			return B_OK;

		case AUDIO_INPUT: 		// output: nodeid
			if (fPhysicalAudioIn == -1)
				return B_NAME_NOT_FOUND;
			*nodeid = fPhysicalAudioIn;
			return B_OK;

		case VIDEO_OUTPUT: 		// output: nodeid
			if (fPhysicalVideoOut == -1)
				return B_NAME_NOT_FOUND;
			*nodeid = fPhysicalVideoOut;
			return B_OK;

		case AUDIO_OUTPUT:		// output: nodeid
			if (fPhysicalAudioOut == -1)
				return B_NAME_NOT_FOUND;
			*nodeid = fPhysicalAudioOut;
			return B_OK;

		case AUDIO_OUTPUT_EX:	// output: nodeid, input_name, input_id
			if (fPhysicalAudioOut == -1)
				return B_NAME_NOT_FOUND;
			*nodeid = fPhysicalAudioOut;
			*inputid = fPhysicalAudioOutInputID;
			strcpy(input_name, fPhysicalAudioOutInputName);
			return B_OK;

		case AUDIO_MIXER:		// output: nodeid
			if (fAudioMixer == -1)
				return B_NAME_NOT_FOUND;
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
			ERROR("DefaultManager::Get Error: called with unknown type %d\n",
				type);
			return B_ERROR;
		}
	}
}


// this is called by the media_server *after* the initial add-on loading
// has been done
status_t
DefaultManager::Rescan()
{
	BAutolock locker(fRescanLock);
	atomic_add(&fRescanRequested, 1);
	if (fRescanThread < 0) {
		fRescanThread = spawn_thread(rescan_thread, "rescan defaults",
			B_NORMAL_PRIORITY - 2, this);
		resume_thread(fRescanThread);
	}

	return B_OK;
}


int32
DefaultManager::rescan_thread(void *arg)
{
	reinterpret_cast<DefaultManager *>(arg)->_RescanThread();
	return 0;
}


void
DefaultManager::_RescanThread()
{
	TRACE("DefaultManager::_RescanThread() enter\n");

	BAutolock locker(fRescanLock);

	while (atomic_and(&fRescanRequested, 0) != 0) {
		locker.Unlock();

		// We do not search for the system time source,
		// it should already exist
		ASSERT(fSystemTimeSource != -1);

		if (fPhysicalVideoOut == -1) {
			_FindPhysical(&fPhysicalVideoOut, kMsgTypeVideoOut, false,
				B_MEDIA_RAW_VIDEO);
			_FindPhysical(&fPhysicalVideoOut, kMsgTypeVideoOut, false,
				B_MEDIA_ENCODED_VIDEO);
		}
		if (fPhysicalVideoIn == -1) {
			_FindPhysical(&fPhysicalVideoIn, kMsgTypeVideoIn, true,
				B_MEDIA_RAW_VIDEO);
			_FindPhysical(&fPhysicalVideoIn, kMsgTypeVideoIn, true,
				B_MEDIA_ENCODED_VIDEO);
		}
		if (fPhysicalAudioOut == -1)
			_FindPhysical(&fPhysicalAudioOut, kMsgTypeAudioOut, false,
				B_MEDIA_RAW_AUDIO);
		if (fPhysicalAudioIn == -1)
			_FindPhysical(&fPhysicalAudioIn, kMsgTypeAudioIn, true,
				B_MEDIA_RAW_AUDIO);
		if (fAudioMixer == -1)
			_FindAudioMixer();

		// The normal time source is searched for after the
		// Physical Audio Out has been created.
		if (fTimeSource == -1)
			_FindTimeSource();

		// Connect the mixer and physical audio out (soundcard)
		if (!fMixerConnected && fAudioMixer != -1 && fPhysicalAudioOut != -1) {
			fMixerConnected = _ConnectMixerToOutput() == B_OK;
			if (!fMixerConnected)
				TRACE("DefaultManager: failed to connect mixer and "
					"soundcard\n");
		} else {
			TRACE("DefaultManager: Did not try to connect mixer and "
				"soundcard\n");
		}

		if (fMixerConnected) {
			add_on_server_rescan_finished_notify_command cmd;
			SendToAddOnServer(ADD_ON_SERVER_RESCAN_FINISHED_NOTIFY, &cmd,
				sizeof(cmd));
		}

		locker.Lock();
	}

	fRescanThread = -1;

	BMessage msg(MEDIA_SERVER_RESCAN_COMPLETED);
	be_app->PostMessage(&msg);

	TRACE("DefaultManager::_RescanThread() leave\n");
}


void
DefaultManager::_FindPhysical(volatile media_node_id *id, uint32 default_type,
	bool isInput, media_type type)
{
	live_node_info info[MAX_NODE_INFOS];
	media_format format;
	int32 count;
	status_t rv;
	BMessage *msg = NULL;
	BPath msgPath;
	dormant_node_info msgDninfo;
	int32 input_id;
	bool isAudio = (type == B_MEDIA_RAW_AUDIO)
		|| (type == B_MEDIA_ENCODED_AUDIO);

	for (int32 i = 0; i < fMsgList.CountItems(); i++) {
		msg = (BMessage *)fMsgList.ItemAt(i);
		int32 msgType;
		if (msg->FindInt32(kDefaultManagerType, &msgType) == B_OK
			&& ((uint32)msgType == default_type)) {
			const char *name = NULL;
			const char *path = NULL;
			msg->FindInt32(kDefaultManagerAddon, &msgDninfo.addon);
			msg->FindInt32(kDefaultManagerFlavorId, &msgDninfo.flavor_id);
			msg->FindInt32(kDefaultManagerInput, &input_id);
			msg->FindString(kDefaultManagerFlavorName, &name);
			msg->FindString(kDefaultManagerPath, &path);
			if (name)
				strcpy(msgDninfo.name, name);
			if (path)
				msgPath = BPath(path);
			break;
		}
	}

	format.type = type;
	count = MAX_NODE_INFOS;
	rv = fRoster->GetLiveNodes(&info[0], &count,
		isInput ? NULL : &format, isInput ? &format : NULL, NULL,
		isInput ? B_BUFFER_PRODUCER | B_PHYSICAL_INPUT
			: B_BUFFER_CONSUMER | B_PHYSICAL_OUTPUT);
	if (rv != B_OK || count < 1) {
		TRACE("Couldn't find physical %s %s node\n",
			isAudio ? "audio" : "video", isInput ? "input" : "output");
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
				// skip the Firewire audio driver
				if (0 == strcmp(info[i].name, "DV Input"))
					continue;
			} else {
				if (0 == strcmp(info[i].name, "None Out")) {
					// we keep the Null audio driver if none else matchs
					*id = info[i].node.node;
					if (msg)
						fPhysicalAudioOutInputID = input_id;
					continue;
				}
				// skip the Firewire audio driver
				if (0 == strcmp(info[i].name, "DV Output"))
					continue;
			}
		}
		if (msg) {	// we have a default info msg
			dormant_node_info dninfo;
			if (fRoster->GetDormantNodeFor(info[i].node,
					&dninfo) != B_OK) {
				ERROR("Couldn't GetDormantNodeFor\n");
				continue;
			}
			if (dninfo.flavor_id != msgDninfo.flavor_id
				|| strcmp(dninfo.name, msgDninfo.name) != 0) {
				ERROR("Doesn't match flavor or name\n");
				continue;
			}
			BPath path;
			if (gDormantNodeManager->FindAddOnPath(&path, dninfo.addon) != B_OK
				|| path != msgPath) {
				ERROR("Doesn't match : path\n");
				continue;
			}
		}
		TRACE("Default physical %s %s \"%s\" created!\n",
			isAudio ? "audio" : "video", isInput ? "input" : "output",
			info[i].name);
		*id = info[i].node.node;
		if (msg && isAudio && !isInput)
			fPhysicalAudioOutInputID = input_id;
		return;
	}
}


void
DefaultManager::_FindTimeSource()
{
	live_node_info info[MAX_NODE_INFOS];
	media_format input; /* a physical audio output has a logical data input (DAC)*/
	int32 count;
	status_t rv;

	/* First try to use the current default physical audio out
	 */
	if (fPhysicalAudioOut != -1) {
		media_node clone;
		if (fRoster->GetNodeFor(fPhysicalAudioOut,
				&clone) == B_OK) {
			if (clone.kind & B_TIME_SOURCE) {
				fTimeSource = clone.node;
				fRoster->StartTimeSource(clone,
					system_time() + 1000);
				fRoster->ReleaseNode(clone);
				TRACE("Default DAC timesource created!\n");
				return;
			}
			fRoster->ReleaseNode(clone);
		} else {
			TRACE("Default DAC is not a timesource!\n");
		}
	} else {
		TRACE("Default DAC node does not exist!\n");
	}

	/* Now try to find another physical audio out node
	 */
	input.type = B_MEDIA_RAW_AUDIO;
	count = MAX_NODE_INFOS;
	rv = fRoster->GetLiveNodes(&info[0], &count, &input, NULL, NULL,
		B_TIME_SOURCE | B_PHYSICAL_OUTPUT);
	if (rv == B_OK && count >= 1) {
		for (int i = 0; i < count; i++)
			printf("info[%d].name %s\n", i, info[i].name);

		for (int i = 0; i < count; i++) {
			// The BeOS R5 None Out node pretend to be a physical time source,
			// that is pretty dumb
			// skip the Null audio driver
			if (0 == strcmp(info[i].name, "None Out"))
				continue;
			// skip the Firewire audio driver
			if (0 != strstr(info[i].name, "DV Output"))
				continue;
			TRACE("Default DAC timesource \"%s\" created!\n", info[i].name);
			fTimeSource = info[i].node.node;
			fRoster->StartTimeSource(info[i].node,
				system_time() + 1000);
			return;
		}
	} else {
		TRACE("Couldn't find DAC timesource node\n");
	}

	/* XXX we might use other audio or video clock timesources
	 */
}


void
DefaultManager::_FindAudioMixer()
{
	live_node_info info;
	int32 count;
	status_t rv;

	if (fRoster == NULL)
		fRoster = BMediaRoster::Roster();

	count = 1;
	rv = fRoster->GetLiveNodes(&info, &count, NULL, NULL, NULL,
		B_BUFFER_PRODUCER | B_BUFFER_CONSUMER | B_SYSTEM_MIXER);
	if (rv != B_OK || count != 1) {
		TRACE("Couldn't find audio mixer node\n");
		return;
	}
	fAudioMixer = info.node.node;
	TRACE("Default audio mixer node created\n");
}


status_t
DefaultManager::_ConnectMixerToOutput()
{
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

	if (fRoster == NULL)
		fRoster = BMediaRoster::Roster();

	rv = fRoster->GetNodeFor(fPhysicalAudioOut, &soundcard);
	if (rv != B_OK) {
		TRACE("DefaultManager: failed to find soundcard (physical audio "
			"output)\n");
		return B_ERROR;
	}

	rv = fRoster->GetNodeFor(fAudioMixer, &mixer);
	if (rv != B_OK) {
		fRoster->ReleaseNode(soundcard);
		TRACE("DefaultManager: failed to find mixer\n");
		return B_ERROR;
	}

	// we now have the mixer and soundcard nodes,
	// find a free input/output and connect them

	rv = fRoster->GetFreeOutputsFor(mixer, &output, 1, &count,
		B_MEDIA_RAW_AUDIO);
	if (rv != B_OK || count != 1) {
		TRACE("DefaultManager: can't find free mixer output\n");
		rv = B_ERROR;
		goto finish;
	}

	rv = fRoster->GetFreeInputsFor(soundcard, inputs, MAX_INPUT_INFOS, &count,
		B_MEDIA_RAW_AUDIO);
	if (rv != B_OK || count < 1) {
		TRACE("DefaultManager: can't find free soundcard inputs\n");
		rv = B_ERROR;
		goto finish;
	}

	for (int32 i = 0; i < count; i++) {
		input = inputs[i];
		if (input.destination.id == fPhysicalAudioOutInputID)
			break;
	}

	for (int i = 0; i < 6; i++) {
		switch (i) {
			case 0:
				TRACE("DefaultManager: Trying connect in native format (1)\n");
				if (fRoster->GetFormatFor(input, &format) != B_OK) {
					ERROR("DefaultManager: GetFormatFor failed\n");
					continue;
				}
				// XXX BeOS R5 multiaudio node bug workaround
				if (format.u.raw_audio.channel_count == 1) {
					TRACE("##### WARNING! DefaultManager: ignored mono format\n");
					continue;
				}
				break;

			case 1:
				TRACE("DefaultManager: Trying connect in format 1\n");
				format.Clear();
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 44100;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = 0x2;
				break;

			case 2:
				TRACE("DefaultManager: Trying connect in format 2\n");
				format.Clear();
				format.type = B_MEDIA_RAW_AUDIO;
				format.u.raw_audio.frame_rate = 48000;
				format.u.raw_audio.channel_count = 2;
				format.u.raw_audio.format = 0x2;
				break;

			case 3:
				TRACE("DefaultManager: Trying connect in format 3\n");
				format.Clear();
				format.type = B_MEDIA_RAW_AUDIO;
				break;

			case 4:
				// BeOS R5 multiaudio node bug workaround
				TRACE("DefaultManager: Trying connect in native format (2)\n");
				if (fRoster->GetFormatFor(input, &format) != B_OK) {
					ERROR("DefaultManager: GetFormatFor failed\n");
					continue;
				}
				break;

			case 5:
				TRACE("DefaultManager: Trying connect in format 4\n");
				format.Clear();
				break;

		}
		rv = fRoster->Connect(output.source, input.destination, &format,
			&newoutput, &newinput);
		if (rv == B_OK)
			break;
	}
	if (rv != B_OK) {
		ERROR("DefaultManager: connect failed\n");
		goto finish;
	}

	fRoster->SetRunModeNode(mixer, BMediaNode::B_INCREASE_LATENCY);
	fRoster->SetRunModeNode(soundcard, BMediaNode::B_RECORDING);

	fRoster->GetTimeSource(&timesource);
	fRoster->SetTimeSourceFor(mixer.node, timesource.node);
	fRoster->SetTimeSourceFor(soundcard.node, timesource.node);
	fRoster->PrerollNode(mixer);
	fRoster->PrerollNode(soundcard);

	ts = fRoster->MakeTimeSourceFor(mixer);
	start_at = ts->Now() + 50000;
	fRoster->StartNode(mixer, start_at);
	fRoster->StartNode(soundcard, start_at);
	ts->Release();

finish:
	fRoster->ReleaseNode(mixer);
	fRoster->ReleaseNode(soundcard);
	fRoster->ReleaseNode(timesource);
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
