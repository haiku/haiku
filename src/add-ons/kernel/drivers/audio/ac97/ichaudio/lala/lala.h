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
	drv_attach		attach;
	drv_detach		detach;
	drv_powerctl	powerctl;
} driver_info_t;

typedef struct
{
} stream_info_t;

extern driver_info_t driver_info;

// protection is 0 for kernel only access, or B_READ_AREA, B_WRITE_AREA for user space access
area_id		alloc_mem(void **virt, void **phy, size_t size, uint32 protection, const char *name);
area_id		map_mem(void **virt, void *phy, size_t size, uint32 protection, const char *name);

stream_id	create_stream(audio_drv_t *dev, stream_info_t *info);
status_t	control_stream(audio_drv_t *dev, stream_info_t *info);
status_t	delete_stream(stream_id stream);

control_id	create_control_group(control_id parent, audio_drv_t *dev);
control_id	create_control(control_id parent, uint32 flags, void *get, void *set, const char *name);


#define LOG(a)		dprintf a
#define PRINT(a)	dprintf a
