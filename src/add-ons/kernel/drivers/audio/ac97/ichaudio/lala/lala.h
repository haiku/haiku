#include <KernelExport.h>
#include <config_manager.h>
#include <PCI.h>
#include <OS.h>
#include <malloc.h>

typedef void drv_t;
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

typedef status_t (*drv_attach)		(drv_t *dev, void **cookie, int id_table_index);
typedef status_t (*drv_powerctl)	(void *cookie);
typedef status_t (*drv_detach)		(void *cookie);

typedef struct 
{
	id_table_t *	table;
	const char *	basename;
	drv_attach		attach;
	drv_detach		detach;
	drv_powerctl	powerctl;
} driver_info_t;

typedef struct
{
} stream_info_t;

extern driver_info_t driver_info;

stream_id	create_stream(drv_t *dev, stream_info_t *info);
status_t	control_stream(drv_t *dev, stream_info_t *info);
status_t	delete_stream(stream_id stream);

control_id	create_control_group(control_id parent, dev_t *dev);
control_id	create_control(control_id parent, uint32 flags, void *get, void *set, const char *name);

