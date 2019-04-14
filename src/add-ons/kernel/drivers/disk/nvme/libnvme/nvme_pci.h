/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __NVME_PCI_H__
#define __NVME_PCI_H__

#include "nvme_common.h"

#ifndef __HAIKU__
#include <pciaccess.h>
#endif

#define NVME_PCI_PATH_MAX		256
#define NVME_PCI_CFG_SIZE		256
#define NVME_PCI_EXT_CAP_ID_SN		0x03

#define NVME_PCI_ANY_ID			0xffff
#define NVME_PCI_VID_INTEL		0x8086
#define NVME_PCI_VID_MEMBLAZE		0x1c5f

/*
 * PCI class code for NVMe devices.
 *
 * Base class code 01h: mass storage
 * Subclass code 08h: non-volatile memory
 * Programming interface 02h: NVM Express
 */
#define NVME_PCI_CLASS				0x010802

#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB0	0x3c20
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB1	0x3c21
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB2	0x3c22
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB3	0x3c23
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB4	0x3c24
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB5	0x3c25
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB6	0x3c26
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB7	0x3c27
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB8	0x3c2e
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_SNB9	0x3c2f

#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB0	0x0e20
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB1	0x0e21
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB2	0x0e22
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB3	0x0e23
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB4	0x0e24
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB5	0x0e25
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB6	0x0e26
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB7	0x0e27
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB8	0x0e2e
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_IVB9	0x0e2f

#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW0	0x2f20
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW1	0x2f21
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW2	0x2f22
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW3	0x2f23
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW4	0x2f24
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW5	0x2f25
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW6	0x2f26
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW7	0x2f27
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW8	0x2f2e
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_HSW9	0x2f2f

#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BWD0	0x0C50
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BWD1	0x0C51
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BWD2	0x0C52
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BWD3	0x0C53

#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDXDE0	0x6f50
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDXDE1	0x6f51
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDXDE2	0x6f52
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDXDE3	0x6f53

#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX0	0x6f20
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX1	0x6f21
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX2	0x6f22
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX3	0x6f23
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX4	0x6f24
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX5	0x6f25
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX6	0x6f26
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX7	0x6f27
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX8	0x6f2e
#define NVME_PCI_DEVICE_ID_INTEL_IOAT_BDX9	0x6f2f

struct pci_slot_match;

struct pci_id {
	uint16_t  vendor_id;
	uint16_t  device_id;
	uint16_t  subvendor_id;
	uint16_t  subdevice_id;
};

/*
 * Initialize PCI subsystem.
 */
extern int nvme_pci_init(void);

/*
 * Search a PCI device and grab it if found.
 */
extern struct pci_device *
nvme_pci_device_probe(const struct pci_slot_match *slot);

/*
 * Reset a PCI device.
 */
extern int nvme_pci_device_reset(struct pci_device *dev);

/*
 * Get a device serial number.
 */
extern int nvme_pci_device_get_serial_number(struct pci_device *dev,
					     char *sn, size_t len);

/*
 * Compare two devices.
 * Return 0 if the devices are the same, 1 otherwise.
 */
static inline int nvme_pci_dev_cmp(struct pci_device *pci_dev1,
				   struct pci_device *pci_dev2)
{
	if (pci_dev1 == pci_dev2)
		return 0;

	if (pci_dev1->domain == pci_dev2->domain &&
	    pci_dev1->bus == pci_dev2->bus &&
	    pci_dev1->dev == pci_dev2->dev &&
	    pci_dev1->func == pci_dev2->func)
		return 0;

	return 1;
}

/*
 * Get a device PCI ID.
 */
static inline void nvme_pci_get_pci_id(struct pci_device *pci_dev,
				       struct pci_id *pci_id)
{
	pci_id->vendor_id = pci_dev->vendor_id;
	pci_id->device_id = pci_dev->device_id;
	pci_id->subvendor_id = pci_dev->subvendor_id;
	pci_id->subdevice_id = pci_dev->subdevice_id;
}

#ifdef __HAIKU__
int nvme_pcicfg_read8(struct pci_device *dev, uint8_t *value, uint32_t offset);
int nvme_pcicfg_write8(struct pci_device *dev, uint8_t value, uint32_t offset);
int nvme_pcicfg_read16(struct pci_device *dev, uint16_t *value, uint32_t offset);
int nvme_pcicfg_write16(struct pci_device *dev, uint16_t value, uint32_t offset);
int nvme_pcicfg_read32(struct pci_device *dev, uint32_t *value, uint32_t offset);
int nvme_pcicfg_write32(struct pci_device *dev, uint32_t value, uint32_t offset);
int nvme_pcicfg_map_bar(void *devhandle, unsigned int bar, bool read_only,
	void **mapped_addr);
int nvme_pcicfg_map_bar_write_combine(void *devhandle, unsigned int bar,
	void **mapped_addr);
int nvme_pcicfg_unmap_bar(void *devhandle, unsigned int bar, void *addr);
void nvme_pcicfg_get_bar_addr_len(void *devhandle, unsigned int bar,
	uint64_t *addr, uint64_t *size);
#else
/*
 * Read a device config register.
 */
static inline int nvme_pcicfg_read8(struct pci_device *dev,
				    uint8_t *value, uint32_t offset)
{
	return pci_device_cfg_read_u8(dev, value, offset);
}

/*
 * Write a device config register.
 */
static inline int nvme_pcicfg_write8(struct pci_device *dev,
				     uint8_t value, uint32_t offset)
{
	return pci_device_cfg_write_u8(dev, value, offset);
}

/*
 * Read a device config register.
 */
static inline int nvme_pcicfg_read16(struct pci_device *dev,
				     uint16_t *value, uint32_t offset)
{
	return pci_device_cfg_read_u16(dev, value, offset);
}

/*
 * Write a device config register.
 */
static inline int nvme_pcicfg_write16(struct pci_device *dev,
				      uint16_t value, uint32_t offset)
{
	return pci_device_cfg_write_u16(dev, value, offset);
}

/*
 * Read a device config register.
 */
static inline int nvme_pcicfg_read32(struct pci_device *dev,
				     uint32_t *value, uint32_t offset)
{
	return pci_device_cfg_read_u32(dev, value, offset);
}

/*
 * Write a device config register.
 */
static inline int nvme_pcicfg_write32(struct pci_device *dev,
				      uint32_t value, uint32_t offset)
{
	return pci_device_cfg_write_u32(dev, value, offset);
}

/*
 * Map a device PCI BAR.
 */
static inline int nvme_pcicfg_map_bar(void *devhandle, unsigned int bar,
				      bool read_only, void **mapped_addr)
{
	struct pci_device *dev = devhandle;
	uint32_t flags = (read_only ? 0 : PCI_DEV_MAP_FLAG_WRITABLE);

	return pci_device_map_range(dev, dev->regions[bar].base_addr,
				    dev->regions[bar].size, flags, mapped_addr);
}

/*
 * Map a device PCI BAR (write combine).
 */
static inline int nvme_pcicfg_map_bar_write_combine(void *devhandle,
						    unsigned int bar,
						    void **mapped_addr)
{
	struct pci_device *dev = devhandle;
	uint32_t flags = PCI_DEV_MAP_FLAG_WRITABLE |
		PCI_DEV_MAP_FLAG_WRITE_COMBINE;

	return pci_device_map_range(dev, dev->regions[bar].base_addr,
				    dev->regions[bar].size, flags, mapped_addr);
}

/*
 * Unmap a device PCI BAR.
 */
static inline int nvme_pcicfg_unmap_bar(void *devhandle, unsigned int bar,
					void *addr)
{
	struct pci_device *dev = devhandle;

	return pci_device_unmap_range(dev, addr, dev->regions[bar].size);
}

/*
 * Get a device PCI BAR address and length.
 */
static inline void nvme_pcicfg_get_bar_addr_len(void *devhandle,
						unsigned int bar,
						uint64_t *addr, uint64_t *size)
{
	struct pci_device *dev = devhandle;

	*addr = (uint64_t)dev->regions[bar].base_addr;
	*size = (uint64_t)dev->regions[bar].size;
}
#endif

#endif /* __NVME_PCI_H__ */
