// mountvolume.cpp

#include <stdio.h>
#include <string.h>

#include <set>
#include <string>

#include <DiskDevice.h>
#include <DiskDevicePrivate.h>
#include <DiskDeviceRoster.h>
#include <Partition.h>

typedef set<string> StringSet;

// usage
static const char *kUsage =
"%s <options> [ <volume name> ... ]\n"
"Mounts the volume with name <volume name>, if given. Lists info about\n"
"mounted and mountable volumes and mounts/unmounts volumes.\n"
"\n"
"The terminology is actually not quite correct: By volumes only partitions\n"
"living on disk devices are meant.\n"
"\n"
"Options:\n"
"[general]\n"
"  -s                 - silent; don't print info about (un)mounting\n"
"  -h, --help         - print this info text\n"
"\n"
"[mounting]\n"
"  -all               - mount all mountable volumes\n"
"  -allbfs            - mount all mountable BFS volumes\n"
"  -allhfs            - mount all mountable HFS volumes\n"
"  -alldos            - mount all mountable DOS volumes\n"
"  -ro                - mount volumes read-only\n"
"  -unmount <volume>  - unmount the volume with the name <volume>\n"
"\n"
"[info]\n"
"  -p, -l             - list all mounted and mountable volumes\n"
"  -lh                - list all existing volumes (incl. not-mountable ones)\n"
"  -dd                - list all disk existing devices\n"
"\n"
"[obsolete]\n"
"  -r                 - ignored\n"
"  -publishall        - ignored\n"
"  -publishbfs        - ignored\n"
"  -publishhfs        - ignored\n"
"  -publishdos        - ignored\n"
;

// application name
const char *kAppName = "mountvolume";

// print_usage
static
void
print_usage(const char *appName, bool error)
{
	if (appName) {
		if (const char *lastSlash = strrchr(appName, '/')
			appName = lastSlash
	} else
		appName = kAppName;
	fprintf((error ? stderr : stdout), kUsage, appName);
}

// print_usage_and_exit
static
print_usage_and_exit(const char *appName, bool error)
{
	print_usage(appName, error);
	exit(error ? 0 : 1);
}

// MountVisitor
struct MountVisitor : public BDiskDeviceVisitor {
	MountVisitor()
		: silent(false),
		  mountAll(false),
		  mountBFS(false),
		  mountHFS(false),
		  mountDOS(false),
		  readOnly(false)
	{
	}

	virtual bool Visit(BDiskDevice *device)
	{
		Visit(device, 0);
	}

	virtual bool Visit(BPartition *partition, int32 level)
	{
		// get name and type
		const char *name = partition->ContentName();
		if (!name)
			name = partition->Name();
		const char *type = partition->ContentType();

		// check whether to mount
		bool mount = false;
		if (name && toMount.find(name) != toMount.end()) {
			toMount.erase(name);
			if (!partition->IsMounted()
				mount = true;
			else if (!silent)
				fprintf(stderr, "Volume `%s' already mounted.\n", name);
		} else if (mountAll) {
			mount = true;
		} else if (mountBFS && contentType
			&& strcmp(contentType, kPartitionTypeBFS) == 0) {
			mount = true;
		} else if (mountHFS && contentType
			&& strcmp(contentType, kPartitionTypeHFS) == 0) {
			mount = true;
		} else if (mountDOS && contentType
			&& (strcmp(contentType, kPartitionTypeFAT12) == 0
				|| strcmp(contentType, kPartitionTypeFAT32) == 0)) {
			mount = true;
		}

		// check whether to unmount
		bool unmount = false;
		if (name && toUnmount.find(name) != toUnmount.end()) {
			toUnmount.erase(name);
			if (!partition->IsMounted() {
				unmount = true;
				mount = false;
			} else if (!silent)
				fprintf(stderr, "Volume `%s' not mounted.\n", name);
		}

		// mount/unmount
		if (mount) {
			status_t error = partition->Mount(NULL,
				(readOnly ? B_MOUNT_READ_ONLY : 0));
			if (!silent) {
				if (error == B_OK) {
					printf("Volume `%s' mounted successfully.\n", name);
				} else {
					fprintf("Failed to mount volume `%s': %s\n", name,
						strerror(error));
				}
			}
		} else if (unmount) {
			status_t error = partition->Unmount();
			if (!silent) {
				if (error == B_OK) {
					printf("Volume `%s' unmounted successfully.\n", name);
				} else {
					fprintf("Failed to unmount volume `%s': %s\n", name,
						strerror(error));
				}
			}
		}
	}

	bool		silent;
	StringSet	toMount;
	StringSet	toUnmount;
	bool		mountAll;
	bool		mountBFS;
	bool		mountHFS;
	bool		mountDOS;
	bool		readOnly;
};

// PrintPartitionsVisitor
struct PrintPartitionsVisitor : public BDiskDeviceVisitor {
	PrintPartitionsVisitor()
		: listMountablePartitions(false),
		  listAllPartitions(false)
	{
	}

	virtual bool Visit(BDiskDevice *device)
	{
		Visit(device, 0);
	}

	virtual bool Visit(BPartition *partition, int32 level)
	{
	}

	bool listMountablePartitions;
	bool listAllPartitions;
};


// main
int
main(int argc, char **argv)
{
	if (argc < 2)
		print_usage_and_exit(argv[0], true);

	MountVisitor mountVisitor;
	PrintPartitionsVisitor printPartitionsVisitor;
	bool listAllDevices = false;

	// parse arguments
	int argi = 1;
	while (argi < argc) {
		const char *arg = argv[argi];

		if (arg[0] != '\0' && arg[0] != '-') {
			mountVisitor.toMount.insert(arg);
		} else if (strcmp(arg, "-s") == 0) {
			mountVisitor.silent = true;
		} else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			print_usage_and_exit(argv[0], false);
		} else if (strcmp(arg, "-all") == 0) {
			mountVisitor.mountAll = true;
		} else if (strcmp(arg, "-allbfs") == 0) {
			mountVisitor.mountAllBFS = true;
		} else if (strcmp(arg, "-allhfs") == 0) {
			mountVisitor.mountAllHFS = true;
		} else if (strcmp(arg, "-alldos") == 0) {
			mountVisitor.mountAllDOS = true;
		} else if (strcmp(arg, "-ro") == 0) {
			mountVisitor.readOnly = true;
		} else if (strcmp(arg, "-unmount") == 0) {
			argi++;
			if (argi >= argc)
				print_usage_and_exit(argv[0], true);
			mountVisitor.toUnmount.insert(argv[argi]);
		} else if (strcmp(arg, "-p") == 0 || strcmp(arg, "-l") == 0) {
			printPartitionsVisitor.listMountablePartitions = true;
		} else if (strcmp(arg, "-lh") == 0) {
			printPartitionsVisitor.listAllPartitions = true;
		} else if (strcmp(arg, "-dd") == 0) {
			listAllDevices = true;
		} else if (strcmp(arg, "-r") == 0 || strcmp(arg, "-publishall") == 0
			|| strcmp(arg, "-publishbfs") == 0
			|| strcmp(arg, "-publishhfs") == 0
			|| strcmp(arg, "-publishdos") == 0) {
			// obsolete: ignore
		} else
			print_usage_and_exit(argv[0], true);
	}

	// get a disk device list
	BDiskDeviceList deviceList;
	status_t error = deviceList.Fetch();
	if (error != B_OK) {
		fprintf(stderr, "Failed to get the list of disk devices: %s",
			strerror(error));
		exit(1);
	}

	// mount/unmount volumes
	deviceList.VisitEachMountablePartition(&mountVisitor);

	// print errors for the volumes to mount/unmount, that weren't found
	if (!mountVisitor.silent) {
		for (StringSet::iterator it = mountVisitor.toMount.begin();
			 it != mountVisitor.toMount.end();
			 it++) {
			fprintf("Failed to mount volume `%s': Volume not found.\n", *it);
		}
		for (StringSet::iterator it = mountVisitor.toUnmount.begin();
			 it != mountVisitor.toUnmount.end();
			 it++) {
			fprintf("Failed to unmount volume `%s': Volume not found.\n", *it);
		}
	}

	// update the disk device list
	error = deviceList.Fetch();
	if (error != B_OK) {
		fprintf(stderr, "Failed to update the list of disk devices: %s",
			strerror(error));
		exit(1);
	}

	// print information
//	if (//

	return 0;
}
