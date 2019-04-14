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

#include "nvme_internal.h"
#include "nvme_pci.h"

struct nvme_quirk {
	struct pci_id	id;
	unsigned int	flags;
};

static const struct nvme_quirk nvme_quirks[] = {
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x3702 },
		NVME_INTEL_QUIRK_READ_LATENCY | NVME_INTEL_QUIRK_WRITE_LATENCY
	},
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x3703 },
		NVME_INTEL_QUIRK_READ_LATENCY | NVME_INTEL_QUIRK_WRITE_LATENCY
	},
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x3704 },
		NVME_INTEL_QUIRK_READ_LATENCY | NVME_INTEL_QUIRK_WRITE_LATENCY
	},
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x3705 },
		NVME_INTEL_QUIRK_READ_LATENCY | NVME_INTEL_QUIRK_WRITE_LATENCY
	},
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x3709 },
		NVME_INTEL_QUIRK_READ_LATENCY | NVME_INTEL_QUIRK_WRITE_LATENCY
	},
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x370a },
		NVME_INTEL_QUIRK_READ_LATENCY | NVME_INTEL_QUIRK_WRITE_LATENCY
	},
	{
		{ NVME_PCI_VID_MEMBLAZE, 0x0540, NVME_PCI_ANY_ID, NVME_PCI_ANY_ID },
		NVME_QUIRK_DELAY_BEFORE_CHK_RDY
	},
	{
		{ NVME_PCI_VID_INTEL, 0x0953, NVME_PCI_VID_INTEL, 0x370d },
		NVME_QUIRK_DELAY_AFTER_RDY
	},
	{
		{ 0x0000, 0x0000, 0x0000, 0x0000 },
		0
	}
};

/*
 * Compare each field. NVME_PCI_ANY_ID in s1 matches everything.
 */
static bool nvme_quirks_pci_id_match(const struct pci_id *id,
				     struct pci_device *pdev)
{
	if ((id->vendor_id == NVME_PCI_ANY_ID ||
	     id->vendor_id == pdev->vendor_id) &&
	    (id->device_id == NVME_PCI_ANY_ID ||
	     id->device_id == pdev->device_id) &&
	    (id->subvendor_id == NVME_PCI_ANY_ID ||
	     id->subvendor_id == pdev->subvendor_id) &&
	    (id->subdevice_id == NVME_PCI_ANY_ID ||
	     id->subdevice_id == pdev->subdevice_id))
		return true;

	return false;
}

unsigned int nvme_ctrlr_get_quirks(struct pci_device *pdev)
{
	const struct nvme_quirk *quirk = nvme_quirks;

	while (quirk->id.vendor_id) {
		if (nvme_quirks_pci_id_match(&quirk->id, pdev))
			return quirk->flags;
		quirk++;
	}

	return 0;
}
