#ifndef __LALA_H
#define __LALA_H

#include <KernelExport.h>
#include <config_manager.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>

typedef struct
{
	pci_module_info *	pci;

	uint8				bus;
	uint8				device;
	uint8				function;

	const char *		name;
	void *				param;
} audio_drv_t;
 
typedef struct
{
	int32			vendor;
	int32			device;
	int32			revision;
	int32			class;
	int32			subclass;
	int32			subsystem_vendor;
	int32			subsystem_device;
	const char *	name;
	void *			param;
} pci_id_table_t;


typedef int32 	control_id;


typedef status_t (*drv_attach)		(audio_drv_t *drv, void **cookie);
typedef status_t (*drv_powerctl)	(audio_drv_t *drv, void *cookie, int op);
typedef status_t (*drv_detach)		(audio_drv_t *drv, void *cookie);


typedef struct 
{
	const char *	basename;
	pci_id_table_t *pci_id_table;

	uint32			_reserved1[16];

	drv_attach		attach;
	drv_detach		detach;
	drv_powerctl	powerctl;

	uint32			_reserved2[16];
} driver_info_t;


typedef struct
{
	char *	base;		/* pointer to first sample for channel for buffer */
	size_t	stride;		/* offset to next sample */
	uint32	_reserved[4];
} stream_buffer_desc_t;


typedef status_t (*stream_attach)			(audio_drv_t *drv, void *cookie, int stream_id);
typedef status_t (*stream_detach)			(audio_drv_t *drv, void *cookie, int stream_id);
typedef status_t (*stream_control)			(audio_drv_t *drv, void *cookie, int stream_id, int op);
typedef status_t (*stream_process)			(audio_drv_t *drv, void *cookie, int stream_id, int buffer);
typedef status_t (*stream_set_buffers)		(audio_drv_t *drv, void *cookie, int stream_id, uint32 *buffer_size, stream_buffer_desc_t *desc);
typedef status_t (*stream_set_frame_rate)	(audio_drv_t *drv, void *cookie, int stream_id, uint32 *frame_rate);


enum {
	B_RECORDING_STREAM			= 0x00000001,
	B_PLAYBACK_STREAM			= 0x00000002,
	B_BUFFER_SIZE_LINEAR		= 0x00001000,	
	B_BUFFER_SIZE_EXPONETIAL	= 0x00002000,	
	B_SAMPLE_TYPE_INT16			= 0x00010000,	
	B_SAMPLE_TYPE_INT32			= 0x00020000,	
	B_SAMPLE_TYPE_FLOAT32		= 0x00040000,	
};

enum {
	B_CHANNEL_LEFT	= 0x0001,	
	B_CHANNEL_RIGHT	= 0x0002,	
};

enum {
	B_FRAME_RATE_32000		= 0x0100,	
	B_FRAME_RATE_44100		= 0x0200,	
	B_FRAME_RATE_48000		= 0x0400,	
	B_FRAME_RATE_96000		= 0x0800,
	B_FRAME_RATE_VARIABLE	= 0x20000000,
};

typedef struct
{
	uint32	flags;
	uint32	cookie_size;
	uint32	sample_bits;
	uint32	channel_count;
	uint32	channel_mask;
	uint32	frame_rate_min;
	uint32	frame_rate_max;
	uint32	frame_rate_mask;
	uint32	buffer_size_min;
	uint32	buffer_size_max;
	uint32	buffer_count;
	
	uint32	_reserved1[32];

	stream_attach			attach;
	stream_detach			detach;
	stream_control			control;
	stream_process			process;
	
	stream_set_buffers		set_buffers;
	stream_set_frame_rate	set_frame_rate;

	uint32					_reserved2[16];
	
} stream_info_t;


// protection is 0 for kernel only access, or B_READ_AREA, B_WRITE_AREA for user space access
area_id		alloc_mem(void **virt, void **phy, size_t size, uint32 protection, const char *name);
area_id		map_mem(void **virt, void *phy, size_t size, uint32 protection, const char *name);

status_t	create_stream(audio_drv_t *drv, int stream_id, stream_info_t *info);
status_t	delete_stream(audio_drv_t *drv, int stream_id);

void		report_stream_buffer_ready(audio_drv_t *drv, int stream_id, int buffer, bigtime_t end_time, int64 total_frames);

control_id	create_control_group(control_id parent, audio_drv_t *dev);
control_id	create_control(control_id parent, uint32 flags, void *get, void *set, const char *name);


#define LOG(a)		dprintf a
#define PRINT(a)	dprintf a

#endif
