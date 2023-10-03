/*-
 * Copyright IBM Corp. 2007
 *
 * Authors:
 *  Anthony Liguori  <aliguori@us.ibm.com>
 *
 * This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#ifndef _VIRTIO_PCI_H
#define _VIRTIO_PCI_H

/* VirtIO PCI vendor/device ID. */
#define VIRTIO_PCI_VENDORID	0x1AF4
#define VIRTIO_PCI_DEVICEID_MIN	0x1000
#define VIRTIO_PCI_DEVICEID_LEGACY_MAX	0x103F
#define VIRTIO_PCI_DEVICEID_MODERN_MIN	0x1040
#define VIRTIO_PCI_DEVICEID_MODERN_MAX	0x107F

/*
 * VirtIO Header, located in BAR 0.
 */
#define VIRTIO_PCI_HOST_FEATURES  0  /* host's supported features (32bit, RO)*/
#define VIRTIO_PCI_GUEST_FEATURES 4  /* guest's supported features (32, RW) */
#define VIRTIO_PCI_QUEUE_PFN      8  /* physical address of VQ (32, RW) */
#define VIRTIO_PCI_QUEUE_NUM      12 /* number of ring entries (16, RO) */
#define VIRTIO_PCI_QUEUE_SEL      14 /* current VQ selection (16, RW) */
#define VIRTIO_PCI_QUEUE_NOTIFY	  16 /* notify host regarding VQ (16, RW) */
#define VIRTIO_PCI_STATUS         18 /* device status register (8, RW) */
#define VIRTIO_PCI_ISR            19 /* interrupt status register, reading
				      * also clears the register (8, RO) */
/* Only if MSIX is enabled: */
#define VIRTIO_MSI_CONFIG_VECTOR  20 /* configuration change vector (16, RW) */
#define VIRTIO_MSI_QUEUE_VECTOR   22 /* vector for selected VQ notifications
					(16, RW) */

/* The bit of the ISR which indicates a device has an interrupt. */
#define VIRTIO_PCI_ISR_INTR	0x1
/* The bit of the ISR which indicates a device configuration change. */
#define VIRTIO_PCI_ISR_CONFIG	0x2
/* Vector value used to disable MSI for queue. */
#define VIRTIO_MSI_NO_VECTOR	0xFFFF

/*
 * The remaining space is defined by each driver as the per-driver
 * configuration space.
 */
#define VIRTIO_PCI_CONFIG(bus) \
    ((bus->irq_type != VIRTIO_IRQ_LEGACY) ? 24 : 20)

/*
 * How many bits to shift physical queue address written to QUEUE_PFN.
 * 12 is historical, and due to x86 page size.
 */
#define VIRTIO_PCI_QUEUE_ADDR_SHIFT	12

/* The alignment to use between consumer and producer parts of vring. */
#define VIRTIO_PCI_VRING_ALIGN	4096


/*
 * Virtio 1.0 specific
 */

struct virtio_pci_cap {
	uint8_t cap_vndr;	/* Generic PCI field: PCI_CAP_ID_VNDR */
	uint8_t cap_next;	/* Generic PCI field: next ptr. */
	uint8_t cap_len;	/* Generic PCI field: capability length */
	uint8_t cfg_type;	/* Identifies the structure. */
	uint8_t bar;		/* Where to find it. */
	uint8_t padding[3];	/* Pad to full dword. */
	uint32_t offset;	/* Offset within bar. */
	uint32_t length;	/* Length of the structure, in bytes. */
} _PACKED;

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG	1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG	2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG		3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG	4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG		5

struct virtio_pci_notify_cap {
	struct virtio_pci_cap cap;
	uint32_t notify_off_multiplier;	/* Multiplier for queue_notify_off. */
} _PACKED;

struct virtio_pci_cfg_cap {
	struct virtio_pci_cap cap;
	uint8_t pci_cfg_data[4];	/* Data for BAR access. */
} _PACKED;

struct virtio_pci_common_cfg {
	/* About the whole device. */
	uint32_t device_feature_select;	/* read-write */
	uint32_t device_feature;	/* read-only for driver */
	uint32_t driver_feature_select;	/* read-write */
	uint32_t driver_feature;	/* read-write */
	uint16_t config_msix_vector;	/* read-write */
	uint16_t num_queues;		/* read-only for driver */
	uint8_t device_status;		/* read-write */
	uint8_t config_generation;	/* read-only for driver */

	/* About a specific virtqueue. */
	uint16_t queue_select;		/* read-write */
	uint16_t queue_size;		/* read-write, power of 2, or 0. */
	uint16_t queue_msix_vector;	/* read-write */
	uint16_t queue_enable;		/* read-write */
	uint16_t queue_notify_off;	/* read-only for driver */
	uint64_t queue_desc;		/* read-write */
	uint64_t queue_avail;		/* read-write */
	uint64_t queue_used;		/* read-write */
} _PACKED;

#endif /* _VIRTIO_PCI_H */
