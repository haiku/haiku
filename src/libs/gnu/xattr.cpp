/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <sys/xattr.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <fs_attr.h>
#include <TypeConstants.h>

#include <syscall_utils.h>


static const char* const kXattrNamespace = "user.haiku.";
static const size_t kXattrNamespaceLength = 11;


namespace {


struct AttributeName {
	char	name[B_ATTR_NAME_LENGTH + 32];
	uint32	type;

	void Init(const char* haikuName, uint32 type)
	{
		if (type == B_XATTR_TYPE) {
			// a simple xattr -- copy the name verbatim
			strlcpy(name, haikuName, sizeof(name));
			this->type = type;
		} else {
			// a Haiku attribute -- map the name

			// create a type string -- if the four bytes of the type are
			// printable, just use them as the type string, otherwise convert
			// to hex
			char typeString[9];
			uint8 typeBytes[4] = { (uint8)((type >> 24) & 0xff),
				(uint8)((type >> 16) & 0xff), (uint8)((type >> 8) & 0xff),
				(uint8)(type & 0xff) };
			if (isprint(typeBytes[0]) && isprint(typeBytes[1])
				&& isprint(typeBytes[2]) && isprint(typeBytes[3])) {
				typeString[0] = typeBytes[0];
				typeString[1] = typeBytes[1];
				typeString[2] = typeBytes[2];
				typeString[3] = typeBytes[3];
				typeString[4] = '\0';
			} else
				sprintf(typeString, "%08" B_PRIx32 , type);

			snprintf(name, sizeof(name), "%s%s#%s", kXattrNamespace,
				haikuName, typeString);
		}
	}

	void Init(const char* xattrName)
	{
		if (strncmp(xattrName, kXattrNamespace, kXattrNamespaceLength) == 0) {
			// a Haiku attribute -- extract the actual name and type
			xattrName += kXattrNamespaceLength;

			if (_DecodeNameAndType(xattrName))
				return;
		}

		// a simple xattr
		strlcpy(name, xattrName, sizeof(name));
		this->type = B_XATTR_TYPE;
	}

private:

	bool _DecodeNameAndType(const char* xattrName)
	{
		const char* typeString = strrchr(xattrName, '#');
		if (typeString == NULL || typeString == xattrName)
			return false;
		typeString++;

		size_t typeStringLength = strlen(typeString);
		if (typeStringLength == 4) {
			// the type string consists of the literal type bytes
			type = ((uint32)typeString[0] << 24) | ((uint32)typeString[1] << 16)
				| ((uint32)typeString[2] << 8) | (uint32)typeString[3];
		} else if (typeStringLength == 8) {
			// must be hex-encoded
			char* numberEnd;
			type = strtoul(typeString, &numberEnd, 16);
			if (numberEnd != typeString + 8)
				return false;
		} else
			return false;

		strlcpy(name, xattrName,
			std::min(sizeof(name), (size_t)(typeString - xattrName)));
			// typeString - xattrName - 1 is the name length, but we need to
			// specify one more for the terminating null
		return true;
	}
};


struct Node {
	Node(const char* path, bool traverseSymlinks)
	{
		fFileFD = open(path, O_RDONLY | (traverseSymlinks ? 0 : O_NOTRAVERSE));
		fOwnsFileFD = true;
	}

	Node(int fileFD)
	{
		fFileFD = fileFD;
		fOwnsFileFD = false;

		if (fileFD < 0)
			errno = B_FILE_ERROR;
	}

	~Node()
	{
		if (fFileFD >= 0 && fOwnsFileFD)
			close(fFileFD);
	}

	int Set(const char* attribute, int flags, const void* buffer, size_t size)
	{
		if (fFileFD < 0)
			return -1;

		// flags to open mode
		int openMode = O_WRONLY | O_TRUNC;
		if (flags == XATTR_CREATE) {
			openMode |= O_CREAT | O_EXCL;
		} else if (flags == XATTR_REPLACE) {
			// pure open -- attribute must exist
		} else
			openMode |= O_CREAT;

		AttributeName attributeName;
		attributeName.Init(attribute);

		int attributeFD = fs_fopen_attr(fFileFD, attributeName.name,
			attributeName.type, openMode);
		if (attributeFD < 0) {
			// translate B_ENTRY_NOT_FOUND to ENOATTR
			if (errno == B_ENTRY_NOT_FOUND)
				errno = ENOATTR;

			return -1;
		}

		ssize_t written = write(attributeFD, buffer, size);

		fs_close_attr(attributeFD);

		if (written < 0)
			return -1;
		if ((size_t)written != size)
			RETURN_AND_SET_ERRNO(B_FILE_ERROR);

		return 0;
	}

	ssize_t Get(const char* attribute, void* buffer, size_t size)
	{
		if (fFileFD < 0)
			return -1;

		AttributeName attributeName;
		attributeName.Init(attribute);

		// get the attribute size -- we read all or nothing
		attr_info info;
		if (fs_stat_attr(fFileFD, attributeName.name, &info) != 0) {
			// translate B_ENTRY_NOT_FOUND to ENOATTR
			if (errno == B_ENTRY_NOT_FOUND)
				errno = ENOATTR;
			return -1;
		}

		// if an empty buffer is given, return the attribute size
		if (size == 0)
			return info.size;

		// if the buffer is too small, fail
		if (size < info.size) {
			errno = ERANGE;
			return -1;
		}

		ssize_t bytesRead = fs_read_attr(fFileFD, attributeName.name,
			info.type, 0, buffer, info.size);

		// translate B_ENTRY_NOT_FOUND to ENOATTR
		if (bytesRead < 0 && errno == B_ENTRY_NOT_FOUND)
			errno = ENOATTR;

		return bytesRead;
	}

	int Remove(const char* attribute)
	{
		if (fFileFD < 0)
			return -1;

		AttributeName attributeName;
		attributeName.Init(attribute);

		int result = fs_remove_attr(fFileFD, attributeName.name);

		// translate B_ENTRY_NOT_FOUND to ENOATTR
		if (result != 0 && errno == B_ENTRY_NOT_FOUND)
			errno = ENOATTR;

		return result;
	}

	ssize_t GetList(void* _buffer, size_t size)
	{
		char* buffer = (char*)_buffer;

		if (fFileFD < 0)
			return -1;

		// open attribute directory
		DIR* dir = fs_fopen_attr_dir(fFileFD);
		if (dir == NULL)
			return -1;

		// read the attributes
		size_t remainingSize = size;
		size_t totalSize = 0;
		while (struct dirent* entry = readdir(dir)) {
			attr_info info;
			if (fs_stat_attr(fFileFD, entry->d_name, &info) != 0)
				continue;

			AttributeName attributeName;
			attributeName.Init(entry->d_name, info.type);

			size_t nameLength = strlen(attributeName.name);
			totalSize += nameLength + 1;

			if (remainingSize > nameLength) {
				strcpy((char*)buffer, attributeName.name);
				buffer += nameLength + 1;
				remainingSize -= nameLength + 1;
			} else
				remainingSize = 0;
		}

		closedir(dir);

		// If the buffer was too small, fail.
		if (size != 0 && totalSize > size) {
			errno = ERANGE;
			return -1;
		}

		return totalSize;
	}

private:
	int		fFileFD;
	bool	fOwnsFileFD;
};


}	// namespace


// #pragma mark -


ssize_t
getxattr(const char* path, const char* attribute, void* buffer, size_t size)
{
	return Node(path, true).Get(attribute, buffer, size);
}


ssize_t
lgetxattr(const char* path, const char* attribute, void* buffer, size_t size)
{
	return Node(path, false).Get(attribute, buffer, size);
}


ssize_t
fgetxattr(int fd, const char* attribute, void* buffer, size_t size)
{
	return Node(fd).Get(attribute, buffer, size);
}


int
setxattr(const char* path, const char* attribute, const void* buffer,
	size_t size, int flags)
{
	return Node(path, true).Set(attribute, flags, buffer, size);
}


int
lsetxattr(const char* path, const char* attribute, const void* buffer,
	size_t size, int flags)
{
	return Node(path, false).Set(attribute, flags, buffer, size);
}


int
fsetxattr(int fd, const char* attribute, const void* buffer, size_t size,
	int flags)
{
	return Node(fd).Set(attribute, flags, buffer, size);
}


int
removexattr (const char* path, const char* attribute)
{
	return Node(path, true).Remove(attribute);
}


int
lremovexattr (const char* path, const char* attribute)
{
	return Node(path, false).Remove(attribute);
}


int
fremovexattr (int fd, const char* attribute)
{
	return Node(fd).Remove(attribute);
}


ssize_t
listxattr(const char* path, char* buffer, size_t size)
{
	return Node(path, true).GetList(buffer, size);
}


ssize_t
llistxattr(const char* path, char* buffer, size_t size)
{
	return Node(path, false).GetList(buffer, size);
}


ssize_t
flistxattr(int fd, char* buffer, size_t size)
{
	return Node(fd).GetList(buffer, size);
}
