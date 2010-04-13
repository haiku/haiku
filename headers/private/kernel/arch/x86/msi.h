#ifndef _KERNEL_ARCH_x86_MSI_H
#define _KERNEL_ARCH_x86_MSI_H

#include <SupportDefs.h>

// address register
#define MSI_ADDRESS_BASE				0xfee00000
#define MSI_DESTINATION_ID_SHIFT		12
#define MSI_REDIRECTION					0x00000008
#define MSI_NO_REDIRECTION				0x00000000
#define MSI_DESTINATION_MODE_LOGICAL	0x00000004
#define MSI_DESTINATION_MODE_PHYSICAL	0x00000000

// data register
#define MSI_TRIGGER_MODE_EDGE			0x00000000
#define MSI_TRIGGER_MODE_LEVEL			0x00008000
#define MSI_LEVEL_DEASSERT				0x00000000
#define MSI_LEVEL_ASSERT				0x00004000
#define MSI_DELIVERY_MODE_FIXED			0x00000000
#define MSI_DELIVERY_MODE_LOWEST_PRIO	0x00000100
#define MSI_DELIVERY_MODE_SMI			0x00000200
#define MSI_DELIVERY_MODE_NMI			0x00000400
#define MSI_DELIVERY_MODE_INIT			0x00000500
#define MSI_DELIVERY_MODE_EXT_INT		0x00000700


void		msi_init();
bool		msi_supported();
status_t	msi_allocate_vectors(uint8 count, uint8 *startVector,
				uint64 *address, uint16 *data);
void		msi_free_vectors(uint8 count, uint8 startVector);

#endif // _KERNEL_ARCH_x86_MSI_H
