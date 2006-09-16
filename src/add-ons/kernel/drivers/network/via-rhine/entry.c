/*
 * VIA VT86C100A Rhine-II and VIA VT3043 Rhine Based Card Driver
 * for the BeOS Release 5
 */
 
/*
 * Standard Libraries
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/*
 * Driver-Related Libraries
 */
#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>

/*
 * VIA Rhine Libraries
 */
#include "via-rhine.h"

/*
 * Standard Driver Entry Functions
 */
status_t       init_driver    (void);
void           uninit_driver  (void);
const char   **publish_devices(void);
device_hooks  *find_device    (const char *name);

char            *pDevNameList[MAX_CARDS+1];
pci_info        *pDevList    [MAX_CARDS+1];
pci_module_info *pModuleInfo;

const char **publish_devices(void)
{
#ifdef __VIARHINE_DEBUG__
	debug_printf("publish_devices\n");
#endif
	return (const char **)pDevNameList;
}

status_t init_hardware(void )
{
#ifdef __VIARHINE_DEBUG__
	debug_printf("init_hardware\n");
#endif
	return B_NO_ERROR;
}

status_t init_driver()
{
	status_t  status;
	int32     entries;
	char      devName[64];
	int32     i;

#ifdef __VIARHINE_DEBUG__
	debug_printf("init_driver\n");
#endif

	if ((status = get_module(B_PCI_MODULE_NAME, (module_info **)&pModuleInfo)) != B_OK)
	{
#ifdef __VIARHINE_DEBUG__
		debug_printf(DevName ": init_driver: get_module failed! (%s)\n", strerror(status));
#endif
		return status;
	}

	// Find VIA Rhine Cards
	if ((entries = pci_getlist(pDevList, MAX_CARDS)) == 0)
	{
#ifdef __VIARHINE_DEBUG__
		debug_printf(DevName ": init_driver: Device(s) Not Found\n");
#endif
		pci_freelist(pDevList);
		put_module  (B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	// Create Device Name List
	for (i = 0; i < entries; i++)
	{
		sprintf(devName, "%s%ld", DevDir, i );
		pDevNameList[i] = (char*)malloc(strlen(devName) + 1);
		strcpy(pDevNameList[i], devName);
	}
	
	pDevNameList[i] = NULL;
	return B_OK;
}

void uninit_driver(void)
{
	int32  i;
	void  *item;

#ifdef __VIARHINE_DEBUG__
	debug_printf("uninit_driver\n");
#endif

	// Free Device Name List
	for (i = 0; (item = pDevNameList[i]); i++)
		free(item);
	
	// Free Device List
	pci_freelist(pDevList);
	put_module  (B_PCI_MODULE_NAME);
}

device_hooks *find_device(const char *name)
{
	int32 	i;
	char 	*item;

	if (name == NULL)
		return NULL;

	// Find Device Name
	for (i = 0; (item = pDevNameList[i]); i++)
	{
		if (strcmp(name, item) == 0)
		{
#ifdef __VIARHINE_DEBUG__
			debug_printf("find_device: Device Found (%s)\n", name);
#endif
			return &pDeviceHooks;
		}
	}

#ifdef __VIARHINE_DEBUG__
	debug_printf("find_device: Device (%s) Not Found\n", name);
#endif
	return NULL;
}

int32 api_version = B_CUR_DRIVER_API_VERSION;
