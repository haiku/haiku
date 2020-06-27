/*
 * Copyright 2001-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <List.h>
#include <Node.h>
#include <Path.h>
#include <String.h>
#include <fs_attr.h>
#include <fs_index.h>
#include <fs_info.h>


extern const char *__progname;
static const char *kProgramName = __progname;

bool gRecursive = false;		// enter directories recursively
bool gVerbose = false;
char *gAttrPattern;
bool gIsPattern = false;
bool gFromVolume = false;	// copy indices from another volume
BList gAttrList;				// list of indices of that volume


class Attribute {
public:
	Attribute(const char *name);
	~Attribute();

	status_t ReadFromFile(BNode *node);
	status_t WriteToFile(BNode *node);
	status_t RemoveFromFile(BNode *node);

	const char	*Name() const { return fName.String(); }
	type_code	Type() const { return fType; }
	size_t		Length() const { return fLength; }

protected:
	BString		fName;
	type_code	fType;
	void		*fBuffer;
	size_t		fLength;
};


Attribute::Attribute(const char *name)
	:
	fName(name),
	fBuffer(NULL),
	fLength(0)
{
}


Attribute::~Attribute()
{
	free(fBuffer);
}


status_t
Attribute::ReadFromFile(BNode *node)
{
	attr_info info;
	status_t status = node->GetAttrInfo(fName.String(), &info);
	if (status != B_OK)
		return status;

	fType = info.type;
	fLength = info.size;

	if ((fBuffer = malloc(fLength)) == NULL)
		return B_NO_MEMORY;

	ssize_t bytesRead = node->ReadAttr(fName.String(), fType, 0, fBuffer,
		fLength);
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead < (ssize_t)fLength)
		return B_IO_ERROR;

	return B_OK;
}


status_t
Attribute::WriteToFile(BNode *node)
{
	ssize_t bytesWritten = node->WriteAttr(fName.String(), fType, 0, fBuffer,
		fLength);
	if (bytesWritten < B_OK)
		return bytesWritten;
	if (bytesWritten < (ssize_t)fLength)
		return B_IO_ERROR;

	return B_OK;
}


status_t
Attribute::RemoveFromFile(BNode *node)
{
	return node->RemoveAttr(fName.String());
}


//	#pragma mark -


bool
nameMatchesPattern(char *name)
{
	if (!gIsPattern)
		return !strcmp(name,gAttrPattern);

	// test the beginning
	int i = 0;
	for(; name[i]; i++) {
		if (gAttrPattern[i] == '*')
			break;
		if (name[i] != gAttrPattern[i])
			return false;
	}

	// test the end
	int j = strlen(name) - 1;
	int k = strlen(gAttrPattern) - 1;
	for(; j >= i && k >= i; j--, k--) {
		if (gAttrPattern[k] == '*')
			return true;
		if (name[j] != gAttrPattern[k])
			return false;
	}
	if (gAttrPattern[k] != '*')
		return false;

	return true;
}


bool
isAttrInList(char *name)
{
	for (int32 index = gAttrList.CountItems();index-- > 0;) {
		const char *attr = (const char *)gAttrList.ItemAt(index);
		if (!strcmp(attr, name))
			return true;
	}
	return false;
}


void
handleFile(BEntry *entry, BNode *node)
{
	// Recurse the directories
	if (gRecursive && entry->IsDirectory()) {
		BDirectory dir(entry);
		BEntry entryIterator;

		dir.Rewind();
		while (dir.GetNextEntry(&entryIterator, false) == B_OK) {
			BNode innerNode;
			handleFile(&entryIterator, &innerNode);
		}

		// also rewrite the attributes of the directory
	}

	char name[B_FILE_NAME_LENGTH];
	entry->GetName(name);

	status_t status = node->SetTo(entry);
	if (status != B_OK) {
		fprintf(stderr, "%s: could not open \"%s\": %s\n", kProgramName, name,
			strerror(status));
		return;
	}

	// rewrite file attributes

	char attrName[B_ATTR_NAME_LENGTH];
	Attribute *attr;
	BList list;

	// building list
	node->RewindAttrs();
	while (node->GetNextAttrName(attrName) == B_OK) {
		if (gFromVolume) {
			if (!isAttrInList(attrName))
				continue;
		} else if (!nameMatchesPattern(attrName))
			continue;

		attr = new(std::nothrow) Attribute(attrName);
		if (attr == NULL) {
			fprintf(stderr, "%s: out of memory.\n", kProgramName);
			exit(1);
		}

		status = attr->ReadFromFile(node);
		if (status != B_OK) {
			fprintf(stderr, "%s: could not read attribute \"%s\" of file "
				"\"%s\": %s\n", kProgramName, attrName, name, strerror(status));
			delete attr;
			continue;
		}
		if (gVerbose) {
			printf("%s: read attribute '%s' (%ld bytes of data)\n", name,
				attrName, attr->Length());
		}
		if (!list.AddItem(attr)) {
			fprintf(stderr, "%s: out of memory.\n", kProgramName);
			exit(1);
		}

		if (!gFromVolume) {
			// creates index to that attribute if necessary
			entry_ref ref;
			if (entry->GetRef(&ref) == B_OK) {
				index_info indexInfo;
				if (fs_stat_index(ref.device, attrName, &indexInfo) != B_OK)
					fs_create_index(ref.device, attrName, attr->Type(), 0);
			}
		}
	}

	// remove attrs
	for (int32 i = list.CountItems(); i-- > 0;) {
		attr = static_cast<Attribute *>(list.ItemAt(i));
		if (attr->RemoveFromFile(node) != B_OK) {
			fprintf(stderr, "%s: could not remove attribute '%s' from file "
				"'%s'.\n", kProgramName, attr->Name(), name);
		}
	}

	// rewrite attrs and empty the list
	while ((attr = static_cast<Attribute *>(list.RemoveItem((int32)0))) != NULL) {
		// write attribute back to file
		status = attr->WriteToFile(node);
		if (status != B_OK) {
			fprintf(stderr, "%s: could not write attribute '%s' to file \"%s\":"
				" %s\n", kProgramName, attr->Name(), name, strerror(status));
		} else if (gVerbose) {
			printf("%s: wrote attribute '%s' (%ld bytes of data)\n", name,
				attr->Name(), attr->Length());
		}

		delete attr;
	}
}


void
copyIndicesFromVolume(const char *path, BEntry &to)
{
	entry_ref ref;
	if (to.GetRef(&ref) != B_OK) {
		fprintf(stderr, "%s: Could not open target volume.\n", kProgramName);
		return;
	}

	dev_t targetDevice = ref.device;
	dev_t sourceDevice = dev_for_path(path);
	if (sourceDevice < B_OK) {
		fprintf(stderr, "%s: Could not open source volume: %s\n", kProgramName,
			strerror(sourceDevice));
		return;
	}

	DIR *indexDirectory = fs_open_index_dir(sourceDevice);
	if (indexDirectory == NULL)
		return;

	while (dirent *index = fs_read_index_dir(indexDirectory)) {
		index_info indexInfo;
		if (fs_stat_index(sourceDevice, index->d_name, &indexInfo) != B_OK) {
			fprintf(stderr, "%s: Could not get information about index "
				"\"%s\": %s\n", kProgramName, index->d_name, strerror(errno));
			continue;
		}

		if (fs_create_index(targetDevice, index->d_name, indexInfo.type, 0)
				== -1 && errno != B_FILE_EXISTS && errno != B_BAD_VALUE) {
			fprintf(stderr, "Could not create index '%s' (type = %" B_PRIu32
				"): %s\n", index->d_name, indexInfo.type, strerror(errno));
		} else
			gAttrList.AddItem(strdup(index->d_name));
	}
	fs_close_index_dir(indexDirectory);
}


void
printUsage(char *cmd)
{
	printf("usage: %s [-rvf] attr <list of filenames and/or directories>\n"
		"  -r\tenter directories recursively\n"
		"  -v\tverbose output\n"
		"  -f\tcreate/update all indices from the source volume,\n\t\"attr\" is "
			"the path to the source volume\n", cmd);
}


int
main(int argc, char **argv)
{
	char *cmd = argv[0];

	if (argc < 3) {
		printUsage(cmd);
		return 1;
	}

	while (*++argv && **argv == '-') {
		for (int i = 1; (*argv)[i]; i++) {
			switch ((*argv)[i]) {
				case 'f':
					gFromVolume = true;
					break;
				case 'r':
					gRecursive = true;
					break;
				case 'v':
					gVerbose = true;
					break;
				default:
					printUsage(cmd);
					return(1);
			}
		}
	}
	gAttrPattern = *argv;
	if (strchr(gAttrPattern,'*'))
		gIsPattern = true;

	while (*++argv) {
		BEntry entry(*argv);
		BNode node;

		if (entry.InitCheck() == B_OK) {
			if (gFromVolume)
				copyIndicesFromVolume(gAttrPattern, entry);
			handleFile(&entry, &node);
		} else
			fprintf(stderr, "%s: could not find \"%s\".\n", kProgramName, *argv);
	}

	return 0;
}
