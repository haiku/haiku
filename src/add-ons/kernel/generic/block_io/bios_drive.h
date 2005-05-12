#ifndef __BIOS_INFO_H__
#define __BIOS_INFO_H__

// length: 0x84
typedef struct tagbios_drive {
	char name[32];				// 0
	uint8 bios_id;				// 20
	uint8 padding[3];
	uint32 cylinder_count;		// 24
	uint32 head_count;			// 28
	uint32 sectors_per_track;	// 2c
	
	uint32 num_chksums;			// 30
	
	struct {
		uint64 offset;			// 34+
		uint32 len;				// 3c+
		uint32 chksum;			// 40+
	} chksums[5];				// 34
} bios_drive;

extern bios_drive *bios_drive_info;

extern uint32 boot_calculate_hash( void *buffer, size_t len );

#endif
