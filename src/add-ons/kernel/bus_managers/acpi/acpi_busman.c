#include <ACPI.h>
#include <KernelExport.h>
#include <stdio.h>

#include "acpi.h"
#include "acpixf.h"

status_t acpi_std_ops(int32 op,...);
status_t acpi_rescan_stub(void);

void enable_fixed_event (uint32 event);
void disable_fixed_event (uint32 event);

uint32 fixed_event_status (uint32 event);
void reset_fixed_event (uint32 event);

status_t install_fixed_event_handler	(uint32 event, interrupt_handler *handler, void *data); 
status_t remove_fixed_event_handler	(uint32 event, interrupt_handler *handler); 

struct acpi_module_info acpi_module = {
	{
		{
			B_ACPI_MODULE_NAME,
			B_KEEP_LOADED,
			acpi_std_ops
		},
		
		acpi_rescan_stub
	},
	
	enable_fixed_event,
	disable_fixed_event,
	fixed_event_status,
	reset_fixed_event,
	install_fixed_event_handler,
	remove_fixed_event_handler
};

_EXPORT module_info *modules[] = {
	(module_info *) &acpi_module,
	NULL
};

status_t acpi_std_ops(int32 op,...) {
	ACPI_STATUS Status;
	
	switch(op) {
		case B_MODULE_INIT:
			
			#ifdef ACPI_DEBUG_OUTPUT
				AcpiDbgLevel = ACPI_DEBUG_ALL | ACPI_LV_VERBOSE;
				AcpiDbgLayer = ACPI_ALL_COMPONENTS;
			#endif
			
			/* Bring up ACPI */
			Status = AcpiInitializeSubsystem();
			if (Status != AE_OK) {
				dprintf("ACPI: AcpiInitializeSubsystem() failed (%s)",AcpiFormatException(Status));
				return B_ERROR;
			}
			Status = AcpiLoadTables();
			if (Status != AE_OK) {
				dprintf("ACPI: AcpiLoadTables failed (%s)",AcpiFormatException(Status));
				return B_ERROR;
			}
			Status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
			if (Status != AE_OK) {
				dprintf("ACPI: AcpiEnableSubsystem failed (%s)",AcpiFormatException(Status));
				return B_ERROR;
			}
			
			/* Phew. Now in ACPI mode */
			dprintf("ACPI: ACPI initialized\n");
			break;
		
		case B_MODULE_UNINIT:
			Status = AcpiTerminate();
			if (Status != AE_OK)
				dprintf("Could not bring system out of ACPI mode. Oh well.\n");
			
				/* This isn't so terrible. We'll just fail silently */
			break;
		default:
			return B_ERROR;
	}
	return B_OK;
}

status_t acpi_rescan_stub(void) {
	return B_OK;
}

void enable_fixed_event (uint32 event) {
	AcpiEnableEvent(event,0);
}

void disable_fixed_event (uint32 event) {
	AcpiDisableEvent(event,0);
}

uint32 fixed_event_status (uint32 event) {
	ACPI_EVENT_STATUS status = 0;
	AcpiGetEventStatus(event,&status);
	return (status/* & ACPI_EVENT_FLAG_SET*/);
}

void reset_fixed_event (uint32 event) {
	AcpiClearEvent(event);
}

status_t install_fixed_event_handler (uint32 event, interrupt_handler *handler, void *data) {
	return ((AcpiInstallFixedEventHandler(event,handler,data) == AE_OK) ? B_OK : B_ERROR);
}

status_t remove_fixed_event_handler	(uint32 event, interrupt_handler *handler) {
	return ((AcpiRemoveFixedEventHandler(event,handler) == AE_OK) ? B_OK : B_ERROR);
}
