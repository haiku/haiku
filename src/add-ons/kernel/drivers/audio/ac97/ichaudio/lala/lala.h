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

} drv_t;
 
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

typedef status_t (*drv_attach)		(drv_t *drv, void **cookie);
typedef status_t (*drv_powerctl)	(drv_t *drv, void *cookie);
typedef status_t (*drv_detach)		(drv_t *drv, void *cookie);

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

