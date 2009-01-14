#ifndef _EST_H
#define _EST_H

#include <KernelExport.h>
#include <ACPI.h>

#include "frequency.h"

// Model Specific Register
#define MSR_MISC				0x1a0 
#define MSR_EST_ENABLED			(1<<16) 


struct est_cookie {
	// this three variables are not needed yet but helpfull when extend this
	// driver to use acpi
	device_node				*node;
	acpi_device_module_info	*acpi;
	acpi_device				acpi_cookie;
	
	// array of states don't delete it
	freq_info*				available_states;
	uint8					number_states;
	
	vint32					stop_watching;
};


#endif /* _EST_H */
