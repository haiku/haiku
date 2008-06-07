/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <DiskDeviceVisitor.h>
#include <DiskSystem.h>
#include <PartitioningInfo.h>
#include <Path.h>
#include <String.h>

#include <ObjectList.h>


extern "C" const char* __progname;
static const char* kProgramName = __progname;

static const char* kUsage =
"Usage: %s <options> <device>\n"
"\n"
"Options:\n"
"  -h, --help   - print this help text\n"
;


static void
print_usage(bool error)
{
	fprintf(error ? stderr : stdout, kUsage, kProgramName);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


static void
get_size_string(off_t _size, BString& string)
{
	const char* suffixes[] = {
		"", "K", "M", "G", "T", NULL
	};

	double size = _size;
	int index = 0;
	while (size > 1024 && suffixes[index + 1]) {
		size /= 1024;
		index++;
	}

	char buffer[128];
	snprintf(buffer, sizeof(buffer), "%.2f%s", size, suffixes[index]);

	string = buffer;
}


// PrintLongVisitor
class PrintLongVisitor : public BDiskDeviceVisitor {
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
		printf("device %ld: \"%s\"\n", device->ID(), pathString);
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
			printf("%spartition %ld: \"%s\"\n", prefix, partition->ID(),
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
		printf("%s  name:           \"%s\"\n", prefix, partition->Name());
		printf("%s  content name:   \"%s\"\n", prefix,
			partition->ContentName());
		printf("%s  type:           \"%s\"\n", prefix, partition->Type());
		printf("%s  content type:   \"%s\"\n", prefix,
			partition->ContentType());
		printf("%s  params:         \"%s\"\n", prefix, partition->Parameters());
		printf("%s  content params: \"%s\"\n", prefix,
			partition->ContentParameters());
		// volume, icon,...
		return false;
	}
};


static void
print_partition_table_header()
{
	printf("   Index     Start      Size                Content Type      "
		"Content Name\n");
	printf("--------------------------------------------------------------"
		"------------\n");
}


static void
print_partition(BPartition* partition, int level, int index)
{
	BString offset, size;
	get_size_string(partition->Offset(), offset);
	get_size_string(partition->Size(), size);

	printf("%*s%02d%*s  %8s  %8s  %26.26s  %16.16s\n", 2 * level, "",
		index,
		2 * max_c(3 - level, 0), "",
		offset.String(), size.String(),
		(partition->ContentType() ? partition->ContentType() : "-"),
		(partition->ContentName() ? partition->ContentName() : ""));
}


// PrintShortVisitor
class PrintShortVisitor : public BDiskDeviceVisitor {
public:
	virtual bool Visit(BDiskDevice* device)
	{
		fIndex = 0;

		// get path
		BPath path;
		status_t error = device->GetPath(&path);
		const char *pathString = NULL;
		if (error == B_OK)
			pathString = path.Path();
		else
			pathString = strerror(error);

		// check media present; if so which read/write abilities
		const char* media;
		const char* readWrite;
		if (device->HasMedia()) {
			if (device->IsRemovableMedia()) {
				media = "removable media";
			} else {
				media = "fixed media";
			}

			if (device->IsReadOnlyMedia()) {
				if (device->IsWriteOnceMedia())
					readWrite = ", write once";
				else
					readWrite = ", read-only";
			} else
				readWrite = ", read/write";
		} else {
			media = "no media";
			readWrite = "";
		}

		printf("\ndevice %ld: \"%s\": %s%s\n\n", device->ID(), pathString,
			media, readWrite);
		print_partition_table_header();

		Visit(device, 0);
		return false;
	}
	
	virtual bool Visit(BPartition* partition, int32 level)
	{
		print_partition(partition, level, fIndex++);
		return false;
	}

private:
	int	fIndex;
};


// FindPartitionByIndexVisitor
class FindPartitionByIndexVisitor : public BDiskDeviceVisitor {
public:
	FindPartitionByIndexVisitor(int index)
		: fIndex(index)
	{
	}

	virtual bool Visit(BDiskDevice *device)
	{
		return Visit(device, 0);
	}
	
	virtual bool Visit(BPartition *partition, int32 level)
	{
		return (fIndex-- == 0);
	}

private:
	int	fIndex;
};


// Partitioner
class Partitioner {
public:
	Partitioner(BDiskDevice* device)
		: fDevice(device),
		  fPrepared(false)
	{
	}

	void Run()
	{
		// prepare device modifications
		if (fDevice->IsReadOnly()) {
			printf("Device is read-only. Can't change anything.\n");
		} else {
			status_t error = fDevice->PrepareModifications();
			fPrepared = (error == B_OK);
			if (error != B_OK) {
				printf("Error: Failed to prepare device for modifications: "
					"%s\n", strerror(error));
			}
		}

		// main input loop
		while (true) {
			BString line;
			if (!_ReadLine("party> ", line))
				return;

			if (line == "") {
				// do nothing
			} else if (line == "h" || line == "help") {
				_PrintHelp();
			} else if (line == "i") {
				_InitializePartition();
			} else if (line == "l") {
				_PrintPartitionsShort();
			} else if (line == "ll") {
				_PrintPartitionsLong();
			} else if (line == "n") {
				_NewPartition();
			} else if (line == "q" || line == "quit") {
				return;
			} else if (line == "w") {
				_WriteChanges();
			} else {
				printf("Invalid command \"%s\", type \"help\" for help\n",
					line.String());
			}
		}
	}

private:
	void _PrintHelp()
	{
		printf("Valid commands:\n"
			"  h, help  - prints this help text\n"
			"  i        - initialize a partition\n"
			"  l        - lists the device's partitions recursively\n"
			"  ll       - lists the device's partitions recursively, long "
				"format\n"
			"  l        - create a new child partition\n"
			"  q, quit  - quits the program, changes are discarded\n"
			"  w        - write changes to disk\n");
	}

	void _PrintPartitionsShort()
	{
		PrintShortVisitor visitor;
		fDevice->VisitEachDescendant(&visitor);
	}

	void _PrintPartitionsLong()
	{
		PrintLongVisitor visitor;
		fDevice->VisitEachDescendant(&visitor);
	}

	void _InitializePartition()
	{
		if (!fPrepared) {
			if (fDevice->IsReadOnly())
				printf("Device is read-only!\n");
			else
				printf("Sorry, not prepared for modifications!\n");
			return;
		}

		// get the partition
		int32 partitionIndex;
		BPartition* partition = NULL;
		if (!_SelectPartition("partition index [-1 to abort]: ", partition,
				partitionIndex)) {
			return;
		}

		printf("\nselected partition:\n\n");
		print_partition_table_header();
		print_partition(partition, 0, partitionIndex);

		// get available disk systems
		BObjectList<BDiskSystem> diskSystems(20, true);
		BDiskDeviceRoster roster;
		{
			BDiskSystem diskSystem;
			while (roster.GetNextDiskSystem(&diskSystem) == B_OK) {
				if (partition->CanInitialize(diskSystem.PrettyName()))
					diskSystems.AddItem(new BDiskSystem(diskSystem));
			}
		}

		if (diskSystems.IsEmpty()) {
			printf("There are no disk systems, that can initialize this "
				"partition.\n");
			return;
		}

		// print the available disk systems
		printf("\ndisk systems that can initialize the selected partition:\n");
		for (int32 i = 0; BDiskSystem* diskSystem = diskSystems.ItemAt(i); i++)
			printf("%2ld  %s\n", i, diskSystem->PrettyName());

		printf("\n");

		// get the disk system
		int64 diskSystemIndex;
		if (!_ReadNumber("disk system index [-1 to abort]: ", 0,
				diskSystems.CountItems() - 1, -1, "invalid index",
				diskSystemIndex)) {
			return;
		}
		BDiskSystem* diskSystem = diskSystems.ItemAt(diskSystemIndex);

		bool supportsName = diskSystem->SupportsContentName();
		BString name;
		BString parameters;
		while (true) {
			// let the user enter name and parameters
			if (supportsName && !_ReadLine("partition name: ", name)
				|| !_ReadLine("partition parameters: ", parameters)) {
				return;
			}

			// validate parameters
			BString validatedName(name);
			if (partition->ValidateInitialize(diskSystem->PrettyName(),
					supportsName ? &validatedName : NULL, parameters.String())
					!= B_OK) {
				printf("Validation of the given values failed. Sorry, can't "
					"continue.\n");
				return;
			}

			// did the disk system change the name?
			if (!supportsName || name == validatedName) {
				printf("Everything looks dandy.\n");
			} else {
				printf("The disk system adjusted the file name to \"%s\".\n",
					validatedName.String());
				name = validatedName;
			}

			// let the user decide whether to continue, change parameters, or
			// abort
			bool changeParameters = false;
			while (true) {
				BString line;
				_ReadLine("[c]ontinue, change [p]arameters, or [a]bort? ", line);
				if (line == "a")
					return;
				if (line == "p") {
					changeParameters = true;
					break;
				}
				if (line == "c")
					break;

				printf("invalid input\n");
			}

			if (!changeParameters)
				break;
		}

		// initialize
		status_t error = partition->Initialize(diskSystem->PrettyName(),
			supportsName ? name.String() : NULL, parameters.String());
		if (error != B_OK)
			printf("Initialization failed: %s\n", strerror(error));
	}

	void _NewPartition()
	{
		if (!fPrepared) {
			if (fDevice->IsReadOnly())
				printf("Device is read-only!\n");
			else
				printf("Sorry, not prepared for modifications!\n");
			return;
		}

		// get the parent partition
		BPartition* partition = NULL;
		int32 partitionIndex;
		if (!_SelectPartition("parent partition index [-1 to abort]: ",
				partition, partitionIndex)) {
			return;
		}

		printf("\nselected partition:\n\n");
		print_partition_table_header();
		print_partition(partition, 0, partitionIndex);

		if (!partition->ContainsPartitioningSystem()) {
			printf("The selected partition does not contain a partitioning "
				"system.\n");
			return;
		}

		// get supported types
		BObjectList<BString> supportedTypes(20, true);
		BString typeBuffer;
		int32 cookie = 0;
		while (partition->GetNextSupportedChildType(&cookie, &typeBuffer)
				== B_OK) {
			supportedTypes.AddItem(new BString(typeBuffer));
		}

		if (supportedTypes.IsEmpty()) {
			printf("The partitioning system is not able to create any "
				"child partition (no supported types).\n");
			return;
		}

		// get partitioning info
		BPartitioningInfo partitioningInfo;
		status_t error = partition->GetPartitioningInfo(&partitioningInfo);
		if (error != B_OK) {
			printf("Failed to get partitioning info for partition: %s\n",
				strerror(error));
			return;
		}

		int32 spacesCount = partitioningInfo.CountPartitionableSpaces();
		if (spacesCount == 0) {
			printf("There's no space on the partition where a child partition "
				"could be created\n");
			return;
		}

		// let the user select the partition type, if there's more than one
		int64 typeIndex = 0;
		int32 supportedTypesCount = supportedTypes.CountItems();
		if (supportedTypesCount > 1) {
			// list them
			printf("Possible partition types:\n");
			for (int32 i = 0; i < supportedTypesCount; i++)
				printf("%2ld  %s\n", i, supportedTypes.ItemAt(i)->String());

			if (!_ReadNumber("supported type index [-1 to abort]: ", 0,
					supportedTypesCount - 1, -1, "invalid index", typeIndex)) {
				return;
			}
		}

		const char* type = supportedTypes.ItemAt(typeIndex)->String();

		// list partitionable spaces
		printf("Unused regions where the new partition could be created:\n");
		for (int32 i = 0; i < spacesCount; i++) {
			off_t _offset;
			off_t _size;
			partitioningInfo.GetPartitionableSpaceAt(i, &_offset, &_size);
			BString offset, size;
			get_size_string(_offset, offset);
			get_size_string(_size, size);
			printf("%2ld  start: %8s,  size:  %8s\n", i, offset.String(),
				size.String());
		}

		// let the user select the partitionable space, if there's more than one
		int64 spaceIndex = 0;
		if (spacesCount > 1) {
			if (!_ReadNumber("unused region index [-1 to abort]: ", 0,
					spacesCount - 1, -1, "invalid index", spaceIndex)) {
				return;
			}
		}

		off_t spaceOffset;
		off_t spaceSize;
		partitioningInfo.GetPartitionableSpaceAt(spaceIndex, &spaceOffset,
			&spaceSize);

		off_t start;
		off_t size;
		BString parameters;
		while (true) {
			// let the user enter start, size, and parameters

			// start
			while (true) {
				BString spaceOffsetString;
				get_size_string(spaceOffset, spaceOffsetString);
				BString prompt("partition start [default: ");
				prompt << spaceOffsetString << "]: ";
				if (!_ReadSize(prompt.String(), spaceOffset, start))
					return;

				if (start >= spaceOffset && start <= spaceOffset + spaceSize)
					break;

				printf("invalid partition start\n");
			}

			// size
			off_t maxSize = spaceOffset + spaceSize - start;
			while (true) {
				BString maxSizeString;
				get_size_string(maxSize, maxSizeString);
				BString prompt("partition size [default: ");
				prompt << maxSizeString << "]: ";
				if (!_ReadSize(prompt.String(), maxSize, size))
					return;

				if (size >= 0 && start + size <= spaceOffset + spaceSize)
					break;

				printf("invalid partition size\n");
			}

			// parameters
			if (!_ReadLine("partition parameters: ", parameters))
				return;

			// validate parameters
			off_t validatedStart = start;
			off_t validatedSize = size;
// TODO: Support the name parameter!
			if (partition->ValidateCreateChild(&start, &size, type, NULL,
					parameters.String()) != B_OK) {
				printf("Validation of the given values failed. Sorry, can't "
					"continue.\n");
				return;
			}

			// did the disk system change offset or size?
			if (validatedStart == start && validatedSize == size) {
				printf("Everything looks dandy.\n");
			} else {
				BString startString, sizeString;
				get_size_string(validatedStart, startString);
				get_size_string(validatedSize, sizeString);
				printf("The disk system adjusted the partition start and "
					"size to %lld (%s) and %lld (%s).\n",
					validatedStart, startString.String(), validatedSize,
					sizeString.String());
				start = validatedStart;
				size = validatedSize;
			}

			// let the user decide whether to continue, change parameters, or
			// abort
			bool changeParameters = false;
			while (true) {
				BString line;
				_ReadLine("[c]ontinue, change [p]arameters, or [a]bort? ", line);
				if (line == "a")
					return;
				if (line == "p") {
					changeParameters = true;
					break;
				}
				if (line == "c")
					break;

				printf("invalid input\n");
			}

			if (!changeParameters)
				break;
		}

		// create child
		error = partition->CreateChild(start, size, type, NULL,
			parameters.String());
		if (error != B_OK)
			printf("Creating the partition failed: %s\n", strerror(error));
	}

	void _WriteChanges()
	{
		if (!fPrepared || !fDevice->IsModified()) {
			printf("No changes have been made!\n");
			return;
		}

		printf("Writing changes to disk. This can take a while...\n");

		// commit
		status_t error = fDevice->CommitModifications();
		if (error == B_OK) {
			printf("All changes have been written successfully!\n");
		} else {
			printf("Failed to write all changes: %s\n", strerror(error));
		}

		// prepare again
		error = fDevice->PrepareModifications();
		fPrepared = (error == B_OK);
		if (error != B_OK) {
			printf("Error: Failed to prepare device for modifications: "
				"%s\n", strerror(error));
		}
	}

	bool _SelectPartition(const char* prompt, BPartition*& partition,
		int32& _partitionIndex)
	{
		// if the disk device has no children, we select it without asking
		if (fDevice->CountChildren() == 0) {
			partition = fDevice;
			_partitionIndex = 0;
			return true;
		}

		// otherwise let the user select
		_PrintPartitionsShort();

		partition = NULL;
		int64 partitionIndex;
		while (true) {
			if (!_ReadNumber(prompt, partitionIndex) || partitionIndex < 0)
				return false;

			FindPartitionByIndexVisitor visitor(partitionIndex);
			partition = fDevice->VisitEachDescendant(&visitor);
			if (partition) {
				_partitionIndex = partitionIndex;
				return true;
			}

			printf("invalid partition index\n");
		}
	}

	bool _ReadLine(const char* prompt, BString& _line)
	{
		// prompt
		printf(prompt);
		fflush(stdout);

		// read line
		char line[256];
		if (!fgets(line, sizeof(line), stdin))
			return false;

		// remove trailing '\n'
		if (char* trailingNL = strchr(line, '\n'))
			*trailingNL = '\0';

		_line = line;
		return true;
	}

	bool _ReadNumber(const char* prompt, int64& number)
	{
		while (true) {
			BString line;
			if (!_ReadLine(prompt, line))
				return false;

			char buffer[256];
			if (sscanf(line.String(), "%lld%s", &number, buffer) == 1)
				return true;

			printf("invalid input\n");
		}
	}

	bool _ReadNumber(const char* prompt, int64 minNumber, int64 maxNumber,
		int64 abortNumber, const char* invalidNumberMessage, int64& number)
	{
		while (true) {
			BString line;
			if (!_ReadLine(prompt, line))
				return false;

			char buffer[256];
			if (sscanf(line.String(), "%lld%s", &number, buffer) != 1) {
				printf("invalid input\n");
				continue;
			}

			if (number == abortNumber)
				return false;

			if (number >= minNumber && number <= maxNumber)
				return true;

			puts(invalidNumberMessage);
		}
	}

	bool _ReadSize(const char* prompt, off_t defaultValue, off_t& size)
	{
		while (true) {
			BString _line;
			if (!_ReadLine(prompt, _line))
				return false;
			const char* line = _line.String();

			// skip whitespace
			while (isspace(*line))
				line++;

			if (*line == '\0') {
				size = defaultValue;
				return true;
			}

			// get the number
			int32 endIndex = 0;
			while (isdigit(line[endIndex]))
				endIndex++;

			if (endIndex == 0) {
				printf("invalid input\n");
				continue;
			}

			size = atoll(BString(line, endIndex).String());

			// skip whitespace
			line += endIndex;
			while (isspace(*line))
				line++;

			// get the size modifier
			if (*line != '\0') {
				switch (*line) {
					case 'K':
						size *= 1024;
						break;
					case 'M':
						size *= 1024 * 1024;
						break;
					case 'G':
						size *= 1024LL * 1024 * 1024;
						break;
					case 'T':
						size *= 1024LL * 1024 * 1024 * 1024;
						break;
					default:
						line--;
				}
		
				line++;
			}

			if (*line == 'B')
				line++;

			// skip whitespace
			while (isspace(*line))
				line++;

			if (*line == '\0')
				return true;

			printf("invalid input\n");
		}
	}

private:
	BDiskDevice*	fDevice;
	bool			fPrepared;
};


int
main(int argc, const char* const* argv)
{
	// parse arguments
	int argi = 1;
	for (; argi < argc; argi++) {
		const char* arg = argv[argi];
		if (arg[0] == '-') {
			if (arg[1] == '-') {
				// a double '-' option
				if (strcmp(arg, "--help") == 0) {
					print_usage_and_exit(false);
				} else
					print_usage_and_exit(true);
			} else {
				// single '-' options
				for (int i = 1; arg[i] != '\0'; i++) {
					switch (arg[i]) {
						case 'h':
							print_usage_and_exit(false);
						default:
							print_usage_and_exit(true);
					}
				}
			}
		} else
			break;
	}

	// only the device path should remain
	if (argi != argc - 1) 
		print_usage_and_exit(true);
	const char* devicePath = argv[argi];

	// get the disk device
	BDiskDeviceRoster roster;
	BDiskDevice device;
	status_t error = roster.GetDeviceForPath(devicePath, &device);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to get disk device for path \"%s\": "
			"%s\n", devicePath, strerror(error));
	}

	Partitioner partitioner(&device);
	partitioner.Run();

	return 0;
}
