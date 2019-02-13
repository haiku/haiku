/*
 * Copyright 2005-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <fs_attr.h>
#include <Mime.h>
#include <Node.h>
#include <Path.h>
#include <SymLink.h>
#include <TypeConstants.h>

#include <EntryFilter.h>


using BPrivate::EntryFilter;


static const char *kCommandName = "copyattr";
static const int kCopyBufferSize = 64 * 1024;	// 64 KB

static int kArgc;
static const char *const *kArgv;

// usage
const char *kUsage =
"Usage: %s <options> <source> [ ... ] <destination>\n"
"\n"
"Copies attributes from one or more files to another, or copies one or more\n"
"files or directories, with all or a specified subset of their attributes, to\n"
"another location.\n"
"\n"
"If option \"-d\"/\"--data\" is given, the behavior is similar to \"cp -df\",\n"
"save that attributes are copied. That is, if more than one source file is\n"
"given, the destination file must be a directory. If the destination is a\n"
"directory (or a symlink to a directory), the source files are copied into\n"
"the destination directory. Entries that are in the way are removed, unless\n"
"they are directories. If the source is a directory too, the attributes will\n"
"be copied and, if recursive operation is specified, the program continues\n"
"copying the contents of the source directory. If the source is not a\n"
"directory the program aborts with an error message.\n"
"\n"
"If option \"-d\"/\"--data\" is not given, only attributes are copied.\n"
"Regardless of the file type of the destination, the attributes of the source\n"
"files are copied to it. If an attribute with the same name as one to be\n"
"copied already exists, it is replaced. If more than one source file is\n"
"specified the semantics are similar to invoking the program multiple times\n"
"with the same options and destination and only one source file at a time,\n"
"in the order the source files are given. If recursive operation is\n"
"specified, the program recursively copies the attributes of the directory\n"
"contents; if the destination file is not a directory, or for a source entry\n"
"there exists no destination entry, the program aborts with an error\n"
"message.\n"
"\n"
"Note, that the behavior of the program differs from the one shipped with\n"
"BeOS R5.\n"
"\n"
"Options:\n"
"  -d, --data         - Copy the data of the file(s), too.\n"
"  -h, --help         - Print this help text and exit.\n"
"  -m, --move         - If -d is given, the source files are removed after\n"
"                       being copied. Has no effect otherwise.\n"
"  -n, --name <name>  - Only copy the attribute with name <name>.\n"
"  -r, --recursive    - Copy directories recursively.\n"
"  -t, --type <type>  - Copy only the attributes of type <type>. If -n is\n"
"                       specified too, only the attribute matching the name\n"
"                       and the type is copied.\n"
"  -x <pattern>       - Exclude source entries matching <pattern>.\n"
"  -X <pattern>       - Exclude source paths matching <pattern>.\n"
"  -v, --verbose      - Print more messages.\n"
"   -, --             - Marks the end of options. The arguments after, even\n"
"                       if starting with \"-\" are considered file names.\n"
"\n"
"Parameters:\n"
"  <type>             - One of: int, llong, string, mimestr, float, double,\n"
"                       boolean.\n"
;

// supported attribute types
struct supported_attribute_type {
	const char	*type_name;
	type_code	type;
};

const supported_attribute_type kSupportedAttributeTypes[] = {
	{ "int",		B_INT32_TYPE },
	{ "llong",		B_INT64_TYPE },
	{ "string",		B_STRING_TYPE },
	{ "mimestr",	B_MIME_STRING_TYPE },
	{ "float",		B_FLOAT_TYPE },
	{ "double",		B_DOUBLE_TYPE },
	{ "boolean",	B_BOOL_TYPE },
	{ NULL, 0 },
};

// AttributeFilter
struct AttributeFilter {
	AttributeFilter()
		:
		fName(NULL),
		fType(B_ANY_TYPE)
	{
	}

	void SetTo(const char *name, type_code type)
	{
		fName = name;
		fType = type;
	}

	bool Filter(const char *name, type_code type) const {
		if (fName && strcmp(name, fName) != 0)
			return false;

		return (fType == B_ANY_TYPE || type == fType);
	}

private:
	const char	*fName;
	type_code	fType;
};


// Parameters
struct Parameters {
	Parameters()
		:
		copy_data(false),
		recursive(false),
		move_files(false),
		verbose(false)
	{
	}

	bool			copy_data;
	bool			recursive;
	bool			move_files;
	bool			verbose;
	AttributeFilter	attribute_filter;
	EntryFilter		entry_filter;
};


// print_usage
static void
print_usage(bool error)
{
	// get command name
	const char *commandName = NULL;
	if (kArgc > 0) {
		if (const char *lastSlash = strchr(kArgv[0], '/'))
			commandName = lastSlash + 1;
		else
			commandName = kArgv[0];
	}

	if (!commandName || strlen(commandName) == 0)
		commandName = kCommandName;

	// print usage
	fprintf((error ? stderr : stdout), kUsage, commandName);
}


// print_usage_and_exit
static void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}


// next_arg
static const char *
next_arg(int &argi, bool optional = false)
{
	if (argi >= kArgc) {
		if (!optional)
			print_usage_and_exit(true);
		return NULL;
	}

	return kArgv[argi++];
}


// copy_attributes
static void
copy_attributes(const char *sourcePath, BNode &source, const char *destPath,
	BNode &destination, const Parameters &parameters)
{
	char attrName[B_ATTR_NAME_LENGTH];
	while (source.GetNextAttrName(attrName) == B_OK) {
		// get attr info
		attr_info attrInfo;
		status_t error = source.GetAttrInfo(attrName, &attrInfo);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to get info of attribute \"%s\" "
				"of file \"%s\": %s\n", attrName, sourcePath, strerror(error));
			exit(1);
		}

		// filter
		if (!parameters.attribute_filter.Filter(attrName, attrInfo.type))
			continue;

		// copy the attribute
		char buffer[kCopyBufferSize];
		off_t offset = 0;
		off_t bytesLeft = attrInfo.size;
		// go at least once through the loop, so that empty attribute will be
		// created as well
		do {
			size_t toRead = kCopyBufferSize;
			if ((off_t)toRead > bytesLeft)
				toRead = bytesLeft;

			// read
			ssize_t bytesRead = source.ReadAttr(attrName, attrInfo.type,
				offset, buffer, toRead);
			if (bytesRead < 0) {
				fprintf(stderr, "Error: Failed to read attribute \"%s\" "
					"of file \"%s\": %s\n", attrName, sourcePath,
					strerror(bytesRead));
				exit(1);
			}

			// write
			ssize_t bytesWritten = destination.WriteAttr(attrName,
				attrInfo.type, offset, buffer, bytesRead);
			if (bytesWritten < 0) {
				fprintf(stderr, "Error: Failed to write attribute \"%s\" "
					"of file \"%s\": %s\n", attrName, destPath,
					strerror(bytesWritten));
				exit(1);
			}

			bytesLeft -= bytesRead;
			offset += bytesRead;

		} while (bytesLeft > 0);
	}
}


// copy_file_data
static void
copy_file_data(const char *sourcePath, BFile &source, const char *destPath,
	BFile &destination, const Parameters &parameters)
{
	char buffer[kCopyBufferSize];
	off_t offset = 0;
	while (true) {
		// read
		ssize_t bytesRead = source.ReadAt(offset, buffer, sizeof(buffer));
		if (bytesRead < 0) {
			fprintf(stderr, "Error: Failed to read from file \"%s\": %s\n",
				sourcePath, strerror(bytesRead));
			exit(1);
		}

		if (bytesRead == 0)
			return;

		// write
		ssize_t bytesWritten = destination.WriteAt(offset, buffer, bytesRead);
		if (bytesWritten < 0) {
			fprintf(stderr, "Error: Failed to write to file \"%s\": %s\n",
				destPath, strerror(bytesWritten));
			exit(1);
		}

		offset += bytesRead;
	}
}


// copy_entry
static void
copy_entry(const char *sourcePath, const char *destPath,
	const Parameters &parameters)
{
	// apply entry filter
	if (!parameters.entry_filter.Filter(sourcePath))
		return;

	// stat source
	struct stat sourceStat;
	if (lstat(sourcePath, &sourceStat) < 0) {
		fprintf(stderr, "Error: Couldn't access \"%s\": %s\n", sourcePath,
			strerror(errno));
		exit(1);
	}

	// stat destination
	struct stat destStat;
	bool destExists = lstat(destPath, &destStat) == 0;

	if (!destExists && !parameters.copy_data) {
		fprintf(stderr, "Error: Destination file \"%s\" does not exist.\n",
			destPath);
		exit(1);
	}

	if (parameters.verbose)
		printf("%s\n", destPath);

	// check whether to delete/create the destination
	bool unlinkDest = (destExists && parameters.copy_data);
	bool createDest = parameters.copy_data;
	if (destExists) {
		if (S_ISDIR(destStat.st_mode)) {
			if (S_ISDIR(sourceStat.st_mode)) {
				// both are dirs; nothing to do
				unlinkDest = false;
				createDest = false;
			} else if (parameters.copy_data || parameters.recursive) {
				// destination is directory, but source isn't, and mode is
				// not non-recursive attributes-only copy
				fprintf(stderr, "Error: Can't copy \"%s\", since directory "
					"\"%s\" is in the way.\n", sourcePath, destPath);
				exit(1);
			}
		}
	}

	// unlink the destination
	if (unlinkDest) {
		if (unlink(destPath) < 0) {
			fprintf(stderr, "Error: Failed to unlink \"%s\": %s\n", destPath,
				strerror(errno));
			exit(1);
		}
	}

	// open source node
	BNode _sourceNode;
	BFile sourceFile;
	BDirectory sourceDir;
	BNode *sourceNode = NULL;
	status_t error;

	if (S_ISDIR(sourceStat.st_mode)) {
		error = sourceDir.SetTo(sourcePath);
		sourceNode = &sourceDir;
	} else if (S_ISREG(sourceStat.st_mode)) {
		error = sourceFile.SetTo(sourcePath, B_READ_ONLY);
		sourceNode = &sourceFile;
	} else {
		error = _sourceNode.SetTo(sourcePath);
		sourceNode = &_sourceNode;
	}

	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to open \"%s\": %s\n",
			sourcePath, strerror(error));
		exit(1);
	}

	// create the destination
	BNode _destNode;
	BDirectory destDir;
	BFile destFile;
	BSymLink destSymLink;
	BNode *destNode = NULL;

	if (createDest) {
		if (S_ISDIR(sourceStat.st_mode)) {
			// create dir
			error = BDirectory().CreateDirectory(destPath, &destDir);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to make directory \"%s\": %s\n",
					destPath, strerror(error));
				exit(1);
			}

			destNode = &destDir;

		} else if (S_ISREG(sourceStat.st_mode)) {
			// create file
			error = BDirectory().CreateFile(destPath, &destFile);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to create file \"%s\": %s\n",
					destPath, strerror(error));
				exit(1);
			}

			destNode = &destFile;

			// copy file contents
			copy_file_data(sourcePath, sourceFile, destPath, destFile,
				parameters);

		} else if (S_ISLNK(sourceStat.st_mode)) {
			// read symlink
			char linkTo[B_PATH_NAME_LENGTH + 1];
			ssize_t bytesRead = readlink(sourcePath, linkTo,
				sizeof(linkTo) - 1);
			if (bytesRead < 0) {
				fprintf(stderr, "Error: Failed to read symlink \"%s\": %s\n",
					sourcePath, strerror(errno));
				exit(1);
			}

			// null terminate the link contents
			linkTo[bytesRead] = '\0';

			// create symlink
			error = BDirectory().CreateSymLink(destPath, linkTo, &destSymLink);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to create symlink \"%s\": %s\n",
					destPath, strerror(error));
				exit(1);
			}

			destNode = &destSymLink;

		} else {
			fprintf(stderr, "Error: Source file \"%s\" has unsupported type.\n",
				sourcePath);
			exit(1);
		}

		// copy attributes (before setting the permissions!)
		copy_attributes(sourcePath, *sourceNode, destPath, *destNode,
			parameters);

		// set file owner, group, permissions, times
		destNode->SetOwner(sourceStat.st_uid);
		destNode->SetGroup(sourceStat.st_gid);
		destNode->SetPermissions(sourceStat.st_mode);
		#ifdef HAIKU_TARGET_PLATFORM_HAIKU
			destNode->SetCreationTime(sourceStat.st_crtime);
		#endif
		destNode->SetModificationTime(sourceStat.st_mtime);

	} else {
		// open destination node
		error = _destNode.SetTo(destPath);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to open \"%s\": %s\n",
				destPath, strerror(error));
			exit(1);
		}

		destNode = &_destNode;

		// copy attributes
		copy_attributes(sourcePath, *sourceNode, destPath, *destNode,
			parameters);
	}

	// the destination node is no longer needed
	destNode->Unset();

	// recurse
	if (parameters.recursive && S_ISDIR(sourceStat.st_mode)) {
		char buffer[sizeof(dirent) + B_FILE_NAME_LENGTH];
		dirent *entry = (dirent*)buffer;
		while (sourceDir.GetNextDirents(entry, sizeof(buffer), 1) == 1) {
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			// construct new entry paths
			BPath sourceEntryPath;
			error = sourceEntryPath.SetTo(sourcePath, entry->d_name);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to construct entry path from "
					"dir \"%s\" and name \"%s\": %s\n",
					sourcePath, entry->d_name, strerror(error));
				exit(1);
			}

			BPath destEntryPath;
			error = destEntryPath.SetTo(destPath, entry->d_name);
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to construct entry path from "
					"dir \"%s\" and name \"%s\": %s\n",
					destPath, entry->d_name, strerror(error));
				exit(1);
			}

			// copy the entry
			copy_entry(sourceEntryPath.Path(), destEntryPath.Path(),
				parameters);
		}
	}

	// remove source in move mode
	if (parameters.move_files) {
		if (S_ISDIR(sourceStat.st_mode)) {
			if (rmdir(sourcePath) < 0) {
				fprintf(stderr, "Error: Failed to remove \"%s\": %s\n",
					sourcePath, strerror(errno));
				exit(1);
			}

		} else {
			if (unlink(sourcePath) < 0) {
				fprintf(stderr, "Error: Failed to unlink \"%s\": %s\n",
					sourcePath, strerror(errno));
				exit(1);
			}
		}
	}
}

// copy_files
static void
copy_files(const char **sourcePaths, int sourceCount,
	const char *destPath, const Parameters &parameters)
{
	// check, if destination exists
	BEntry destEntry;
	status_t error = destEntry.SetTo(destPath);
	if (error != B_OK) {
		fprintf(stderr, "Error: Couldn't access \"%s\": %s\n", destPath,
			strerror(error));
		exit(1);
	}
	bool destExists = destEntry.Exists();

	// If it exists, check whether it is a directory. In case we don't copy
	// the data, we pretend the destination is no directory, even if it is
	// one.
	bool destIsDir = false;
	if (destExists && parameters.copy_data) {
		struct stat st;
		error = destEntry.GetStat(&st);
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to stat \"%s\": %s\n", destPath,
				strerror(error));
			exit(1);
		}

		if (S_ISDIR(st.st_mode)) {
			destIsDir = true;
		} else if (S_ISLNK(st.st_mode)) {
			// a symlink -- check if it refers to a dir
			BEntry resolvedDestEntry;
			if (resolvedDestEntry.SetTo(destPath, true) == B_OK
				&& resolvedDestEntry.IsDirectory()) {
				destIsDir = true;
			}
		}
	}

	// If we have multiple source files, the destination should be a directory,
	// if we want to copy the file data.
	if (sourceCount > 1 && parameters.copy_data && !destIsDir) {
		fprintf(stderr, "Error: Destination needs to be a directory when "
			"multiple source files are specified and option \"-d\" is "
			"given.\n");
		exit(1);
	}

	// iterate through the source files
	for (int i = 0; i < sourceCount; i++) {
		const char *sourcePath = sourcePaths[i];
		// If the destination is a directory, we usually want to copy the
		// sources into it. The user might have specified a source path ending
		// in "/." or "/.." however, in which case we copy the contents of the
		// given directory.
		bool copySourceContentsOnly = false;
		if (destIsDir) {
			// skip trailing '/'s
			int sourceLen = strlen(sourcePath);
			while (sourceLen > 1 && sourcePath[sourceLen - 1] == '/')
				sourceLen--;

			// find the start of the leaf name
			int leafStart = sourceLen;
			while (leafStart > 0 && sourcePath[leafStart - 1] != '/')
				leafStart--;

			// If the path is the root directory or the leaf is "." or "..",
			// we copy the contents only.
			int leafLen = sourceLen - leafStart;
			if (leafLen == 0 || (leafLen <= 2
					&& strncmp(sourcePath + leafStart, "..", leafLen) == 0)) {
				copySourceContentsOnly = true;
			}
		}

		if (destIsDir && !copySourceContentsOnly) {
			// construct a usable destination entry path
			// normalize source path
			BPath normalizedSourcePath;
			error = normalizedSourcePath.SetTo(sourcePath);
			if (error != B_OK) {
				fprintf(stderr, "Error: Invalid path \"%s\".\n", sourcePath);
				exit(1);
			}

			BPath destEntryPath;
			error = destEntryPath.SetTo(destPath, normalizedSourcePath.Leaf());
			if (error != B_OK) {
				fprintf(stderr, "Error: Failed to get destination path for "
					"source \"%s\" and destination directory \"%s\".\n",
					sourcePath, destPath);
				exit(1);
			}

			copy_entry(normalizedSourcePath.Path(), destEntryPath.Path(),
				parameters);
		} else {
			copy_entry(sourcePath, destPath, parameters);
		}
	}
}


// main
int
main(int argc, const char *const *argv)
{
	kArgc = argc;
	kArgv = argv;

	// parameters
	Parameters parameters;
	const char *attributeName = NULL;
	const char *attributeTypeString = NULL;
	const char **files = new const char*[argc];
	int fileCount = 0;

	// parse the arguments
	bool moreOptions = true;
	for (int argi = 1; argi < argc; ) {
		const char *arg = argv[argi++];
		if (moreOptions && arg[0] == '-') {
			if (strcmp(arg, "-d") == 0 || strcmp(arg, "--data") == 0) {
				parameters.copy_data = true;

			} else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				print_usage_and_exit(false);

			} else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--move") == 0) {
				parameters.move_files = true;

			} else if (strcmp(arg, "-n") == 0 || strcmp(arg, "--name") == 0) {
				if (attributeName) {
					fprintf(stderr, "Error: Only one attribute name can be "
						"specified.\n");
					exit(1);
				}

				attributeName = next_arg(argi);

			} else if (strcmp(arg, "-r") == 0
					|| strcmp(arg, "--recursive") == 0) {
				parameters.recursive = true;

			} else if (strcmp(arg, "-t") == 0 || strcmp(arg, "--type") == 0) {
				if (attributeTypeString) {
					fprintf(stderr, "Error: Only one attribute type can be "
						"specified.\n");
					exit(1);
				}

				attributeTypeString = next_arg(argi);

			} else if (strcmp(arg, "-v") == 0
					|| strcmp(arg, "--verbose") == 0) {
				parameters.verbose = true;

 			} else if (strcmp(arg, "-x") == 0) {
 				parameters.entry_filter.AddExcludeFilter(next_arg(argi), true);

 			} else if (strcmp(arg, "-X") == 0) {
 				parameters.entry_filter.AddExcludeFilter(next_arg(argi), false);

			} else if (strcmp(arg, "-") == 0 || strcmp(arg, "--") == 0) {
				moreOptions = false;

			} else {
				fprintf(stderr, "Error: Invalid option: \"%s\"\n", arg);
				print_usage_and_exit(true);
			}

		} else {
			// file
			files[fileCount++] = arg;
		}
	}

	// check parameters

	// enough files
	if (fileCount < 2) {
		fprintf(stderr, "Error: Not enough file names specified.\n");
		print_usage_and_exit(true);
	}

	// attribute type
	type_code attributeType = B_ANY_TYPE;
	if (attributeTypeString) {
		bool found = false;
		for (int i = 0; kSupportedAttributeTypes[i].type_name; i++) {
			if (strcmp(attributeTypeString,
					kSupportedAttributeTypes[i].type_name) == 0) {
				found = true;
				attributeType = kSupportedAttributeTypes[i].type;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "Error: Unsupported attribute type: \"%s\"\n",
				attributeTypeString);
			exit(1);
		}
	}

	// init the attribute filter
	parameters.attribute_filter.SetTo(attributeName, attributeType);

	// turn of move_files, if we are not copying the file data
	parameters.move_files &= parameters.copy_data;

	// do the copying
	fileCount--;
	const char *destination = files[fileCount];
	files[fileCount] = NULL;
	copy_files(files, fileCount, destination, parameters);
	delete[] files;

	return 0;
}
