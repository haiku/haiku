/*
 * Copyright 2005-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

/*!	Emulation BeOS-style attributes by mapping them to untyped attributes of
	the host platform (xattr on Linux, extattr on FreeBSD).
*/


#ifdef BUILDING_FS_SHELL
#	include "compat.h"
#	define B_OK					0
#	define B_BAD_VALUE			EINVAL
#	define B_FILE_ERROR			EBADF
#	define B_ERROR				EINVAL
#	define B_ENTRY_NOT_FOUND	ENOENT
#	define B_NO_MEMORY			ENOMEM
#else
#	include <BeOSBuildCompatibility.h>
#	include <syscalls.h>

#	include "fs_impl.h"
#	include "fs_descriptors.h"
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <map>
#include <string>

#include <fs_attr.h>


// Include the interface to the host platform attributes support.
#if defined(HAIKU_HOST_PLATFORM_LINUX)
#	include "fs_attr_xattr.h"
#elif defined(HAIKU_HOST_PLATFORM_FREEBSD)
#	include "fs_attr_extattr.h"
#elif defined(HAIKU_HOST_PLATFORM_DARWIN)
#	include "fs_attr_bsdxattr.h"
#else
#	error No attribute support for this host platform!
#endif


namespace BPrivate {}
using namespace BPrivate;
using std::map;
using std::string;

// the maximum length of an attribute listing we support
static const size_t kMaxAttributeListingLength = 10240;

// the maximum attribute length we support
static const size_t kMaxAttributeLength = 10240 * 4;


// mangle_attribute_name
static string
mangle_attribute_name(const char* name)
{
	// prepend our xattr namespace and translate:
	// '/' -> "%\"
	// '%' -> "%%"

	string mangledName = kAttributeNamespace;
	for (int i = 0; name[i] != '\0'; i++) {
		char c = name[i];
		switch (c) {
			case '/':
				mangledName += "%\\";
				break;
			case '%':
				mangledName += "%%";
				break;
			default:
				mangledName += c;
				break;
		}
	}
	return mangledName;
}


// demangle_attribute_name
static bool
demangle_attribute_name(const char* name, string& demangledName)
{
	// chop of our xattr namespace and translate:
	// "%\" -> '/'
	// "%%" -> '%'

	if (strncmp(name, kAttributeNamespace, kAttributeNamespaceLen) != 0)
		return false;

	name += kAttributeNamespaceLen;

	demangledName = "";

	for (int i = 0; name[i] != '\0'; i++) {
		char c = name[i];
		if (c == '%') {
			c = name[++i];
			if (c == '%')
				demangledName += c;
			else if (c == '\\')
				demangledName += '/';
			else
				return false;
		} else
			demangledName += c;
	}

	return true;
}


namespace {

class AttributeDirectory;

typedef map<DIR*, AttributeDirectory*> AttrDirMap;
static AttrDirMap sAttributeDirectories;

// LongDirent
struct LongDirent : dirent {
	char name[B_FILE_NAME_LENGTH];
};

// AttributeHeader
struct AttributeHeader {
	uint32	type;
};

// AttributeDirectory
class AttributeDirectory {
public:
	AttributeDirectory()
		: fFileFD(-1),
		  fFakeDir(NULL),
		  fListing(NULL),
		  fListingLength(-1),
		  fListingIndex(0)
	{
	}

	~AttributeDirectory()
	{
		if (fFileFD >= 0)
			close(fFileFD);

		if (fFakeDir) {
			AttrDirMap::iterator it = sAttributeDirectories.find(fFakeDir);
			if (it != sAttributeDirectories.end())
				sAttributeDirectories.erase(it);

			closedir(fFakeDir);
		}

		free(fListing);
	}

	static AttributeDirectory* Get(DIR* dir)
	{
		AttrDirMap::iterator it = sAttributeDirectories.find(dir);
		if (it == sAttributeDirectories.end())
			return NULL;
		return it->second;
	}

	status_t Init(const char* path, int fileFD)
	{
		// open a fake DIR
		if (!fFakeDir) {
			fFakeDir = opendir(".");
			if (!fFakeDir)
				return B_ERROR;

			sAttributeDirectories[fFakeDir] = this;
		}

		string tempPath;
		if (!path) {
			// We've got no path. If the file descriptor is one of our own and
			// not a system FD, we need to get a path for it.
			Descriptor*	descriptor = get_descriptor(fileFD);
			if (descriptor && !descriptor->IsSystemFD()) {
				status_t error = descriptor->GetPath(tempPath);
				if (error != B_OK)
					return error;
				path = tempPath.c_str();
				fileFD = -1;
			}
		}

		if (path) {
			// A path was given -- check, if it's a symlink. If so we need to
			// keep the path, otherwise we open a FD.
			struct stat st;
			if (lstat(path, &st))
				return B_ENTRY_NOT_FOUND;

			if (S_ISLNK(st.st_mode)) {
				fFileFD = -1;
				fPath = path;
			} else {
				fFileFD = open(path, O_RDONLY);
				if (fFileFD < 0)
					return errno;
				fPath = "";
			}
		} else {
			// FD was given -- dup it.
			fFileFD = dup(fileFD);
			if (fFileFD < 0)
				return errno;
			fPath = "";
		}

		fListingLength = -1;
		fListingIndex = 0;

		return B_OK;
	}

	DIR* FakeDir() const	{ return fFakeDir; }

	status_t ReadDir(struct dirent** _entry)
	{
		// make sure we have a current listing
		status_t error = _CheckListing();
		if (error != B_OK)
			return error;

		// keep going until we find an entry or hit the end of dir
		while (fListingIndex < fListingLength) {
			// get next entry name
			const char* name = fListing + fListingIndex;
			int nameLen = strlen(name);
			fListingIndex += nameLen + 1;

			// check the attribute namespace
			string demangledName;
			if (!demangle_attribute_name(name, demangledName))
				continue;
			name = demangledName.c_str();
			nameLen = demangledName.length();

			if (nameLen == 0) {
				// Uh, weird attribute.
				return B_ERROR;
			}

			// prepare the dirent
			strcpy(fDirent.d_name, name);
			fDirent.d_ino = 0;
// TODO: We need the node ID!

			*_entry = &fDirent;
			return B_OK;
		}

		// end of dir
		*_entry = NULL;
		return B_OK;
	}

	void RewindDir()
	{
		fListingIndex = 0;
	}

private:
	status_t _CheckListing()
	{
		if (fListing && fListingLength >= 0)
			return B_OK;

		char listing[kMaxAttributeListingLength];

		// get the listing
		ssize_t length = list_attributes(fFileFD, fPath.c_str(), listing,
			kMaxAttributeListingLength);
		if (length < 0)
			return errno;

		// clone the on-stack listing
		char* newListing = (char*)realloc(fListing, length);
		if (!newListing)
			return B_NO_MEMORY;
		memcpy(newListing, listing, length);

		fListing = newListing;
		fListingLength = length;
		fListingIndex = 0;

		return B_OK;
	}

private:
	int			fFileFD;
	string		fPath;
	DIR*		fFakeDir;
	LongDirent	fDirent;
	char*		fListing;
	int			fListingLength;
	int			fListingIndex;
};

// LocalFD
class LocalFD {
public:
	LocalFD()
	{
	}

	~LocalFD()
	{
	}

	status_t Init(int fd)
	{
#ifndef BUILDING_FS_SHELL
		Descriptor* descriptor = get_descriptor(fd);
		if (descriptor && !descriptor->IsSystemFD()) {
			// we need to get a path
			fFD = -1;
			return descriptor->GetPath(fPath);
		}
#endif

		fFD = fd;
		fPath = "";
		return B_OK;
	}

	int FD() const
	{
		return fFD;
	}

	const char* Path() const
	{
		return (fFD < 0 ? fPath.c_str() : NULL);
	}

	bool IsSymlink() const
	{
		struct stat st;
		int result;
		if (Path())
			result = lstat(Path(), &st);
		else
			result = fstat(fFD, &st);

		return (result == 0 && S_ISLNK(st.st_mode));
	}

private:
	string	fPath;
	int		fFD;
};

}	// unnamed namspace


// # pragma mark - Public API


// fs_open_attr_dir
DIR *
fs_open_attr_dir(const char *path)
{
	AttributeDirectory* attrDir = new AttributeDirectory;

	status_t error = attrDir->Init(path, -1);
	if (error != B_OK) {
		errno = error;
		delete attrDir;
		return NULL;
	}

	return attrDir->FakeDir();
}

// fs_fopen_attr_dir
DIR *
fs_fopen_attr_dir(int fd)
{
	AttributeDirectory* attrDir = new AttributeDirectory;

	status_t error = attrDir->Init(NULL, fd);
	if (error != B_OK) {
		errno = error;
		delete attrDir;
		return NULL;
	}

	return attrDir->FakeDir();
}

// fs_close_attr_dir
int
fs_close_attr_dir(DIR *dir)
{
	AttributeDirectory* attrDir = AttributeDirectory::Get(dir);
	if (!attrDir) {
		errno = B_BAD_VALUE;
		return -1;
	}

	delete attrDir;
	return 0;
}

// fs_read_attr_dir
struct dirent *
fs_read_attr_dir(DIR *dir)
{
	// get attr dir
	AttributeDirectory* attrDir = AttributeDirectory::Get(dir);
	if (!attrDir) {
		errno = B_BAD_VALUE;
		return NULL;
	}

	// read attr dir
	dirent* entry = NULL;
	status_t error = attrDir->ReadDir(&entry);
	if (error != B_OK) {
		errno = error;
		return NULL;
	}

	return entry;
}

// fs_rewind_attr_dir
void
fs_rewind_attr_dir(DIR *dir)
{
	// get attr dir
	AttributeDirectory* attrDir = AttributeDirectory::Get(dir);
	if (attrDir)
		attrDir->RewindDir();
}

// fs_fopen_attr
int
fs_fopen_attr(int fd, const char *attribute, uint32 type, int openMode)
{
	if (fd < 0) {
		errno = B_BAD_VALUE;
		return -1;
	}

	AttributeDescriptor* descriptor = new(std::nothrow) AttributeDescriptor(fd,
		attribute, type, openMode);
	if (descriptor == NULL) {
		errno = B_NO_MEMORY;
		return -1;
	}

	status_t error = descriptor->Init();
	if (error != B_OK) {
		delete descriptor;
		errno = error;
		return -1;
	}

	int attributeFD = add_descriptor(descriptor);
	if (attributeFD < 0) {
		delete descriptor;
		errno = B_NO_MEMORY;
		return -1;
	}

	return attributeFD;
}

// fs_close_attr
int
fs_close_attr(int fd)
{
	status_t error = delete_descriptor(fd);
	if (error != 0) {
		errno = error;
		return -1;
	}

	return 0;
}

// fs_read_attr
ssize_t
fs_read_attr(int fd, const char *_attribute, uint32 type, off_t pos,
	void *buffer, size_t readBytes)
{
	// check params
	if (pos < 0 || pos + readBytes > kMaxAttributeLength
		|| !_attribute || !buffer) {
		errno = B_BAD_VALUE;
		return -1;
	}

	// resolve FD
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// mangle the attribute name
	string attribute = mangle_attribute_name(_attribute);

	// read the attribute
	char attributeBuffer[sizeof(AttributeHeader) + kMaxAttributeLength];
	ssize_t bytesRead = min_c((size_t)kMaxAttributeLength, readBytes)
		+ sizeof(AttributeHeader);
	if (localFD.Path()) {
		bytesRead = get_attribute(-1, localFD.Path(), attribute.c_str(),
			attributeBuffer, bytesRead);
	} else {
		bytesRead = get_attribute(localFD.FD(), NULL, attribute.c_str(),
			attributeBuffer, bytesRead);
	}
	if (bytesRead < 0) {
		// Make sure, the error code is B_ENTRY_NOT_FOUND, if the attribute
		// doesn't exist.
		if (errno == ENOATTR || errno == ENODATA)
			errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	// check length for sanity
	if ((size_t)bytesRead < sizeof(AttributeHeader)) {
		fprintf(stderr, "fs_read_attr(): attribute \"%s\" is shorter than the "
			"AttributeHeader!\n", attribute.c_str());
		errno = B_ERROR;
		return -1;
	}

	// copy the result into the provided buffer
	bytesRead -= sizeof(AttributeHeader);
	if (bytesRead > pos) {
		memcpy(buffer, attributeBuffer + sizeof(AttributeHeader) + pos,
			bytesRead - pos);
	} else
		bytesRead = 0;

	return bytesRead;
}

// fs_write_attr
ssize_t
fs_write_attr(int fd, const char *_attribute, uint32 type, off_t pos,
	const void *buffer, size_t writeBytes)
{
	// check params
	if (pos < 0 || pos + writeBytes > kMaxAttributeLength
		|| !_attribute || !buffer) {
		errno = B_BAD_VALUE;
		return -1;
	}

	// resolve FD
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// mangle the attribute name
	string attribute = mangle_attribute_name(_attribute);

	// prepare an attribute buffer
	char attributeBuffer[sizeof(AttributeHeader) + kMaxAttributeLength];
	AttributeHeader* header = (AttributeHeader*)attributeBuffer;
	header->type = type;
	memset(attributeBuffer + sizeof(AttributeHeader), 0, pos);
	memcpy(attributeBuffer + sizeof(AttributeHeader) + pos, buffer, writeBytes);

	// write the attribute
	ssize_t toWrite = sizeof(AttributeHeader) + pos + writeBytes;
	int result;
	if (localFD.Path()) {
		result = set_attribute(-1, localFD.Path(), attribute.c_str(),
			attributeBuffer, toWrite);
	} else {
		result = set_attribute(localFD.FD(), NULL, attribute.c_str(),
			attributeBuffer, toWrite);
	}
	if (result < 0) {
		// Setting user attributes on symlinks is not allowed (xattr). So, if
		// this is a symlink and we're only supposed to write a "BEOS:TYPE"
		// attribute we silently pretend to have succeeded.
		if (localFD.IsSymlink() && strcmp(_attribute, "BEOS:TYPE") == 0)
			return writeBytes;
		return -1;
	}

	return writeBytes;
}

// fs_remove_attr
int
fs_remove_attr(int fd, const char *_attribute)
{
	// check params
	if (!_attribute) {
		errno = B_BAD_VALUE;
		return -1;
	}

	// resolve FD
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// mangle the attribute name
	string attribute = mangle_attribute_name(_attribute);

	// remove attribute
	int result;
	if (localFD.Path())
		result = remove_attribute(-1, localFD.Path(), attribute.c_str());
	else
		result = remove_attribute(localFD.FD(), NULL, attribute.c_str());

	if (result < 0) {
		// Make sure, the error code is B_ENTRY_NOT_FOUND, if the attribute
		// doesn't exist.
		if (errno == ENOATTR || errno == ENODATA)
			errno = B_ENTRY_NOT_FOUND;
		return -1;
	}
	return 0;
}

// fs_stat_attr
int
fs_stat_attr(int fd, const char *_attribute, struct attr_info *attrInfo)
{
	if (!_attribute || !attrInfo) {
		errno = B_BAD_VALUE;
		return -1;
	}

	// resolve FD
	LocalFD localFD;
	status_t error = localFD.Init(fd);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	// mangle the attribute name
	string attribute = mangle_attribute_name(_attribute);

	// read the attribute
	char attributeBuffer[sizeof(AttributeHeader) + kMaxAttributeLength];
	ssize_t bytesRead = sizeof(AttributeHeader) + kMaxAttributeLength;
	if (localFD.Path()) {
		bytesRead = get_attribute(-1, localFD.Path(), attribute.c_str(),
			attributeBuffer, bytesRead);
	} else {
		bytesRead = get_attribute(localFD.FD(), NULL, attribute.c_str(),
			attributeBuffer, bytesRead);
	}
	if (bytesRead < 0) {
		// Make sure, the error code is B_ENTRY_NOT_FOUND, if the attribute
		// doesn't exist.
		if (errno == ENOATTR || errno == ENODATA)
			errno = B_ENTRY_NOT_FOUND;
		return -1;
	}

	// check length for sanity
	if ((size_t)bytesRead < sizeof(AttributeHeader)) {
		fprintf(stderr, "fs_stat_attr(): attribute \"%s\" is shorter than the "
			"AttributeHeader!\n", attribute.c_str());
		errno = B_ERROR;
		return -1;
	}

	attrInfo->size = bytesRead - sizeof(AttributeHeader);
	attrInfo->type = ((AttributeHeader*)attributeBuffer)->type;

	return 0;
}


// #pragma mark - Private Syscalls


#ifndef BUILDING_FS_SHELL

// _kern_open_attr_dir
int
_kern_open_attr_dir(int fd, const char *path)
{
	// get node ref for the node
	struct stat st;
	status_t error = _kern_read_stat(fd, path, false, &st,
		sizeof(struct stat));
	if (error != B_OK) {
		errno = error;
		return -1;
	}
	NodeRef ref(st);

	DIR* dir;
	if (path) {
		// If a path was given, get a usable path.
		string realPath;
		status_t error = get_path(fd, path, realPath);
		if (error != B_OK)
			return error;

		dir = fs_open_attr_dir(realPath.c_str());
	} else
		dir = fs_fopen_attr_dir(fd);

	if (!dir)
		return errno;

	// create descriptor
	AttrDirDescriptor *descriptor = new AttrDirDescriptor(dir, ref);
	return add_descriptor(descriptor);
}

// _kern_rename_attr
status_t
_kern_rename_attr(int fromFile, const char *fromName, int toFile,
	const char *toName)
{
	// not supported ATM
	return B_BAD_VALUE;
}

// _kern_remove_attr
status_t
_kern_remove_attr(int fd, const char *name)
{
	if (!name)
		return B_BAD_VALUE;

	if (fs_remove_attr(fd, name) < 0)
		return errno;
	return B_OK;
}

#endif	// ! BUILDING_FS_SHELL
