#include "lala/lala.h"
#include "ichaudio.h"

status_t ichaudio_attach(audio_drv_t *drv);
status_t ichaudio_powerctl(audio_drv_t *drv);
status_t ichaudio_detach(audio_drv_t *drv);

id_table_t ichaudio_id_table[] = {
	{ 0x8086, 0x7195, -1, -1, -1, -1, -1, "Intel 82443MX AC97 audio" },
	{ 0x8086, 0x2415, -1, -1, -1, -1, -1, "Intel 82801AA (ICH) AC97 audio" },
	{ 0x8086, 0x2425, -1, -1, -1, -1, -1, "Intel 82801AB (ICH0) AC97 audio" },
	{ 0x8086, 0x2445, -1, -1, -1, -1, -1, "Intel 82801BA (ICH2) AC97 audio" },
	{ 0x8086, 0x2485, -1, -1, -1, -1, -1, "Intel 82801CA (ICH3) AC97 audio" },
	{ 0x8086, 0x24C5, -1, -1, -1, -1, -1, "Intel 82801DB (ICH4) AC97 audio", TYPE_ICH4 },
	{ 0x8086, 0x24D5, -1, -1, -1, -1, -1, "Intel 82801EB (ICH5)  AC97 audio", TYPE_ICH4 },
	{ 0x1039, 0x7012, -1, -1, -1, -1, -1, "SiS SI7012 AC97 audio", TYPE_SIS7012 },
	{ 0x10DE, 0x01B1, -1, -1, -1, -1, -1, "NVIDIA nForce (MCP)  AC97 audio" },
	{ 0x10DE, 0x006A, -1, -1, -1, -1, -1, "NVIDIA nForce 2 (MCP2)  AC97 audio" },
	{ 0x10DE, 0x00DA, -1, -1, -1, -1, -1, "NVIDIA nForce 3 (MCP3)  AC97 audio" },
	{ 0x1022, 0x764D, -1, -1, -1, -1, -1, "AMD AMD8111 AC97 audio" },
	{ 0x1022, 0x7445, -1, -1, -1, -1, -1, "AMD AMD768 AC97 audio" },
	{ 0x1103, 0x0004, -1, -1, -1, -1, -1, "Highpoint HPT372 RAID" },
	{ 0x1095, 0x3112, -1, -1, -1, -1, -1, "Silicon Image SATA Si3112" },
	{ 0x8086, 0x244b, -1, -1, -1, -1, -1, "Intel IDE Controller" },
	{ 0x8979, 0x6456, -1, -1, -1, -1, -1, "does not exist" },
	{ /* empty = end */}
};


driver_info_t driver_info = {
	ichaudio_id_table,
	"audio/lala/ichaudio",
	sizeof(ichaudio_cookie),
	ichaudio_attach,
	ichaudio_detach,
	ichaudio_powerctl,
};


status_t
ichaudio_attach(audio_drv_t *drv)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)drv->cookie;
	uint32 value;
	
	dprintf("ichaudio_attach\n");

	// For old ICHs enable programmed IO and busmaster access,
	// for ICH4 and up enable memory mapped IO and busmaster access
	value = drv->pci->read_pci_config(drv->bus, drv->device, drv->function, PCI_command, 2);
	value |= PCI_PCICMD_BME | (drv->flags & TYPE_ICH4) ? PCI_PCICMD_MSE : PCI_PCICMD_IOS;
	drv->pci->write_pci_config(drv->bus, drv->device, drv->function, PCI_command, 2, value);

	// get IRQ, we can compensate it later if IRQ is not available (hack!)
	cookie->irq = drv->pci->read_pci_config(drv->bus, drv->device, drv->function, PCI_interrupt_line, 1);
	if (cookie->irq == 0xff) cookie->irq = 0;
	if (cookie->irq == 0) dprintf("ichaudio_attach: no interrupt configured\n");

	if (drv->flags & TYPE_ICH4) {
		// memory mapped access
		uint32 phy_mmbar = PCI_address_memory_32_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x18, 4);
		uint32 phy_mbbar = PCI_address_memory_32_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x1C, 4);
		if (!phy_mmbar || !phy_mbbar) {
			dprintf("ichaudio_attach: memory mapped io unconfigured\n");
			goto err;			
		}
		// map into memory
		cookie->area_mmbar = map_mem(&cookie->mmbar, (void *)phy_mmbar, ICH4_MMBAR_SIZE, 0, "ichaudio mmbar io");
		cookie->area_mbbar = map_mem(&cookie->mbbar, (void *)phy_mbbar, ICH4_MBBAR_SIZE, 0, "ichaudio mbbar io");
		if (cookie->area_mmbar < B_OK || cookie->area_mbbar < B_OK) {
			dprintf("ichaudio_attach: mapping io into memory failed\n");
			goto err;			
		}
	} else {
		// pio access
		cookie->nambar = PCI_address_io_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x10, 4);
		cookie->nabmbar = PCI_address_io_mask & drv->pci->read_pci_config(drv->bus, drv->device, drv->function, 0x14, 4);
		if (!cookie->nambar || !cookie->nabmbar) {
			dprintf("ichaudio_attach: io unconfiugured\n");
			goto err;			
		}
	}


	return B_OK;

err:
	// unmap io memory
	if (cookie->area_mmbar > 0) delete_area(cookie->area_mmbar);
	if (cookie->area_mbbar > 0) delete_area(cookie->area_mbbar);
	return B_ERROR;
}


status_t
ichaudio_detach(audio_drv_t *drv)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)drv->cookie;

	dprintf("ichaudio_detach\n");

	// unmap io memory
	if (cookie->area_mmbar > 0) delete_area(cookie->area_mmbar);
	if (cookie->area_mbbar > 0) delete_area(cookie->area_mbbar);
	return B_OK;
}


status_t
ichaudio_powerctl(audio_drv_t *drv)
{
	ichaudio_cookie *cookie = (ichaudio_cookie *)drv->cookie;

	return B_OK;
}

