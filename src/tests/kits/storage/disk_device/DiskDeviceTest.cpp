//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <DiskDevice.h>
#include <DiskDeviceList.h>
#include <DiskDeviceRoster.h>
#include <Message.h>
#include <Messenger.h>
#include <Partition.h>
#include <Path.h>
#include <ObjectList.h>
#include <OS.h>
#include <Session.h>

// DumpVisitor
class DumpVisitor : public BDiskDeviceVisitor {
public:
	virtual bool Visit(BDiskDevice *device)
	{
		printf("device `%s'\n", device->Path());
		BString name;
		if (device->GetName(&name, false, false) == B_OK)
			printf("  name(0,0):     `%s'\n", name.String());
		if (device->GetName(&name, false, true) == B_OK)
			printf("  name(0,1):     `%s'\n", name.String());
		if (device->GetName(&name, true, false) == B_OK)
			printf("  name(1,0):     `%s'\n", name.String());
		if (device->GetName(&name, true, true) == B_OK)
			printf("  name(1,1):     `%s'\n", name.String());
		printf("  size:          %lld\n", device->Size());
		printf("  block size:    %ld\n", device->BlockSize());
		printf("  read-only:     %d\n", device->IsReadOnly());
		printf("  removable:     %d\n", device->IsRemovable());
		printf("  has media:     %d\n", device->HasMedia());
		printf("  type:          0x%x\n", device->Type());
		return false;
	}
	
	virtual bool Visit(BSession *session)
	{
		printf("  session %ld:\n", session->Index());
		printf("    offset:        %lld\n", session->Offset());
		printf("    size:          %lld\n", session->Size());
		printf("    block size:    %ld\n", session->BlockSize());
		printf("    flags:         %lx\n", session->Flags());
		printf("    partitioning : `%s'\n", session->PartitioningSystem());
		return false;
	}
	
	virtual bool Visit(BPartition *partition)
	{
		printf("    partition %ld:\n", partition->Index());
		printf("      offset:         %lld\n", partition->Offset());
		printf("      size:           %lld\n", partition->Size());
//		printf("      device:         `%s'\n", partition->Size());
		printf("      flags:          %lx\n", partition->Flags());
		printf("      partition name: `%s'\n", partition->Name());
		printf("      partition type: `%s'\n", partition->Type());
		printf("      FS short name:  `%s'\n",
			   partition->FileSystemShortName());
		printf("      FS long name:   `%s'\n",
			   partition->FileSystemLongName());
		printf("      volume name:    `%s'\n", partition->VolumeName());
		printf("      FS flags:       0x%lx\n", partition->FileSystemFlags());
		return false;
	}
};

// print_time
void
print_time(const char *format, bigtime_t &time)
{
	bigtime_t lastTime = time;
	time = system_time();
	printf("%lld: %s took: %lld us\n", time, format, time - lastTime);
}

// TestApp
class TestApp : public BApplication {
public:
	TestApp(const char *signature)
		: BApplication(signature),
		  fDevices(20, true)
	{
		BDiskDeviceRoster roster;
		bool done = false;
		do {
			BDiskDevice *device = new BDiskDevice;
			done = (roster.GetNextDevice(device) != B_OK);
			if (done)
				delete device;
			else
				fDevices.AddItem(device);
		} while (!done);
	}

	~TestApp()
	{
		for (int32 i = 0; BDiskDevice *device = fDevices.ItemAt(i); i++) {
			status_t error = device->Eject(true);
			printf("eject device `%s': %s\n", device->Path(), strerror(error));
		}
	}

	virtual void MessageReceived(BMessage *message)
	{
printf("TestApp::MessageReceived(%.4s)\n", (char*)&message->what);
		switch (message->what) {
			case B_DEVICE_UPDATE:
			{
				uint32 event;
				if (message->FindInt32("event", (int32*)&event) == B_OK) {
					switch (event) {
						case B_DEVICE_MOUNT_POINT_MOVED:
							MountPointMoved(message);
							break;
						case B_DEVICE_PARTITION_MOUNTED:
							PartitionMounted(message);
							break;
						case B_DEVICE_PARTITION_UNMOUNTED:
							PartitionUnmounted(message);
							break;
						case B_DEVICE_PARTITION_CHANGED:
							PartitionChanged(message);
							break;
						case B_DEVICE_PARTITION_ADDED:
							PartitionAdded(message);
							break;
						case B_DEVICE_PARTITION_REMOVED:
							PartitionRemoved(message);
							break;
						case B_DEVICE_SESSION_ADDED:
							SessionAdded(message);
							break;
						case B_DEVICE_SESSION_REMOVED:
							SessionRemoved(message);
							break;
						case B_DEVICE_MEDIA_CHANGED:
							MediaChanged(message);
							break;
						case B_DEVICE_ADDED:
							DeviceAdded(message);
							break;
						case B_DEVICE_REMOVED:
							DeviceRemoved(message);
							break;
						default:
							printf("unhandled event\n");
							message->PrintToStream();
							break;
					}
				}
				break;
			}
			default:
				BApplication::MessageReceived(message);
				break;
		}
	}


	void MountPointMoved(BMessage *message)
	{
		printf("TestApp::MountPointMoved()\n");
		PrintPartitionInfo(message);
		entry_ref oldRoot, newRoot;
		BPath oldPath, newPath;
		if (message->FindRef("old_directory", &oldRoot) == B_OK
			&& message->FindRef("new_directory", &newRoot) == B_OK
			&& oldPath.SetTo(&oldRoot) == B_OK
			&& newPath.SetTo(&newRoot) == B_OK) {
			printf("old mount point: `%s'\n", oldPath.Path());
			printf("new mount point: `%s'\n", newPath.Path());
		}

	}

	void PartitionMounted(BMessage *message)
	{
		printf("TestApp::PartitionMounted()\n");
		PrintPartitionInfo(message);
	}

	void PartitionUnmounted(BMessage *message)
	{
		printf("TestApp::PartitionUnmounted()\n");
		PrintPartitionInfo(message);
	}

	// PartitionChanged
	void PartitionChanged(BMessage *message)
	{
		printf("TestApp::PartitionUnmounted()\n");
		PrintPartitionInfo(message);
	}

	// PartitionAdded
	void PartitionAdded(BMessage *message)
	{
		printf("TestApp::PartitionAdded()\n");
		PrintPartitionInfo(message);
	}

	// PartitionRemoved
	void PartitionRemoved(BMessage *message)
	{
		printf("TestApp::PartitionRemoved()\n");
		PrintPartitionInfo(message);
	}

	// SessionAdded
	void SessionAdded(BMessage *message)
	{
		printf("TestApp::SessionAdded()\n");
		PrintSessionInfo(message);
	}

	// SessionRemoved
	void SessionRemoved(BMessage *message)
	{
		printf("TestApp::SessionRemoved()\n");
		PrintSessionInfo(message);
	}

	// MediaChanged
	void MediaChanged(BMessage *message)
	{
		printf("TestApp::MediaChanged()\n");
		PrintDeviceInfo(message);
		int32 id;
		if (message->FindInt32("device_id", &id) == B_OK) {
			for (int32 i = 0; BDiskDevice *device = fDevices.ItemAt(i); i++) {
				if (device->UniqueID() == id) {
					bool updated;
					status_t error = device->Update(&updated);
					printf("updated: %d\n", updated);
					if (error == B_OK) {
						DumpVisitor visitor;
						device->Traverse(&visitor);
					} else {
						printf("device->Update() failed: %s\n",
							   strerror(error));
					}
					break;
				}
			}
		}
	}

	// DeviceAdded
	void DeviceAdded(BMessage *message)
	{
		printf("TestApp::DeviceAdded()\n");
		PrintDeviceInfo(message);
	}

	// DeviceRemoved
	void DeviceRemoved(BMessage *message)
	{
		printf("TestApp::DeviceRemoved()\n");
		PrintDeviceInfo(message);
	}

	// PrintPartitionInfo
	void PrintPartitionInfo(BMessage *message)
	{
		int32 deviceID;
		int32 sessionID;
		int32 partitionID;
		if (message->FindInt32("device_id", &deviceID) == B_OK
			&& message->FindInt32("session_id", &sessionID) == B_OK
			&& message->FindInt32("partition_id", &partitionID) == B_OK) {
			BDiskDeviceRoster roster;
			BDiskDevice device;
			BPartition *partition;
			if (roster.GetPartitionWithID(partitionID, &device, &partition)
				== B_OK) {
				DumpVisitor().Visit(partition);
			}
		}
	}

	// PrintSessionInfo
	void PrintSessionInfo(BMessage *message)
	{
		int32 deviceID;
		int32 sessionID;
		if (message->FindInt32("device_id", &deviceID) == B_OK
			&& message->FindInt32("session_id", &sessionID) == B_OK) {
			BDiskDeviceRoster roster;
			BDiskDevice device;
			BSession *session;
			if (roster.GetSessionWithID(sessionID, &device, &session)
				== B_OK) {
				DumpVisitor().Visit(session);
			}
		}
	}

	// PrintDeviceInfo
	void PrintDeviceInfo(BMessage *message)
	{
		int32 deviceID;
		if (message->FindInt32("device_id", &deviceID) == B_OK) {
			BDiskDeviceRoster roster;
			BDiskDevice device;
			if (roster.GetDeviceWithID(deviceID, &device) == B_OK)
				DumpVisitor().Visit(&device);
		}
	}

private:
	BObjectList<BDiskDevice>	fDevices;
};

class MyDeviceList : public BDiskDeviceList {
public:
	MyDeviceList(bool useOwnLocker)
		: BDiskDeviceList(useOwnLocker)
	{
	}

	virtual void MountPointMoved(BPartition *partition)
	{
		printf("MountPointMoved()\n");
		DumpVisitor().Visit(partition);
	}

	virtual void PartitionMounted(BPartition *partition)
	{
		printf("PartitionMounted()\n");
		DumpVisitor().Visit(partition);
	}

	virtual void PartitionUnmounted(BPartition *partition)
	{
		printf("PartitionUnmounted()\n");
		DumpVisitor().Visit(partition);
	}

	virtual void PartitionChanged(BPartition *partition)
	{
		printf("PartitionChanged()\n");
		DumpVisitor().Visit(partition);
	}

	virtual void PartitionAdded(BPartition *partition)
	{
		printf("PartitionAdded()\n");
		DumpVisitor().Visit(partition);
	}

	virtual void PartitionRemoved(BPartition *partition)
	{
		printf("PartitionRemoved()\n");
		DumpVisitor().Visit(partition);
	}

	virtual void SessionAdded(BSession *session)
	{
		printf("SessionAdded()\n");
		DumpVisitor().Visit(session);
	}

	virtual void SessionRemoved(BSession *session)
	{
		printf("SessionRemoved()\n");
		DumpVisitor().Visit(session);
	}

	virtual void MediaChanged(BDiskDevice *device)
	{
		printf("MediaChanged()\n");
		DumpVisitor().Visit(device);
	}

	virtual void DeviceAdded(BDiskDevice *device)
	{
		printf("DeviceAdded()\n");
		DumpVisitor().Visit(device);
	}

	virtual void DeviceRemoved(BDiskDevice *device)
	{
		printf("DeviceRemoved()\n");
		DumpVisitor().Visit(device);
	}
};


// TestApp2
class TestApp2 : public BApplication {
public:
	TestApp2(const char *signature)
		: BApplication(signature),
		  fDeviceList(false)
	{
		printf("dumping empty list...\n");
		DumpVisitor visitor;
		fDeviceList.Traverse(&visitor);
		printf("dumping done\n");
		printf("fetching\n");
		status_t error = fDeviceList.Fetch();
		if (error == B_OK)
			fDeviceList.Traverse(&visitor);
		else
			printf("fetching failed: %s\n", strerror(error));
		printf("unset\n");
		fDeviceList.Unset();
		printf("dumping empty list...\n");
		fDeviceList.Traverse(&visitor);
		printf("dumping done\n");
		AddHandler(&fDeviceList);
		error = fDeviceList.Fetch();
		if (error != B_OK)
			printf("fetching failed: %s\n", strerror(error));
	}

	~TestApp2()
	{
		Lock();
		RemoveHandler(&fDeviceList);
		Unlock();
	}

	virtual void ReadyToRun()
	{
		// list the available partition add-ons
		BDiskDeviceRoster roster;
		char shortName[B_FILE_NAME_LENGTH];
		char longName[B_FILE_NAME_LENGTH];
		printf("partition add-ons:\n");
		while (roster.GetNextPartitioningSystem(shortName, longName) == B_OK) {
			printf("  `%s' (`%s')\n", shortName, longName);
		}
		fDeviceList.Lock();
		for (int32 i = 0; BDiskDevice *device = fDeviceList.DeviceAt(i); i++) {
printf("device: `%s'\n", device->Path());
			if (!strcmp(device->Path(), "/dev/disk/virtual/0/raw")) {
				if (BSession *session = device->SessionAt(0)) {
					BString parameters;
					bool cancelled = false;
					status_t error = session->GetPartitioningParameters(
						"intel", &parameters, BRect(), &cancelled);
					if (error == B_OK) {
						printf("partitioning parameters: `%s'\n",
							   parameters.String());
					} else if (error == B_ERROR && cancelled) {
						printf("partitioning dialog cancelled\n");
					} else {
						printf("error getting partitioning parameters: %s\n",
							   strerror(error));
					}
				}
				break;
			}
		}
		fDeviceList.Unlock();
	}

private:
	MyDeviceList	fDeviceList;
};

// main
int
main()
{
/*
	TestApp app("application/x-vnd.obos-disk-device-test");
	BDiskDeviceRoster roster;
	DumpVisitor visitor;
	roster.Traverse(&visitor);
	status_t error = B_OK;
	error = roster.StartWatching(BMessenger(&app));
	if (error != B_OK)
		printf("start watching failed: %s\n", strerror(error));
	if (error == B_OK)
		app.Run();
	error = roster.StopWatching(BMessenger(&app));
	if (error != B_OK)
		printf("stop watching failed: %s\n", strerror(error));
*/
	TestApp2 app("application/x-vnd.obos-disk-device-test");
	app.Run();
	return 0;
}

