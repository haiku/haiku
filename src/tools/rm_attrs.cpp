/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


// exported by the generic attribute support in libroot_build.so
extern "C" bool __get_attribute_dir_path(const struct stat* st, char* buffer);


class Path {
public:
	bool Init(const char* path)
	{
		size_t len = strlen(path);
		if (len == 0 || len >= PATH_MAX)
			return false;

		strcpy(fPath, path);
		fPathLen = len;

		return true;
	}

	const char* GetPath() const
	{
		return fPath;
	}

	char* Buffer()
	{
		return fPath;
	}

	void BufferChanged()
	{
		fPathLen = strlen(fPath);
	}

	bool PushLeaf(const char* leaf)
	{
		size_t leafLen = strlen(leaf);

		int separatorLen = (fPath[fPathLen - 1] == '/' ? 0 : 1);
		if (fPathLen + separatorLen + leafLen >= PATH_MAX)
			return false;

		if (separatorLen > 0)
			fPath[fPathLen++] = '/';

		strcpy(fPath + fPathLen, leaf);
		fPathLen += leafLen;

		return true;
	}

	bool PopLeaf()
	{
		char* lastSlash = strrchr(fPath, '/');
		if (lastSlash == NULL || lastSlash == fPath)
			return false;

		*lastSlash = '\0';
		fPathLen = lastSlash - fPath;

		return true;
	}

	char	fPath[PATH_MAX];
	size_t	fPathLen;
};


static bool remove_entry(Path& entry, bool recursive, bool force,
	bool removeAttributes);


static void
remove_dir_contents(Path& path, bool force, bool removeAttributes)
{
	// open the dir
	DIR* dir = opendir(path.GetPath());
	if (dir == NULL) {
		fprintf(stderr, "Error: Failed to open dir \"%s\": %s\n",
			path.GetPath(), strerror(errno));
		return;
	}

	// iterate through the entries
	errno = 0;
	while (dirent* entry = readdir(dir)) {
		// skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (!path.PushLeaf(entry->d_name)) {
			fprintf(stderr, "Error: Path name of entry too long: dir: \"%s\", "
				"entry: \"%s\"\n", path.GetPath(), entry->d_name);
			continue;
		}

		remove_entry(path, true, force, removeAttributes);

		path.PopLeaf();

		errno = 0;
	}

	if (errno != 0) {
		fprintf(stderr, "Error: Failed to read directory \"%s\": %s\n",
			path.GetPath(), strerror(errno));
	}

	// close
	closedir(dir);
}


static bool
remove_entry(Path& path, bool recursive, bool force, bool removeAttributes)
{
	// stat the file
	struct stat st;
	if (lstat(path.GetPath(), &st) < 0) {
		// errno == 0 shouldn't happen, but found on OpenSUSE Linux 10.3
		if (force && (errno == ENOENT || errno == 0))
			return true;

		fprintf(stderr, "Error: Failed to remove \"%s\": %s\n", path.GetPath(),
			strerror(errno));
		return false;
	}

	// remove the file's attributes
	if (removeAttributes) {
		Path attrDirPath;
		if (__get_attribute_dir_path(&st, attrDirPath.Buffer())) {
			attrDirPath.BufferChanged();
			remove_entry(attrDirPath, true, true, false);
		}
	}

	if (S_ISDIR(st.st_mode)) {
		if (!recursive) {
			fprintf(stderr, "Error: \"%s\" is a directory.\n", path.GetPath());
			return false;
		}

		// remove the contents
		remove_dir_contents(path, force, removeAttributes);

		// remove the directory
		if (rmdir(path.GetPath()) < 0) {
			fprintf(stderr, "Error: Failed to remove directory \"%s\": %s\n",
				path.GetPath(), strerror(errno));
			return false;
		}
	} else {
		// remove the entry
		if (unlink(path.GetPath()) < 0) {
			fprintf(stderr, "Error: Failed to remove entry \"%s\": %s\n",
				path.GetPath(), strerror(errno));
			return false;
		}
	}

	return true;
}


int
main(int argc, const char* const* argv)
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
			exit(1);
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
					exit(1);
			}
		}
	}
	
	// check params
	if (argi >= argc) {
		fprintf(stderr, "Usage: %s [ -rf ] <file>...\n", argv[0]);
		exit(1);
	}

	// remove loop
	for (; argi < argc; argi++) {
		Path path;
		if (!path.Init(argv[argi])) {
			fprintf(stderr, "Error: Invalid path: \"%s\".\n", argv[argi]);
			continue;
		}

		remove_entry(path, recursive, force, true);
	}

	return 0;
}
