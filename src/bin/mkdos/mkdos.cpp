/*

mkdos shell tool

Initialize FAT16 or FAT32 partitions, FAT12 floppy disks not supported

Copyright (c) 2002 Marcus Overhagen <marcus@overhagen.de>, OpenBeOS project

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#include <ByteOrder.h>
#include <Drivers.h>
#include <OS.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fat.h"

#define WITH_FLOPPY_SUPPORT

void PrintUsage();
void CreateVolumeLabel(void *sector, const char *label);
status_t Initialize(int fatbits, const char *device, const char *label, bool noprompt, bool testmode);

int 
main(int argc, char *argv[])
{
	if (sizeof(bootsector1216) != 512 || sizeof(bootsector32) != 512 || sizeof(fsinfosector32) != 512) {
		printf("compilation error: struct alignment wrong\n");
		return 1;
	}
	
	const char *device = NULL;
	const char *label = NULL;
	bool noprompt = false;
	bool test = false;
	int fat = 0;
	
	while (1) { 
		int c;
		int option_index = 0; 
		static struct option long_options[] = 
		{ 
		 	{"noprompt", no_argument, 0, 'n'}, 
			{"test", no_argument, 0, 't'}, 
			{"fat", required_argument, 0, 'f'}, 
			{0, 0, 0, 0} 
		}; 
		
		c = getopt_long (argc, argv, "ntf:", long_options, &option_index); 
		if (c == -1) 
			break; 

		switch (c) { 
			case 'n': 
				noprompt = true;
		 		break; 

			case 't':
				test = true;
				break; 

			case 'f':
				fat = strtol(optarg, NULL, 10);
				if (fat == 0) 
					fat = -1;
				break; 

			default: 
		        printf("\n");
				PrintUsage();
				return 1;
		} 
	} 

	if (optind < argc)
		device = argv[optind];
	if ((optind + 1) < argc)
		label = argv[optind + 1];

	if (fat != 0 && fat != 12 && fat != 16 && fat != 32) {
		printf("mkdos error: fat must be 12, 16, or 32 bits\n");
		PrintUsage();
		return 1;
	}

	if (device == NULL) {
		printf("mkdos error: you must specify a device or partition or image\n");
        printf("             such as /dev/disk/ide/ata/1/master/0/0_0\n");
		PrintUsage();
		return 1;
	}

	if (label == NULL) {
		label = "no name";
	}

	if (noprompt)
		printf("will not prompt for confirmation\n");

	if (test)
		printf("test mode enabled (no writes will occur)\n");
		
	status_t s;
	s = Initialize(fat, device, label, noprompt, test);

	if (s != 0) {
		printf("Initializing failed!\n");
	}

	return (s == B_OK) ? 0 : 1;
}

status_t Initialize(int fatbits, const char *device, const char *label, bool noprompt, bool testmode)
{
	if (fatbits != 0 && fatbits != 12 && fatbits != 16 && fatbits != 32) {
		fprintf(stderr,"Error: don't know how to create a %d bit fat\n",fatbits);
		return B_ERROR;
	}
	//XXX the following two checks can be removed when this is fixed:
#ifndef WITH_FLOPPY_SUPPORT
	if (0 != strstr(device,"floppy")) {
		fprintf(stderr,"Error: floppy B_GET_GEOMETRY and B_GET_BIOS_GEOMETRY calls are broken, floppy not supported\n");
		return B_ERROR;
	}
	if (fatbits == 12) {
		fprintf(stderr,"Error: can't create a 12 bit fat on a device other than floppy\n");
		return B_ERROR;
	}
#endif

	printf("device = %s\n",device);

	int fd = open(device, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Error: couldn't open file for device %s (%s)\n", device, strerror(errno));
		return B_ERROR;
	}

	bool isRawDevice;
	bool hasBiosGeometry;
	bool hasDeviceGeometry;
	bool hasPartitionInfo;
	device_geometry biosGeometry;
	device_geometry deviceGeometry;
	partition_info 	partitionInfo;

	isRawDevice = 0 != strstr(device, "/raw");
	hasBiosGeometry = B_OK == ioctl(fd, B_GET_BIOS_GEOMETRY, &biosGeometry, sizeof(biosGeometry));
	hasDeviceGeometry = B_OK == ioctl(fd, B_GET_GEOMETRY, &deviceGeometry, sizeof(deviceGeometry));
	hasPartitionInfo = B_OK == ioctl(fd, B_GET_PARTITION_INFO, &partitionInfo, sizeof(partitionInfo));

	if (!isRawDevice && !hasBiosGeometry && !hasDeviceGeometry && !hasPartitionInfo)
		isRawDevice = true;

	if (hasBiosGeometry) {
		printf("bios geometry: %ld heads, %ld cylinders, %ld sectors/track, %ld bytes/sector\n",
			biosGeometry.head_count,biosGeometry.cylinder_count,biosGeometry.sectors_per_track,biosGeometry.bytes_per_sector);
	}
	if (hasBiosGeometry) {
		printf("device geometry: %ld heads, %ld cylinders, %ld sectors/track, %ld bytes/sector\n",
			deviceGeometry.head_count,deviceGeometry.cylinder_count,deviceGeometry.sectors_per_track,deviceGeometry.bytes_per_sector);
	}
	if (hasPartitionInfo) {
		printf("partition info: start at %Ld bytes (%Ld sectors), %Ld KB, %Ld MB, %Ld GB\n",
			partitionInfo.offset,
			partitionInfo.offset / 512,
			partitionInfo.offset / 1024,
			partitionInfo.offset / (1024 * 1024),
			partitionInfo.offset / (1024 * 1024 * 1024));
		printf("partition info: size %Ld bytes, %Ld KB, %Ld MB, %Ld GB\n",
			partitionInfo.size,
			partitionInfo.size / 1024,
			partitionInfo.size / (1024 * 1024),
			partitionInfo.size / (1024 * 1024 * 1024));
	}

	if (!isRawDevice && !hasPartitionInfo) {
		fprintf(stderr,"Warning: couldn't get partition information\n");
	}
	if ((hasBiosGeometry && biosGeometry.bytes_per_sector != 512) 
		||	(hasDeviceGeometry && deviceGeometry.bytes_per_sector != 512)) {
		fprintf(stderr,"Error: geometry block size not 512 bytes\n");
		close(fd);
		return B_ERROR;
	} else if (hasPartitionInfo && partitionInfo.logical_block_size != 512) {
		printf("partition logical block size is not 512, it's %ld bytes\n",
			partitionInfo.logical_block_size);
	}

	if (hasDeviceGeometry && deviceGeometry.read_only) {
		fprintf(stderr,"Error: this is a read-only device\n");
		close(fd);
		return B_ERROR;
	}
	if (hasDeviceGeometry && deviceGeometry.write_once) {
		fprintf(stderr,"Error: this is a write-once device\n");
		close(fd);
		return B_ERROR;
	}
	uint64 size = 0;

	if (hasPartitionInfo) {
		size = partitionInfo.size;
	} else if (hasDeviceGeometry) {
		size = uint64(deviceGeometry.bytes_per_sector) * deviceGeometry.sectors_per_track * deviceGeometry.cylinder_count * deviceGeometry.head_count;
	} else if (hasBiosGeometry) {
		size = uint64(biosGeometry.bytes_per_sector) * biosGeometry.sectors_per_track * biosGeometry.cylinder_count * biosGeometry.head_count;
	} else {
		// maybe it's just a file
		struct stat stat;
		if (fstat(fd, &stat) < 0) {
			fprintf(stderr, "Error: couldn't get device partition or geometry information, nor size\n");
			close(fd);
			return B_ERROR;
		}
		size = stat.st_size;
	}

	// TODO still valid on Haiku ?
	/*if (isRawDevice && size > FLOPPY_MAX_SIZE) {
		fprintf(stderr,"Error: device too large for floppy, or raw devices not supported\n");
		close(fd);
		return B_ERROR;
	}*/

	printf("size = %Ld bytes (%Ld sectors), %Ld KB, %Ld MB, %Ld GB\n",
		size,
		size / 512,
		size / 1024,
		size / (1024 * 1024),
		size / (1024 * 1024 * 1024));
	
	if (fatbits == 0) {
		//auto determine fat type
		if (isRawDevice && size <= FLOPPY_MAX_SIZE && (size / FAT12_CLUSTER_MAX_SIZE) < FAT12_MAX_CLUSTER_COUNT) {
			fatbits = 12;
		} else if ((size / CLUSTER_MAX_SIZE) < FAT16_MAX_CLUSTER_COUNT) {
			fatbits = 16;
		} else if ((size / CLUSTER_MAX_SIZE) < FAT32_MAX_CLUSTER_COUNT) {
			fatbits = 32;
		}
	}

	if (fatbits == 0) {
		fprintf(stderr,"Error: device too large for 32 bit fat\n");
		close(fd);
		return B_ERROR;
	}

	int sectorPerCluster;

	sectorPerCluster = 0;
	if (fatbits == 12) {
		sectorPerCluster = 0;
		if (size <= 4182016LL)
			sectorPerCluster = 2;	// XXX don't know the correct value
		if (size <= 2091008LL)
			sectorPerCluster = 1;	// XXX don't know the correct value
	} else if (fatbits == 16) {
		// special BAD_CLUSTER value is 0xFFF7, 
		// but this should work anyway, since space required by 
		// two FATs will make maximum cluster count smaller.
		// at least, this is what I think *should* happen
		sectorPerCluster = 0;				//larger than 2 GB must fail
		if (size <= (2048 * 1024 * 1024LL))	// up to 2GB, use 32k clusters
			sectorPerCluster = 64;
		if (size <= (1024 * 1024 * 1024LL))	// up to 1GB, use 16k clusters
			sectorPerCluster = 32;
		if (size <= (512 * 1024 * 1024LL))	// up to 512MB, use 8k clusters
			sectorPerCluster = 16;
		if (size <= (256 * 1024 * 1024LL))	// up to 256MB, use 4k clusters
			sectorPerCluster = 8;
		if (size <= (128 * 1024 * 1024LL))	// up to 128MB, use 2k clusters
			sectorPerCluster = 4;
		if (size <= (16 * 1024 * 1024LL))	// up to 16MB, use 2k clusters
			sectorPerCluster = 2;
		if (size <= 4182016LL)				// smaller than fat32 must fail
			sectorPerCluster = 0;
	} if (fatbits == 32) {
		sectorPerCluster = 64;				// default is 32k clusters
		if (size <= (32 * 1024 * 1024 * 1024LL))	// up to 32GB, use 16k clusters
			sectorPerCluster = 32;
		if (size <= (16 * 1024 * 1024 * 1024LL))	// up to 16GB, use 8k clusters
			sectorPerCluster = 16;
		if (size <= (8 * 1024 * 1024 * 1024LL))		// up to 8GB, use 4k clusters
			sectorPerCluster = 8;
		if (size <= (532480 * 512LL))				// up to 260 MB, use 0.5k clusters
			sectorPerCluster = 1;
		if (size <= (66600 * 512LL))		// smaller than 32.5 MB must fail
			sectorPerCluster = 0;
	}

	if (sectorPerCluster == 0) {
		fprintf(stderr,"Error: failed to determine sector per cluster value, partition too large for %d bit fat\n",fatbits);
		close(fd);
		return B_ERROR;
	}

	int reservedSectorCount = 0; // avoid compiler warning
	int rootEntryCount = 0; // avoid compiler warning
	int numFATs;
	int sectorSize;
	uint8 biosDriveId;

	// get bios drive-id, or use 0x80
	if (B_OK != ioctl(fd, B_GET_BIOS_DRIVE_ID, &biosDriveId, sizeof(biosDriveId))) {
		biosDriveId = 0x80;
	} else {
		printf("bios drive id: 0x%02x\n", (int)biosDriveId);
	}

	// default parameters for the bootsector
	numFATs = 2;
	sectorSize = 512;
	if (fatbits == 12 || fatbits == 16)
		reservedSectorCount = 1;
	if (fatbits == 32)
		reservedSectorCount = 32;
	if (fatbits == 12)
		rootEntryCount = 128; // XXX don't know the correct value
	if (fatbits == 16)
		rootEntryCount = 512;
	if (fatbits == 32)
		rootEntryCount = 0;

	// Determine FATSize
	// calculation done as MS recommends
	uint64 dskSize = size / sectorSize;
	uint32 rootDirSectors = ((rootEntryCount * 32) + (sectorSize - 1)) / sectorSize;
	uint64 tmpVal1 = dskSize - (reservedSectorCount + rootDirSectors);
	uint64 tmpVal2 = (256 * sectorPerCluster) + numFATs;
	if (fatbits == 32)
		tmpVal2 = tmpVal2 / 2;
	uint32 FATSize = (tmpVal1 + (tmpVal2 - 1)) / tmpVal2;
	// FATSize should now contain the size of *one* FAT, measured in sectors
	// RootDirSectors should now contain the size of the fat12/16 root directory, measured in sectors

	printf("fatbits = %d, clustersize = %d\n", fatbits, sectorPerCluster * 512);
	printf("FAT size is %ld sectors\n", FATSize);
	printf("disk label: %s\n", label);

	char bootsector[512];
	memset(bootsector,0x00,512);
	memcpy(bootsector + BOOTJMP_START_OFFSET, bootjmp, sizeof(bootjmp));
	memcpy(bootsector + BOOTCODE_START_OFFSET, bootcode, sizeof(bootcode));
	
	if (fatbits == 32) {
		bootsector32 *bs = (bootsector32 *)bootsector;
		uint16 temp16;
		uint32 temp32;
		memcpy(bs->BS_OEMName,"Haiku   ",8);
		bs->BPB_BytsPerSec = B_HOST_TO_LENDIAN_INT16(sectorSize);
		bs->BPB_SecPerClus = sectorPerCluster;
		bs->BPB_RsvdSecCnt = B_HOST_TO_LENDIAN_INT16(reservedSectorCount);
		bs->BPB_NumFATs = numFATs;
		bs->BPB_RootEntCnt = B_HOST_TO_LENDIAN_INT16(rootEntryCount);
		bs->BPB_TotSec16 = B_HOST_TO_LENDIAN_INT16(0);
		bs->BPB_Media = 0xF8;
		bs->BPB_FATSz16 = B_HOST_TO_LENDIAN_INT16(0);
		temp16 = hasBiosGeometry ? biosGeometry.sectors_per_track : 63;
		bs->BPB_SecPerTrk = B_HOST_TO_LENDIAN_INT16(temp16);
		temp16 = hasBiosGeometry ? biosGeometry.head_count : 255;
		bs->BPB_NumHeads = B_HOST_TO_LENDIAN_INT16(temp16);
		temp32 = hasPartitionInfo ? (partitionInfo.size / 512) : 0;
		bs->BPB_HiddSec = B_HOST_TO_LENDIAN_INT32(temp32);
		temp32 = size / 512;
		bs->BPB_TotSec32 = B_HOST_TO_LENDIAN_INT32(temp32);
		bs->BPB_FATSz32 = B_HOST_TO_LENDIAN_INT32(FATSize);
		bs->BPB_ExtFlags = B_HOST_TO_LENDIAN_INT16(0);
		bs->BPB_FSVer = B_HOST_TO_LENDIAN_INT16(0);
		bs->BPB_RootClus = B_HOST_TO_LENDIAN_INT32(FAT32_ROOT_CLUSTER);
		bs->BPB_FSInfo = B_HOST_TO_LENDIAN_INT16(FSINFO_SECTOR_NUM);
		bs->BPB_BkBootSec = B_HOST_TO_LENDIAN_INT16(BACKUP_SECTOR_NUM);
		memset(bs->BPB_Reserved,0,12);
		bs->BS_DrvNum = biosDriveId;
		bs->BS_Reserved1 = 0x00;
		bs->BS_BootSig = 0x29;
		*(uint32*)bs->BS_VolID = (uint32)system_time();
		memcpy(bs->BS_VolLab,"NO NAME    ",11);
		memcpy(bs->BS_FilSysType,"FAT32   ",8);
		bs->signature = B_HOST_TO_LENDIAN_INT16(0xAA55);
	} else {
		bootsector1216 *bs = (bootsector1216 *)bootsector;
		uint16 temp16;
		uint32 temp32;
		uint32 sectorcount = size / 512;
		memcpy(bs->BS_OEMName, "Haiku   ", 8);
		bs->BPB_BytsPerSec = B_HOST_TO_LENDIAN_INT16(sectorSize);
		bs->BPB_SecPerClus = sectorPerCluster;
		bs->BPB_RsvdSecCnt = B_HOST_TO_LENDIAN_INT16(reservedSectorCount);
		bs->BPB_NumFATs = numFATs;
		bs->BPB_RootEntCnt = B_HOST_TO_LENDIAN_INT16(rootEntryCount);
		temp16 = (sectorcount <= 65535) ? sectorcount : 0;
		bs->BPB_TotSec16 = B_HOST_TO_LENDIAN_INT16(temp16);
		bs->BPB_Media = 0xF8;
		bs->BPB_FATSz16 = B_HOST_TO_LENDIAN_INT16(FATSize);
		temp16 = hasBiosGeometry ? biosGeometry.sectors_per_track : 63;
		bs->BPB_SecPerTrk = B_HOST_TO_LENDIAN_INT16(temp16);
		temp16 = hasBiosGeometry ? biosGeometry.head_count : 255;
		bs->BPB_NumHeads = B_HOST_TO_LENDIAN_INT16(temp16);
		temp32 = hasPartitionInfo ? (partitionInfo.size / 512) : 0;
		bs->BPB_HiddSec = B_HOST_TO_LENDIAN_INT32(temp32);
		temp32 = (sectorcount <= 65535) ? 0 : sectorcount;
		bs->BPB_TotSec32 = B_HOST_TO_LENDIAN_INT32(temp32);
		bs->BS_DrvNum = biosDriveId;
		bs->BS_Reserved1 = 0x00;
		bs->BS_BootSig = 0x29;
		*(uint32*)bs->BS_VolID = (uint32)system_time();
		memcpy(bs->BS_VolLab,"NO NAME    ",11);
		memcpy(bs->BS_FilSysType,(fatbits == 12) ? "FAT12   " : "FAT16   ",8);
		bs->signature = B_HOST_TO_LENDIAN_INT16(0xAA55);
	}

	if (!noprompt) {
		printf("\n");
		printf("Initializing will erase all existing data on the drive.\n");
		printf("Do you wish to proceed? ");
		char answer[1000];
		char *p;
		memset(answer, 0, 1000);
		fflush(stdout);
		p = fgets(answer, 1000, stdin);
		if (p && (p=strchr(p, '\n')))
			*p = '\0'; /* remove newline */
		if ((p == NULL) || (strlen(answer) < 1) || (0 != strncasecmp(answer, "yes", strlen(answer)))) {
			printf("drive NOT initialized\n");
			close(fd);
			return B_OK;
		}
	}
	if (testmode) {
		close(fd);
		return B_OK;
	}
	
	// Disk layout:
	// 0) reserved sectors, this includes the bootsector, fsinfosector and bootsector backup
	// 1) FAT
	// 2) root directory (not on fat32)
	// 3) file & directory data

	ssize_t written;

	// initialize everything with zero first
	// avoid doing 512 byte writes here, they are slow
	printf("Writing FAT\n");
	char * zerobuffer = (char *)malloc(65536);
	memset(zerobuffer,0,65536);
	int64 bytes_to_write = 512LL * (reservedSectorCount + (numFATs * FATSize) + rootDirSectors);
	int64 pos = 0;
	while (bytes_to_write > 0) {
		ssize_t writesize = min_c(bytes_to_write, 65536);
		written = write_pos(fd, pos, zerobuffer, writesize);
		if (written != writesize) {
			fprintf(stderr,"Error: write error near sector %Ld\n",pos / 512);
			close(fd);
			return B_ERROR;
		}
		bytes_to_write -= writesize;
		pos += writesize;
	}
	free(zerobuffer);
	
	//write boot sector
	printf("Writing boot block\n");
	written = write_pos(fd, BOOT_SECTOR_NUM * 512, bootsector, 512);
	if (written != 512) {
		fprintf(stderr,"Error: write error at sector %d\n", BOOT_SECTOR_NUM);
		close(fd);
		return B_ERROR;
	}

	if (fatbits == 32) {
		written = write_pos(fd, BACKUP_SECTOR_NUM * 512, bootsector, 512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %d\n", BACKUP_SECTOR_NUM);
			close(fd);
			return B_ERROR;
		}
	}
	
	//write first fat sector
	printf("Writing first FAT sector\n");
	uint8 sec[512];
	memset(sec,0,512);
	if (fatbits == 12) {
		//FAT[0] contains media byte in lower 8 bits, all other bits set to 1
		//FAT[1] contains EOF marker
		sec[0] = 0xF8;
		sec[1] = 0xFF;
		sec[2] = 0xFF;
	} else if (fatbits == 16) {
		//FAT[0] contains media byte in lower 8 bits, all other bits set to 1
		sec[0] = 0xF8;
		sec[1] = 0xFF;
		//FAT[1] contains EOF marker
		sec[2] = 0xFF;
		sec[3] = 0xFF;
	} else if (fatbits == 32) {
		//FAT[0] contains media byte in lower 8 bits, all other bits set to 1
		sec[0] = 0xF8;
		sec[1] = 0xFF;
		sec[2] = 0xFF;
		sec[3] = 0xFF;
		//FAT[1] contains EOF marker
		sec[4] = 0xFF;
		sec[5] = 0xFF;
		sec[6] = 0xFF;
		sec[7] = 0x0F;
		//FAT[2] contains EOF marker, used to terminate root directory
		sec[8] = 0xFF;
		sec[9] = 0xFF;
		sec[10] = 0xFF;
		sec[11] = 0x0F;
	}
	written = write_pos(fd, reservedSectorCount * 512, sec, 512);
	if (written != 512) {
		fprintf(stderr,"Error: write error at sector %d\n", reservedSectorCount);
		close(fd);
		return B_ERROR;
	}
	if (numFATs > 1) {
		written = write_pos(fd, (reservedSectorCount + FATSize) * 512,sec,512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %ld\n", reservedSectorCount + FATSize);
			close(fd);
			return B_ERROR;
		}
	}

	//write fsinfo sector
	if (fatbits == 32) {
		printf("Writing boot info\n");
		//calculate total sector count first
		uint64 free_count = size / 512;
		//now account for already by metadata used sectors
		free_count -= reservedSectorCount + (numFATs * FATSize) + rootDirSectors;
		//convert from sector to clustercount
		free_count /= sectorPerCluster;
		//and account for 1 already used cluster of root directory
		free_count -= 1; 
		fsinfosector32 fsinfosector;
		memset(&fsinfosector,0x00,512);
		fsinfosector.FSI_LeadSig 	= B_HOST_TO_LENDIAN_INT32(0x41615252);
		fsinfosector.FSI_StrucSig 	= B_HOST_TO_LENDIAN_INT32(0x61417272);
		fsinfosector.FSI_Free_Count = B_HOST_TO_LENDIAN_INT32((uint32)free_count);
		fsinfosector.FSI_Nxt_Free 	= B_HOST_TO_LENDIAN_INT32(3);
		fsinfosector.FSI_TrailSig 	= B_HOST_TO_LENDIAN_INT32(0xAA550000);
		written = write_pos(fd, FSINFO_SECTOR_NUM * 512, &fsinfosector, 512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %d\n", FSINFO_SECTOR_NUM);
			close(fd);
			return B_ERROR;
		}
	}

	//write volume label into root directory
	printf("Writing root directory\n");
	if (fatbits == 12 || fatbits == 16) {
		uint8 data[512];
		memset(data, 0, 512);
		CreateVolumeLabel(data, label);
		uint32 rootDirSector = reservedSectorCount + (numFATs * FATSize);
		written = write_pos(fd, rootDirSector * 512, data, 512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %ld\n", rootDirSector);
			close(fd);
			return B_ERROR;
		}
	} else if (fatbits == 32) {
		int size = 512 * sectorPerCluster;
		uint8 *cluster = (uint8*)malloc(size);
		memset(cluster, 0, size);
		CreateVolumeLabel(cluster, label);
		uint32 rootDirSector = reservedSectorCount + (numFATs * FATSize) + rootDirSectors;
		written = write_pos(fd, rootDirSector * 512, cluster, size);
		free(cluster);
		if (written != size) {
			fprintf(stderr,"Error: write error at sector %ld\n", rootDirSector);
			close(fd);
			return B_ERROR;
		}
	}

	ioctl(fd, B_FLUSH_DRIVE_CACHE);
	close(fd);
	
	return B_OK;
}

void CreateVolumeLabel(void *sector, const char *label)
{
	// create a volume name directory entry in the 512 byte sector
	// XXX convert from UTF8, and check for valid characters
	// XXX this could be changed to use long file name entrys,
	// XXX but the dosfs would have to be updated, too
	
	dirent *d = (dirent *)sector;
	memset(d, 0, sizeof(*d));
	memset(d->Name, 0x20, 11);
	memcpy(d->Name, label, min_c(11, strlen(label)));
	d->Attr = 0x08;
}

void PrintUsage()
{
	printf("\n");
	printf("(c) 2002 by Marcus Overhagen\n");
	printf("\n");
	printf("usage: mkdos [-n] [-t] [-f 12|16|32] device [volume_label]\n");
	printf("       -n, --noprompt  do not prompt before writing\n");
	printf("       -t, --test      enable test mode (will not write to disk)\n");
	printf("       -f, --fat       use FAT entries of the specified size\n");
}       
