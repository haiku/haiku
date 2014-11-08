#ifndef DISPLAY_ADAPTER_H
#define DISPLAY_ADAPTER_H


#include <sys/cdefs.h>

#include <device_manager.h>
#include <KernelExport.h>
//#include <Drivers.h>
//#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <ACPI.h>

#define DISPLAYADAPTER_MODULE_NAME "drivers/display_adapter/display_adapter/driver_v1"
#define DISPLAY_DEVICE_MODULE_NAME "drivers/display_adapter/display_adapter/display/device_v1"
#define DISPLAYADAPTER_DEVICE_MODULE_NAME "drivers/display_adapter/display_adapter/device_v1"

#define DISPLAYADAPTER_BASENAME "display_adapter/%d"
#define DISPLAYADAPTER_PATHID_GENERATOR "display_adapter/path_id"


#define OS_DISPLAY_SWITCH 0
#define BIOS_DISPLAY_SWITCH 1
#define LOCK_DISPLAY_SWITCH 2
#define NOTIFY_DISPLAY_SWITCH 3

#define OS_BRIGHTNESS_CONTROL (1 << 2)
#define BIOS_BRIGHTNESS_CONTROL (0 << 2)


__BEGIN_DECLS

extern device_manager_info *gDeviceManager;
extern acpi_module_info *gAcpi;

extern struct device_module_info display_device_module;

__END_DECLS


#endif //DISPLAY_ADAPTER_H
