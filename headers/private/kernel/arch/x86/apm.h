#ifndef KERNEL_APM_H
#define KERNEL_APM_H


#include <SupportDefs.h>


typedef struct apm_info {
	uint16	version;
	uint16	flags;
	uint16	code32_segment_base;
	uint32	code32_segment_offset;
	uint16	code32_segment_length;
	uint16	code16_segment_base;
	uint16	code16_segment_length;
	uint16	data_segment_base;
	uint16	data_segment_length;
} apm_info;


#endif	/* KERNEL_APM_H */
