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
	uint32				flags;

	void *				cookie;

// private:
	int32				open_count;
} audio_drv_t;
 
typedef void stream_id;
typedef void control_id;

typedef struct {
	int32	vendor;
	int32	device;
	int32	revision;
	int32	class;
	int32	subclass;
	int32	subsystem_vendor;
	int32	subsystem_device;
	const char *name;
	uint32	flags;
} id_table_t;


typedef status_t (*drv_attach)		(audio_drv_t *drv);
typedef status_t (*drv_powerctl)	(audio_drv_t *drv);
typedef status_t (*drv_detach)		(audio_drv_t *drv);


typedef struct 
{
	id_table_t *	table;
	const char *	basename;
	size_t			cookie_size;

	uint32			_reserved1[16];

	drv_attach		attach;
	drv_detach		detach;
	drv_powerctl	powerctl;

	uint32			_reserved2[16];
} driver_info_t;


typedef struct {
	char *	base;		/* pointer to first sample for channel for buffer */
	size_t	stride;		/* offset to next sample */
	uint32	_reserved[4];
} stream_buffer_desc_t;


typedef struct {
	uint32	frame_rate;
	uint32	buffer_size;
	uint32	_reserved[16];
} stream_config_t;


typedef status_t (*stream_attach)			(audio_drv_t *drv, void *stream_cookie);
typedef status_t (*stream_detach)			(audio_drv_t *drv, void *stream_cookie);
typedef status_t (*stream_control)			(audio_drv_t *drv, void *stream_cookie, int op);
typedef status_t (*stream_get_buffer_info)	(audio_drv_t *drv, void *stream_cookie, stream_buffer_desc_t *desc);
typedef status_t (*stream_set_config)		(audio_drv_t *drv, void *stream_cookie, stream_config_t *config);


typedef struct
{
	uint32	flags;
	uint32	cookie_size;
	uint32	user_param;
	uint32	user_cookie;
	uint32	sample_bits;
	uint32	sample_flags;
	uint32	channel_count;
	uint32	channel_flags;
	uint32	frame_rate_min;
	uint32	frame_rate_max;
	uint32	frame_rate_flags;
	uint32	buffer_size_min;
	uint32	buffer_size_max;
	uint32	buffer_size_flags;
	
	uint32	_reserved1[32];

	stream_attach			attach;
	stream_detach			detach;
	stream_control			control;
	
	stream_get_buffer_info	get_buffer_info;

	stream_set_config		set_config;

	uint32					_reserved2[16];
	
} stream_info_t;

extern driver_info_t driver_info;

// protection is 0 for kernel only access, or B_READ_AREA, B_WRITE_AREA for user space access
area_id		alloc_mem(void **virt, void **phy, size_t size, uint32 protection, const char *name);
area_id		map_mem(void **virt, void *phy, size_t size, uint32 protection, const char *name);

stream_id	create_stream(audio_drv_t *dev, stream_info_t *info, void *config);
status_t	delete_stream(stream_id stream);

void		report_stream_event(stream_id stream, int );


control_id	create_control_group(control_id parent, audio_drv_t *dev);
control_id	create_control(control_id parent, uint32 flags, void *get, void *set, const char *name);


#define LOG(a)		dprintf a
#define PRINT(a)	dprintf a

#endif
