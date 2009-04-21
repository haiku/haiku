/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <string>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <SupportDefs.h>


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

typedef std::vector<std::string> EntryVector;


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
static off_t sMaxFileSize = kDefaultMaxFileSize;
static uint32 sCount = 0;


static off_t
to_size(const char* valueWithUnit)
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


static void
usage(int status)
{
	fprintf(stderr,
		"Usage: %s [options]\n"
		"Performs some random file actions.\n"
		"\n"
		"  -r, --runs=<count>\tThe number of actions to perform. Defaults to 100.\n"
		"  -s, --seed=<seed>\tThe base seed to use for the random numbers.\n"
		"  -f, --file-count=<count>\tThe maximal number of files to create.\n"
		"  -d, --dir-count=<count>\tThe maximal number of directories to create.\n"
		"  -m, --max-file-size=<size>\tThe maximal file size of the files.\n"
		"  -n, --no-cache\t\tDisables the file cache when doing I/O on a file.\n"
		"  -v, --verbose\t\tShow the actions as being performed\n",
		kProgramName);

	exit(status);
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
	return entries[choose_index(entries)];
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
	return open(name.c_str(), mode | (sDisableFileCache ? O_DIRECT : 0));
}


static void
generate_block(char* block, const std::string& name, off_t offset)
{
	// TODO: generate unique and verifiable file contents.
}


static void
write_blocks(int fd, const std::string& name, bool append = false)
{
	off_t size = min_c(rand() % sMaxFileSize, sMaxFileSize);
	off_t offset = 0;

	if (append) {
		// in the append case, we need to check the file size
		struct stat stat;
		if (fstat(fd, &stat) != 0) {
			fprintf(stderr, "%s: stat file failed: %s\n", kProgramName,
				strerror(errno));
			exit(1);
		}

		if (size + stat.st_size > sMaxFileSize)
			size = sMaxFileSize - stat.st_size;

		offset = stat.st_size;
	}

	verbose("  write %lu bytes", size);

	while (size > 0) {
		char block[kBlockSize];
		generate_block(block, name, offset);

		ssize_t toWrite = min_c(size, kBlockSize);
		ssize_t bytesWritten = write(fd, block, toWrite);
		if (bytesWritten != toWrite) {
			fprintf(stderr, "%s: writing failed: %s\n", kProgramName,
				strerror(errno));
			exit(1);
		}

		offset += toWrite;
		size -= toWrite;
	}
}


//	#pragma mark - Actions


static void
create_dir(EntryVector& dirs)
{
	std::string parent = choose_parent(dirs);
	std::string name = create_name(parent, "dir");

	verbose("create dir %s", name.c_str());

	if (mkdir(name.c_str(), 0777) != 0) {
		fprintf(stderr, "%s: creating dir \"%s\" failed: %s\n", kProgramName,
			name.c_str(), strerror(errno));
		exit(1);
	}

	dirs.push_back(name);
}


static void
remove_dir(EntryVector& dirs)
{
	if (dirs.empty())
		return;

	int index = choose_index(dirs);

	if (rmdir(dirs[index].c_str()) != 0) {
		if (errno == ENOTEMPTY || errno == EEXIST) {
			// TODO: in rare cases, we could remove all files
			return;
		}

		fprintf(stderr, "%s: removing dir \"%s\" failed: %s\n", kProgramName,
			dirs[index].c_str(), strerror(errno));
		exit(1);
	}

	verbose("removed dir %s", dirs[index].c_str());

	EntryVector::iterator iterator = dirs.begin();
	dirs.erase(iterator + index);
}


static void
create_file(const EntryVector& dirs, EntryVector& files)
{
	std::string parent = choose_parent(dirs);
	std::string name = create_name(parent, "file");

	verbose("create file %s", name.c_str());

	int fd = open_file(name, O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0) {
		fprintf(stderr, "%s: creating file \"%s\" failed: %s\n", kProgramName,
			name.c_str(), strerror(errno));
		exit(1);
	}

	write_blocks(fd, name);
	close(fd);
	files.push_back(name);
}


static void
remove_file(EntryVector& files)
{
	if (files.empty())
		return;

	int index = choose_index(files);

	if (remove(files[index].c_str()) != 0) {
		fprintf(stderr, "%s: removing file \"%s\" failed: %s\n", kProgramName,
			files[index].c_str(), strerror(errno));
		exit(1);
	}

	verbose("removed file %s", files[index].c_str());

	EntryVector::iterator iterator = files.begin();
	files.erase(iterator + index);
}


static void
rename_file(const EntryVector& dirs, EntryVector& files)
{
	if (files.empty())
		return;

	std::string parent = choose_parent(dirs);
	std::string name = create_name(parent, "renamed-file");

	int index = choose_index(files);

	verbose("rename file \"%s\" to \"%s\"", files[index].c_str(), name.c_str());

	if (rename(files[index].c_str(), name.c_str()) != 0) {
		fprintf(stderr, "%s: renaming file \"%s\" to \"%s\" failed: %s\n",
			kProgramName, files[index].c_str(), name.c_str(), strerror(errno));
		exit(1);
	}

	files[index] = name;
}


static void
append_file(const EntryVector& files)
{
	if (files.empty())
		return;

	const std::string& name = files[choose_index(files)];

	verbose("append to \"%s\"", name.c_str());

	int fd = open_file(name, O_WRONLY | O_APPEND);
	if (fd < 0) {
		fprintf(stderr, "%s: appending to file \"%s\" failed: %s\n",
			kProgramName, name.c_str(), strerror(errno));
		exit(1);
	}

	write_blocks(fd, name, true);
	close(fd);
}


static void
replace_file(const EntryVector& files)
{
	if (files.empty())
		return;

	const std::string& name = files[choose_index(files)];

	verbose("replace \"%s\"", name.c_str());

	int fd = open_file(name, O_CREAT | O_WRONLY | O_TRUNC);
	if (fd < 0) {
		fprintf(stderr, "%s: replacing file \"%s\" failed: %s\n",
			kProgramName, name.c_str(), strerror(errno));
		exit(1);
	}

	write_blocks(fd, name);
	close(fd);
}


static void
truncate_file(const EntryVector& files)
{
	if (files.empty())
		return;

	const std::string& name = files[choose_index(files)];

	verbose("truncate \"%s\"", name.c_str());

	int fd = open_file(name, O_WRONLY | O_TRUNC);
	if (fd < 0) {
		fprintf(stderr, "%s: truncating file \"%s\" failed: %s\n",
			kProgramName, name.c_str(), strerror(errno));
		exit(1);
	}

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
		{"max-file-size", required_argument, 0, 'm'},
		{"no-cache", no_argument, 0, 'n'},
		{"verbose", no_argument, 0, 'v'},
		{"help", no_argument, 0, 'h'},
		{NULL}
	};

	uint32 maxFileCount = kDefaultFileCount;
	uint32 maxDirCount = kDefaultDirCount;
	uint32 runs = kDefaultRunCount;

	int c;
	while ((c = getopt_long(argc, argv, "r:s:f:d:m:nvh", kOptions,
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
				srand(strtoul(optarg, NULL, 0));
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
			case 'm':
			{
				// max file size
				sMaxFileSize = to_size(optarg);
				break;
			}
			case 'n':
				sDisableFileCache = true;
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

	EntryVector dirs;
	EntryVector files;

	std::string baseName = "./random_file_temp";
	if (mkdir(baseName.c_str(), 0777) != 0 && errno != EEXIST) {
		fprintf(stderr, "%s: cannot create base directory: %s\n",
			kProgramName, strerror(errno));
		return 1;
	}

	dirs.push_back(baseName);
	
	for (uint32 run = 0; run < runs; run++) {
		file_action action = choose_action();
		
		switch (action) {
			case kCreateFile:
				if (files.size() > maxFileCount / 2) {
					// create a single file
					if (files.size() < maxFileCount)
						create_file(dirs, files);
				} else {
					// create some more files to fill up the list
					uint32 count = min_c(maxFileCount, files.size() + 10);
					for (uint32 i = 0; i < count; i++) {
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
					// create some more directories to fill up the list
					uint32 count = min_c(maxDirCount, dirs.size() + 5);
					for (uint32 i = 0; i < count; i++) {
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
	}

	for (int i = files.size(); i-- > 0;) {
		remove_file(files);
	}
	for (int i = dirs.size(); i-- > 0;) {
		remove_dir(dirs);
	}

	return 0;
}
