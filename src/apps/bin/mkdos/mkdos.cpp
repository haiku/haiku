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
#include <OS.h>
#include <Drivers.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <File.h>
#include <ByteOrder.h>
#include <getopt.h>
#include "fat.h"

void PrintUsage();
void CreateVolumeLabel(void *sector, const char *label);
status_t Initialize(int fatbits, const char *device, const char *label, bool noprompt, bool testmode);

int main(int argc, char *argv[])
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
		
		c = getopt_long (argc, argv, "ntf:",long_options, &option_index); 
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
		printf("mkdos error: you must specify a device or partition\n");
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
	if (0 != strstr(device,"floppy")) {
		fprintf(stderr,"Error: floppy B_GET_GEOMETRY and B_GET_BIOS_GEOMETRY calls are broken, floppy not supported\n");
		return B_ERROR;
	}
	if (fatbits == 12) {
		fprintf(stderr,"Error: can't create a 12 bit fat on a device other than floppy\n");
		return B_ERROR;
	}

	printf("device = %s\n",device);

	BFile file(device,B_READ_WRITE);
	status_t result = file.InitCheck();
	if (result != B_OK) {
		fprintf(stderr,"Error: couldn't open device %s\n",device);
		return result;
	}

	int fd = file.Dup();
	if (fd < 0) {
		fprintf(stderr,"Error: couldn't open file descriptor for device %s\n",device);
		return B_ERROR;
	}

	bool IsRawDevice;
	bool HasBiosGeometry;
	bool HasDeviceGeometry;
	bool HasPartitionInfo;
	device_geometry BiosGeometry;
	device_geometry DeviceGeometry;
	partition_info 	PartitionInfo;
	
	IsRawDevice = 0 != strstr(device,"/raw");
	HasBiosGeometry = B_OK == ioctl(fd,B_GET_BIOS_GEOMETRY,&BiosGeometry);
	HasDeviceGeometry = B_OK == ioctl(fd,B_GET_GEOMETRY,&DeviceGeometry);
	HasPartitionInfo = B_OK == ioctl(fd,B_GET_PARTITION_INFO,&PartitionInfo);

	if (HasBiosGeometry) {
		printf("bios geometry: %ld heads, %ld cylinders, %ld sectors/track, %ld bytes/sector\n",
			BiosGeometry.head_count,BiosGeometry.cylinder_count,BiosGeometry.sectors_per_track,BiosGeometry.bytes_per_sector);
	}
	if (HasBiosGeometry) {
		printf("device geometry: %ld heads, %ld cylinders, %ld sectors/track, %ld bytes/sector\n",
			DeviceGeometry.head_count,DeviceGeometry.cylinder_count,DeviceGeometry.sectors_per_track,DeviceGeometry.bytes_per_sector);
	}
	if (HasPartitionInfo) {
		printf("partition info: start at %Ld bytes (%Ld sectors), %Ld KB, %Ld MB, %Ld GB\n",
			PartitionInfo.offset,
			PartitionInfo.offset / 512,
			PartitionInfo.offset / 1024,
			PartitionInfo.offset / (1024 * 1024),
			PartitionInfo.offset / (1024 * 1024 * 1024));
		printf("partition info: size %Ld bytes, %Ld KB, %Ld MB, %Ld GB\n",
			PartitionInfo.size,
			PartitionInfo.size / 1024,
			PartitionInfo.size / (1024 * 1024),
			PartitionInfo.size / (1024 * 1024 * 1024));
	}

	if (!HasBiosGeometry && !HasDeviceGeometry && !HasPartitionInfo) {
		fprintf(stderr,"Error: couldn't get device partition or geometry information\n");
		close(fd);
		return B_ERROR;
	}
	
	if (!IsRawDevice && !HasPartitionInfo) {
		fprintf(stderr,"Warning: couldn't get partition information\n");
	}
	if (	(HasPartitionInfo && PartitionInfo.logical_block_size != 512) 
		||	(HasBiosGeometry && BiosGeometry.bytes_per_sector != 512) 
		||	(HasDeviceGeometry && DeviceGeometry.bytes_per_sector != 512)) {
		fprintf(stderr,"Error: block size not 512 bytes\n");
		close(fd);
		return B_ERROR;
	}
	if (HasDeviceGeometry && DeviceGeometry.read_only) {
		fprintf(stderr,"Error: this is a read-only device\n");
		close(fd);
		return B_ERROR;
	}
	if (HasDeviceGeometry && DeviceGeometry.write_once) {
		fprintf(stderr,"Error: this is a write-once device\n");
		close(fd);
		return B_ERROR;
	}
	uint64 size = 0;

	if (HasPartitionInfo) {
		size = PartitionInfo.size;
	} else if (HasDeviceGeometry) {
		size = uint64(DeviceGeometry.bytes_per_sector) * DeviceGeometry.sectors_per_track * DeviceGeometry.cylinder_count * DeviceGeometry.head_count;
	} else if (HasBiosGeometry) {
		size = uint64(BiosGeometry.bytes_per_sector) * BiosGeometry.sectors_per_track * BiosGeometry.cylinder_count * BiosGeometry.head_count;
	}

	if (IsRawDevice && size > FLOPPY_MAX_SIZE) {
		fprintf(stderr,"Error: device too large for floppy, or raw devices not supported\n");
		close(fd);
		return B_ERROR;
	}

	printf("size = %Ld bytes (%Ld sectors), %Ld KB, %Ld MB, %Ld GB\n",
		size,
		size / 512,
		size / 1024,
		size / (1024 * 1024),
		size / (1024 * 1024 * 1024));
	
	if (fatbits == 0) {
		//auto determine fat type
		if (IsRawDevice && size <= FLOPPY_MAX_SIZE && (size / FAT12_CLUSTER_MAX_SIZE) < FAT12_MAX_CLUSTER_COUNT) {
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

	int SectorPerCluster;

	SectorPerCluster = 0;
	if (fatbits == 12) {
		SectorPerCluster = 0;
		if (size <= 4182016LL)
			SectorPerCluster = 2;	// XXX don't know the correct value
		if (size <= 2091008LL)
			SectorPerCluster = 1;	// XXX don't know the correct value
	} else if (fatbits == 16) {
		// special BAD_CLUSTER value is 0xFFF7, 
		// but this should work anyway, since space required by 
		// two FATs will make maximum cluster count smaller.
		// at least, this is what I think *should* happen
		SectorPerCluster = 0;				//larger than 2 GB must fail
		if (size <= (2048 * 1024 * 1024LL))	// up to 2GB, use 32k clusters
			SectorPerCluster = 64;
		if (size <= (1024 * 1024 * 1024LL))	// up to 1GB, use 16k clusters
			SectorPerCluster = 32;
		if (size <= (512 * 1024 * 1024LL))	// up to 512MB, use 8k clusters
			SectorPerCluster = 16;
		if (size <= (256 * 1024 * 1024LL))	// up to 256MB, use 4k clusters
			SectorPerCluster = 8;
		if (size <= (128 * 1024 * 1024LL))	// up to 128MB, use 2k clusters
			SectorPerCluster = 4;
		if (size <= (16 * 1024 * 1024LL))	// up to 16MB, use 2k clusters
			SectorPerCluster = 2;
		if (size <= 4182016LL)				// smaller than fat32 must fail
			SectorPerCluster = 0;
	} if (fatbits == 32) {
		SectorPerCluster = 64;				// default is 32k clusters
		if (size <= (32 * 1024 * 1024 * 1024LL))	// up to 32GB, use 16k clusters
			SectorPerCluster = 32;
		if (size <= (16 * 1024 * 1024 * 1024LL))	// up to 16GB, use 8k clusters
			SectorPerCluster = 16;
		if (size <= (8 * 1024 * 1024 * 1024LL))		// up to 8B, use 4k clusters
			SectorPerCluster = 8;
		if (size <= (532480 * 512LL))				// up to 260 MB, use 0.5k clusters
			SectorPerCluster = 1;
		if (size <= (66600 * 512LL))		// smaller than 32.5 MB must fail
			SectorPerCluster = 0;
	}

	if (SectorPerCluster == 0) {
		fprintf(stderr,"Error: failed to determine sector per cluster value, partition too large for %d bit fat\n",fatbits);
		close(fd);
		return B_ERROR;
	}

	int ReservedSectorCount = 0; // avoid compiler warning
	int RootEntryCount = 0; // avoid compiler warning
	int NumFATs;
	int SectorSize;
	uint8 BiosDriveId;

	// get bios drive-id, or use 0x80
	if (B_OK != ioctl(fd,B_GET_BIOS_DRIVE_ID,&BiosDriveId)) {
		BiosDriveId = 0x80;
	} else {
		printf("bios drive id: 0x%02x\n",(int)BiosDriveId);
	}

	// default parameters for the bootsector
	NumFATs = 2;
	SectorSize = 512;
	if (fatbits == 12 || fatbits == 16)
		ReservedSectorCount = 1;
	if (fatbits == 32)
		ReservedSectorCount = 32;
	if (fatbits == 12)
		RootEntryCount = 128; // XXX don't know the correct value
	if (fatbits == 16)
		RootEntryCount = 512;
	if (fatbits == 32)
		RootEntryCount = 0;

	// Determine FATSize
	// calculation done as MS recommends
	uint64 DskSize = size / SectorSize;
	uint32 RootDirSectors = ((RootEntryCount * 32) + (SectorSize - 1)) / SectorSize;
	uint64 TmpVal1 = DskSize - (ReservedSectorCount + RootDirSectors);
	uint64 TmpVal2 = (256 * SectorPerCluster) + NumFATs;
	if (fatbits == 32)
		TmpVal2 = TmpVal2 / 2;
	uint32 FATSize = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
	// FATSize should now contain the size of *one* FAT, measured in sectors
	// RootDirSectors should now contain the size of the fat12/16 root directory, measured in sectors

	printf("fatbits = %d, clustersize = %d\n",fatbits,SectorPerCluster * 512);
	printf("FAT size is %ld sectors\n",FATSize);
	printf("disk label: %s\n",label);

	char bootsector[512];
	memset(bootsector,0x00,512);
	memcpy(bootsector + BOOTJMP_START_OFFSET,bootjmp,sizeof(bootjmp));
	memcpy(bootsector + BOOTCODE_START_OFFSET,bootcode,sizeof(bootcode));
	
	if (fatbits == 32) {
		bootsector32 *bs = (bootsector32 *)bootsector;
		uint16 temp16;
		uint32 temp32;
		memcpy(bs->BS_OEMName,"OpenBeOS",8);
		bs->BPB_BytsPerSec = B_HOST_TO_LENDIAN_INT16(SectorSize);
		bs->BPB_SecPerClus = SectorPerCluster;
		bs->BPB_RsvdSecCnt = B_HOST_TO_LENDIAN_INT16(ReservedSectorCount);
		bs->BPB_NumFATs = NumFATs;
		bs->BPB_RootEntCnt = B_HOST_TO_LENDIAN_INT16(RootEntryCount);
		bs->BPB_TotSec16 = B_HOST_TO_LENDIAN_INT16(0);
		bs->BPB_Media = 0xF8;
		bs->BPB_FATSz16 = B_HOST_TO_LENDIAN_INT16(0);
		temp16 = HasBiosGeometry ? BiosGeometry.sectors_per_track : 63;
		bs->BPB_SecPerTrk = B_HOST_TO_LENDIAN_INT16(temp16);
		temp16 = HasBiosGeometry ? BiosGeometry.head_count : 255;
		bs->BPB_NumHeads = B_HOST_TO_LENDIAN_INT16(temp16);
		temp32 = HasPartitionInfo ? (PartitionInfo.size / 512) : 0;
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
		bs->BS_DrvNum = BiosDriveId;
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
		memcpy(bs->BS_OEMName,"OpenBeOS",8);
		bs->BPB_BytsPerSec = B_HOST_TO_LENDIAN_INT16(SectorSize);
		bs->BPB_SecPerClus = SectorPerCluster;
		bs->BPB_RsvdSecCnt = B_HOST_TO_LENDIAN_INT16(ReservedSectorCount);
		bs->BPB_NumFATs = NumFATs;
		bs->BPB_RootEntCnt = B_HOST_TO_LENDIAN_INT16(RootEntryCount);
		temp16 = (sectorcount <= 65535) ? sectorcount : 0;
		bs->BPB_TotSec16 = B_HOST_TO_LENDIAN_INT16(temp16);
		bs->BPB_Media = 0xF8;
		bs->BPB_FATSz16 = B_HOST_TO_LENDIAN_INT16(FATSize);
		temp16 = HasBiosGeometry ? BiosGeometry.sectors_per_track : 63;
		bs->BPB_SecPerTrk = B_HOST_TO_LENDIAN_INT16(temp16);
		temp16 = HasBiosGeometry ? BiosGeometry.head_count : 255;
		bs->BPB_NumHeads = B_HOST_TO_LENDIAN_INT16(temp16);
		temp32 = HasPartitionInfo ? (PartitionInfo.size / 512) : 0;
		bs->BPB_HiddSec = B_HOST_TO_LENDIAN_INT32(temp32);
		temp32 = (sectorcount <= 65535) ? 0 : sectorcount;
		bs->BPB_TotSec32 = B_HOST_TO_LENDIAN_INT32(temp32);
		bs->BS_DrvNum = BiosDriveId;
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
		scanf("%s",answer); //XXX who wants to fix this buffer overflow?
		if (0 != strcasecmp(answer,"yes")) {
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
	int64 bytes_to_write = 512LL * (ReservedSectorCount + (NumFATs * FATSize) + RootDirSectors);
	int64 pos = 0;
	while (bytes_to_write > 0) {
		ssize_t writesize = min_c(bytes_to_write,65536);
		written = file.WriteAt(pos,zerobuffer,writesize);
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
	written = file.WriteAt(BOOT_SECTOR_NUM * 512,bootsector,512);
	if (written != 512) {
		fprintf(stderr,"Error: write error at sector %d\n",BOOT_SECTOR_NUM);
		close(fd);
		return B_ERROR;
	}

	if (fatbits == 32) {
		written = file.WriteAt(BACKUP_SECTOR_NUM * 512,bootsector,512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %d\n",BACKUP_SECTOR_NUM);
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
	written = file.WriteAt(ReservedSectorCount * 512,sec,512);
	if (written != 512) {
		fprintf(stderr,"Error: write error at sector %d\n",ReservedSectorCount);
		close(fd);
		return B_ERROR;
	}
	if (NumFATs > 1) {
		written = file.WriteAt((ReservedSectorCount + FATSize) * 512,sec,512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %ld\n",ReservedSectorCount + FATSize);
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
		free_count -= ReservedSectorCount + (NumFATs * FATSize) + RootDirSectors;
		//convert from sector to clustercount
		free_count /= SectorPerCluster;
		//and account for 1 already used cluster of root directory
		free_count -= 1; 
		fsinfosector32 fsinfosector;
		memset(&fsinfosector,0x00,512);
		fsinfosector.FSI_LeadSig 	= B_HOST_TO_LENDIAN_INT32(0x41615252);
		fsinfosector.FSI_StrucSig 	= B_HOST_TO_LENDIAN_INT32(0x61417272);
		fsinfosector.FSI_Free_Count = B_HOST_TO_LENDIAN_INT32((uint32)free_count);
		fsinfosector.FSI_Nxt_Free 	= B_HOST_TO_LENDIAN_INT32(3);
		fsinfosector.FSI_TrailSig 	= B_HOST_TO_LENDIAN_INT32(0xAA550000);
		written = file.WriteAt(FSINFO_SECTOR_NUM * 512,&fsinfosector,512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %d\n",FSINFO_SECTOR_NUM);
			close(fd);
			return B_ERROR;
		}
	}

	//write volume label into root directory
	printf("Writing root directory\n");
	if (fatbits == 12 || fatbits == 16) {
		uint8 data[512];
		memset(data,0,512);
		CreateVolumeLabel(data,label);
		uint32 RootDirSector = ReservedSectorCount + (NumFATs * FATSize);
		written = file.WriteAt(RootDirSector * 512,data,512);
		if (written != 512) {
			fprintf(stderr,"Error: write error at sector %ld\n",RootDirSector);
			close(fd);
			return B_ERROR;
		}
	} else if (fatbits == 32) {
		int size = 512 * SectorPerCluster;
		uint8 *cluster = (uint8*)malloc(size);
		memset(cluster,0,size);
		CreateVolumeLabel(cluster,label);
		uint32 RootDirSector = ReservedSectorCount + (NumFATs * FATSize) + RootDirSectors;
		written = file.WriteAt(RootDirSector * 512,cluster,size);
		free(cluster);
		if (written != size) {
			fprintf(stderr,"Error: write error at sector %ld\n",RootDirSector);
			close(fd);
			return B_ERROR;
		}
	}

	ioctl(fd,B_FLUSH_DRIVE_CACHE);
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
	memset(d,0,sizeof(*d));
	memset(d->Name,0x20,11);
	memcpy(d->Name,label,min_c(11,strlen(label)));
	d->Attr = 0x08;
}

void PrintUsage()
{
	printf("\n");
	printf("usage: mkdos [-n] [-t] [-f 12|16|32] device [volume_label]\n");
	printf("       -n, --noprompt  do not prompt before writing\n");
	printf("       -t, --test      enable test mode (will not write to disk)\n");
	printf("       -f, --fat       use FAT entries of the specified size\n");
}       
