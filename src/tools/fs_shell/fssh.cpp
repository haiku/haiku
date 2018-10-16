/*
 * Copyright 2007-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "compatibility.h"

#include "fssh.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <vector>

#include "command_cp.h"
#include "driver_settings.h"
#include "external_commands.h"
#include "fd.h"
#include "fssh_dirent.h"
#include "fssh_errno.h"
#include "fssh_errors.h"
#include "fssh_fs_info.h"
#include "fssh_fcntl.h"
#include "fssh_module.h"
#include "fssh_node_monitor.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "fssh_type_constants.h"
#include "module.h"
#include "partition_support.h"
#include "path_util.h"
#include "syscalls.h"
#include "vfs.h"


extern fssh_module_info *modules[];


extern fssh_file_system_module_info gRootFileSystem;

namespace FSShell {

const char* kMountPoint = "/myfs";

// command line args
static	int					sArgc;
static	const char* const*	sArgv;

static mode_t sUmask = 0022;


static fssh_status_t
init_kernel()
{
	fssh_status_t error;

	// init module subsystem
	error = module_init(NULL);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "module_init() failed: %s\n", fssh_strerror(error));
		return error;
	}

	// init driver settings
	error = driver_settings_init();
	if (error != FSSH_B_OK) {
		fprintf(stderr, "initializing driver settings failed: %s\n",
			fssh_strerror(error));
		return error;
	}

	// register built-in modules, i.e. the rootfs and the client FS
	register_builtin_module(&gRootFileSystem.info);
	for (int i = 0; modules[i]; i++)
		register_builtin_module(modules[i]);

	// init VFS
	error = vfs_init(NULL);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "initializing VFS failed: %s\n", fssh_strerror(error));
		return error;
	}

	// init kernel IO context
	gKernelIOContext = (io_context*)vfs_new_io_context(NULL);
	if (!gKernelIOContext) {
		fprintf(stderr, "creating IO context failed!\n");
		return FSSH_B_NO_MEMORY;
	}

	// mount root FS
	fssh_dev_t rootDev = _kern_mount("/", NULL, "rootfs", 0, NULL, 0);
	if (rootDev < 0) {
		fprintf(stderr, "mounting rootfs failed: %s\n", fssh_strerror(rootDev));
		return rootDev;
	}

	// set cwd to "/"
	error = _kern_setcwd(-1, "/");
	if (error != FSSH_B_OK) {
		fprintf(stderr, "setting cwd failed: %s\n", fssh_strerror(error));
		return error;
	}

	// create mount point for the client FS
	error = _kern_create_dir(-1, kMountPoint, 0775);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "creating mount point failed: %s\n",
			fssh_strerror(error));
		return error;
	}

	return FSSH_B_OK;
}


// #pragma mark - Command

Command::Command(const char* name, const char* description)
	: fName(name),
	  fDescription(description)
{
}


Command::Command(command_function* function, const char* name,
	const char* description)
	: fName(name),
	  fDescription(description),
	  fFunction(function)
{
}


Command::~Command()
{
}


const char*
Command::Name() const
{
	return fName.c_str();
}


const char*
Command::Description() const
{
	return fDescription.c_str();
}


fssh_status_t
Command::Do(int argc, const char* const* argv)
{
	if (!fFunction) {
		fprintf(stderr, "No function given for command \"%s\"\n", Name());
		return FSSH_B_BAD_VALUE;
	}

	return (*fFunction)(argc, argv);
}


// #pragma mark - CommandManager

CommandManager::CommandManager()
{
}


CommandManager*
CommandManager::Default()
{
	if (!sManager)
		sManager = new CommandManager;
	return sManager;
}


void
CommandManager::AddCommand(Command* command)
{
	// The command name may consist of several aliases. Split them and
	// register the command for each of them.
	char _names[1024];
	char* names = _names;
	strcpy(names, command->Name());

	char* cookie;
	while (char* name = strtok_r(names, " /", &cookie)) {
		fCommands[name] = command;
		names = NULL;
	}
}


void
CommandManager::AddCommand(command_function* function, const char* name,
	const char* description)
{
	AddCommand(new Command(function, name, description));
}


void
CommandManager::AddCommands(command_function* function, const char* name,
	const char* description, ...)
{
	va_list args;
	va_start(args, description);

	while (function) {
		AddCommand(function, name, description);

		function = va_arg(args, command_function*);
		if (function) {
			name = va_arg(args, const char*);
			description = va_arg(args, const char*);
		}
	}

	va_end(args);
}


Command*
CommandManager::FindCommand(const char* name) const
{
	CommandMap::const_iterator it = fCommands.find(name);
	if (it == fCommands.end())
		return NULL;

	return it->second;
}


void
CommandManager::ListCommands() const
{
	for (CommandMap::const_iterator it = fCommands.begin();
			it != fCommands.end(); ++it) {
		const char* name = it->first.c_str();
		Command* command = it->second;
		printf("%-16s - %s\n", name, command->Description());
	}
}


CommandManager*	CommandManager::sManager = NULL;


// #pragma mark - Command support functions


static bool
get_permissions(const char* modeString, fssh_mode_t& _permissions)
{
	// currently only octal mode is supported
	if (strlen(modeString) != 3)
		return false;

	fssh_mode_t permissions = 0;
	for (int i = 0; i < 3; i++) {
		char c = modeString[i];
		if (c < '0' || c > '7')
			return false;
		permissions = (permissions << 3) | (c - '0');
	}

	_permissions = permissions;
	return true;
}


static fssh_dev_t
get_volume_id()
{
	struct fssh_stat st;
	fssh_status_t error = _kern_read_stat(-1, kMountPoint, false, &st,
		sizeof(st));
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to stat() mount point: %s\n",
			fssh_strerror(error));
		return error;
	}

	return st.fssh_st_dev;
}


static const char *
byte_string(int64_t numBlocks, int64_t blockSize)
{
	double blocks = 1. * numBlocks * blockSize;
	static char string[64];

	if (blocks < 1024)
		sprintf(string, "%" FSSH_B_PRId64, numBlocks * blockSize);
	else {
		const char* units[] = {"K", "M", "G", NULL};
		int i = -1;

		do {
			blocks /= 1024.0;
			i++;
		} while (blocks >= 1024 && units[i + 1]);

		sprintf(string, "%.1f%s", blocks, units[i]);
	}

	return string;
}


void
print_flag(uint32_t deviceFlags, uint32_t testFlag, const char *yes,
	const char *no)
{
	printf("%s", (deviceFlags & testFlag) != 0 ? yes : no);
}


static void
list_entry(const char* file, const char* name = NULL)
{
	// construct path, if a leaf name is given
	std::string path;
	if (name) {
		path = file;
		path += '/';
		path += name;
		file = path.c_str();
	} else
		name = file;

	// stat the file
	struct fssh_stat st;
	fssh_status_t error = _kern_read_stat(-1, file, false, &st, sizeof(st));
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to stat() \"%s\": %s\n", file,
			fssh_strerror(error));
		return;
	}

	// get time
	struct tm time;
	time_t fileTime = st.fssh_st_mtime;
	localtime_r(&fileTime, &time);

	// get permissions
	std::string permissions;
	fssh_mode_t mode = st.fssh_st_mode;
	// user
	permissions += ((mode & FSSH_S_IRUSR) ? 'r' : '-');
	permissions += ((mode & FSSH_S_IWUSR) ? 'w' : '-');
	if (mode & FSSH_S_ISUID)
		permissions += 's';
	else
		permissions += ((mode & FSSH_S_IXUSR) ? 'x' : '-');
	// group
	permissions += ((mode & FSSH_S_IRGRP) ? 'r' : '-');
	permissions += ((mode & FSSH_S_IWGRP) ? 'w' : '-');
	if (mode & FSSH_S_ISGID)
		permissions += 's';
	else
		permissions += ((mode & FSSH_S_IXGRP) ? 'x' : '-');
	// others
	permissions += ((mode & FSSH_S_IROTH) ? 'r' : '-');
	permissions += ((mode & FSSH_S_IWOTH) ? 'w' : '-');
	permissions += ((mode & FSSH_S_IXOTH) ? 'x' : '-');

	// get file type
	char fileType = '?';
	if (FSSH_S_ISREG(mode)) {
		fileType = '-';
	} else if (FSSH_S_ISLNK(mode)) {
		fileType = 'l';
	} else if (FSSH_S_ISBLK(mode)) {
		fileType = 'b';
	} else if (FSSH_S_ISDIR(mode)) {
		fileType = 'd';
	} else if (FSSH_S_ISCHR(mode)) {
		fileType = 'c';
	} else if (FSSH_S_ISFIFO(mode)) {
		fileType = 'f';
	} else if (FSSH_S_ISINDEX(mode)) {
		fileType = 'i';
	}

	// get link target
	std::string nameSuffix;
	if (FSSH_S_ISLNK(mode)) {
		char buffer[FSSH_B_PATH_NAME_LENGTH];
		fssh_size_t size = sizeof(buffer) - 1;
		error = _kern_read_link(-1, file, buffer, &size);
		if (error != FSSH_B_OK)
			snprintf(buffer, sizeof(buffer), "(%s)", fssh_strerror(error));

		buffer[size] = '\0';
		nameSuffix += " -> ";
		nameSuffix += buffer;
	}

	printf("%c%s %2d %2d %10" FSSH_B_PRIdOFF
		" %d-%02d-%02d %02d:%02d:%02d %s%s\n",
		fileType, permissions.c_str(), (int)st.fssh_st_uid, (int)st.fssh_st_gid,
		st.fssh_st_size,
		1900 + time.tm_year, 1 + time.tm_mon, time.tm_mday,
		time.tm_hour, time.tm_min, time.tm_sec,
		name, nameSuffix.c_str());
}


static fssh_status_t
create_dir(const char *path, bool createParents)
{
	// stat the entry
	struct fssh_stat st;
	fssh_status_t error = _kern_read_stat(-1, path, false, &st, sizeof(st));
	if (error == FSSH_B_OK) {
		if (createParents && FSSH_S_ISDIR(st.fssh_st_mode))
			return FSSH_B_OK;

		fprintf(stderr, "Error: Cannot make dir, entry \"%s\" is in the way.\n",
			path);
		return FSSH_B_FILE_EXISTS;
	}

	// the dir doesn't exist yet
	// if we shall create all parents, do that first
	if (createParents) {
		// create the parent dir path
		// eat the trailing '/'s
		int len = strlen(path);
		while (len > 0 && path[len - 1] == '/')
			len--;

		// eat the last path component
		while (len > 0 && path[len - 1] != '/')
			len--;

		// eat the trailing '/'s
		while (len > 0 && path[len - 1] == '/')
			len--;

		// Now either nothing remains, which means we had a single component,
		// a root subdir -- in those cases we can just fall through (we should
		// actually never be here in case of the root dir, but anyway) -- or
		// there is something left, which we can call a parent directory and
		// try to create it.
		if (len > 0) {
			char *parentPath = (char*)malloc(len + 1);
			if (!parentPath) {
				fprintf(stderr, "Error: Failed to allocate memory for parent "
					"path.\n");
				return FSSH_B_NO_MEMORY;
			}
			memcpy(parentPath, path, len);
			parentPath[len] = '\0';

			error = create_dir(parentPath, createParents);

			free(parentPath);

			if (error != FSSH_B_OK)
				return error;
		}
	}

	// make the directory
	error = _kern_create_dir(-1,
		path, (FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO) & ~sUmask);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to make directory \"%s\": %s\n", path,
			fssh_strerror(error));
		return error;
	}

	return FSSH_B_OK;
}


static fssh_status_t remove_entry(int dir, const char *entry, bool recursive,
	bool force);


static fssh_status_t
remove_dir_contents(int parentDir, const char *name, bool force)
{
	// open the dir
	int dir = _kern_open_dir(parentDir, name);
	if (dir < 0) {
		fprintf(stderr, "Error: Failed to open dir \"%s\": %s\n", name,
			fssh_strerror(dir));
		return dir;
	}

	fssh_status_t error = FSSH_B_OK;

	// iterate through the entries
	fssh_ssize_t numRead;
	char buffer[sizeof(fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
	fssh_dirent *entry = (fssh_dirent*)buffer;
	while ((numRead = _kern_read_dir(dir, entry, sizeof(buffer), 1)) > 0) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		error = remove_entry(dir, entry->d_name, true, force);
		if (error != FSSH_B_OK)
			break;
	}

	if (numRead < 0) {
		fprintf(stderr, "Error: Failed to read directory \"%s\": %s\n", name,
			fssh_strerror(numRead));
		error = numRead;
	}

	// close
	_kern_close(dir);

	return error;
}


static fssh_status_t
remove_entry(int dir, const char *entry, bool recursive, bool force)
{
	// stat the file
	struct fssh_stat st;
	fssh_status_t error = _kern_read_stat(dir, entry, false, &st, sizeof(st));
	if (error != FSSH_B_OK) {
		if (force && error == FSSH_B_ENTRY_NOT_FOUND)
			return FSSH_B_OK;

		fprintf(stderr, "Error: Failed to remove \"%s\": %s\n", entry,
			fssh_strerror(error));
		return error;
	}

	if (FSSH_S_ISDIR(st.fssh_st_mode)) {
		if (!recursive) {
			fprintf(stderr, "Error: \"%s\" is a directory.\n", entry);
				// TODO: get the full path
			return FSSH_EISDIR;
		}

		// remove the contents
		error = remove_dir_contents(dir, entry, force);
		if (error != FSSH_B_OK)
			return error;

		// remove the directory
		error = _kern_remove_dir(dir, entry);
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error: Failed to remove directory \"%s\": %s\n",
				entry, fssh_strerror(error));
			return error;
		}
	} else {
		// remove the entry
		error = _kern_unlink(dir, entry);
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error: Failed to remove entry \"%s\": %s\n", entry,
				fssh_strerror(error));
			return error;
		}
	}

	return FSSH_B_OK;
}


static fssh_status_t
move_entry(int dir, const char *entry, int targetDir, const char* target,
	bool force)
{
	// stat the file
	struct fssh_stat st;
	fssh_status_t status = _kern_read_stat(dir, entry, false, &st, sizeof(st));
	if (status != FSSH_B_OK) {
		if (force && status == FSSH_B_ENTRY_NOT_FOUND)
			return FSSH_B_OK;

		fprintf(stderr, "Error: Failed to move \"%s\": %s\n", entry,
			fssh_strerror(status));
		return status;
	}

	return _kern_rename(dir, entry, targetDir, target);
}


// #pragma mark - Commands


static fssh_status_t
command_cd(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}
	const char* directory = argv[1];

	fssh_status_t error = FSSH_B_OK;
	if (directory[0] == ':') {
		if (chdir(directory + 1) < 0)
			error = fssh_get_errno();
	} else
		error = _kern_setcwd(-1, directory);

	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: cd %s: %s\n", directory, fssh_strerror(error));
		return error;
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_chmod(int argc, const char* const* argv)
{
	bool recursive = false;

	// parse parameters
	int argi = 1;
	for (argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] != '-')
			break;

		if (arg[1] == '\0') {
			fprintf(stderr, "Error: Invalid option \"-\"\n");
			return FSSH_B_BAD_VALUE;
		}

		for (int i = 1; arg[i]; i++) {
			switch (arg[i]) {
				case 'R':
					recursive = true;
					fprintf(stderr, "Sorry, recursive mode not supported "
						"yet.\n");
					return FSSH_B_BAD_VALUE;
				default:
					fprintf(stderr, "Error: Unknown option \"-%c\"\n", arg[i]);
					return FSSH_B_BAD_VALUE;
			}
		}
	}

	// get mode
	fssh_mode_t permissions;
	if (argi + 1 >= argc || !get_permissions(argv[argi++], permissions)) {
		printf("Usage: %s [ -R ] <octal mode> <file>...\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	fssh_struct_stat st;
	st.fssh_st_mode = permissions;

	// chmod loop
	for (; argi < argc; argi++) {
		const char *file = argv[argi];
		if (strlen(file) == 0) {
			fprintf(stderr, "Error: An empty path is not a valid argument!\n");
			return FSSH_B_BAD_VALUE;
		}

		fssh_status_t error = _kern_write_stat(-1, file, false, &st, sizeof(st),
			FSSH_B_STAT_MODE);
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error: Failed to change mode of \"%s\"!\n", file);
			return error;
		}
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_cat(int argc, const char* const* argv)
{
	size_t numBytes = 10;
	int fileStart = 1;
	if (argc < 2 || strcmp(argv[1], "--help") == 0) {
		printf("Usage: %s [ -n ] [FILE]...\n"
			"\t -n\tNumber of bytes to read\n",
			argv[0]);
		return FSSH_B_OK;
	}

	if (argc > 3 && strcmp(argv[1], "-n") == 0) {
		fileStart += 2;
		numBytes = strtol(argv[2], NULL, 10);
	}

	const char* const* files = argv + fileStart;
	for (; *files; files++) {
		const char* file = *files;
		int fd = _kern_open(-1, file, FSSH_O_RDONLY, FSSH_O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "error: %s\n", fssh_strerror(fd));
			return FSSH_B_BAD_VALUE;
		}

		char buffer[numBytes + 1];
		if (buffer == NULL) {
			fprintf(stderr, "error: No memory\n");
			_kern_close(fd);
			return FSSH_B_NO_MEMORY;
		}

		if (_kern_read(fd, 0, buffer, numBytes) != (ssize_t)numBytes) {
			fprintf(stderr, "error reading: %s\n", fssh_strerror(fd));
			_kern_close(fd);
			return FSSH_B_BAD_VALUE;
		}

		_kern_close(fd);
		buffer[numBytes] = '\0';
		printf("%s\n", buffer);
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_help(int argc, const char* const* argv)
{
	printf("supported commands:\n");
	CommandManager::Default()->ListCommands();
	return FSSH_B_OK;
}


static fssh_status_t
command_info(int argc, const char* const* argv)
{
	fssh_dev_t volumeID = get_volume_id();
	if (volumeID < 0)
		return volumeID;

	fssh_fs_info info;
	fssh_status_t status = _kern_read_fs_info(volumeID, &info);
	if (status != FSSH_B_OK)
		return status;

	printf("root inode:   %" FSSH_B_PRIdINO "\n", info.root);
	printf("flags:        ");
	print_flag(info.flags, FSSH_B_FS_HAS_QUERY, "Q", "-");
	print_flag(info.flags, FSSH_B_FS_HAS_ATTR, "A", "-");
	print_flag(info.flags, FSSH_B_FS_HAS_MIME, "M", "-");
	print_flag(info.flags, FSSH_B_FS_IS_SHARED, "S", "-");
	print_flag(info.flags, FSSH_B_FS_IS_PERSISTENT, "P", "-");
	print_flag(info.flags, FSSH_B_FS_IS_REMOVABLE, "R", "-");
	print_flag(info.flags, FSSH_B_FS_IS_READONLY, "-", "W");

	printf("\nblock size:   %" FSSH_B_PRIdOFF "\n", info.block_size);
	printf("I/O size:     %" FSSH_B_PRIdOFF "\n", info.io_size);
	printf("total size:   %s (%" FSSH_B_PRIdOFF " blocks)\n",
		byte_string(info.total_blocks, info.block_size), info.total_blocks);
	printf("free size:    %s (%" FSSH_B_PRIdOFF " blocks)\n",
		byte_string(info.free_blocks, info.block_size), info.free_blocks);
	printf("total nodes:  %" FSSH_B_PRIdOFF "\n", info.total_nodes);
	printf("free nodes:   %" FSSH_B_PRIdOFF "\n", info.free_nodes);
	printf("volume name:  %s\n", info.volume_name);
	printf("fs name:      %s\n", info.fsh_name);

	return FSSH_B_OK;
}


static fssh_status_t
command_ln(int argc, const char* const* argv)
{
	bool force = false;
	bool symbolic = false;
	bool dereference = true;

	// parse parameters
	int argi = 1;
	for (argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] != '-')
			break;

		if (arg[1] == '\0') {
			fprintf(stderr, "Error: Invalid option \"-\"\n");
			return FSSH_B_BAD_VALUE;
		}

		for (int i = 1; arg[i]; i++) {
			switch (arg[i]) {
				case 'f':
					force = true;
					break;
				case 's':
					symbolic = true;
					break;
				case 'n':
					dereference = false;
					break;
				default:
					fprintf(stderr, "Error: Unknown option \"-%c\"\n", arg[i]);
					return FSSH_B_BAD_VALUE;
			}
		}
	}

	if (argc - argi != 2) {
		fprintf(stderr, "Usage: %s [Options] <source> <target>\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	const char *source = argv[argi];
	const char *target = argv[argi + 1];

	// check, if the the target is an existing directory
	struct fssh_stat st;
	char targetBuffer[FSSH_B_PATH_NAME_LENGTH];
	fssh_status_t error = _kern_read_stat(-1, target, dereference, &st,
		sizeof(st));
	if (error == FSSH_B_OK) {
		if (FSSH_S_ISDIR(st.fssh_st_mode)) {
			// get source leaf
			char leaf[FSSH_B_FILE_NAME_LENGTH];
			error = get_last_path_component(source, leaf, sizeof(leaf));
			if (error != FSSH_B_OK) {
				fprintf(stderr, "Error: Failed to get leaf name of source "
					"path: %s\n", fssh_strerror(error));
				return error;
			}

			// compose a new path
			int len = strlen(target) + 1 + strlen(leaf);
			if (len > (int)sizeof(targetBuffer)) {
				fprintf(stderr, "Error: Resulting target path is too long.\n");
				return FSSH_B_BAD_VALUE;
			}

			strcpy(targetBuffer, target);
			strcat(targetBuffer, "/");
			strcat(targetBuffer, leaf);
			target = targetBuffer;
		}
	}

	// check, if the target exists
	error = _kern_read_stat(-1, target, false, &st, sizeof(st));
	if (error == FSSH_B_OK) {
		if (!force) {
			fprintf(stderr, "Error: Can't create link. \"%s\" is in the way.\n",
				target);
			return FSSH_B_FILE_EXISTS;
		}

		// unlink the entry
		error = _kern_unlink(-1, target);
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error: Failed to remove \"%s\" to make way for "
				"link: %s\n", target, fssh_strerror(error));
			return error;
		}
	}

	// finally create the link
	if (symbolic) {
		error = _kern_create_symlink(-1, target, source,
			FSSH_S_IRWXU | FSSH_S_IRWXG | FSSH_S_IRWXO);
	} else
		error = _kern_create_link(target, source);

	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to create link: %s\n",
			fssh_strerror(error));
	}

	return error;
}


static fssh_status_t
command_ls(int argc, const char* const* argv)
{
	const char* const currentDirFiles[] = { ".", NULL };
	const char* const* files;
	if (argc >= 2)
		files = argv + 1;
	else
		files = currentDirFiles;

	for (; *files; files++) {
		const char* file = *files;
		// stat file
		struct fssh_stat st;
		fssh_status_t error = _kern_read_stat(-1, file, false, &st, sizeof(st));
		if (error != FSSH_B_OK) {
			fprintf(stderr, "Error: Failed to stat() \"%s\": %s\n", file,
				fssh_strerror(error));
			continue;
		}

		// if it is a directory, print its entries
		if (FSSH_S_ISDIR(st.fssh_st_mode)) {
			printf("%s:\n", file);

			// open dir
			int fd = _kern_open_dir(-1, file);
			if (fd < 0) {
				fprintf(stderr, "Error: Failed to open dir \"%s\": %s\n",
					file, fssh_strerror(fd));
				continue;
			}

			// iterate through the entries
			char buffer[sizeof(fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
			fssh_dirent* entry = (fssh_dirent*)buffer;
			fssh_ssize_t entriesRead = 0;
			while ((entriesRead = _kern_read_dir(fd, entry, sizeof(buffer), 1))
					== 1) {
				list_entry(file, entry->d_name);
			}

			if (entriesRead < 0) {
				fprintf(stderr, "Error: reading dir \"%s\" failed: %s\n",
					file, fssh_strerror(entriesRead));
			}

			// close dir
			error = _kern_close(fd);
			if (error != FSSH_B_OK) {
				fprintf(stderr, "Error: Closing dir \"%s\" (fd: %d) failed: "
					"%s\n", file, fd, fssh_strerror(error));
				continue;
			}
		} else
			list_entry(file);
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_mkdir(int argc, const char* const* argv)
{
	bool createParents = false;

	// parse parameters
	int argi = 1;
	for (argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] != '-')
			break;

		if (arg[1] == '\0') {
			fprintf(stderr, "Error: Invalid option \"-\"\n");
			return FSSH_B_BAD_VALUE;
		}

		for (int i = 1; arg[i]; i++) {
			switch (arg[i]) {
				case 'p':
					createParents = true;
					break;
				default:
					fprintf(stderr, "Error: Unknown option \"-%c\"\n", arg[i]);
					return FSSH_B_BAD_VALUE;
			}
		}
	}

	if (argi >= argc) {
		printf("Usage: %s [ -p ] <dir>...\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	// create loop
	for (; argi < argc; argi++) {
		const char *dir = argv[argi];
		if (strlen(dir) == 0) {
			fprintf(stderr, "Error: An empty path is not a valid argument!\n");
			return FSSH_B_BAD_VALUE;
		}

		fssh_status_t error = create_dir(dir, createParents);
		if (error != FSSH_B_OK)
			return error;
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_mkindex(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <index name>\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	const char* indexName = argv[1];

	// get the volume ID
	fssh_dev_t volumeID = get_volume_id();
	if (volumeID < 0)
		return volumeID;

	// create the index
	fssh_status_t error =_kern_create_index(volumeID, indexName,
		FSSH_B_STRING_TYPE, 0);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Failed to create index \"%s\": %s\n",
			indexName, fssh_strerror(error));
		return error;
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_mv(int argc, const char* const* argv)
{
	bool force = false;

	// parse parameters
	int argi = 1;
	for (argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] != '-')
			break;

		if (arg[1] == '\0') {
			fprintf(stderr, "Error: Invalid option \"-\"\n");
			return FSSH_B_BAD_VALUE;
		}

		for (int i = 1; arg[i]; i++) {
			switch (arg[i]) {
				case 'f':
					force = true;
					break;
				default:
					fprintf(stderr, "Error: Unknown option \"-%c\"\n", arg[i]);
					return FSSH_B_BAD_VALUE;
			}
		}
	}

	// check params
	int count = argc - 1 - argi;
	if (count <= 0) {
		fprintf(stderr, "Usage: %s [-f] <file>... <target>\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	const char* target = argv[argc - 1];

	// stat the target
	struct fssh_stat st;
	fssh_status_t status = _kern_read_stat(-1, target, true, &st, sizeof(st));
	if (status != FSSH_B_OK && count != 1) {
		fprintf(stderr, "Error: Failed to stat target \"%s\": %s\n", target,
			fssh_strerror(status));
		return status;
	}

	if (status == FSSH_B_OK && FSSH_S_ISDIR(st.fssh_st_mode)) {
		// move several entries
		int targetDir = _kern_open_dir(-1, target);
		if (targetDir < 0) {
			fprintf(stderr, "Error: Failed to open dir \"%s\": %s\n", target,
				fssh_strerror(targetDir));
			return targetDir;
		}

		// move loop
		for (; argi < argc - 1; argi++) {
			status = move_entry(-1, argv[argi], targetDir, argv[argi], force);
			if (status != FSSH_B_OK) {
				_kern_close(targetDir);
				return status;
			}
		}

		_kern_close(targetDir);
		return FSSH_B_OK;
	}

	// rename single entry
	return move_entry(-1, argv[argi], -1, target, force);
}


static fssh_status_t
command_query(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <query string>\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	const char* query = argv[1];

	// get the volume ID
	fssh_dev_t volumeID = get_volume_id();
	if (volumeID < 0)
		return volumeID;

	// open query
	int fd = _kern_open_query(volumeID, query, strlen(query), 0, -1, -1);
	if (fd < 0) {
		fprintf(stderr, "Error: Failed to open query: %s\n", fssh_strerror(fd));
		return fd;
	}

	// iterate through the entries
	fssh_status_t error = FSSH_B_OK;
	char buffer[sizeof(fssh_dirent) + FSSH_B_FILE_NAME_LENGTH];
	fssh_dirent* entry = (fssh_dirent*)buffer;
	fssh_ssize_t entriesRead = 0;
	while ((entriesRead = _kern_read_dir(fd, entry, sizeof(buffer), 1)) == 1) {
		char path[FSSH_B_PATH_NAME_LENGTH];
		error = _kern_entry_ref_to_path(volumeID, entry->d_pino, entry->d_name,
			path, sizeof(path));
		if (error == FSSH_B_OK) {
			printf("  %s\n", path);
		} else {
			fprintf(stderr, "  failed to resolve entry (%8" FSSH_B_PRIdINO
				", \"%s\")\n", entry->d_pino, entry->d_name);
		}
	}

	if (entriesRead < 0) {
		fprintf(stderr, "Error: reading query failed: %s\n",
			fssh_strerror(entriesRead));
	}

	// close query
	error = _kern_close(fd);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Closing query (fd: %d) failed: %s\n",
			fd, fssh_strerror(error));
	}

	return error;
}


static fssh_status_t
command_quit(int argc, const char* const* argv)
{
	return COMMAND_RESULT_EXIT;
}


static fssh_status_t
command_rm(int argc, const char* const* argv)
{
	bool recursive = false;
	bool force = false;

	// parse parameters
	int argi = 1;
	for (argi = 1; argi < argc; argi++) {
		const char *arg = argv[argi];
		if (arg[0] != '-')
			break;

		if (arg[1] == '\0') {
			fprintf(stderr, "Error: Invalid option \"-\"\n");
			return FSSH_B_BAD_VALUE;
		}

		for (int i = 1; arg[i]; i++) {
			switch (arg[i]) {
				case 'f':
					force = true;
					break;
				case 'r':
					recursive = true;
					break;
				default:
					fprintf(stderr, "Error: Unknown option \"-%c\"\n", arg[i]);
					return FSSH_B_BAD_VALUE;
			}
		}
	}

	// check params
	if (argi >= argc) {
		fprintf(stderr, "Usage: %s [ -r ] <file>...\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	// remove loop
	for (; argi < argc; argi++) {
		fssh_status_t error = remove_entry(-1, argv[argi], recursive, force);
		if (error != FSSH_B_OK)
			return error;
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_sync(int argc, const char* const* argv)
{
	fssh_status_t error = _kern_sync();
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: syncing: %s\n", fssh_strerror(error));
		return error;
	}

	return FSSH_B_OK;
}


static fssh_status_t
command_ioctl(int argc, const char* const* argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <opcode>\n", argv[0]);
		return FSSH_B_BAD_VALUE;
	}

	int rootDir = _kern_open_dir(-1, "/myfs");
	if (rootDir < 0)
		return rootDir;

	fssh_status_t status = _kern_ioctl(rootDir, atoi(argv[1]), NULL, 0);

	_kern_close(rootDir);

	if (status != FSSH_B_OK) {
		fprintf(stderr, "Error: ioctl failed: %s\n", fssh_strerror(status));
		return status;
	}

	return FSSH_B_OK;
}


static void
register_commands()
{
	CommandManager::Default()->AddCommands(
		command_cd,			"cd",			"change current directory",
		command_chmod,		"chmod",		"change file permissions",
		command_cp,			"cp",			"copy files and directories",
		command_cat,		"cat",	"concatenate file(s) to stdout",
		command_help,		"help",			"list supported commands",
		command_info,		"info",			"prints volume informations",
		command_ioctl,		"ioctl",		"ioctl() on root, for FS debugging only",
		command_ln,			"ln",			"create a hard or symbolic link",
		command_ls,			"ls",			"list files or directories",
		command_mkdir,		"mkdir",		"create directories",
		command_mkindex,	"mkindex",		"create an index",
		command_mv,			"mv",			"move/rename files and directories",
		command_query,		"query",		"query for files",
		command_quit,		"quit/exit",	"quit the shell",
		command_rm,			"rm",			"remove files and directories",
		command_sync,		"sync",			"syncs the file system",
		NULL
	);
}


// #pragma mark - ArgVector


class ArgVector {
public:
	ArgVector()
		: fArgc(0),
		  fArgv(NULL)
	{
	}

	~ArgVector()
	{
		_Cleanup();
	}

	int Argc() const
	{
		return fArgc;
	}

	const char* const* Argv() const
	{
		return fArgv;
	}

	bool Parse(const char* commandLine)
	{
		_Cleanup();

		// init temporary arg/argv storage
		std::string currentArg;
		std::vector<std::string> argVector;

		fCurrentArg = &currentArg;
		fCurrentArgStarted = false;
		fArgVector = &argVector;

		for (; *commandLine; commandLine++) {
			char c = *commandLine;

			// whitespace delimits args and is otherwise ignored
			if (isspace(c)) {
				_PushCurrentArg();
				continue;
			}

			switch (c) {
				case '\'':
					// quoted string -- no quoting
					while (*++commandLine != '\'') {
						c = *commandLine;
						if (c == '\0') {
							fprintf(stderr, "Error: Unterminated quoted "
								"string.\n");
							return false;
						}
						_PushCharacter(c);
					}
					break;

				case '"':
					// quoted string -- some quoting
					while (*++commandLine != '"') {
						c = *commandLine;
						if (c == '\0') {
							fprintf(stderr, "Error: Unterminated quoted "
								"string.\n");
							return false;
						}

						if (c == '\\') {
							c = *++commandLine;
							if (c == '\0') {
								fprintf(stderr, "Error: Unterminated quoted "
									"string.\n");
								return false;
							}

							// only '\' and '"' can be quoted, otherwise the
							// the '\' is treated as a normal char
							if (c != '\\' && c != '"')
								_PushCharacter('\\');
						}

						_PushCharacter(c);
					}
					break;

				case '\\':
					// quoted char
					c = *++commandLine;
					if (c == '\0') {
						fprintf(stderr, "Error: Command line ends with "
							"'\\'.\n");
						return false;
					}
					_PushCharacter(c);
					break;

				default:
					// normal char
					_PushCharacter(c);
					break;
			}
		}

		// commit last arg
		_PushCurrentArg();

		// build arg vector
		fArgc = argVector.size();
		fArgv = new char*[fArgc + 1];
		for (int i = 0; i < fArgc; i++) {
			int len = argVector[i].length();
			fArgv[i] = new char[len + 1];
			memcpy(fArgv[i], argVector[i].c_str(), len + 1);
		}
		fArgv[fArgc] = NULL;

		return true;
	}

private:
	void _Cleanup()
	{
		if (fArgv) {
			for (int i = 0; i < fArgc; i++)
				delete[] fArgv[i];
			delete[] fArgv;
		}
	}

	void _PushCurrentArg()
	{
		if (fCurrentArgStarted) {
			fArgVector->push_back(*fCurrentArg);
			fCurrentArgStarted = false;
		}
	}

	void _PushCharacter(char c)
	{
		if (!fCurrentArgStarted) {
			*fCurrentArg = "";
			fCurrentArgStarted = true;
		}

		*fCurrentArg += c;
	}

private:
	// temporaries
	std::string*				fCurrentArg;
	bool						fCurrentArgStarted;
	std::vector<std::string>*	fArgVector;

	int							fArgc;
	char**						fArgv;
};


// #pragma mark - input loop


static char*
read_command_line(char* buffer, int bufferSize)
{
	// print prompt (including cwd, if available)
	char directory[FSSH_B_PATH_NAME_LENGTH];
	if (_kern_getcwd(directory, sizeof(directory)) == FSSH_B_OK)
		printf("fssh:%s> ", directory);
	else
		printf("fssh> ");
	fflush(stdout);

	// read input line
	return fgets(buffer, bufferSize, stdin);
}


static void
input_loop(bool interactive)
{
	static const int kInputBufferSize = 100 * 1024;
	char* inputBuffer = new char[kInputBufferSize];

	for (;;) {
		// read command line
		if (interactive) {
			if (!read_command_line(inputBuffer, kInputBufferSize))
				break;
		} else {
			if (!get_external_command(inputBuffer, kInputBufferSize))
				break;
		}

		// construct argv vector
		int result = FSSH_B_BAD_VALUE;
		ArgVector argVector;
		if (argVector.Parse(inputBuffer) && argVector.Argc() > 0) {
			int argc = argVector.Argc();
			const char* const* argv = argVector.Argv();

			// find command
			Command* command = CommandManager::Default()->FindCommand(argv[0]);
			if (command) {
				// execute it
				result = command->Do(argc, argv);
				if (result == COMMAND_RESULT_EXIT) {
					if (!interactive)
						reply_to_external_command(0);
					break;
				}
			} else {
				fprintf(stderr, "Error: Invalid command \"%s\". Type \"help\" "
					"for a list of supported commands\n", argv[0]);
			}
		}

		if (!interactive)
			reply_to_external_command(fssh_to_host_error(result));
	}

	if (!interactive)
		external_command_cleanup();

	delete[] inputBuffer;
}


static int
standard_session(const char* device, const char* fsName, bool interactive)
{
	// mount FS
	fssh_dev_t fsDev = _kern_mount(kMountPoint, device, fsName, 0, NULL, 0);
	if (fsDev < 0) {
		fprintf(stderr, "Error: Mounting FS failed: %s\n",
			fssh_strerror(fsDev));
		return 1;
	}

	// register commands
	register_commands();
	register_additional_commands();

	// process commands
	input_loop(interactive);

	// unmount FS
	_kern_setcwd(-1, "/");	// avoid a "busy" vnode
	fssh_status_t error = _kern_unmount(kMountPoint, 0);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Unmounting FS failed: %s\n",
			fssh_strerror(error));
		return 1;
	}

	return 0;
}


static int
initialization_session(const char* device, const char* fsName,
	const char* volumeName, const char* initParameters)
{
	fssh_status_t error = _kern_initialize_volume(fsName, device,
		volumeName, initParameters);
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Initializing volume failed: %s\n",
			fssh_strerror(error));
		return 1;
	}

	return 0;
}


static void
print_usage(bool error)
{
	fprintf((error ? stderr : stdout),
		"Usage: %s [ --start-offset <startOffset>]\n"
		"          [ --end-offset <endOffset>] [-n] <device>\n"
		"       %s [ --start-offset <startOffset>]\n"
		"          [ --end-offset <endOffset>]\n"
		"          --initialize [-n] <device> <volume name> "
			"[ <init parameters> ]\n",
		sArgv[0], sArgv[0]
	);
}


static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


}	// namespace FSShell


using namespace FSShell;


int
main(int argc, const char* const* argv)
{
	sArgc = argc;
	sArgv = argv;

	// process arguments
	bool interactive = true;
	bool initialize = false;
	const char* device = NULL;
	const char* volumeName = NULL;
	const char* initParameters = NULL;
	fssh_off_t startOffset = 0;
	fssh_off_t endOffset = -1;

	// eat options
	int argi = 1;
	while (argi < argc && argv[argi][0] == '-') {
		const char* arg = argv[argi++];
		if (strcmp(arg, "--help") == 0) {
			print_usage_and_exit(false);
		} else if (strcmp(arg, "--initialize") == 0) {
			initialize = true;
		} else if (strcmp(arg, "-n") == 0) {
			interactive = false;
		} else if (strcmp(arg, "--start-offset") == 0) {
			if (argi >= argc)
				print_usage_and_exit(true);
			startOffset = atoll(argv[argi++]);
		} else if (strcmp(arg, "--end-offset") == 0) {
			if (argi >= argc)
				print_usage_and_exit(true);
			endOffset = atoll(argv[argi++]);
		} else {
			print_usage_and_exit(true);
		}
	}

	// get device
	if (argi >= argc)
		print_usage_and_exit(true);
	device = argv[argi++];

	// get volume name and init parameters
	if (initialize) {
		// volume name
		if (argi >= argc)
			print_usage_and_exit(true);
		volumeName = argv[argi++];

		// (optional) init paramaters
		if (argi < argc)
			initParameters = argv[argi++];
	}

	// more parameters are excess
	if (argi < argc)
		print_usage_and_exit(true);

	// get FS module
	if (!modules[0]) {
		fprintf(stderr, "Error: Couldn't find FS module!\n");
		return 1;
	}
	const char* fsName = modules[0]->name;

	fssh_status_t error;

	// init kernel
	error = init_kernel();
	if (error != FSSH_B_OK) {
		fprintf(stderr, "Error: Initializing kernel failed: %s\n",
			fssh_strerror(error));
		return error;
	}

	// restrict access if requested
	if (startOffset != 0 || endOffset != -1)
		add_file_restriction(device, startOffset, endOffset);

	// start the action
	int result;
	if (initialize) {
		result = initialization_session(device, fsName, volumeName,
			initParameters);
	} else
		result = standard_session(device, fsName, interactive);

	return result;
}
