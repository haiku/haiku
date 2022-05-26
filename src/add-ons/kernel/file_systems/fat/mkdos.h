/*

mkdos shell tool

Initialize FAT16 or FAT32 partitions, FAT12 floppy disks not supported

Copyright (c) 2002 Marcus Overhagen <marcus@overhagen.de>, Haiku project

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

#include "system_dependencies.h"


struct initialize_parameters {
	uint32	fatBits;
	uint32	flags;
	bool	verbose;
};

#ifdef __cplusplus
extern "C" {
#endif

status_t
dosfs_initialize(int fd, partition_id partitionID, const char* name,
	const char* parameterString, off_t /*partitionSize*/, disk_job_id job);
status_t
dosfs_uninitialize(int fd, partition_id partitionID, off_t partitionSize,
	uint32 blockSize, disk_job_id job);

#ifdef __cplusplus
}
#endif


#ifdef MKDOS

#define ATTRIBUTE_PACKED __attribute__((packed))

struct bootsector1216 {
	uint8 	BS_jmpBoot[3];
	uint8 	BS_OEMName[8];
	uint16 	BPB_BytsPerSec; 
	uint8 	BPB_SecPerClus;
	uint16	BPB_RsvdSecCnt;
	uint8 	BPB_NumFATs;
	uint16 	BPB_RootEntCnt;
	uint16 	BPB_TotSec16;
	uint8 	BPB_Media;
	uint16 	BPB_FATSz16;
	uint16 	BPB_SecPerTrk;
	uint16 	BPB_NumHeads;
	uint32 	BPB_HiddSec;
	uint32 	BPB_TotSec32;
	uint8 	BS_DrvNum;
	uint8 	BS_Reserved1;
	uint8 	BS_BootSig;
	uint8 	BS_VolID[4];
	uint8 	BS_VolLab[11];
	uint8	BS_FilSysType[8];
	uint8   bootcode[448];
	uint16	signature;
} ATTRIBUTE_PACKED;

struct bootsector32 {
	uint8 	BS_jmpBoot[3];
	uint8 	BS_OEMName[8];
	uint16 	BPB_BytsPerSec;
	uint8 	BPB_SecPerClus;
	uint16	BPB_RsvdSecCnt;
	uint8 	BPB_NumFATs;
	uint16 	BPB_RootEntCnt;
	uint16 	BPB_TotSec16;
	uint8 	BPB_Media;
	uint16 	BPB_FATSz16;
	uint16 	BPB_SecPerTrk;
	uint16 	BPB_NumHeads;
	uint32 	BPB_HiddSec;
	uint32 	BPB_TotSec32; 
	uint32 	BPB_FATSz32;
	uint16 	BPB_ExtFlags;
	uint16 	BPB_FSVer;
	uint32 	BPB_RootClus;
	uint16 	BPB_FSInfo;
	uint16 	BPB_BkBootSec;
	uint8 	BPB_Reserved[12];
	uint8 	BS_DrvNum;
	uint8 	BS_Reserved1;
	uint8 	BS_BootSig;
	uint8 	BS_VolID[4];
	uint8 	BS_VolLab[11];
	uint8 	BS_FilSysType[8];
	uint8   bootcode[420];
	uint16	signature;
} ATTRIBUTE_PACKED;

struct fsinfosector32 {
	uint32	FSI_LeadSig;
	uint8	FSI_Reserved1[480];
	uint32	FSI_StrucSig;
	uint32	FSI_Free_Count;
	uint32	FSI_Nxt_Free;
	uint8	FSI_Reserved2[12];
	uint32	FSI_TrailSig;
} ATTRIBUTE_PACKED;


// a FAT directory entry
struct fatdirent {
	uint8 Name[11];
	uint8 Attr;
	uint8 NTRes;
	uint8 CrtTimeTenth;
	uint16 CrtTime;
	uint16 CrtDate;
	uint16 LstAccDate;
	uint16 FstClusHI;
	uint16 WrtTime;
	uint16 WrtDate;
	uint16 FstClusLO;
	uint32 FileSize;
} ATTRIBUTE_PACKED;


//maximum size of a 2,88 MB floppy 
#define FLOPPY_MAX_SIZE (2 * 80 * 36 * 512)

//maximum size of a cluster
#define CLUSTER_MAX_SIZE 32768

//not sure if that's correct
#define FAT12_CLUSTER_MAX_SIZE	1024

//limits
#define FAT12_MAX_CLUSTER_COUNT 4084LL
#define FAT16_MAX_CLUSTER_COUNT 65524LL
#define FAT32_MAX_CLUSTER_COUNT 0x0fffffffLL


#define BOOTJMP_START_OFFSET 0x00
uint8 bootjmp[] = {
	0xeb, 0x7e, 0x90
};

#define BOOTCODE_START_OFFSET 0x80
uint8 bootcode[] = {
	0x31, 0xc0, 0x8e, 0xd0, 0xbc, 0x00, 0x7c, 0x8e, 
	0xd8, 0xb4, 0x0e, 0x31, 0xdb, 0xbe, 0x00, 0x7d,
	0xfc, 0xac, 0x08, 0xc0, 0x74, 0x04, 0xcd, 0x10, 
	0xeb, 0xf7, 0xb4, 0x00, 0xcd, 0x16, 0xcd, 0x19,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x53, 0x6f, 0x72, 0x72, 0x79, 0x2c, 0x20, 0x74, 
	0x68, 0x69, 0x73, 0x20, 0x64, 0x69, 0x73, 0x6b,
	0x20, 0x69, 0x73, 0x20, 0x6e, 0x6f, 0x74, 0x20, 
	0x62, 0x6f, 0x6f, 0x74, 0x61, 0x62, 0x6c, 0x65,
	0x2e, 0x0d, 0x0a, 0x50, 0x6c, 0x65, 0x61, 0x73, 
	0x65, 0x20, 0x70, 0x72, 0x65, 0x73, 0x73, 0x20,
	0x61, 0x6e, 0x79, 0x20, 0x6b, 0x65, 0x79, 0x20, 
	0x74, 0x6f, 0x20, 0x72, 0x65, 0x62, 0x6f, 0x6f,
	0x74, 0x2e, 0x0d, 0x0a, 0x00
};

#define BOOT_SECTOR_NUM 0
#define FSINFO_SECTOR_NUM 1
#define BACKUP_SECTOR_NUM 6
#define FAT32_ROOT_CLUSTER 2

#endif
