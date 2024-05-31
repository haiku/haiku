/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <string>
#include <vector>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fs_attr.h>
#include <fs_volume.h>
#include <OS.h>
#include <TypeConstants.h>


enum file_action {
	kCreateFile,
	kCreateDir,
	kRenameFile,
	kRemoveFile,
	kRemoveDir,
	kAppendFile,
	kReplaceFile,
	kTruncateFile,

	kNumActions
};

struct block_identifier {
	off_t		offset;
	uint32		identifier;
	uint16		data[0];
};

struct entry {
	std::string	name;
	uint32		identifier;
	off_t		size;
};

typedef std::vector<entry> EntryVector;


const char* kDefaultBaseDir = "./random_file_temp";
const char* kIdentifierAttribute = "rfa:identifier";

const uint32 kDefaultDirCount = 1;
const uint32 kDefaultFileCount = 10;
const uint32 kDefaultRunCount = 100;
const off_t kDefaultMaxFileSize = 32768;

const uint32 kMaxFileCount = 1000000;
const uint32 kMaxDirCount = 100000;

const uint32 kBlockSize = 256;


extern const char *__progname;
static const char *kProgramName = __progname;

static bool sDisableFileCache = false;
static bool sVerbose = false;
static bool sCheckBeforeRemove = false;
static off_t sMaxFileSize = kDefaultMaxFileSize;
static uint32 sCount = 0;

static off_t sWriteTotal = 0;
static off_t sReadTotal = 0;
static bigtime_t sWriteTime = 0;
static bigtime_t sReadTime = 0;
static uint32 sRun = 0;


static off_t
string_to_size(const char* valueWithUnit)
{
	char* unit;
	off_t size = strtoull(valueWithUnit, &unit, 10);

	if (strchr(unit, 'G') || strchr(unit, 'g'))
		size *= 1024 * 1024 * 1024;
	else if (strchr(unit, 'M') || strchr(unit, 'm'))
		size *= 1024 * 1024;
	else if (strchr(unit, 'K') || strchr(unit, 'k'))
		size *= 1024;

	return size;
}


static std::string
size_to_string(off_t size)
{
	char buffer[256];

	if (size > 10LL * 1024 * 1024 * 1024) {
		snprintf(buffer, sizeof(buffer), "%g GB",
			size / (1024.0 * 1024 * 1024));
	} else if (size > 10 * 1024 * 1024)
		snprintf(buffer, sizeof(buffer), "%g MB", size / (1024.0 * 1024));
	else if (size > 10 * 1024)
		snprintf(buffer, sizeof(buffer), "%g KB", size / (1024.0));
	else
		snprintf(buffer, sizeof(buffer), "%lld B", size);

	return buffer;
}


static std::string
time_to_string(bigtime_t usecs)
{
	static const bigtime_t kSecond = 1000000ULL;
	static const bigtime_t kHour = 3600 * kSecond;
	static const bigtime_t kMinute = 60 * kSecond;

	uint32 hours = usecs / kHour;
	uint32 minutes = usecs / kMinute;
	uint32 seconds = usecs / kSecond;

	char buffer[256];
	if (usecs >= kHour) {
		minutes %= 60;
		seconds %= 60;
		snprintf(buffer, sizeof(buffer), "%luh %02lum %02lus", hours, minutes,
			seconds);
	} else if (usecs > 100 * kSecond) {
		seconds %= 60;
		snprintf(buffer, sizeof(buffer), "%lum %02lus", minutes, seconds);
	} else
		snprintf(buffer, sizeof(buffer), "%gs", 1.0 * usecs / kSecond);

	return buffer;
}


static void
usage(int status)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"Performs some random file actions for file system testing.\n"
		"\n"
		"  -r, --runs=<count>\t\tThe number of actions to perform.\n"
		"\t\t\t\tDefaults to %lu.\n"
		"  -s, --seed=<seed>\t\tThe base seed to use for the random numbers.\n"
		"  -f, --file-count=<count>\tThe maximal number of files to create.\n"
		"\t\t\t\tDefaults to %lu.\n"
		"  -d, --dir-count=<count>\tThe maximal number of directories to create.\n"
		"\t\t\t\tDefaults to %lu.\n"
		"  -m, --max-file-size=<size>\tThe maximal file size of the files.\n"
		"\t\t\t\tDefaults to %lld.\n"
		"  -b, --base-dir=<path>\t\tThe base directory for the actions. "
			"Defaults\n"
		"\t\t\t\tto %s.\n"
		"  -c, --check-interval=<count>\tCheck after every <count> runs. "
			"Defaults to 0,\n"
		"\t\t\t\tmeaning only check once at the end.\n"
		"  -n, --no-cache\t\tDisables the file cache when doing I/O on\n"
		"\t\t\t\ta file.\n"
		"  -a, --always-check\t\tAlways check contents before removing data.\n"
		"  -k, --keep-dirty\t\tDo not remove the working files on quit.\n"
		"  -i, --mount-image=<image>\tMounts an image for the actions, and "
			"remounts\n"
		"\t\t\t\tit before checking (each time).\n"
		"  -v, --verbose\t\t\tShow the actions as being performed\n",
		kProgramName, kDefaultRunCount, kDefaultFileCount, kDefaultDirCount,
		kDefaultMaxFileSize, kDefaultBaseDir);

	exit(status);
}


static void
error(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	fprintf(stderr, "%s: ", kProgramName);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);

	va_end(args);
	fflush(stderr);

	exit(1);
}


static void
warning(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	fprintf(stderr, "%s: ", kProgramName);
	vfprintf(stderr, format, args);
	fputc('\n', stderr);

	va_end(args);
	fflush(stderr);
}


static void
verbose(const char* format, ...)
{
	if (!sVerbose)
		return;

	va_list args;
	va_start(args, format);

	vprintf(format, args);
	putchar('\n');

	va_end(args);
	fflush(stdout);
}


static void
action(const char* format, ...)
{
	if (!sVerbose)
		return;

	va_list args;
	va_start(args, format);

	printf("%7lu  ", sRun + 1);
	vprintf(format, args);
	putchar('\n');

	va_end(args);
	fflush(stdout);
}


static file_action
choose_action()
{
	return (file_action)(rand() % kNumActions);
}


static inline int
choose_index(const EntryVector& entries)
{
	return rand() % entries.size();
}


static inline const std::string&
choose_parent(const EntryVector& entries)
{
	return entries[choose_index(entries)].name;
}


static std::string
create_name(const std::string& parent, const char* prefix)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), "%s/%s-%lu", parent.c_str(), prefix,
		sCount++);

	std::string name = buffer;
	return name;
}


static int
open_file(const std::string& name, int mode)
{
	return open(name.c_str(), mode | (sDisableFileCache ? O_DIRECT : 0), 0666);
}


static void
generate_block(char* buffer, const struct entry& entry, off_t offset)
{
	block_identifier* block = (block_identifier*)buffer;
	block->offset = offset;
	block->identifier = entry.identifier;

	uint32 count = (kBlockSize - offsetof(block_identifier, data))  / 2;
	offset += offsetof(block_identifier, data);

	for (uint32 i = 0; i < count; i++) {
		block->data[i] = offset + i * 2;
	}
}


static void
write_blocks(int fd, struct entry& entry, bool append = false)
{
	off_t size = min_c(rand() % sMaxFileSize, sMaxFileSize);
	off_t offset = 0;

	if (append) {
		// in the append case, we need to check the file size
		struct stat stat;
		if (fstat(fd, &stat) != 0)
			error("stat file failed: %s\n", strerror(errno));

		if (size + stat.st_size > sMaxFileSize)
			size = sMaxFileSize - stat.st_size;

		offset = stat.st_size;
	}

	verbose("\t\twrite %lu bytes", size);

	entry.size += size;
	uint32 blockOffset = offset % kBlockSize;
	sWriteTotal += size;

	bigtime_t start = system_time();

	while (size > 0) {
		char block[kBlockSize];
		generate_block(block, entry, offset - blockOffset);

		ssize_t toWrite = min_c(size, kBlockSize - blockOffset);
		ssize_t bytesWritten = write(fd, block + blockOffset, toWrite);
		if (bytesWritten != toWrite)
			error("writing failed: %s", strerror(errno));

		offset += toWrite;
		size -= toWrite;
		blockOffset = 0;
	}

	sWriteTime += system_time() - start;
}


static void
dump_block(const char* buffer, int size, const char* prefix)
{
	const int DUMPED_BLOCK_SIZE = 16;
	int i;

	for (i = 0; i < size;) {
		int start = i;

		printf("%s%04x ", prefix, i);
		for (; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char*)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			} else
				break;
		}
		printf("\n");
	}
}


static void
check_file(const struct entry& file)
{
	int fd = open_file(file.name, O_RDONLY);
	if (fd < 0) {
		error("opening file \"%s\" failed: %s", file.name.c_str(),
			strerror(errno));
	}

	// first check if size matches

	struct stat stat;
	if (fstat(fd, &stat) != 0)
		error("stat file \"%s\" failed: %s", file.name.c_str(), strerror(errno));

	if (file.size != stat.st_size) {
		warning("size does not match for \"%s\"! Expected %lld reported %lld",
			file.name.c_str(), file.size, stat.st_size);
		close(fd);
		return;
	}

	// check contents

	off_t size = file.size;
	off_t offset = 0;
	sReadTotal += size;

	bigtime_t start = system_time();

	while (size > 0) {
		// read block
		char block[kBlockSize];
		ssize_t toRead = min_c(size, kBlockSize);
		ssize_t bytesRead = read(fd, block, toRead);
		if (bytesRead != toRead) {
			error("reading \"%s\" failed: %s", file.name.c_str(),
				strerror(errno));
		}

		// compare with generated block
		char generatedBlock[kBlockSize];
		generate_block(generatedBlock, file, offset);

		if (memcmp(generatedBlock, block, bytesRead) != 0) {
			dump_block(generatedBlock, bytesRead, "generated: ");
			dump_block(block, bytesRead, "read:      ");
			error("block at %lld differ in \"%s\"!", offset, file.name.c_str());
		}

		offset += toRead;
		size -= toRead;
	}

	sReadTime += system_time() - start;

	close(fd);
}


static void
check_files(EntryVector& files)
{
	verbose("check all files...");

	for (EntryVector::iterator i = files.begin(); i != files.end(); i++) {
		const struct entry& file = *i;

		check_file(file);
	}
}


static void
remove_dirs(const std::string& path)
{
	DIR* dir = opendir(path.c_str());
	if (dir == NULL) {
		warning("Could not open directory \"%s\": %s", path.c_str(),
			strerror(errno));
		return;
	}

	while (struct dirent* entry = readdir(dir)) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;

		std::string subPath = path + "/" + entry->d_name;
		remove_dirs(subPath);
	}

	closedir(dir);
	rmdir(path.c_str());
}


static void
mount_image(const char* image, const char* mountPoint)
{
	dev_t volume = fs_mount_volume(mountPoint, image, NULL, 0, NULL);
	if (volume < 0)
		error("mounting failed: %s", strerror(volume));
}


static void
unmount_image(const char* mountPoint)
{
	status_t status = fs_unmount_volume(mountPoint, 0);
	if (status != B_OK)
		error("unmounting failed: %s", strerror(status));
}


//	#pragma mark - Actions


static void
create_dir(EntryVector& dirs)
{
	std::string parent = choose_parent(dirs);
	std::string name = create_name(parent, "dir");

	action("create dir %s (identifier %lu)", name.c_str(), sCount);

	if (mkdir(name.c_str(), 0777) != 0)
		error("creating dir \"%s\" failed: %s", name.c_str(), strerror(errno));

	struct entry dir;
	dir.name = name;
	dir.identifier = sCount;
	dir.size = 0;

	dirs.push_back(dir);
}


static void
remove_dir(EntryVector& dirs)
{
	if (dirs.empty())
		return;

	int index = choose_index(dirs);
	if (index == 0)
		return;

	const std::string& name = dirs[index].name;

	if (rmdir(name.c_str()) != 0) {
		if (errno == ENOTEMPTY || errno == EEXIST) {
			// TODO: in rare cases, we could remove all files
			return;
		}

		error("removing dir \"%s\" failed: %s", name.c_str(), strerror(errno));
	}

	action("removed dir %s", name.c_str());

	EntryVector::iterator iterator = dirs.begin();
	dirs.erase(iterator + index);
}


static void
create_file(const EntryVector& dirs, EntryVector& files)
{
	std::string parent = choose_parent(dirs);
	std::string name = create_name(parent, "file");

	action("create file %s (identifier %lu)", name.c_str(), sCount);

	int fd = open_file(name, O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0)
		error("creating file \"%s\" failed: %s", name.c_str(), strerror(errno));

	struct entry file;
	file.name = name;
	file.identifier = sCount;
	file.size = 0;
	write_blocks(fd, file);

	files.push_back(file);

	fs_write_attr(fd, kIdentifierAttribute, B_UINT32_TYPE, 0, &file.identifier,
		sizeof(uint32));

	close(fd);
}


static void
remove_file(EntryVector& files)
{
	if (files.empty())
		return;

	int index = choose_index(files);
	const std::string& name = files[index].name;

	if (sCheckBeforeRemove)
		check_file(files[index]);

	if (remove(name.c_str()) != 0)
		error("removing file \"%s\" failed: %s", name.c_str(), strerror(errno));

	action("removed file %s", name.c_str());

	EntryVector::iterator iterator = files.begin();
	files.erase(iterator + index);
}


static void
rename_file(const EntryVector& dirs, EntryVector& files)
{
	if (files.empty())
		return;

	std::string parent = choose_parent(dirs);
	std::string newName = create_name(parent, "renamed-file");

	int index = choose_index(files);
	const std::string& oldName = files[index].name;

	action("rename file \"%s\" to \"%s\"", oldName.c_str(), newName.c_str());

	if (rename(oldName.c_str(), newName.c_str()) != 0) {
		error("renaming file \"%s\" to \"%s\" failed: %s", oldName.c_str(),
			newName.c_str(), strerror(errno));
	}

	files[index].name = newName;
}


static void
append_file(EntryVector& files)
{
	if (files.empty())
		return;

	struct entry& file = files[choose_index(files)];

	action("append to \"%s\"", file.name.c_str());

	int fd = open_file(file.name, O_WRONLY | O_APPEND);
	if (fd < 0) {
		error("appending to file \"%s\" failed: %s", file.name.c_str(),
			strerror(errno));
	}

	write_blocks(fd, file, true);
	close(fd);
}


static void
replace_file(EntryVector& files)
{
	if (files.empty())
		return;

	struct entry& file = files[choose_index(files)];

	action("replace \"%s\" contents", file.name.c_str());

	if (sCheckBeforeRemove)
		check_file(file);

	int fd = open_file(file.name, O_CREAT | O_WRONLY | O_TRUNC);
	if (fd < 0) {
		error("replacing file \"%s\" failed: %s", file.name.c_str(),
			strerror(errno));
	}

	file.size = 0;
	write_blocks(fd, file);

	close(fd);
}


static void
truncate_file(EntryVector& files)
{
	if (files.empty())
		return;

	struct entry& file = files[choose_index(files)];

	action("truncate \"%s\"", file.name.c_str());

	if (sCheckBeforeRemove)
		check_file(file);

	int fd = open_file(file.name, O_WRONLY | O_TRUNC);
	if (fd < 0) {
		error("truncating file \"%s\" failed: %s", file.name.c_str(),
			strerror(errno));
	}

	file.size = 0;

	close(fd);
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	// parse arguments

	const static struct option kOptions[] = {
		{"runs", required_argument, 0, 'r'},
		{"seed", required_argument, 0, 's'},
		{"file-count", required_argument, 0, 'f'},
		{"dir-count", required_argument, 0, 'd'},
		{"check-interval", required_argument, 0, 'c'},
		{"max-file-size", required_argument, 0, 'm'},
		{"base-dir", required_argument, 0, 'b'},
		{"no-cache", no_argument, 0, 'n'},
		{"always-check", no_argument, 0, 'a'},
		{"keep-dirty", no_argument, 0, 'k'},
		{"mount-image", required_argument, 0, 'i'},
		{"verbose", no_argument, 0, 'v'},
		{"help", no_argument, 0, 'h'},
		{NULL}
	};

	uint32 maxFileCount = kDefaultFileCount;
	uint32 maxDirCount = kDefaultDirCount;
	uint32 runs = kDefaultRunCount;
	uint32 checkInterval = 0;
	uint32 seed = 0;
	bool keepDirty = false;
	const char* mountImage = NULL;

	struct entry base;
	base.name = kDefaultBaseDir;
	base.identifier = 0;
	base.size = 0;

	int c;
	while ((c = getopt_long(argc, argv, "r:s:f:d:c:m:b:naki:vh", kOptions,
			NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'r':
				runs = strtoul(optarg, NULL, 0);
				if (runs < 1)
					runs = 1;
				break;
			case 's':
				// seed
				seed = strtoul(optarg, NULL, 0);
				break;
			case 'f':
				// file count
				maxFileCount = strtoul(optarg, NULL, 0);
				if (maxFileCount < 5)
					maxFileCount = 5;
				else if (maxFileCount > kMaxFileCount)
					maxFileCount = kMaxFileCount;
				break;
			case 'd':
				// directory count
				maxDirCount = strtoul(optarg, NULL, 0);
				if (maxDirCount < 1)
					maxDirCount = 1;
				else if (maxDirCount > kMaxDirCount)
					maxDirCount = kMaxDirCount;
				break;
			case 'c':
				// check interval
				checkInterval = strtoul(optarg, NULL, 0);
				if (checkInterval < 0)
					checkInterval = 0;
				break;
			case 'm':
				// max file size
				sMaxFileSize = string_to_size(optarg);
				break;
			case 'b':
				base.name = optarg;
				break;
			case 'n':
				sDisableFileCache = true;
				break;
			case 'a':
				sCheckBeforeRemove = true;
				break;
			case 'k':
				keepDirty = true;
				break;
			case 'i':
				mountImage = optarg;
				break;
			case 'v':
				sVerbose = true;
				break;
			case 'h':
				usage(0);
				break;
			default:
				usage(1);
				break;
		}
	}

	if (mkdir(base.name.c_str(), 0777) != 0 && errno != EEXIST) {
		fprintf(stderr, "%s: cannot create base directory: %s\n",
			kProgramName, strerror(errno));
		return 1;
	}
	if (mountImage != NULL)
		mount_image(mountImage, base.name.c_str());

	EntryVector dirs;
	EntryVector files;

	dirs.push_back(base);

	srand(seed);

	verbose("%lu runs, %lu files (up to %s in size), %lu dirs, seed %lu\n", runs,
		maxFileCount, size_to_string(sMaxFileSize).c_str(), maxDirCount, seed);

	for (sRun = 0; sRun < runs; sRun++) {
		file_action action = choose_action();

		switch (action) {
			case kCreateFile:
				if (files.size() > maxFileCount / 2) {
					// create a single file
					if (files.size() < maxFileCount)
						create_file(dirs, files);
				} else {
					// create some more files to fill up the list (ie. 10%)
					uint32 count
						= min_c(maxFileCount, files.size() + maxFileCount / 10);
					for (uint32 i = files.size(); i < count; i++) {
						create_file(dirs, files);
					}
				}
				break;
			case kCreateDir:
				if (dirs.size() > maxDirCount / 2) {
					// create a single directory
					if (dirs.size() < maxDirCount)
						create_dir(dirs);
				} else {
					// create some more directories to fill up the list (ie. 10%)
					uint32 count
						= min_c(maxDirCount, dirs.size() + maxDirCount / 10);
					for (uint32 i = dirs.size(); i < count; i++) {
						create_dir(dirs);
					}
				}
				break;
			case kRenameFile:
				rename_file(dirs, files);
				break;
			case kRemoveFile:
				remove_file(files);
				break;
			case kRemoveDir:
				remove_dir(dirs);
				break;
			case kAppendFile:
				append_file(files);
				break;
			case kReplaceFile:
				replace_file(files);
				break;
			case kTruncateFile:
				truncate_file(files);
				break;

			default:
				break;
		}

		if (checkInterval != 0 && sRun > 0 && (sRun % checkInterval) == 0
			&& sRun + 1 < runs) {
			if (mountImage != NULL) {
				// Always remount image before checking its contents
				unmount_image(base.name.c_str());
				mount_image(mountImage, base.name.c_str());
			}
			check_files(files);
		}
	}

	if (mountImage != NULL) {
		unmount_image(base.name.c_str());
		mount_image(mountImage, base.name.c_str());
	}

	check_files(files);

	if (!keepDirty) {
		for (int i = files.size(); i-- > 0;) {
			remove_file(files);
		}
		remove_dirs(base.name);
	}

	if (mountImage != NULL) {
		unmount_image(base.name.c_str());
		if (!keepDirty)
			remove_dirs(base.name);
	}

	printf("%s written in %s, %s/s\n", size_to_string(sWriteTotal).c_str(),
		time_to_string(sWriteTime).c_str(),
		size_to_string(int64(0.5 + sWriteTotal
			/ (sWriteTime / 1000000.0))).c_str());
	printf("%s read in %s, %s/s\n", size_to_string(sReadTotal).c_str(),
		time_to_string(sReadTime).c_str(),
		size_to_string(int64(0.5 + sReadTotal
			/ (sReadTime / 1000000.0))).c_str());

	return 0;
}
