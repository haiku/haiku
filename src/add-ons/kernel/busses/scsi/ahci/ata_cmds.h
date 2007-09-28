#ifndef _ATA_CMDS_H
#define _ATA_CMDS_H

#include <SupportDefs.h>

struct ata_res_identify_device
{
	union {
		uint16 		words[256];
		struct {
			uint16	data1[10];
			char	serial_number[20];
			uint16	data2[3];
			char	firmware_revision[8];
			char	model_number[40];
		};
	};
};

#endif
