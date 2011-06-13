/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <ctype.h>
#include <errno.h>
#include <string.h>

#include <new>

#include <KernelExport.h>

#include <AutoDeleter.h>
#include <ddm_modules.h>
#include <disk_device_types.h>

#include <vmdk.h>

//#define TRACE_VMDK 1
#ifdef _BOOT_MODE
#	include <boot/partitions.h>
#	include <util/kernel_cpp.h>
#	undef TRACE_VMDK
#else
#	include <DiskDeviceTypes.h>
#endif

#if TRACE_VMDK
#	define TRACE(x...) dprintf("vmdk: " x)
#else
#	define TRACE(x...) do { } while (false)
#endif


// module name
#define VMDK_PARTITION_MODULE_NAME "partitioning_systems/vmdk/v1"


// #pragma mark - VMDK header/descriptor parsing


static const off_t kMaxDescriptorSize = 64 * 1024;


struct VmdkCookie {
	VmdkCookie(off_t contentOffset, off_t contentSize)
		:
		contentOffset(contentOffset),
		contentSize(contentSize)
	{
	}

	off_t	contentOffset;
	off_t	contentSize;
};


enum {
	TOKEN_END,
	TOKEN_STRING,
	TOKEN_ASSIGN
};

struct Token {
	int			type;
	size_t		length;
	char		string[1024];

	void SetToEnd()
	{
		type = TOKEN_END;
		string[0] = '\0';
		length = 0;
	}

	void SetToAssign()
	{
		type = TOKEN_ASSIGN;
		string[0] = '=';
		string[1] = '\0';
		length = 0;
	}

	void SetToString()
	{
		type = TOKEN_STRING;
		string[0] = '\0';
		length = 0;
	}

	void PushChar(char c)
	{
		if (length + 1 < sizeof(string)) {
			string[length++] = c;
			string[length] = '\0';
		}
	}

	bool operator==(const char* other) const
	{
		return strcmp(string, other) == 0;
	}

	bool operator!=(const char* other) const
	{
		return !(*this == other);
	}
};


static status_t
read_file(int fd, off_t offset, void* buffer, size_t size)
{
	ssize_t bytesRead = pread(fd, buffer, size, offset);
	if (bytesRead < 0)
		return errno;

	return (size_t)bytesRead == size ? B_OK : B_ERROR;
}


static int
next_token(char*& line, const char* lineEnd, Token& token)
{
	// skip whitespace
	while (line != lineEnd && isspace(*line))
		line++;

	// comment/end of line
	if (line == lineEnd || *line == '#') {
		token.SetToEnd();
		return token.type;
	}

	switch (*line) {
		case '=':
		{
			line++;
			token.SetToAssign();
			return token.type;
		}

		case '"':
		{
			// quoted string
			token.SetToString();
			line++;
			while (line != lineEnd) {
				if (*line == '"') {
					// end of string
					line++;
					break;
				}

				if (*line == '\\') {
					// escaped char
					line++;
					if (line == lineEnd)
						break;
				}

				token.PushChar(*(line++));
			}

			return token.type;
		}

		default:
		{
			// unquoted string
			token.SetToString();
			while (line != lineEnd && *line != '#' && *line != '='
				&& !isspace(*line)) {
				token.PushChar(*(line++));
			}
			return token.type;
		}
	}
}


static status_t
parse_vmdk_header(int fd, off_t fileSize, VmdkCookie*& _cookie)
{
	// read the header
	SparseExtentHeader header;
	status_t error = read_file(fd, 0, &header, sizeof(header));
	if (error != B_OK)
		return error;

	// check the header
	if (header.magicNumber != VMDK_SPARSE_MAGICNUMBER) {
		TRACE("Error: Header magic mismatch!\n");
		return B_BAD_DATA;
	}

	if (header.version != VMDK_SPARSE_VERSION) {
		TRACE("Error: Header version mismatch!\n");
		return B_BAD_DATA;
	}

	if (header.overHead > (uint64_t)fileSize / 512) {
		TRACE("Error: Header overHead invalid!\n");
		return B_BAD_DATA;
	}
	off_t headerSize = header.overHead * 512;

	if (header.descriptorOffset < (sizeof(header) + 511) / 512
		|| header.descriptorOffset >= header.overHead
		|| header.descriptorSize == 0
		|| header.overHead - header.descriptorOffset < header.descriptorSize) {
		TRACE("Error: Invalid descriptor location!\n");
		return B_BAD_DATA;
	}
	off_t descriptorOffset = header.descriptorOffset * 512;
	off_t descriptorSize = header.descriptorSize * 512;

	if (descriptorSize > kMaxDescriptorSize) {
		TRACE("Error: Unsupported descriptor size!\n");
		return B_UNSUPPORTED;
	}

	// read descriptor
	char* descriptor = (char*)malloc(descriptorSize + 1);
	if (descriptor == NULL) {
		TRACE("Error: Descriptor allocation failed!\n");
		return B_NO_MEMORY;
	}
	MemoryDeleter descriptorDeleter(descriptor);

	error = read_file(fd, descriptorOffset, descriptor, descriptorSize);
	if (error != B_OK)
		return error;

	// determine the actual descriptor size
	descriptor[descriptorSize] = '\0';
	descriptorSize = strlen(descriptor);

	// parse descriptor
	uint64_t extendOffset = 0;
	uint64_t extendSize = 0;

	char* line = descriptor;
	char* descriptorEnd = line + descriptorSize;
	while (line < descriptorEnd) {
		// determine the end of the line
		char* lineEnd = strchr(line, '\n');
		if (lineEnd != NULL)
			*lineEnd = '\0';
		else
			lineEnd = descriptorEnd;

		Token token;
		if (next_token(line, lineEnd, token) == TOKEN_END) {
			line = lineEnd + 1;
			continue;
		}

		Token token2;
		switch (next_token(line, lineEnd, token2)) {
			case TOKEN_END:
				break;

			case TOKEN_ASSIGN:
				if (next_token(line, lineEnd, token2) != TOKEN_STRING) {
					TRACE("Line not understood: %s = ?\n", token.string);
					break;
				}

				if (token == "version") {
					if (token2 != "1") {
						TRACE("Unsupported descriptor version: %s\n",
							token2.string);
						return B_UNSUPPORTED;
					}
				} else if (token == "createType") {
					if (token2 != "monolithicFlat") {
						TRACE("Unsupported descriptor createType: %s\n",
							token2.string);
						return B_UNSUPPORTED;
					}
				}

				break;

			case TOKEN_STRING:
				if (token != "RW")
					break;

				extendSize = strtoll(token2.string, NULL, 0);
				if (extendSize == 0) {
					TRACE("Bad extend size.\n");
					return B_BAD_DATA;
				}

				if (next_token(line, lineEnd, token) != TOKEN_STRING
					|| token != "FLAT"
					|| next_token(line, lineEnd, token) != TOKEN_STRING
						// image name
					|| next_token(line, lineEnd, token2) != TOKEN_STRING) {
					TRACE("Invalid/unsupported extend line\n");
					break;
				}

				extendOffset = strtoll(token2.string, NULL, 0);
				if (extendOffset == 0) {
					TRACE("Bad extend offset.\n");
					return B_BAD_DATA;
				}

				break;
		}

		line = lineEnd + 1;
	}

	if (extendOffset < (uint64_t)headerSize / 512
		|| extendOffset >= (uint64_t)fileSize / 512
		|| extendSize == 0
		|| (uint64_t)fileSize / 512 - extendOffset < extendSize) {
		TRACE("Error: Invalid extend location!\n");
		return B_BAD_DATA;
	}

	TRACE("descriptor len: %lld\n", descriptorSize);
	TRACE("header size:    %lld\n", headerSize);
	TRACE("file size:      %lld\n", fileSize);
	TRACE("extend offset:  %lld\n", extendOffset * 512);
	TRACE("extend size:    %lld\n", extendSize * 512);

	VmdkCookie* cookie = new(std::nothrow) VmdkCookie(extendOffset * 512,
		extendSize * 512);
	if (cookie == NULL)
		return B_NO_MEMORY;

	_cookie = cookie;
	return B_OK;
}


// #pragma mark - module hooks


static status_t
vmdk_std_ops(int32 op, ...)
{
	TRACE("vmdk_std_ops(0x%lx)\n", op);
	switch(op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}
	return B_ERROR;
}


static float
vmdk_identify_partition(int fd, partition_data* partition, void** _cookie)
{
	TRACE("vmdk_identify_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		partition->id, partition->offset, partition->size,
		partition->block_size);

	VmdkCookie* cookie;
	status_t error = parse_vmdk_header(fd, partition->size, cookie);
	if (error != B_OK)
		return -1;

	*_cookie = cookie;
	return 0.8f;
}


static status_t
vmdk_scan_partition(int fd, partition_data* partition, void* _cookie)
{
	TRACE("vmdk_scan_partition(%d, %ld: %lld, %lld, %ld)\n", fd,
		partition->id, partition->offset, partition->size,
		partition->block_size);

	VmdkCookie* cookie = (VmdkCookie*)_cookie;
	ObjectDeleter<VmdkCookie> cookieDeleter(cookie);

	// fill in the partition_data structure
	partition->status = B_PARTITION_VALID;
	partition->flags |= B_PARTITION_PARTITIONING_SYSTEM;
	partition->content_size = partition->size;
	// (no content_name and content_parameters)
	// (content_type is set by the system)
	partition->content_cookie = cookie;

	// child
	partition_data* child = create_child_partition(partition->id, 0,
		partition->offset + cookie->contentOffset, cookie->contentSize, -1);
	if (child == NULL) {
		partition->content_cookie = NULL;
		return B_ERROR;
	}

	child->block_size = partition->block_size;
	// (no name)
	child->type = strdup(kPartitionTypeUnrecognized);
	child->parameters = NULL;
	child->cookie = NULL;

	// check for allocation problems
	if (child->type == NULL) {
		partition->content_cookie = NULL;
		return B_NO_MEMORY;
	}

	cookieDeleter.Detach();
	return B_OK;
}


static void
vmdk_free_identify_partition_cookie(partition_data*/* partition*/, void* cookie)
{
	delete (VmdkCookie*)cookie;
}


static void
vmdk_free_partition_cookie(partition_data* partition)
{
	// called for the child partition -- it doesn't have a cookie
}


static void
vmdk_free_partition_content_cookie(partition_data* partition)
{
	delete (VmdkCookie*)partition->content_cookie;
}


#ifdef _BOOT_MODE
partition_module_info gVMwarePartitionModule =
#else
static partition_module_info vmdk_partition_module =
#endif
{
	{
		VMDK_PARTITION_MODULE_NAME,
		0,
		vmdk_std_ops
	},
	"vmdk",								// short_name
	VMDK_PARTITION_NAME,				// pretty_name

	// flags
	0,

	// scanning
	vmdk_identify_partition,				// identify_partition
	vmdk_scan_partition,					// scan_partition
	vmdk_free_identify_partition_cookie,	// free_identify_partition_cookie
	vmdk_free_partition_cookie,				// free_partition_cookie
	vmdk_free_partition_content_cookie,		// free_partition_content_cookie

#ifdef _BOOT_MODE
	NULL
#endif	// _BOOT_MODE
};


#ifndef _BOOT_MODE
extern "C" partition_module_info* modules[];
_EXPORT partition_module_info* modules[] =
{
	&vmdk_partition_module,
	NULL
};
#endif
