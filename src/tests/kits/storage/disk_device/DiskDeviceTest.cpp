//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <DiskDevice.h>
#include <DiskDeviceJob.h>
//#include <DiskDeviceList.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <DiskSystem.h>
#include <KDiskDeviceManager.h>	// for userland testing only
#include <Message.h>
#include <Messenger.h>
#include <Partition.h>
#include <Path.h>
//#include <ObjectList.h>
#include <OS.h>

const char *kTestFileDevice = "/boot/home/tmp/test-file-device";

// DumpVisitor
class DumpVisitor : public BDiskDeviceVisitor {
public:
	virtual bool Visit(BDiskDevice *device)
	{
		BPath path;
		status_t error = device->GetPath(&path);
		const char *pathString = NULL;
		if (error == B_OK)
			pathString = path.Path();
		else
			pathString = strerror(error);
		printf("device %ld: `%s'\n", device->ID(), pathString);
		printf("  has media:      %d\n", device->HasMedia());
		printf("  removable:      %d\n", device->IsRemovableMedia());
		printf("  read only:      %d\n", device->IsReadOnlyMedia());
		printf("  write once:     %d\n", device->IsWriteOnceMedia());
		printf("  ---\n");
		Visit(device, 0);
		return false;
	}
	
	virtual bool Visit(BPartition *partition, int32 level)
	{
		char prefix[128];
		sprintf(prefix, "%*s", 2 * (int)level, "");
		if (level > 0) {
			BPath path;
			status_t error = partition->GetPath(&path);
			const char *pathString = NULL;
			if (error == B_OK)
				pathString = path.Path();
			else
				pathString = strerror(error);
			printf("%spartition %ld: `%s'\n", prefix, partition->ID(),
				   pathString);
		}
		printf("%s  offset:         %lld\n", prefix, partition->Offset());
		printf("%s  size:           %lld\n", prefix, partition->Size());
		printf("%s  block size:     %lu\n", prefix, partition->BlockSize());
		printf("%s  index:          %ld\n", prefix, partition->Index());
		printf("%s  status:         %lu\n", prefix, partition->Status());
		printf("%s  file system:    %d\n", prefix,
			   partition->ContainsFileSystem());
		printf("%s  part. system:   %d\n", prefix,
			   partition->ContainsPartitioningSystem());
		printf("%s  device:         %d\n", prefix, partition->IsDevice());
		printf("%s  read only:      %d\n", prefix, partition->IsReadOnly());
		printf("%s  mounted:        %d\n", prefix, partition->IsMounted());
		printf("%s  flags:          %lx\n", prefix, partition->Flags());
		printf("%s  name:           `%s'\n", prefix, partition->Name());
		printf("%s  content name:   `%s'\n", prefix, partition->ContentName());
		printf("%s  type:           `%s'\n", prefix, partition->Type());
		printf("%s  content type:   `%s'\n", prefix, partition->ContentType());
		// volume, icon,...
		return false;
	}
};

/*
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
				if (device->ID() == id) {
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
*/

// wait_for_jobs
void
wait_for_jobs()
{
	BDiskDeviceRoster roster;
	bool activeJobs = false;
	do {
		if (activeJobs)
			snooze(50000);
		activeJobs = false;
		printf("disk device jobs at time %lld\n", system_time());
		roster.RewindActiveJobs();
		BDiskDeviceJob job;
		while (roster.GetNextActiveJob(&job) == B_OK) {
			activeJobs = true;
			printf("  job %ld:\n", job.ID());
			printf("    type:        %lu\n", job.Type());
			printf("    description: `%s'\n", job.Description());
			printf("    partition:   %ld\n", job.PartitionID());
			printf("    status:      %lu\n", job.Status());
			printf("    progress:    %f\n", job.Progress());
		}
		if (!activeJobs)
			printf("  none\n");
	} while (activeJobs);
}

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
/*
	TestApp2 app("application/x-vnd.obos-disk-device-test");
	app.Run();
	return 0;
*/

	// for userland testing only
	KDiskDeviceManager::CreateDefault();
	KDiskDeviceManager::Default()->InitialDeviceScan();

	// wait till all (scan) jobs are done
	wait_for_jobs();
	// add a file device
	BDiskDeviceRoster roster;
	partition_id id = roster.RegisterFileDevice(kTestFileDevice);
	if (id < B_OK)
		printf("registering the file device failed: %s\n", strerror(id));
	else {
		// wait till all (scan) jobs are done
		wait_for_jobs();
	}
	// list all disk devices and partitions
	DumpVisitor visitor;
	roster.VisitEachPartition(&visitor);
	// list disk systems
	BDiskSystem diskSystem;
	while (roster.GetNextDiskSystem(&diskSystem) == B_OK) {
		printf("disk system:\n");
		printf("  name:        `%s'\n", diskSystem.Name());
		printf("  pretty name: `%s'\n", diskSystem.PrettyName());
		printf("  file system: %d (!%d)\n", diskSystem.IsFileSystem(),
			   diskSystem.IsPartitioningSystem());
		// flags
		printf("  flags:\n");
		bool mounted = false;
		bool supports = diskSystem.SupportsDefragmenting(&mounted);
		printf("    defragmenting:          %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsRepairing(true, &mounted);
		printf("    checking:               %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsRepairing(false, &mounted);
		printf("    repairing:              %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsResizing(&mounted);
		printf("    resizing:               %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsResizingChild();
		printf("    resizing child:         %d\n", supports);
		supports = diskSystem.SupportsMoving(&mounted);
		printf("    moving:                 %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsMovingChild();
		printf("    moving child:           %d\n", supports);
		supports = diskSystem.SupportsSettingName();
		printf("    setting name:           %d\n", supports);
		supports = diskSystem.SupportsSettingContentName(&mounted);
		printf("    setting content name:   %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsSettingType();
		printf("    setting type:           %d\n", supports);
		supports = diskSystem.SupportsSettingParameters();
		printf("    setting params:         %d\n", supports);
		supports = diskSystem.SupportsSettingContentParameters(&mounted);
		printf("    setting content params: %d (%d)\n", supports, mounted);
		supports = diskSystem.SupportsCreatingChild();
		printf("    creating child:         %d\n", supports);
		supports = diskSystem.SupportsDeletingChild();
		printf("    deleting child:         %d\n", supports);
		supports = diskSystem.SupportsInitializing();
		printf("    initializing:           %d\n", supports);
	}
	// get the file device
	BDiskDevice device;
	status_t error = roster.GetDeviceWithID(id, &device);
	if (error != B_OK)
		printf("Couldn't get device: %s\n", strerror(error));
	// prepare for modifications
	error = device.PrepareModifications();
	if (error != B_OK)
		printf("Preparing modifications failed: %s\n", strerror(error));
	// test resize a partition
	if (error == B_OK) {
		if (BPartition *partition = device.ChildAt(1)) {
			// uninitialize contents
			status_t status = partition->Uninitialize();
			if (status != B_OK) {
				printf("Uninitializing the partition failed: %s\n",
					   strerror(status));
			}
			// resize
			status = partition->Resize(1024 * 200);
			if (status != B_OK) {
				printf("Resizing the partition failed: %s\n",
					   strerror(status));
			}
		}
		printf("\nDevice after changing it:\n");
		device.VisitEachDescendant(&visitor);
	}
	// cancel modifications
/*	if (error == B_OK) {
		error = device.CancelModifications();
		if (error != B_OK)
			printf("Cancelling modifications failed: %s\n", strerror(error));
	}
*/	// commit modifications
	if (error == B_OK) {
		error = device.CommitModifications();
		if (error != B_OK)
			printf("Committing modifications failed: %s\n", strerror(error));
	}
	wait_for_jobs();
//	printf("\nDevice after cancelling the changes:\n");
	printf("\nDevice after committing the changes:\n");
	device.VisitEachDescendant(&visitor);

	// for userland testing only
	KDiskDeviceManager::DeleteDefault();
}

