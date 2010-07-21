#include <Application.h>
#include <Autolock.h>
#include <String.h>

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Path.h>
#include <NodeMonitor.h>


#include <image.h>
#include <stdio.h>
#include <string.h>

#include "DeviceManager.h"
#include "LocalDeviceImpl.h"

#include "Output.h"
#include "BluetoothServer.h"

#include <bluetoothserver_p.h>


void
DeviceManager::MessageReceived(BMessage* msg)
{
	if (msg->what == B_NODE_MONITOR) {
		int32 opcode;
		if (msg->FindInt32("opcode", &opcode) == B_OK) {
			switch (opcode)	{
				case B_ENTRY_CREATED:
				case B_ENTRY_MOVED:
				{
					entry_ref ref;
					const char *name;
					BDirectory dir;

					Output::Instance()->Post("Something new in the bus ... ", BLACKBOARD_DEVICEMANAGER);

					if ((msg->FindInt32("device", &ref.device)!=B_OK)
						|| (msg->FindInt64("directory", &ref.directory)!=B_OK)
						|| (msg->FindString("name",	&name) != B_OK))
						return;

					Output::Instance()->Postf(BLACKBOARD_DEVICEMANAGER, " -> %s\n",	name);

					ref.set_name(name);

					// Check if	the	entry is a File	or a directory
					if (dir.SetTo(&ref) == B_OK) {
						printf("%s: Entry %s is taken as a dir\n", __FUNCTION__, name);
					    node_ref nref;
					    dir.GetNodeRef(&nref);
						AddDirectory(&nref);

					} else {
						printf("%s: Entry %s is taken as a file\n", __FUNCTION__, name);
                        AddDevice(&ref);
					}
				}
				break;
				case B_ENTRY_REMOVED:
				{
					Output::Instance()->Post("Something removed from the bus ...\n",
												BLACKBOARD_DEVICEMANAGER);

				}
				break;
				case B_STAT_CHANGED:
				case B_ATTR_CHANGED:
				case B_DEVICE_MOUNTED:
				case B_DEVICE_UNMOUNTED:
				default:
					BLooper::MessageReceived(msg);
				break;
			}
		}
	}
}


status_t
DeviceManager::AddDirectory(node_ref *nref)
{
	BDirectory directory(nref);
	status_t status	= directory.InitCheck();
	if (status != B_OK)	{
		Output::Instance()->Post("AddDirectory::Initcheck Failed\n", BLACKBOARD_DEVICEMANAGER);
		return status;
	}

	status = watch_node(nref, B_WATCH_DIRECTORY, this);
	if (status != B_OK)	{
		Output::Instance()->Post("AddDirectory::watch_node	Failed\n", BLACKBOARD_DEVICEMANAGER);
		return status;
	}

//	BPath path(*nref);
//	BString	str(path.Path());
//
//	Output::Instance()->Postf(BLACKBOARD_DEVICEMANAGER,
//								"Exploring entries in %s\n", str.String());

	entry_ref ref;
	status_t error;
	while ((error =	directory.GetNextRef(&ref))	== B_OK) {
		// its suposed to be devices ...
		AddDevice(&ref);
	}

	Output::Instance()->Postf(BLACKBOARD_DEVICEMANAGER,
								"Finished exploring entries(%s)\n", strerror(error));

	return (error == B_OK || error == B_ENTRY_NOT_FOUND)?B_OK:error;
}


status_t
DeviceManager::RemoveDirectory(node_ref* nref)
{
	BDirectory directory(nref);
	status_t status	= directory.InitCheck();
	if (status != B_OK)
		return status;

	status = watch_node(nref, B_STOP_WATCHING, this);
	if (status != B_OK)
		return status;

	BEntry entry;
	while (directory.GetNextEntry(&entry, true)	== B_OK) {
		entry_ref ref;
		entry.GetRef(&ref);
		BMessage msg(B_NODE_MONITOR);
		msg.AddInt32("opcode", B_ENTRY_REMOVED);
		msg.AddInt32("device", nref->device);
		msg.AddInt64("directory", nref->node);
		msg.AddString("name", ref.name);
		//addon->fDevice->Control(NULL,	NULL, msg.what,	&msg);
	}

	return B_OK;
}


status_t
DeviceManager::AddDevice(entry_ref* ref)
{
	BPath path(ref);
	BString* str = new BString(path.Path());

	BMessage* msg =	new	BMessage(BT_MSG_ADD_DEVICE);
	msg->AddInt32("opcode",	B_ENTRY_CREATED);
	msg->AddInt32("device",	ref->device);
	msg->AddInt64("directory", ref->directory);

	msg->AddString("name", *str	);

	Output::Instance()->Postf(BLACKBOARD_DEVICEMANAGER, "Device %s registered\n", path.Path());
	return be_app_messenger.SendMessage(msg);
}


DeviceManager::DeviceManager() :
	fLock("device manager")
{

}


DeviceManager::~DeviceManager()
{

}


void
DeviceManager::LoadState()
{
	if (!Lock())
		return;
	Run();
	Unlock();
}


void
DeviceManager::SaveState()
{

}


status_t
DeviceManager::StartMonitoringDevice(const char	*device)
{

	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");

	/* Build the path */
	if ((err = path.Append(device))	!= B_OK) {
		printf("DeviceManager::StartMonitoringDevice BPath::Append() error %s: %s\n", path.Path(), strerror(err));
		return err;
	}

	/* Check the path */
	if ((err = directory.SetTo(path.Path())) !=	B_OK) {
		/* Entry not there ... */
		if (err	!= B_ENTRY_NOT_FOUND) {	// something else we cannot	handle
			printf("DeviceManager::StartMonitoringDevice SetTo error %s: %s\n",	path.Path(), strerror(err));
			return err;
		}
		/* Create it */
		if ((err = create_directory(path.Path(), S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != B_OK
			|| (err	= directory.SetTo(path.Path()))	!= B_OK) {
			printf("DeviceManager::StartMonitoringDevice CreateDirectory error %s: %s\n", path.Path(), strerror(err));
			return err;
		}
	}

	// get noderef
	if ((err = directory.GetNodeRef(&nref))	!= B_OK) {
		printf("DeviceManager::StartMonitoringDevice GetNodeRef	error %s: %s\n", path.Path(), strerror(err));
		return err;
	}

	// start monitoring	the	root
	status_t error = watch_node(&nref, B_WATCH_DIRECTORY, this);
	if (error != B_OK)
		return error;

	Output::Instance()->Postf(BLACKBOARD_DEVICEMANAGER,	"%s	path being monitored\n", path.Path());

	// We are monitoring the root we may have already directories inside
	// to be monitored
	entry_ref driverRef;
	while ((error =	directory.GetNextRef(&driverRef)) == B_OK) {

		// its suposed to be directories that needs	to be monitored...
		BNode driverNode(&driverRef);
		node_ref driverNRef;
		driverNode.GetNodeRef(&driverNRef);
		AddDirectory(&driverNRef);
	}

    Output::Instance()->Postf(BLACKBOARD_DEVICEMANAGER, "Finished exploring entries(%s)\n", strerror(error));

#if	0
	HCIDelegate	*tmphd = NULL;
	int32 i	= 0;

	// TODO!! ask the server if	this needs to be monitored

	while ((tmphd =	(HCIDelegate *)fDelegatesList.ItemAt(i++)) !=NULL) {

		/* Find	out	the	reference*/
		node_ref *dnref	= (node_ref	*)tmphd->fMonitoredRefs	;
		if (*dnref == nref)	{
			printf("StartMonitoringDevice already monitored\n");
			alreadyMonitored = true;
			break;
		}

	}
#endif

	return B_OK;
}


status_t
DeviceManager::StopMonitoringDevice(const char *device)
{
	status_t err;
	node_ref nref;
	BDirectory directory;
	BPath path("/dev");
	if (((err =	path.Append(device)) !=	B_OK)
		|| ((err = directory.SetTo(path.Path())) !=	B_OK)
		|| ((err = directory.GetNodeRef(&nref))	!= B_OK))
		return err;

	// test	if still monitored
/*
	bool stillMonitored	= false;
	int32 i	= 0;
	while ((tmpaddon = (_BDeviceAddOn_ *)fDeviceAddons.ItemAt(i++))	!=NULL)	{
		if (addon == tmpaddon)
			continue;

		int32 j=0;
		node_ref *dnref	= NULL;
		while ((dnref =	(node_ref *)tmpaddon->fMonitoredRefs.ItemAt(j++)) != NULL) {
			if (*dnref == nref)	{
				stillMonitored = true;
				break;
			}
		}
		if (stillMonitored)
			break;
	}

	// remove from list
	node_ref *dnref	= NULL;
	int32 j=0;
	while ((dnref =	(node_ref *)addon->fMonitoredRefs.ItemAt(j)) !=	NULL) {
		if (*dnref == nref)	{
			addon->fMonitoredRefs.RemoveItem(j);
			delete dnref;
			break;
		}
		j++;
	}

	// stop	monitoring if needed
	if (!stillMonitored) {
		if ((err = RemoveDirectory(&nref, addon)) != B_OK)
			return err;
	}
*/
	return B_OK;
}
