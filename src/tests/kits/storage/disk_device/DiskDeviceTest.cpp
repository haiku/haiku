//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Message.h>
#include <Messenger.h>
#include <Partition.h>
#include <Path.h>
#include <ObjectList.h>
#include <OS.h>
#include <Session.h>

// needed only until we can link against libopenbeos
#include <RegistrarDefs.h>
#include <Roster.h>
#include <RosterPrivate.h>

// Hack to make BDiskDeviceRoster communicate with our registrar.

BRoster _be_roster;
const BRoster *be_roster = &_be_roster;

// init_roster
void
init_roster()
{
	bool initialized = false;
	// find the registrar port
	port_id rosterPort = find_port(kRosterPortName);
	port_info info;
	if (rosterPort >= 0 && get_port_info(rosterPort, &info) == B_OK) {
		// construct the roster messenger
		struct {
			port_id	fPort;
			int32	fHandlerToken;
			team_id	fTeam;
			int32	extra0;
			int32	extra1;
			bool	fPreferredTarget;
			bool	extra2;
			bool	extra3;
			bool	extra4;
		} fakeMessenger;
		fakeMessenger.fPort = rosterPort;
		fakeMessenger.fHandlerToken = 0;
		fakeMessenger.fTeam = info.team;
		fakeMessenger.fPreferredTarget = true;
		BMessenger mainMessenger = *(BMessenger*)&fakeMessenger;
		// ask for the MIME messenger
		BMessage reply;
		status_t error = mainMessenger.SendMessage(B_REG_GET_MIME_MESSENGER,
												   &reply);
		if (error == B_OK && reply.what == B_REG_SUCCESS) {
			BMessenger mimeMessenger;
			reply.FindMessenger("messenger", &mimeMessenger);
			BRoster::Private(_be_roster).SetTo(mainMessenger, mimeMessenger);
			initialized = true;
		} else {
		}
	}
	if (!initialized) {
		printf("initializing be_roster failed!\n");
		exit(1);
	}
}

//----------------------------------------------------------------------------

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
bigtime_t time = system_time();
		printf("TestApp::MediaChanged()\n");
		PrintDeviceInfo(message);
print_time("PrintDeviceInfo()", time);
		int32 id;
		if (message->FindInt32("device_id", &id) == B_OK) {
			for (int32 i = 0; BDiskDevice *device = fDevices.ItemAt(i); i++) {
				if (device->UniqueID() == id) {
					bool updated;
print_time("finding device", time);
					status_t error = device->Update(&updated);
print_time("updating device", time);
					printf("updated: %d\n", updated);
					if (error == B_OK) {
						DumpVisitor visitor;
						device->Traverse(&visitor);
print_time("dumping device", time);
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
bigtime_t time = system_time();
		int32 deviceID;
		if (message->FindInt32("device_id", &deviceID) == B_OK) {
print_time("finding device_id", time);
			BDiskDeviceRoster roster;
print_time("creating roster", time);
			BDiskDevice device;
			if (roster.GetDeviceWithID(deviceID, &device) == B_OK)
{
print_time("getting device", time);
				DumpVisitor().Visit(&device);
print_time("dumping device", time);
}
		}
	}

private:
	BObjectList<BDiskDevice>	fDevices;
};

// main
int
main()
{
	init_roster();
	// -----------
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
	return 0;
}

