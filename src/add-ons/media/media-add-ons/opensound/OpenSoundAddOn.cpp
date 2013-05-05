/*
 * OpenSound media addon for BeOS and Haiku
 *
 * Copyright (c) 2007, François Revol (revol@free.fr)
 * Distributed under the terms of the MIT License.
 * 
 * Based on MultiAudio media addon
 * Copyright (c) 2002, 2003 Jerome Duval (jerome.duval@free.fr)
 */
#include "OpenSoundAddOn.h"

#include <MediaDefs.h>
#include <MediaAddOn.h>
#include <Errors.h>
#include <Node.h>
#include <Mime.h>
#include <StorageDefs.h>
#include <Path.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Debug.h>
#include <errno.h>

#include "OpenSoundNode.h"
#include "OpenSoundDevice.h"
#include "OpenSoundDeviceEngine.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"

#define MULTI_SAVE


// instantiation function
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id image) {
	CALLED();
	return new OpenSoundAddOn(image);
}

// -------------------------------------------------------- //
// ctor/dtor
// -------------------------------------------------------- //

OpenSoundAddOn::~OpenSoundAddOn()
{
	CALLED();

	void *device = NULL;
	for (int32 i = 0; (device = fDevices.ItemAt(i)); i++)
		delete (OpenSoundDevice *)device;

	SaveSettings();
}

OpenSoundAddOn::OpenSoundAddOn(image_id image) :
	BMediaAddOn(image),
	fDevices()
{
	CALLED();
	fInitCheckStatus = B_NO_INIT;

	/* unix paths */
	if (RecursiveScan("/dev/oss/") != B_OK)
		return;
	/*
	if (RecursiveScan("/dev/audio/oss/") != B_OK)
		return;
	*/
	
	LoadSettings();

	fInitCheckStatus = B_OK;
}

// -------------------------------------------------------- //
// BMediaAddOn impl
// -------------------------------------------------------- //

status_t OpenSoundAddOn::InitCheck(
	const char ** out_failure_text)
{
	CALLED();
	return B_OK;
}

int32 OpenSoundAddOn::CountFlavors()
{
	CALLED();
	PRINT(("%" B_PRId32 " flavours\n", fDevices.CountItems()));
	return fDevices.CountItems();
}

status_t OpenSoundAddOn::GetFlavorAt(
	int32 n,
	const flavor_info ** out_info)
{
	CALLED();
	if (n < 0 || n > fDevices.CountItems() - 1) {
		fprintf(stderr, "<- B_BAD_INDEX\n");
		return B_BAD_INDEX;
	}

	OpenSoundDevice *device = (OpenSoundDevice *) fDevices.ItemAt(n);

	flavor_info * infos = new flavor_info[1];
	OpenSoundNode::GetFlavor(&infos[0], n);
	infos[0].name = device->fCardInfo.longname;
	(*out_info) = infos;
	return B_OK;
}

BMediaNode * OpenSoundAddOn::InstantiateNodeFor(
	const flavor_info * info,
	BMessage * config,
	status_t * out_error)
{
	CALLED();
	
	OpenSoundDevice *device = (OpenSoundDevice*)fDevices.ItemAt(
		info->internal_id);
	if (device == NULL) {
		*out_error = B_ERROR;
		return NULL;
	}

#ifdef MULTI_SAVE
	if (fSettings.FindMessage(device->fCardInfo.longname, config) == B_OK) {
		fSettings.RemoveData(device->fCardInfo.longname);
	}
#endif

	OpenSoundNode * node =
		new OpenSoundNode(this,
			device->fCardInfo.longname,
			device,
			info->internal_id,
			config);
	if (node == 0) {
		*out_error = B_NO_MEMORY;
		fprintf(stderr, "<- B_NO_MEMORY\n");
	} else {
		*out_error = node->InitCheck();
	}
	return node;
}

status_t
OpenSoundAddOn::GetConfigurationFor(BMediaNode * your_node, BMessage * into_message)
{
	CALLED();
#ifdef MULTI_SAVE
	{
		into_message = new BMessage();
		OpenSoundNode * node = dynamic_cast<OpenSoundNode*>(your_node);
		if (node == 0) {
			fprintf(stderr, "<- B_BAD_TYPE\n");
			return B_BAD_TYPE;
		}
		if (node->GetConfigurationFor(into_message) == B_OK) {
			fSettings.AddMessage(your_node->Name(), into_message);
		}
		return B_OK;
	}
#endif
	// currently never called by the media kit. Seems it is not implemented.
	OpenSoundNode * node = dynamic_cast<OpenSoundNode*>(your_node);
	if (node == 0) {
		fprintf(stderr, "<- B_BAD_TYPE\n");
		return B_BAD_TYPE;
	}
	return node->GetConfigurationFor(into_message);
}


bool OpenSoundAddOn::WantsAutoStart()
{
	CALLED();
	return false;
}

status_t OpenSoundAddOn::AutoStart(
	int in_count,
	BMediaNode ** out_node,
	int32 * out_internal_id,
	bool * out_has_more)
{
	CALLED();
	return B_OK;
}

status_t
OpenSoundAddOn::RecursiveScan(const char* rootPath, BEntry *rootEntry)
{
	status_t err;
	int mixer;
	oss_sysinfo sysinfo;
	oss_card_info cardinfo;
	int card, i, j;
	BList devs;
	
	CALLED();

	// make sure directories are scanned in this order
	BDirectory("/dev/audio/hmulti");
	BDirectory("/dev/audio/old");
	// OSS last, to give precedence to native drivers.
	// If other addons are loaded first it's ok as well.
	// Also, we must open it to make sure oss_loader is here,
	// else we don't get /dev/sndstat since we don't have a symlink in dev/.
	BDirectory("/dev/oss");

	mixer = open(OSS_MIXER_DEV, O_RDWR);
	if (mixer < 0) {
		// try to rescan
		// only works in BeOS
		BFile fDevFS("/dev/.", B_WRITE_ONLY);
		const char *drv = "oss_loader";
		fDevFS.Write(drv, strlen(drv));
		mixer = open(OSS_MIXER_DEV, O_RDWR);
		if (mixer < 0) {
			err = errno;
			goto err0;
		}
	}

	if (ioctl(mixer, SNDCTL_SYSINFO, &sysinfo) < 0) {
		err = errno;
		goto err1;
	}
	
	PRINT(("OSS: %s %s (0x%08X)\n", sysinfo.product, sysinfo.version, sysinfo.versionnum));
	PRINT(("OSS: %d audio cards, %d audio devs, %d audio engines, %d midi, %d mixers\n", sysinfo.numcards, sysinfo.numaudios, sysinfo.numaudioengines, sysinfo.nummidis, sysinfo.nummixers));

	/* construct an empty SoundDevice per card */
	
	for (card = 0; card < sysinfo.numcards; card++) {
		cardinfo.card = card;
		if (ioctl(mixer, SNDCTL_CARDINFO, &cardinfo) < 0) {
			err = errno;
			goto err1;
		}
		OpenSoundDevice *device = new OpenSoundDevice(&cardinfo);
		if (device)
			devs.AddItem(device);
		else {
			err = ENOMEM;
			goto err1;
		}
	}
	
	/* Add its audio engines to it */
	
	for (i = 0; i < sysinfo.numaudioengines; i++) {
		oss_audioinfo audioinfo;
		audioinfo.dev = i;
		if (ioctl(mixer, SNDCTL_ENGINEINFO, &audioinfo, sizeof(oss_audioinfo)) < 0) {
			err = errno;
			goto err1;
		}
		PRINT(("OSS: engine[%d]: card=%d, port=%d, legacy=%d, next_play=%d, next_rec=%d\n", i, audioinfo.card_number, audioinfo.port_number, audioinfo.legacy_device, audioinfo.next_play_engine, audioinfo.next_rec_engine));
		OpenSoundDevice *device = (OpenSoundDevice *)(devs.ItemAt(audioinfo.card_number));
		if (device)
			device->AddEngine(&audioinfo);
	}
	
	/* Add its mixers to it */
	
	for (i = 0; i < sysinfo.nummixers; i++) {
		oss_mixerinfo mixerinfo;
		mixerinfo.dev = i;
		if (ioctl(mixer, SNDCTL_MIXERINFO, &mixerinfo) < 0) {
			err = errno;
			goto err1;
		}
		PRINT(("OSS: mixer[%d]: card=%d\n", i, mixerinfo.card_number));
		OpenSoundDevice *device = (OpenSoundDevice *)(devs.ItemAt(mixerinfo.card_number));
		if (device)
			device->AddMixer(&mixerinfo);
	}
	
	/* resolve engine chains of shadow engines */
	
	for (card = 0; card < sysinfo.numcards; card++) {
		OpenSoundDevice *device = (OpenSoundDevice *)(devs.ItemAt(card));
		if (!device)
			continue;
		for (i = 0; i < device->CountEngines(); i++) {
			OpenSoundDeviceEngine *engine = device->EngineAt(i);
			if (engine) {
				if (engine->Info()->next_play_engine) {
					for (j = 0; j < device->CountEngines(); j++) {
						OpenSoundDeviceEngine *next = device->EngineAt(j);
						if (!next || (engine == next))
							continue;
						if (next->Info()->dev == engine->Info()->next_play_engine) {
							PRINT(("OSS: engine[%d].next_play = engine[%d]\n", i, j));
							engine->fNextPlay = next;
							break;
						}
					}
				}
				if (engine->Info()->next_rec_engine) {
					for (j = 0; j < device->CountEngines(); j++) {
						OpenSoundDeviceEngine *next = device->EngineAt(j);
						if (!next || (engine == next))
							continue;
						if (next->Info()->dev == engine->Info()->next_rec_engine) {
							PRINT(("OSS: engine[%d].next_rec = engine[%d]\n", i, j));
							engine->fNextRec = next;
							break;
						}
					}
				}
			}
		}
	}
	
	/* copy correctly initialized devs to fDevices */
	
	for (card = 0; card < sysinfo.numcards; card++) {
		OpenSoundDevice *device = (OpenSoundDevice *)(devs.ItemAt(card));
		if (device) {
			if (card == 0) { /* skip the "oss0" pseudo card device */
				delete device;
				continue;
			}
			if ((device->InitDriver() == B_OK) && (device->InitCheck() == B_OK))
				fDevices.AddItem(device);
			else
				delete device;
		}
	}
	if (fDevices.CountItems())
		err = B_OK;
	else
		err = ENOENT;
err1:
	close(mixer);
err0:
	return err;

#if MA
	BDirectory root;
	if (rootEntry != NULL)
		root.SetTo(rootEntry);
	else if (rootPath != NULL) {
		root.SetTo(rootPath);
	} else {
		PRINT(("Error in OpenSoundAddOn::RecursiveScan null params\n"));
		return B_ERROR;
	}

	BEntry entry;

	while (root.GetNextEntry(&entry) > B_ERROR) {

		if (entry.IsDirectory()) {
			BPath path;
			entry.GetPath(&path);
			OpenSoundDevice *device = new OpenSoundDevice(path.Path() + strlen(rootPath), path.Path());
			if (device) {
				if (device->InitCheck() == B_OK)
					fDevices.AddItem(device);
				else
					delete device;
			}
		}
	}

	return B_OK;
#endif
}


void
OpenSoundAddOn::SaveSettings(void)
{
	CALLED();
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(), B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
		if (file.InitCheck() == B_OK)
			fSettings.Flatten(&file);
	}
}


void
OpenSoundAddOn::LoadSettings(void)
{
	CALLED();
	fSettings.MakeEmpty();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(SETTINGS_FILE);
		BFile file(path.Path(), B_READ_ONLY);
		if ((file.InitCheck() == B_OK) && (fSettings.Unflatten(&file) == B_OK))
		{
			PRINT_OBJECT(fSettings);
		} else {
			PRINT(("Error unflattening settings file %s\n", path.Path()));
		}
	}
}


void
OpenSoundAddOn::RegisterMediaFormats(void)
{
	CALLED();
	// register non-raw audio formats to the Media Kit
#ifdef ENABLE_NON_RAW_SUPPORT
	OpenSoundDevice::register_media_formats();
#endif

	
}
