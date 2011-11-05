/*
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 *
 * Copyright 2001, pinc Software.  All Rights Reserved.
 */


#include "iso9660.h"

#include <ctype.h>

#ifndef FS_SHELL
#	include <dirent.h>
#	include <fcntl.h>
#	include <stdlib.h>
#	include <string.h>
#	include <sys/stat.h>
#	include <time.h>
#	include <unistd.h>

#	include <ByteOrder.h>
#	include <Drivers.h>
#	include <KernelExport.h>
#	include <fs_cache.h>
#endif

#include "rock_ridge.h"

//#define TRACE_ISO9660 1
#if TRACE_ISO9660
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// Just needed here
static status_t unicode_to_utf8(const char	*src, int32	*srcLen, char *dst,
	int32 *dstLen);


// ISO9660 should start with this string
const char *kISO9660IDString = "CD001";


static int
get_device_block_size(int fd)
{
	device_geometry geometry;

	if (ioctl(fd, B_GET_GEOMETRY, &geometry) < 0)  {
		struct stat st;
		if (fstat(fd, &st) < 0 || S_ISDIR(st.st_mode))
			return 0;

		return 512;   /* just assume it's a plain old file or something */
	}

	return geometry.bytes_per_sector;
}


// From EncodingComversions.cpp

// Pierre's (modified) Uber Macro

// NOTE: iso9660 seems to store the unicode text in big-endian form
#define u_to_utf8(str, uni_str)\
{\
	if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xff80) == 0)\
		*str++ = B_BENDIAN_TO_HOST_INT16(*uni_str++);\
	else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xf800) == 0) {\
		str[0] = 0xc0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6);\
		str[1] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 2;\
	} else if ((B_BENDIAN_TO_HOST_INT16(uni_str[0])&0xfc00) != 0xd800) {\
		str[0] = 0xe0|(B_BENDIAN_TO_HOST_INT16(uni_str[0])>>12);\
		str[1] = 0x80|((B_BENDIAN_TO_HOST_INT16(uni_str[0])>>6)&0x3f);\
		str[2] = 0x80|(B_BENDIAN_TO_HOST_INT16(*uni_str++)&0x3f);\
		str += 3;\
	} else {\
		int   val;\
		val = ((B_BENDIAN_TO_HOST_INT16(uni_str[0])-0xd7c0)<<10) | (B_BENDIAN_TO_HOST_INT16(uni_str[1])&0x3ff);\
		str[0] = 0xf0 | (val>>18);\
		str[1] = 0x80 | ((val>>12)&0x3f);\
		str[2] = 0x80 | ((val>>6)&0x3f);\
		str[3] = 0x80 | (val&0x3f);\
		uni_str += 2; str += 4;\
	}\
}


static status_t
unicode_to_utf8(const char	*src, int32	*srcLen, char *dst, int32 *dstLen)
{
	int32 srcLimit = *srcLen;
	int32 dstLimit = *dstLen;
	int32 srcCount = 0;
	int32 dstCount = 0;

	for (srcCount = 0; srcCount < srcLimit; srcCount += 2) {
		uint16 *UNICODE = (uint16 *)&src[srcCount];
		uint8 utf8[4];
		uint8 *UTF8 = utf8;
		int32 utf8Len;
		int32 j;

		u_to_utf8(UTF8, UNICODE);

		utf8Len = UTF8 - utf8;
		if (dstCount + utf8Len > dstLimit)
			break;

		for (j = 0; j < utf8Len; j++)
			dst[dstCount + j] = utf8[j];
		dstCount += utf8Len;
	}

	*srcLen = srcCount;
	*dstLen = dstCount;

	return dstCount > 0 ? B_NO_ERROR : B_ERROR;
}


static void
sanitize_iso_name(iso9660_inode* node, bool removeTrailingPoints)
{
	// Get rid of semicolons, which are used to delineate file versions.
	if (char* semi = strchr(node->name, ';')) {
		semi[0] = '\0';
		node->name_length = semi - node->name;
	}

	if (removeTrailingPoints) {
		// Get rid of trailing points
		if (node->name_length > 2 && node->name[node->name_length - 1] == '.')
			node->name[--node->name_length] = '\0';
	}
}


static int
init_volume_date(ISOVolDate *date, char *buffer)
{
	memcpy(date, buffer, 17);
	return 0;
}


static int
init_node_date(ISORecDate *date, char *buffer)
{
	memcpy(date, buffer, 7);
	return 0;
}


static status_t
InitVolDesc(iso9660_volume *volume, char *buffer)
{
	TRACE(("InitVolDesc - ENTER\n"));

	volume->volDescType = *(uint8 *)buffer++;

	volume->stdIDString[5] = '\0';
	strncpy(volume->stdIDString, buffer, 5);
	buffer += 5;

	volume->volDescVersion = *(uint8 *)buffer;
	buffer += 2; // 8th byte unused

	volume->systemIDString[32] = '\0';
	strncpy(volume->systemIDString, buffer, 32);
	buffer += 32;
	TRACE(("InitVolDesc - system id string is %s\n", volume->systemIDString));

	volume->volIDString[32] = '\0';
	strncpy(volume->volIDString, buffer, 32);
	buffer += (32 + 80-73 + 1);	// bytes 80-73 unused
	TRACE(("InitVolDesc - volume id string is %s\n", volume->volIDString));

	volume->volSpaceSize[LSB_DATA] = *(uint32 *)buffer;
	buffer += 4;
	volume->volSpaceSize[MSB_DATA] = *(uint32 *)buffer;
	buffer+= (4 + 120-89 + 1); 		// bytes 120-89 unused

	volume->volSetSize[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->volSetSize[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	volume->volSeqNum[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->volSeqNum[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	volume->logicalBlkSize[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->logicalBlkSize[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	volume->pathTblSize[LSB_DATA] = *(uint32*)buffer;
	buffer += 4;
	volume->pathTblSize[MSB_DATA] = *(uint32*)buffer;
	buffer += 4;

	volume->lPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->lPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	volume->optLPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->optLPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	volume->mPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->mPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	volume->optMPathTblLoc[LSB_DATA] = *(uint16*)buffer;
	buffer += 2;
	volume->optMPathTblLoc[MSB_DATA] = *(uint16*)buffer;
	buffer += 2;

	// Fill in directory record.
	volume->joliet_level = 0;
	InitNode(volume, &volume->rootDirRec, buffer, NULL);

	volume->rootDirRec.id = ISO_ROOTNODE_ID;
	buffer += 34;

	volume->volSetIDString[128] = '\0';
	strncpy(volume->volSetIDString, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume set id string is %s\n", volume->volSetIDString));

	volume->pubIDString[128] = '\0';
	strncpy(volume->pubIDString, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume pub id string is %s\n", volume->pubIDString));

	volume->dataPreparer[128] = '\0';
	strncpy(volume->dataPreparer, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume dataPreparer string is %s\n", volume->dataPreparer));

	volume->appIDString[128] = '\0';
	strncpy(volume->appIDString, buffer, 128);
	buffer += 128;
	TRACE(("InitVolDesc - volume app id string is %s\n", volume->appIDString));

	volume->copyright[38] = '\0';
	strncpy(volume->copyright, buffer, 38);
	buffer += 38;
	TRACE(("InitVolDesc - copyright is %s\n", volume->copyright));

	volume->abstractFName[38] = '\0';
	strncpy(volume->abstractFName, buffer, 38);
	buffer += 38;

	volume->biblioFName[38] = '\0';
	strncpy(volume->biblioFName, buffer, 38);
	buffer += 38;

	init_volume_date(&volume->createDate, buffer);
	buffer += 17;

	init_volume_date(&volume->modDate, buffer);
	buffer += 17;

	init_volume_date(&volume->expireDate, buffer);
	buffer += 17;

	init_volume_date(&volume->effectiveDate, buffer);
	buffer += 17;

	volume->fileStructVers = *(uint8*)buffer;
	return B_OK;
}


static status_t
parse_rock_ridge(iso9660_volume* volume, iso9660_inode* node, char* buffer,
	char* end, bool relocated)
{
	// Now we're at the start of the rock ridge stuff
	char* altName = NULL;
	char* slName = NULL;
	uint16 altNameSize = 0;
	uint16 slNameSize = 0;
	uint8 slFlags = 0;
	uint8 length = 0;
	bool done = false;

	TRACE(("RR: Start of extensions at %p\n", buffer));

	while (!done) {
		buffer += length;
		if (buffer + 2 >= end)
			break;
		length = *(uint8*)(buffer + 2);
		if (buffer + length > end)
			break;
		if (length == 0)
			break;

		switch (((int)buffer[0] << 8) + buffer[1]) {
			// Stat structure stuff
			case 'PX':
			{
				uint8 bytePos = 3;
				TRACE(("RR: found PX, length %u\n", length));
				node->attr.pxVer = *(uint8*)(buffer + bytePos++);

				// st_mode
				node->attr.stat[LSB_DATA].st_mode
					= *(mode_t*)(buffer + bytePos);
				bytePos += 4;
				node->attr.stat[MSB_DATA].st_mode
					= *(mode_t*)(buffer + bytePos);
				bytePos += 4;

				// st_nlink
				node->attr.stat[LSB_DATA].st_nlink
					= *(nlink_t*)(buffer+bytePos);
				bytePos += 4;
				node->attr.stat[MSB_DATA].st_nlink
					= *(nlink_t*)(buffer + bytePos);
				bytePos += 4;

				// st_uid
				node->attr.stat[LSB_DATA].st_uid
					= *(uid_t*)(buffer + bytePos);
				bytePos += 4;
				node->attr.stat[MSB_DATA].st_uid
					= *(uid_t*)(buffer + bytePos);
				bytePos += 4;

				// st_gid
				node->attr.stat[LSB_DATA].st_gid
					= *(gid_t*)(buffer + bytePos);
				bytePos += 4;
				node->attr.stat[MSB_DATA].st_gid
					= *(gid_t*)(buffer + bytePos);
				bytePos += 4;
				break;
			}

			case 'PN':
				TRACE(("RR: found PN, length %u\n", length));
				break;

			// Symbolic link info
			case 'SL':
			{
				uint8 bytePos = 3;
				uint8 lastCompFlag = 0;
				uint8 addPos = 0;
				bool slDone = false;
				bool useSeparator = true;

				TRACE(("RR: found SL, length %u\n", length));
				TRACE(("Buffer is at %p\n", buffer));
				TRACE(("Current length is %u\n", slNameSize));
				//kernel_debugger("");
				node->attr.slVer = *(uint8*)(buffer + bytePos++);
				slFlags = *(uint8*)(buffer + bytePos++);

				TRACE(("sl flags are %u\n", slFlags));
				while (!slDone && bytePos < length) {
					uint8 compFlag = *(uint8*)(buffer + bytePos++);
					uint8 compLen = *(uint8*)(buffer + bytePos++);

					if (slName == NULL)
						useSeparator = false;

					addPos = slNameSize;

					TRACE(("sl comp flags are %u, length is %u\n", compFlag, compLen));
					TRACE(("Current name size is %u\n", slNameSize));

					switch (compFlag) {
						case SLCP_CONTINUE:
							useSeparator = false;
						default:
							// Add the component to the total path.
							slNameSize += compLen;
							if (useSeparator)
								slNameSize++;
							slName = (char*)realloc(slName,
								slNameSize + 1);
							if (slName == NULL)
								return B_NO_MEMORY;

							if (useSeparator) {
								TRACE(("Adding separator\n"));
								slName[addPos++] = '/';
							}

							TRACE(("doing memcopy of %u bytes at offset %d\n", compLen, addPos));
							memcpy(slName + addPos, buffer + bytePos,
								compLen);

							addPos += compLen;
							useSeparator = true;
							break;

						case SLCP_CURRENT:
							TRACE(("InitNode - found link to current directory\n"));
							slNameSize += 2;
							slName = (char*)realloc(slName,
								slNameSize + 1);
							if (slName == NULL)
								return B_NO_MEMORY;

							memcpy(slName + addPos, "./", 2);
							useSeparator = false;
							break;

						case SLCP_PARENT:
							slNameSize += 3;
							slName = (char*)realloc(slName,
								slNameSize + 1);
							if (slName == NULL)
								return B_NO_MEMORY;

							memcpy(slName + addPos, "../", 3);
							useSeparator = false;
							break;

						case SLCP_ROOT:
							TRACE(("InitNode - found link to root directory\n"));
							slNameSize += 1;
							slName = (char*)realloc(slName,
								slNameSize + 1);
							if (slName == NULL)
								return B_NO_MEMORY;
							memcpy(slName + addPos, "/", 1);
							useSeparator = false;
							break;

						case SLCP_VOLROOT:
							slDone = true;
							break;

						case SLCP_HOST:
							slDone = true;
							break;
					}
					if (slName != NULL)
						slName[slNameSize] = '\0';
					lastCompFlag = compFlag;
					bytePos += compLen;
					TRACE(("Current sl name is \'%s\'\n", slName));
				}
				node->attr.slName = slName;
				TRACE(("InitNode = symlink name is \'%s\'\n", slName));
				break;
			}

			// Altername name
			case 'NM':
			{
				uint8 bytePos = 3;
				uint8 flags = 0;
				uint16 oldEnd = altNameSize;

				altNameSize += length - 5;
				altName = (char*)realloc(altName, altNameSize + 1);
				if (altName == NULL)
					return B_NO_MEMORY;

				TRACE(("RR: found NM, length %u\n", length));
				// Read flag and version.
				node->attr.nmVer = *(uint8 *)(buffer + bytePos++);
				flags = *(uint8 *)(buffer + bytePos++);

				TRACE(("RR: nm buffer is %s, start at %p\n", (buffer + bytePos), buffer + bytePos));

				// Build the file name.
				memcpy(altName + oldEnd, buffer + bytePos, length - 5);
				altName[altNameSize] = '\0';
				TRACE(("RR: alt name is %s\n", altName));

				// If the name is not continued in another record, update
				// the record name.
				if (!(flags & NM_CONTINUE)) {
					// Get rid of the ISO name, replace with RR name.
					if (node->name != NULL)
						free(node->name);
					node->name = altName;
					node->name_length = altNameSize;
				}
				break;
			}

			// Deep directory record masquerading as a file.
			case 'CL':
			{
				TRACE(("RR: found CL, length %u\n", length));
				// Reinitialize the node with the information at the
				// "." entry of the pointed to directory data
				node->startLBN[LSB_DATA] = *(uint32*)(buffer+4);
				node->startLBN[MSB_DATA] = *(uint32*)(buffer+8);

				char* buffer = (char*)block_cache_get(volume->fBlockCache,
					node->startLBN[FS_DATA_FORMAT]);
				if (buffer == NULL)
					break;

				InitNode(volume, node, buffer, NULL, true);
				block_cache_put(volume->fBlockCache,
					node->startLBN[FS_DATA_FORMAT]);
				break;
			}

			case 'PL':
				TRACE(("RR: found PL, length %u\n", length));
				break;

			case 'RE':
				// Relocated directory, we should skip.
				TRACE(("RR: found RE, length %u\n", length));
				if (!relocated)
					return B_UNSUPPORTED;
				break;

			case 'TF':
				TRACE(("RR: found TF, length %u\n", length));
				break;

			case 'RR':
				TRACE(("RR: found RR, length %u\n", length));
				break;

			case 'SF':
				TRACE(("RR: found SF, sparse files not supported!\n"));
				// TODO: support sparse files
				return B_UNSUPPORTED;

			default:
				if (buffer[0] == '\0') {
					TRACE(("RR: end of extensions\n"));
					done = true;
				} else
					TRACE(("RR: Unknown tag %c%c\n", buffer[0], buffer[1]));
				break;
		}
	}

	return B_OK;
}

//	#pragma mark - ISO-9660 specific exported functions


status_t
ISOMount(const char *path, uint32 flags, iso9660_volume **_newVolume,
	bool allowJoliet)
{
	// path: 		path to device (eg, /dev/disk/scsi/030/raw)
	// partition:	partition number on device ????
	// flags:		currently unused

	// determine if it is an ISO volume.
	char buffer[ISO_PVD_SIZE];
	bool done = false;
	bool isISO = false;
	off_t offset = 0x8000;
	ssize_t retval;
	partition_info partitionInfo;
	int deviceBlockSize, multiplier;
	iso9660_volume *volume;

	(void)flags;

	TRACE(("ISOMount - ENTER\n"));

	volume = (iso9660_volume *)calloc(sizeof(iso9660_volume), 1);
	if (volume == NULL) {
		TRACE(("ISOMount - mem error \n"));
		return B_NO_MEMORY;
	}

	memset(&partitionInfo, 0, sizeof(partition_info));

	/* open and lock the device */
	volume->fdOfSession = open(path, O_RDONLY);

	/* try to open the raw device to get access to the other sessions as well */
	if (volume->fdOfSession >= 0) {
		if (ioctl(volume->fdOfSession, B_GET_PARTITION_INFO, &partitionInfo) < 0) {
			TRACE(("B_GET_PARTITION_INFO: ioctl returned error\n"));
			strcpy(partitionInfo.device, path);
		}
		TRACE(("ISOMount: open device/file \"%s\"\n", partitionInfo.device));

		volume->fd = open(partitionInfo.device, O_RDONLY);
	}

	if (volume->fdOfSession < 0 || volume->fd < 0) {
		close(volume->fd);
		close(volume->fdOfSession);

		TRACE(("ISO9660 ERROR - Unable to open <%s>\n", path));
		free(volume);
		return B_BAD_VALUE;
	}

	deviceBlockSize = get_device_block_size(volume->fdOfSession);
	if (deviceBlockSize < 0)  {
		TRACE(("ISO9660 ERROR - device block size is 0\n"));
		close(volume->fd);
		close(volume->fdOfSession);

		free(volume);
		return B_BAD_VALUE;
	}

	volume->joliet_level = 0;
	while (!done && offset < 0x10000) {
		retval = read_pos(volume->fdOfSession, offset, (void*)buffer,
			ISO_PVD_SIZE);
		if (retval < ISO_PVD_SIZE) {
			isISO = false;
			break;
		}

		if (strncmp(buffer + 1, kISO9660IDString, 5) == 0) {
			if (*buffer == 0x01 && !isISO) {
				// ISO_VD_PRIMARY
				off_t maxBlocks;

				TRACE(("ISOMount: Is an ISO9660 volume, initting rec\n"));

				InitVolDesc(volume, buffer);
				strncpy(volume->devicePath,path,127);
				volume->id = ISO_ROOTNODE_ID;
				TRACE(("ISO9660: volume->blockSize = %d\n", volume->logicalBlkSize[FS_DATA_FORMAT]));

				multiplier = deviceBlockSize / volume->logicalBlkSize[FS_DATA_FORMAT];
				TRACE(("ISOMount: block size multiplier is %d\n", multiplier));

				// if the session is on a real device, size != 0
				if (partitionInfo.size != 0) {
					maxBlocks = (partitionInfo.size + partitionInfo.offset)
						/ volume->logicalBlkSize[FS_DATA_FORMAT];
				} else
					maxBlocks = volume->volSpaceSize[FS_DATA_FORMAT];

				/* Initialize access to the cache so that we can do cached i/o */
				TRACE(("ISO9660: cache init: dev %d, max blocks %Ld\n", volume->fd, maxBlocks));
				volume->fBlockCache = block_cache_create(volume->fd, maxBlocks,
					volume->logicalBlkSize[FS_DATA_FORMAT], true);
				isISO = true;
			} else if (*buffer == 0x02 && isISO && allowJoliet) {
				// ISO_VD_SUPPLEMENTARY

				// JOLIET extension
				// test escape sequence for level of UCS-2 characterset
			    if (buffer[88] == 0x25 && buffer[89] == 0x2f) {
					switch (buffer[90]) {
						case 0x40: volume->joliet_level = 1; break;
						case 0x43: volume->joliet_level = 2; break;
						case 0x45: volume->joliet_level = 3; break;
					}

					TRACE(("ISO9660 Extensions: Microsoft Joliet Level %d\n", volume->joliet_level));

					// Because Joliet-stuff starts at other sector,
					// update root directory record.
					if (volume->joliet_level > 0) {
						InitNode(volume, &volume->rootDirRec, &buffer[156],
							NULL);
					}
				}
			} else if (*(unsigned char *)buffer == 0xff) {
				// ISO_VD_END
				done = true;
			} else
				TRACE(("found header %d\n",*buffer));
		}
		offset += 0x800;
	}

	if (!isISO) {
		// It isn't an ISO disk.
		close(volume->fdOfSession);
		close(volume->fd);
		free(volume);

		TRACE(("ISOMount: Not an ISO9660 volume!\n"));
		return B_BAD_VALUE;
	}

	TRACE(("ISOMount - EXIT, returning %p\n", volume));
	*_newVolume = volume;
	return B_OK;
}


/*!	Reads in a single directory entry and fills in the values in the
	dirent struct. Uses the cookie to keep track of the current block
	and position within the block. Also uses the cookie to determine when
	it has reached the end of the directory file.
*/
status_t
ISOReadDirEnt(iso9660_volume *volume, dircookie *cookie, struct dirent *dirent,
	size_t bufferSize)
{
	int	result = B_NO_ERROR;
	bool done = false;

	TRACE(("ISOReadDirEnt - ENTER\n"));

	while (!done) {
		off_t totalRead = cookie->pos + (cookie->block - cookie->startBlock)
			* volume->logicalBlkSize[FS_DATA_FORMAT];

		// If we're at the end of the data in a block, move to the next block.
		char *blockData;
		while (true) {
			blockData
				= (char*)block_cache_get(volume->fBlockCache, cookie->block);
			if (blockData != NULL && *(blockData + cookie->pos) == 0) {
				// NULL data, move to next block.
				block_cache_put(volume->fBlockCache, cookie->block);
				blockData = NULL;
				totalRead
					+= volume->logicalBlkSize[FS_DATA_FORMAT] - cookie->pos;
				cookie->pos = 0;
				cookie->block++;
			} else
				break;

			if (totalRead >= cookie->totalSize)
				break;
		}

		off_t cacheBlock = cookie->block;

		if (blockData != NULL && totalRead < cookie->totalSize) {
			iso9660_inode node;
			size_t bytesRead = 0;
			result = InitNode(volume, &node, blockData + cookie->pos,
				&bytesRead);

			// if we hit an entry that we don't support, we just skip it
			if (result != B_OK && result != B_UNSUPPORTED)
				break;

			if (result == B_OK && (node.flags & ISO_IS_ASSOCIATED_FILE) == 0) {
				size_t nameBufferSize = bufferSize - sizeof(struct dirent);

				dirent->d_dev = volume->id;
				dirent->d_ino = ((ino_t)cookie->block << 30)
					+ (cookie->pos & 0x3fffffff);
				dirent->d_reclen = sizeof(struct dirent) + node.name_length + 1;

				if (node.name_length <= nameBufferSize) {
					// need to do some size checking here.
					strlcpy(dirent->d_name, node.name, node.name_length + 1);
					TRACE(("ISOReadDirEnt  - success, name is %s, block %Ld, "
						"pos %Ld, inode id %Ld\n", dirent->d_name, cookie->block,
						cookie->pos, dirent->d_ino));
				} else {
					// TODO: this can be just normal if we support reading more
					// than one entry.
					TRACE(("ISOReadDirEnt - ERROR, name %s does not fit in "
						"buffer of size %d\n", node.name, (int)nameBufferSize));
					result = B_BAD_VALUE;
				}

				done = true;
			}

			cookie->pos += bytesRead;

			if (cookie->pos == volume->logicalBlkSize[FS_DATA_FORMAT]) {
				cookie->pos = 0;
				cookie->block++;
			}
		} else {
			if (totalRead >= cookie->totalSize)
				result = B_ENTRY_NOT_FOUND;
			else
				result = B_NO_MEMORY;
			done = true;
		}

		if (blockData != NULL)
			block_cache_put(volume->fBlockCache, cacheBlock);
	}

	TRACE(("ISOReadDirEnt - EXIT, result is %s, vnid is %Lu\n",
		strerror(result), dirent->d_ino));

	return result;
}


status_t
InitNode(iso9660_volume* volume, iso9660_inode* node, char* buffer,
	size_t* _bytesRead, bool relocated)
{
	uint8 recordLength = *(uint8*)buffer++;
	size_t nameLength;

	TRACE(("InitNode - ENTER, bufstart is %p, record length is %d bytes\n",
		buffer, recordLength));

	if (_bytesRead != NULL)
		*_bytesRead = recordLength;
	if (recordLength == 0)
		return B_ENTRY_NOT_FOUND;

	char* end = buffer + recordLength;

	if (!relocated) {
		node->cache = NULL;
		node->name = NULL;
		node->attr.slName = NULL;
		memset(node->attr.stat, 0, sizeof(node->attr.stat));
	} else
		free(node->attr.slName);

	node->extAttrRecLen = *(uint8*)buffer++;
	TRACE(("InitNode - ext attr length is %d\n", (int)node->extAttrRecLen));

	node->startLBN[LSB_DATA] = *(uint32*)buffer;
	buffer += 4;
	node->startLBN[MSB_DATA] = *(uint32*)buffer;
	buffer += 4;
	TRACE(("InitNode - data start LBN is %d\n",
		(int)node->startLBN[FS_DATA_FORMAT]));

	node->dataLen[LSB_DATA] = *(uint32*)buffer;
	buffer += 4;
	node->dataLen[MSB_DATA] = *(uint32*)buffer;
	buffer += 4;
	TRACE(("InitNode - data length is %d\n",
		(int)node->dataLen[FS_DATA_FORMAT]));

	init_node_date(&node->recordDate, buffer);
	buffer += 7;

	node->flags = *(uint8*)buffer;
	buffer++;
	TRACE(("InitNode - flags are %d\n", node->flags));

	node->fileUnitSize = *(uint8*)buffer;
	buffer++;
	TRACE(("InitNode - fileUnitSize is %d\n", node->fileUnitSize));

	node->interleaveGapSize = *(uint8*)buffer;
	buffer++;
	TRACE(("InitNode - interleave gap size = %d\n", node->interleaveGapSize));

	node->volSeqNum = *(uint32*)buffer;
	buffer += 4;
	TRACE(("InitNode - volume seq num is %d\n", (int)node->volSeqNum));

	nameLength = *(uint8*)buffer;
	buffer++;

	// for relocated directories we take the name from the placeholder entry
	if (!relocated) {
		node->name_length = nameLength;
		TRACE(("InitNode - file id length is %" B_PRIu32 "\n",
			node->name_length));
	}

	// Set defaults, in case there is no RockRidge stuff.
	node->attr.stat[FS_DATA_FORMAT].st_mode |= (node->flags & ISO_IS_DIR) != 0
		? S_IFDIR | S_IXUSR | S_IRUSR | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH
		: S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;

	if (node->name_length == 0) {
		TRACE(("InitNode - File ID String is 0 length\n"));
		return B_ENTRY_NOT_FOUND;
	}

	if (!relocated) {
		// JOLIET extension:
		// on joliet discs, buffer[0] can be 0 for Unicoded filenames,
		// so I've added a check here to test explicitely for
		// directories (which have length 1)
		// Take care of "." and "..", the first two dirents are
		// these in iso.
		if (node->name_length == 1 && buffer[0] == 0) {
			node->name = strdup(".");
			node->name_length = 1;
		} else if (node->name_length == 1 && buffer[0] == 1) {
			node->name = strdup("..");
			node->name_length = 2;
		} else if (volume->joliet_level > 0) {
			// JOLIET extension: convert Unicode16 string to UTF8
			// Assume that the unicode->utf8 conversion produces 4 byte
			// utf8 characters, and allocate that much space
			node->name = (char*)malloc(node->name_length * 2 + 1);
			if (node->name == NULL)
				return B_NO_MEMORY;

			int32 sourceLength = node->name_length;
			int32 destLength = node->name_length * 2;

			status_t status = unicode_to_utf8(buffer, &sourceLength,
				node->name, &destLength);
			if (status < B_OK) {
				dprintf("iso9660: error converting unicode->utf8\n");
				return status;
			}

			node->name[destLength] = '\0';
			node->name_length = destLength;

			sanitize_iso_name(node, false);
		} else {
			node->name = (char*)malloc(node->name_length + 1);
			if (node->name == NULL)
				return B_NO_MEMORY;

			// convert all characters to lower case
			for (uint32 i = 0; i < node->name_length; i++)
				node->name[i] = tolower(buffer[i]);

			node->name[node->name_length] = '\0';

			sanitize_iso_name(node, true);
		}

		if (node->name == NULL) {
			TRACE(("InitNode - unable to allocate memory!\n"));
			return B_NO_MEMORY;
		}
	}

	buffer += nameLength;
	if (nameLength % 2 == 0)
		buffer++;

	TRACE(("DirRec ID String is: %s\n", node->name));

	return parse_rock_ridge(volume, node, buffer, end, relocated);
}


status_t
ConvertRecDate(ISORecDate* inDate, time_t* outDate)
{
	time_t	time;
	int		days, i, year, tz;

	year = inDate->year -70;
	tz = inDate->offsetGMT;

	if (year < 0) {
		time = 0;
	} else {
		const int monlen[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

		days = (year * 365);

		if (year > 2)
			days += (year + 1)/ 4;

		for (i = 1; (i < inDate->month) && (i < 12); i++) {
			days += monlen[i-1];
		}

		if (((year + 2) % 4) == 0 && inDate->month > 2)
			days++;

		days += inDate->date - 1;
		time = ((((days*24) + inDate->hour) * 60 + inDate->minute) * 60)
					+ inDate->second;
		if (tz & 0x80)
			tz |= (-1 << 8);

		if (-48 <= tz && tz <= 52)
			time += tz *15 * 60;
	}
	*outDate = time;
	return 0;
}

