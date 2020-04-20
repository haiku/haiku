/*
 * Copyright 2020, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef WMI_PRIVATE_H
#define WMI_PRIVATE_H


#include <ACPI.h>
#include <new>
#include <stdio.h>
#include <string.h>

#include <lock.h>
#include <smbios.h>
#include <util/AutoLock.h>
#include <wmi.h>


//#define WMI_TRACE
#ifndef DRIVER_NAME
#	define DRIVER_NAME "wmi"
#endif
#ifdef WMI_TRACE
#	define TRACE(x...)		dprintf("\33[33m" DRIVER_NAME ":\33[0m " x)
#else
#	define TRACE(x...)
#endif
#define TRACE_ALWAYS(x...)	dprintf("\33[33m" DRIVER_NAME ":\33[0m " x)
#define ERROR(x...)			TRACE_ALWAYS(x)
#define CALLED() 			TRACE("CALLED %s\n", __PRETTY_FUNCTION__)


#define WMI_ACPI_DRIVER_NAME "drivers/wmi/acpi/driver_v1"
#define WMI_DEVICE_MODULE_NAME "drivers/wmi/device/driver_v1"

#define WMI_ASUS_DRIVER_NAME "drivers/wmi/asus/driver_v1"

#define WMI_BUS_COOKIE "wmi/bus/index"


class WMIACPI;
class WMIDevice;


extern device_manager_info *gDeviceManager;
extern smbios_module_info *gSMBios;
extern wmi_device_interface gWMIDeviceModule;
extern driver_module_info gWMIAsusDriverModule;


class WMIDevice {
public:
								WMIDevice(device_node* node);
								~WMIDevice();

			status_t			InitCheck();
			status_t			EvaluateMethod(uint8 instance, uint32 methodId,
									const acpi_data* in, acpi_data* out);
			status_t			InstallEventHandler(const char* guidString,
									acpi_notify_handler handler, void* context);
			status_t			RemoveEventHandler(const char* guidString);
			status_t			GetEventData(uint32 notify, acpi_data* out);
			const char*			GetUid();
private:

private:
			device_node*		fNode;
			WMIACPI* 			fBus;
			uint32				fBusCookie;
			status_t			fInitStatus;
};


typedef struct {
	uint8	guid[16];
	union {
		char	oid[2];
		struct {
			uint8 notify_id;
		};
	};
	uint8	max_instance;
	uint8	flags;
} guid_info;


struct wmi_info;
typedef struct wmi_info : DoublyLinkedListLinkImpl<wmi_info> {
	guid_info			guid;
	acpi_notify_handler	handler;
	void*				handler_context;
} wmi_info;
typedef DoublyLinkedList<wmi_info> WMIInfoList;



class WMIACPI {
public:
								WMIACPI(device_node *node);
								~WMIACPI();

			status_t			InitCheck();

			status_t			Scan();

			status_t			GetBlock(uint32 busCookie,
									uint8 instance, uint32 methodId,
									acpi_data* out);
			status_t			SetBlock(uint32 busCookie,
									uint8 instance, uint32 methodId,
									const acpi_data* in);
			status_t			EvaluateMethod(uint32 busCookie,
									uint8 instance, uint32 methodId,
									const acpi_data* in, acpi_data* out);
			status_t			InstallEventHandler(const char* guidString,
									acpi_notify_handler handler, void* context);
			status_t			RemoveEventHandler(const char* guidString);
			status_t			GetEventData(uint32 notify, acpi_data* out);
			const char*			GetUid(uint32 busCookie);
private:
			status_t			_SetEventGeneration(wmi_info* info,
									bool enabled);
			status_t			_EvaluateMethodSimple(const char* method,
									uint64 integer);
			void				_Notify(acpi_handle device, uint32 value);
	static	void				_NotifyHandler(acpi_handle device,
									uint32 value, void *context);

			void				_GuidToGuidString(uint8 guid[16],
									char* guidString);
private:
			device_node*		fNode;
			acpi_device_module_info* acpi;
			acpi_device			acpi_cookie;
			status_t			fStatus;
			WMIInfoList			fList;
			wmi_info*			fWMIInfos;
			uint32				fWMIInfoCount;
			const char*			fUid;
};


#endif // WMI_PRIVATE_H
