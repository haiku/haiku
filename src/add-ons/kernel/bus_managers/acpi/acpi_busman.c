#include <ACPI.h>
#include <KernelExport.h>

#include "acpi.h"
#include "acpixf.h"

status_t acpi_std_ops(int32 op,...);
status_t acpi_rescan_stub(void);
uint32 read_acpi_reg(uint32 id);
void write_acpi_reg(uint32 id, uint32 value);

struct acpi_module_info acpi_module = {
	{
		{
			B_ACPI_MODULE_NAME,
			0,
			acpi_std_ops
		},
		
		acpi_rescan_stub
	},
	
	read_acpi_reg,
	write_acpi_reg
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

uint32 read_acpi_reg(uint32 id) {
	uint32 value;
	AcpiGetRegister(id,&value,ACPI_MTX_LOCK);
	return value;
}

void write_acpi_reg(uint32 id, uint32 value) {
	AcpiSetRegister(id,value,ACPI_MTX_LOCK);
}