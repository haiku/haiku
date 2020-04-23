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

/*
 * List of open controllers and its lock.
 */
LIST_HEAD(, nvme_ctrlr)	ctrlr_head = LIST_HEAD_INITIALIZER(ctrlr_head);
static pthread_mutex_t ctrlr_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Search for an open controller.
 */
static struct nvme_ctrlr *nvme_ctrlr_get(struct nvme_ctrlr *ctrlr,
					 bool remove)
{
	struct nvme_ctrlr *c;

	pthread_mutex_lock(&ctrlr_lock);

	LIST_FOREACH(c, &ctrlr_head, link) {
		if (c == ctrlr) {
			if (remove)
				LIST_REMOVE(c, link);
			goto out;
		}
	}

	ctrlr = NULL;

out:
	pthread_mutex_unlock(&ctrlr_lock);

	return ctrlr;
}

#ifndef __HAIKU__
/*
 * Probe a pci device identified by its name.
 * Name should be in the form: [0000:]00:00.0
 * Return NULL if failed
 */
static struct pci_device *nvme_pci_ctrlr_probe(const char *slot_name)
{
	char *domain = NULL, *bus = NULL, *dev = NULL, *func = NULL, *end = NULL;
	char *pciid = strdup(slot_name);
	struct pci_slot_match slot;
	struct pci_device *pci_dev = NULL;

	if (!pciid)
		return NULL;

	memset(&slot, 0, sizeof(struct pci_slot_match));

	func = strrchr(pciid, '.');
	if (func) {
		*func = '\0';
		func++;
	}

	dev = strrchr(pciid, ':');
	if (dev) {
		*dev = '\0';
		dev++;
	}

	bus = strrchr(pciid, ':');
	if (!bus) {
		domain = NULL;
		bus = pciid;
	} else {
		domain = pciid;
		*bus = '\0';
		bus++;
	}

	if (!bus || !dev || !func) {
		nvme_err("Malformed PCI device slot name %s\n",
			 slot_name);
		goto out;
	}

	if (domain) {
		slot.domain = (uint32_t)strtoul(domain, &end, 16);
		if ((end && *end) || (slot.domain > 0xffff)) {
			nvme_err("Invalid domain number: 0x%X\n", slot.domain);
			return NULL;
		}
	} else {
		slot.domain = PCI_MATCH_ANY;
	}

	slot.bus = (uint32_t)strtoul(bus, &end, 16);
	if ((end && *end) || (slot.bus > 0xff)) {
		nvme_err("Invalid bus number: 0x%X\n", slot.bus);
		return NULL;
	}

	slot.dev = strtoul(dev, &end, 16);
	if ((end && *end) || (slot.dev > 0x1f)) {
		nvme_err("Invalid device number: 0x%X\n", slot.dev);
		return NULL;
	}

	slot.func = strtoul(func, &end, 16);
	if ((end && *end) || (slot.func > 7)) {
		nvme_err("Invalid function number: 0x%X\n", slot.func);
		return NULL;
	}

	nvme_debug("PCI URL: domain 0x%X, bus 0x%X, dev 0x%X, func 0x%X\n",
		   slot.domain, slot.bus, slot.dev, slot.func);

	pci_dev = nvme_pci_device_probe(&slot);
	if (pci_dev) {
		slot.domain = pci_dev->domain;
		if (slot.domain == PCI_MATCH_ANY)
			slot.domain = 0;
		nvme_info("Found NVMe controller %04x:%02x:%02x.%1u\n",
			  slot.domain,
			  slot.bus,
			  slot.dev,
			  slot.func);
	}

out:
	free(pciid);

	return pci_dev;
}
#endif

/*
 * Open an NVMe controller.
 */
#ifdef __HAIKU__
struct nvme_ctrlr *nvme_ctrlr_open(struct pci_device *pdev,
				   struct nvme_ctrlr_opts *opts)
#else
struct nvme_ctrlr *nvme_ctrlr_open(const char *url,
				   struct nvme_ctrlr_opts *opts)
#endif
{
	struct nvme_ctrlr *ctrlr;
#ifndef __HAIKU__
	char *slot;

	/* Check url */
	if (strncmp(url, "pci://", 6) != 0) {
		nvme_err("Invalid URL %s\n", url);
		return NULL;
	}

	/* Probe PCI device */
	slot = (char *)url + 6;
	pdev = nvme_pci_ctrlr_probe(slot);
	if (!pdev) {
		nvme_err("Device %s not found\n", url);
		return NULL;
	}
#endif

	pthread_mutex_lock(&ctrlr_lock);

	/* Verify that this controller is not already open */
	LIST_FOREACH(ctrlr, &ctrlr_head, link) {
		if (nvme_pci_dev_cmp(ctrlr->pci_dev, pdev) == 0) {
			nvme_err("Controller already open\n");
			ctrlr = NULL;
			goto out;
		}
	}

	/* Attach the device */
	ctrlr = nvme_ctrlr_attach(pdev, opts);
	if (!ctrlr) {
		nvme_err("Attach failed\n");
		goto out;
	}

	/* Add controller to the list */
	LIST_INSERT_HEAD(&ctrlr_head, ctrlr, link);

out:
	pthread_mutex_unlock(&ctrlr_lock);

	return ctrlr;

}

/*
 * Close an open controller.
 */
int nvme_ctrlr_close(struct nvme_ctrlr *ctrlr)
{

	/*
	 * Verify that this controller is open.
	 * If it is, remove it from the list.
	 */
	ctrlr = nvme_ctrlr_get(ctrlr, true);
	if (!ctrlr) {
		nvme_err("Invalid controller\n");
		return EINVAL;
	}

	nvme_ctrlr_detach(ctrlr);

	return 0;
}

/*
 * Get controller information.
 */
int nvme_ctrlr_stat(struct nvme_ctrlr *ctrlr, struct nvme_ctrlr_stat *cstat)
{
	struct pci_device *pdev = ctrlr->pci_dev;
	unsigned int i;

	/* Verify that this controller is open */
	ctrlr = nvme_ctrlr_get(ctrlr, false);
	if (!ctrlr) {
		nvme_err("Invalid controller\n");
		return EINVAL;
	}

	pthread_mutex_lock(&ctrlr->lock);

	memset(cstat, 0, sizeof(struct nvme_ctrlr_stat));

	/* Controller serial and model number */
	strncpy(cstat->sn, (char *)ctrlr->cdata.sn,
		NVME_SERIAL_NUMBER_LENGTH - 1);
	strncpy(cstat->mn, (char *)ctrlr->cdata.mn,
		NVME_MODEL_NUMBER_LENGTH - 1);

	/* Remove heading and trailling spaces */
	nvme_str_trim(cstat->sn);
	nvme_str_trim(cstat->mn);

	/* PCI device info */
	cstat->vendor_id = pdev->vendor_id;
	cstat->device_id = pdev->device_id;
	cstat->subvendor_id = pdev->subvendor_id;
	cstat->subdevice_id = pdev->subdevice_id;
#ifndef __HAIKU__
	cstat->device_class = pdev->device_class;
	cstat->revision = pdev->revision;
	cstat->domain = pdev->domain;
	cstat->bus = pdev->bus;
	cstat->dev = pdev->dev;
	cstat->func = pdev->func;
#endif

	/* Maximum transfer size */
	cstat->max_xfer_size = ctrlr->max_xfer_size;

	cstat->sgl_supported = (ctrlr->flags & NVME_CTRLR_SGL_SUPPORTED);

	memcpy(&cstat->features, &ctrlr->feature_supported,
	       sizeof(ctrlr->feature_supported));
	memcpy(&cstat->log_pages, &ctrlr->log_page_supported,
	       sizeof(ctrlr->log_page_supported));

	cstat->nr_ns = ctrlr->nr_ns;
	for (i = 0; i < ctrlr->nr_ns; i++) {
		cstat->ns_ids[i] = i + 1;
	}

	/* Maximum io qpair possible */
	cstat->max_io_qpairs = ctrlr->max_io_queues;

	/* Constructed io qpairs */
	cstat->io_qpairs = ctrlr->io_queues;

	/* Enabled io qpairs */
	cstat->enabled_io_qpairs = ctrlr->enabled_io_qpairs;

	/* Max queue depth */
	cstat->max_qd = ctrlr->io_qpairs_max_entries;

	pthread_mutex_unlock(&ctrlr->lock);

	return 0;
}

/*
 * Get controller data
 */
int nvme_ctrlr_data(struct nvme_ctrlr *ctrlr, struct nvme_ctrlr_data *cdata,
		    struct nvme_register_data *rdata)
{
	union nvme_cap_register	cap;

	/* Verify that this controller is open */
	ctrlr = nvme_ctrlr_get(ctrlr, false);
	if (!ctrlr) {
		nvme_err("Invalid controller\n");
		return EINVAL;
	}

	pthread_mutex_lock(&ctrlr->lock);

	/* Controller data */
	if (cdata)
		memcpy(cdata, &ctrlr->cdata, sizeof(struct nvme_ctrlr_data));

	/* Read capabilities register */
	if (rdata) {
		cap.raw = nvme_reg_mmio_read_8(ctrlr, cap.raw);
		rdata->mqes = cap.bits.mqes;
	}

	pthread_mutex_unlock(&ctrlr->lock);

	return 0;
}

/*
 * Get qpair information
 */
int nvme_qpair_stat(struct nvme_qpair *qpair, struct nvme_qpair_stat *qpstat)
{
	struct nvme_ctrlr *ctrlr = qpair->ctrlr;

	/* Verify that this controller is open */
	ctrlr = nvme_ctrlr_get(ctrlr, false);
	if (!ctrlr) {
		nvme_err("Invalid controller\n");
		return EINVAL;
	}

	pthread_mutex_lock(&ctrlr->lock);

	qpstat->id = qpair->id;
	qpstat->qd = qpair->entries;
	qpstat->enabled = qpair->enabled;
	qpstat->qprio = qpair->qprio;

	pthread_mutex_unlock(&ctrlr->lock);

	return 0;
}

/*
 * Close all open controllers on exit.
 */
void nvme_ctrlr_cleanup(void)
{
	struct nvme_ctrlr *ctrlr;

	while ((ctrlr = LIST_FIRST(&ctrlr_head))) {
		LIST_REMOVE(ctrlr, link);
		nvme_ctrlr_detach(ctrlr);
	}
}
