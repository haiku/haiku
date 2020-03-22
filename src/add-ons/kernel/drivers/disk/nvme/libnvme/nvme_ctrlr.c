/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation. All rights reserved.
 *   Copyright (c) 2017, Western Digital Corporation or its affiliates.
 *
 *   Redistribution and use in sourete and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of sourete code must retain the above copyright
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
 * Host software shall wait a minimum of CAP.TO x 500 milleseconds for CSTS.RDY
 * to be set to '1' after setting CC.EN to '1' from a previous value of '0'.
 */
static inline unsigned int
nvme_ctrlr_get_ready_to_in_ms(struct nvme_ctrlr *ctrlr)
{
	union nvme_cap_register	cap;

/* The TO unit in ms */
#define NVME_READY_TIMEOUT_UNIT 500

	cap.raw = nvme_reg_mmio_read_8(ctrlr, cap.raw);

	return (NVME_READY_TIMEOUT_UNIT * cap.bits.to);
}

/*
 * Create a queue pair.
 */
static int nvme_ctrlr_create_qpair(struct nvme_ctrlr *ctrlr,
				   struct nvme_qpair *qpair)
{
	int ret;

	/* Create the completion queue */
	ret = nvme_admin_create_ioq(ctrlr, qpair, NVME_IO_COMPLETION_QUEUE);
	if (ret != 0) {
		nvme_notice("Create completion queue %u failed\n",
			    qpair->id);
		return ret;
	}

	/* Create the submission queue */
	ret = nvme_admin_create_ioq(ctrlr, qpair, NVME_IO_SUBMISSION_QUEUE);
	if (ret != 0) {
		/* Attempt to delete the completion queue */
		nvme_notice("Create submission queue %u failed\n",
			    qpair->id);
		nvme_admin_delete_ioq(ctrlr, qpair, NVME_IO_COMPLETION_QUEUE);
		return ret;
	}

	nvme_qpair_reset(qpair);

	return 0;
}

/*
 * Delete a queue pair.
 */
static int nvme_ctrlr_delete_qpair(struct nvme_ctrlr *ctrlr,
				   struct nvme_qpair *qpair)
{
	int ret;

	/* Delete the submission queue */
	ret = nvme_admin_delete_ioq(ctrlr, qpair, NVME_IO_SUBMISSION_QUEUE);
	if (ret != 0) {
		nvme_notice("Delete submission queue %u failed\n",
			    qpair->id);
		return ret;
	}

	/* Delete the completion queue */
	ret = nvme_admin_delete_ioq(ctrlr, qpair, NVME_IO_COMPLETION_QUEUE);
	if (ret != 0) {
		nvme_notice("Delete completion queue %u failed\n",
			    qpair->id);
		return ret;
	}

	return 0;
}

/*
 * Intel log page.
 */
static void
nvme_ctrlr_construct_intel_support_log_page_list(struct nvme_ctrlr *ctrlr,
				struct nvme_intel_log_page_dir *log_page_dir)
{

	if (ctrlr->cdata.vid != NVME_PCI_VID_INTEL ||
	    log_page_dir == NULL)
		return;

	ctrlr->log_page_supported[NVME_INTEL_LOG_PAGE_DIR] = true;

	if (log_page_dir->read_latency_log_len ||
	    (ctrlr->quirks & NVME_INTEL_QUIRK_READ_LATENCY))
		ctrlr->log_page_supported[NVME_INTEL_LOG_READ_CMD_LATENCY] = true;

	if (log_page_dir->write_latency_log_len ||
	    (ctrlr->quirks & NVME_INTEL_QUIRK_WRITE_LATENCY))
		ctrlr->log_page_supported[NVME_INTEL_LOG_WRITE_CMD_LATENCY] = true;

	if (log_page_dir->temperature_statistics_log_len)
		ctrlr->log_page_supported[NVME_INTEL_LOG_TEMPERATURE] = true;

	if (log_page_dir->smart_log_len)
		ctrlr->log_page_supported[NVME_INTEL_LOG_SMART] = true;

	if (log_page_dir->marketing_description_log_len)
		ctrlr->log_page_supported[NVME_INTEL_MARKETING_DESCRIPTION] = true;
}

/*
 * Intel log page.
 */
static int nvme_ctrlr_set_intel_support_log_pages(struct nvme_ctrlr *ctrlr)
{
	struct nvme_intel_log_page_dir *log_page_dir;
	int ret;

	log_page_dir = nvme_zmalloc(sizeof(struct nvme_intel_log_page_dir), 64);
	if (!log_page_dir) {
		nvme_err("Allocate log_page_directory failed\n");
		return ENOMEM;
	}

	ret = nvme_admin_get_log_page(ctrlr, NVME_INTEL_LOG_PAGE_DIR,
				      NVME_GLOBAL_NS_TAG,
				      log_page_dir,
				      sizeof(struct nvme_intel_log_page_dir));
	if (ret != 0)
		nvme_notice("Get NVME_INTEL_LOG_PAGE_DIR log page failed\n");
	else
		nvme_ctrlr_construct_intel_support_log_page_list(ctrlr,
								 log_page_dir);

	nvme_free(log_page_dir);

	return ret;
}

/*
 * Initialize log page support directory.
 */
static void nvme_ctrlr_set_supported_log_pages(struct nvme_ctrlr *ctrlr)
{

	memset(ctrlr->log_page_supported, 0, sizeof(ctrlr->log_page_supported));

	/* Mandatory pages */
	ctrlr->log_page_supported[NVME_LOG_ERROR] = true;
	ctrlr->log_page_supported[NVME_LOG_HEALTH_INFORMATION] = true;
	ctrlr->log_page_supported[NVME_LOG_FIRMWARE_SLOT] = true;

	if (ctrlr->cdata.lpa.celp)
		ctrlr->log_page_supported[NVME_LOG_COMMAND_EFFECTS_LOG] = true;

	if (ctrlr->cdata.vid == NVME_PCI_VID_INTEL)
		nvme_ctrlr_set_intel_support_log_pages(ctrlr);
}

/*
 * Set Intel device features.
 */
static void nvme_ctrlr_set_intel_supported_features(struct nvme_ctrlr *ctrlr)
{
	bool *supported_feature = ctrlr->feature_supported;

	supported_feature[NVME_INTEL_FEAT_MAX_LBA] = true;
	supported_feature[NVME_INTEL_FEAT_NATIVE_MAX_LBA] = true;
	supported_feature[NVME_INTEL_FEAT_POWER_GOVERNOR_SETTING] = true;
	supported_feature[NVME_INTEL_FEAT_SMBUS_ADDRESS] = true;
	supported_feature[NVME_INTEL_FEAT_LED_PATTERN] = true;
	supported_feature[NVME_INTEL_FEAT_RESET_TIMED_WORKLOAD_COUNTERS] = true;
	supported_feature[NVME_INTEL_FEAT_LATENCY_TRACKING] = true;
}

/*
 * Set device features.
 */
static void nvme_ctrlr_set_supported_features(struct nvme_ctrlr *ctrlr)
{
	bool *supported_feature = ctrlr->feature_supported;

	memset(ctrlr->feature_supported, 0, sizeof(ctrlr->feature_supported));

	/* Mandatory features */
	supported_feature[NVME_FEAT_ARBITRATION] = true;
	supported_feature[NVME_FEAT_POWER_MANAGEMENT] = true;
	supported_feature[NVME_FEAT_TEMPERATURE_THRESHOLD] = true;
	supported_feature[NVME_FEAT_ERROR_RECOVERY] = true;
	supported_feature[NVME_FEAT_NUMBER_OF_QUEUES] = true;
	supported_feature[NVME_FEAT_INTERRUPT_COALESCING] = true;
	supported_feature[NVME_FEAT_INTERRUPT_VECTOR_CONFIGURATION] = true;
	supported_feature[NVME_FEAT_WRITE_ATOMICITY] = true;
	supported_feature[NVME_FEAT_ASYNC_EVENT_CONFIGURATION] = true;

	/* Optional features */
	if (ctrlr->cdata.vwc.present)
		supported_feature[NVME_FEAT_VOLATILE_WRITE_CACHE] = true;
	if (ctrlr->cdata.apsta.supported)
		supported_feature[NVME_FEAT_AUTONOMOUS_POWER_STATE_TRANSITION]
			= true;
	if (ctrlr->cdata.hmpre)
		supported_feature[NVME_FEAT_HOST_MEM_BUFFER] = true;
	if (ctrlr->cdata.vid == NVME_PCI_VID_INTEL)
		nvme_ctrlr_set_intel_supported_features(ctrlr);
}

/*
 * Initialize I/O queue pairs.
 */
static int nvme_ctrlr_init_io_qpairs(struct nvme_ctrlr *ctrlr)
{
	struct nvme_qpair *qpair;
	union nvme_cap_register	cap;
	uint32_t i;

	if (ctrlr->ioq != NULL)
		/*
		 * io_qpairs were already constructed, so just return.
		 * This typically happens when the controller is
		 * initialized a second (or subsequent) time after a
		 * controller reset.
		 */
		return 0;

	/*
	 * NVMe spec sets a hard limit of 64K max entries, but
	 * devices may specify a smaller limit, so we need to check
	 * the MQES field in the capabilities register.
	 */
	cap.raw = nvme_reg_mmio_read_8(ctrlr, cap.raw);
	ctrlr->io_qpairs_max_entries =
		nvme_min(NVME_IO_ENTRIES, (unsigned int)cap.bits.mqes + 1);

	ctrlr->ioq = calloc(ctrlr->io_queues, sizeof(struct nvme_qpair));
	if (!ctrlr->ioq)
		return ENOMEM;

	/* Keep queue pair ID 0 for the admin queue */
	for (i = 0; i < ctrlr->io_queues; i++) {
		qpair = &ctrlr->ioq[i];
		qpair->id = i + 1;
		TAILQ_INSERT_TAIL(&ctrlr->free_io_qpairs, qpair, tailq);
	}

	return 0;
}

/*
 * Shutdown a controller.
 */
static void nvme_ctrlr_shutdown(struct nvme_ctrlr *ctrlr)
{
	union nvme_cc_register	cc;
	union nvme_csts_register csts;
	int ms_waited = 0;

	cc.raw = nvme_reg_mmio_read_4(ctrlr, cc.raw);
	cc.bits.shn = NVME_SHN_NORMAL;
	nvme_reg_mmio_write_4(ctrlr, cc.raw, cc.raw);

	csts.raw = nvme_reg_mmio_read_4(ctrlr, csts.raw);
	/*
	 * The NVMe spec does not define a timeout period for shutdown
	 * notification, so we just pick 5 seconds as a reasonable amount
	 * of time to wait before proceeding.
	 */
#define NVME_CTRLR_SHUTDOWN_TIMEOUT 5000
	while (csts.bits.shst != NVME_SHST_COMPLETE) {
		nvme_usleep(1000);
		csts.raw = nvme_reg_mmio_read_4(ctrlr, csts.raw);
		if (ms_waited++ >= NVME_CTRLR_SHUTDOWN_TIMEOUT)
			break;
	}

	if (csts.bits.shst != NVME_SHST_COMPLETE)
		nvme_err("Controller did not shutdown within %d seconds\n",
			 NVME_CTRLR_SHUTDOWN_TIMEOUT / 1000);
}

/*
 * Enable a controller.
 */
static int nvme_ctrlr_enable(struct nvme_ctrlr *ctrlr)
{
	union nvme_cc_register	cc;
	union nvme_aqa_register	aqa;
	union nvme_cap_register	cap;

	cc.raw = nvme_reg_mmio_read_4(ctrlr, cc.raw);

	if (cc.bits.en != 0) {
		nvme_err("COntroller enable called with CC.EN = 1\n");
		return EINVAL;
	}

	nvme_reg_mmio_write_8(ctrlr, asq, ctrlr->adminq.cmd_bus_addr);
	nvme_reg_mmio_write_8(ctrlr, acq, ctrlr->adminq.cpl_bus_addr);

	aqa.raw = 0;
	/* acqs and asqs are 0-based. */
	aqa.bits.acqs = ctrlr->adminq.entries - 1;
	aqa.bits.asqs = ctrlr->adminq.entries - 1;
	nvme_reg_mmio_write_4(ctrlr, aqa.raw, aqa.raw);

	cc.bits.en = 1;
	cc.bits.css = 0;
	cc.bits.shn = 0;
	cc.bits.iosqes = 6; /* SQ entry size == 64 == 2^6 */
	cc.bits.iocqes = 4; /* CQ entry size == 16 == 2^4 */

	/* Page size is 2 ^ (12 + mps). */
	cc.bits.mps = PAGE_SHIFT - 12;

	cap.raw = nvme_reg_mmio_read_8(ctrlr, cap.raw);

	switch (ctrlr->opts.arb_mechanism) {
	case NVME_CC_AMS_RR:
		break;
	case NVME_CC_AMS_WRR:
		if (NVME_CAP_AMS_WRR & cap.bits.ams)
			break;
		return EINVAL;
	case NVME_CC_AMS_VS:
		if (NVME_CAP_AMS_VS & cap.bits.ams)
			break;
		return EINVAL;
	default:
		return EINVAL;
	}

	cc.bits.ams = ctrlr->opts.arb_mechanism;

	nvme_reg_mmio_write_4(ctrlr, cc.raw, cc.raw);

	return 0;
}

/*
 * Disable a controller.
 */
static inline void nvme_ctrlr_disable(struct nvme_ctrlr *ctrlr)
{
	union nvme_cc_register cc;

	cc.raw = nvme_reg_mmio_read_4(ctrlr, cc.raw);
	cc.bits.en = 0;

	nvme_reg_mmio_write_4(ctrlr, cc.raw, cc.raw);
}

/*
 * Test if a controller is enabled.
 */
static inline int nvme_ctrlr_enabled(struct nvme_ctrlr *ctrlr)
{
	union nvme_cc_register cc;

	cc.raw = nvme_reg_mmio_read_4(ctrlr, cc.raw);

	return cc.bits.en;
}

/*
 * Test if a controller is ready.
 */
static inline int nvme_ctrlr_ready(struct nvme_ctrlr *ctrlr)
{
	union nvme_csts_register csts;

	csts.raw = nvme_reg_mmio_read_4(ctrlr, csts.raw);

	return csts.bits.rdy;
}

/*
 * Set a controller state.
 */
static void nvme_ctrlr_set_state(struct nvme_ctrlr *ctrlr,
				 enum nvme_ctrlr_state state,
				 uint64_t timeout_in_ms)
{
	ctrlr->state = state;
	if (timeout_in_ms == NVME_TIMEOUT_INFINITE)
		ctrlr->state_timeout_ms = NVME_TIMEOUT_INFINITE;
	else
		ctrlr->state_timeout_ms = nvme_time_msec() + timeout_in_ms;
}

/*
 * Get a controller data.
 */
static int nvme_ctrlr_identify(struct nvme_ctrlr *ctrlr)
{
	int ret;

	ret = nvme_admin_identify_ctrlr(ctrlr, &ctrlr->cdata);
	if (ret != 0) {
		nvme_notice("Identify controller failed\n");
		return ret;
	}

	/*
	 * Use MDTS to ensure our default max_xfer_size doesn't
	 * exceed what the controller supports.
	 */
	if (ctrlr->cdata.mdts > 0)
		ctrlr->max_xfer_size = nvme_min(ctrlr->max_xfer_size,
						ctrlr->min_page_size
						* (1 << (ctrlr->cdata.mdts)));
	return 0;
}

/*
 * Set the number of I/O queue pairs.
 */
static int nvme_ctrlr_get_max_io_qpairs(struct nvme_ctrlr *ctrlr)
{
	unsigned int cdw0, cq_allocated, sq_allocated;
	int ret;

	ret = nvme_admin_get_feature(ctrlr, NVME_FEAT_CURRENT,
				     NVME_FEAT_NUMBER_OF_QUEUES,
				     0, &cdw0);
	if (ret != 0) {
		nvme_notice("Get feature NVME_FEAT_NUMBER_OF_QUEUES failed\n");
		return ret;
	}

	/*
	 * Data in cdw0 is 0-based.
	 * Lower 16-bits indicate number of submission queues allocated.
	 * Upper 16-bits indicate number of completion queues allocated.
	 */
	sq_allocated = (cdw0 & 0xFFFF) + 1;
	cq_allocated = (cdw0 >> 16) + 1;

	ctrlr->max_io_queues = nvme_min(sq_allocated, cq_allocated);

	return 0;
}

/*
 * Set the number of I/O queue pairs.
 */
static int nvme_ctrlr_set_num_qpairs(struct nvme_ctrlr *ctrlr)
{
	unsigned int num_queues, cdw0;
	unsigned int cq_allocated, sq_allocated;
	int ret;

	ret = nvme_ctrlr_get_max_io_qpairs(ctrlr);
	if (ret != 0) {
		nvme_notice("Failed to get the maximum of I/O qpairs\n");
		return ret;
	}

	/*
	 * Format number of I/O queue:
	 * Remove 1 as it as be be 0-based,
	 * bits 31:16 represent the number of completion queues,
	 * bits 0:15 represent the number of submission queues
	*/
	num_queues = ((ctrlr->opts.io_queues - 1) << 16) |
		(ctrlr->opts.io_queues - 1);

	/*
	 * Set the number of I/O queues.
	 * Note: The value allocated may be smaller or larger than the number
	 * of queues requested (see specifications).
	 */
	ret = nvme_admin_set_feature(ctrlr, false, NVME_FEAT_NUMBER_OF_QUEUES,
				     num_queues, 0, &cdw0);
	if (ret != 0) {
		nvme_notice("Set feature NVME_FEAT_NUMBER_OF_QUEUES failed\n");
		return ret;
	}

	/*
	 * Data in cdw0 is 0-based.
	 * Lower 16-bits indicate number of submission queues allocated.
	 * Upper 16-bits indicate number of completion queues allocated.
	 */
	sq_allocated = (cdw0 & 0xFFFF) + 1;
	cq_allocated = (cdw0 >> 16) + 1;
	ctrlr->io_queues = nvme_min(sq_allocated, cq_allocated);

	/*
	 * Make sure the number of constructed qpair listed in free_io_qpairs
	 * will not be more than the requested one.
	 */
	ctrlr->io_queues = nvme_min(ctrlr->io_queues, ctrlr->opts.io_queues);

	return 0;
}

static void nvme_ctrlr_destruct_namespaces(struct nvme_ctrlr *ctrlr)
{

	if (ctrlr->ns) {
		free(ctrlr->ns);
		ctrlr->ns = NULL;
		ctrlr->nr_ns = 0;
	}

	if (ctrlr->nsdata) {
		nvme_free(ctrlr->nsdata);
		ctrlr->nsdata = NULL;
	}
}

static int nvme_ctrlr_construct_namespaces(struct nvme_ctrlr *ctrlr)
{
	unsigned int i, nr_ns = ctrlr->cdata.nn;
	struct nvme_ns *ns = NULL;

	/*
	 * ctrlr->nr_ns may be 0 (startup) or a different number of
	 * namespaces (reset), so check if we need to reallocate.
	 */
	if (nr_ns != ctrlr->nr_ns) {

		nvme_ctrlr_destruct_namespaces(ctrlr);

		ctrlr->ns = calloc(nr_ns, sizeof(struct nvme_ns));
		if (!ctrlr->ns)
			goto fail;

		nvme_debug("Allocate %u namespace data\n", nr_ns);
		ctrlr->nsdata = nvme_calloc(nr_ns, sizeof(struct nvme_ns_data),
					    PAGE_SIZE);
		if (!ctrlr->nsdata)
			goto fail;

		ctrlr->nr_ns = nr_ns;

	}

	for (i = 0; i < nr_ns; i++) {
		ns = &ctrlr->ns[i];
		if (nvme_ns_construct(ctrlr, ns, i + 1) != 0)
			goto fail;
	}

	return 0;

fail:
	nvme_ctrlr_destruct_namespaces(ctrlr);

	return -1;
}

/*
 * Forward declaration.
 */
static int nvme_ctrlr_construct_and_submit_aer(struct nvme_ctrlr *ctrlr,
				struct nvme_async_event_request *aer);

/*
 * Async event completion callback.
 */
static void nvme_ctrlr_async_event_cb(void *arg, const struct nvme_cpl *cpl)
{
	struct nvme_async_event_request	*aer = arg;
	struct nvme_ctrlr *ctrlr = aer->ctrlr;

	if (cpl->status.sc == NVME_SC_ABORTED_SQ_DELETION)
		/*
		 *  This is simulated when controller is being shut down, to
		 *  effectively abort outstanding asynchronous event requests
		 *  and make sure all memory is freed. Do not repost the
		 *  request in this case.
		 */
		return;

	if (ctrlr->aer_cb_fn != NULL)
		ctrlr->aer_cb_fn(ctrlr->aer_cb_arg, cpl);

	/*
	 * Repost another asynchronous event request to replace
	 * the one that just completed.
	 */
	if (nvme_ctrlr_construct_and_submit_aer(ctrlr, aer))
		/*
		 * We can't do anything to recover from a failure here,
		 * so just print a warning message and leave the
		 * AER unsubmitted.
		 */
		nvme_err("Initialize AER failed\n");
}

/*
 * Issue an async event request.
 */
static int nvme_ctrlr_construct_and_submit_aer(struct nvme_ctrlr *ctrlr,
					       struct nvme_async_event_request *aer)
{
	struct nvme_request *req;

	req = nvme_request_allocate_null(&ctrlr->adminq,
					 nvme_ctrlr_async_event_cb, aer);
	if (req == NULL)
		return -1;

	aer->ctrlr = ctrlr;
	aer->req = req;
	req->cmd.opc = NVME_OPC_ASYNC_EVENT_REQUEST;

	return nvme_qpair_submit_request(&ctrlr->adminq, req);
}

/*
 * Configure async event management.
 */
static int nvme_ctrlr_configure_aer(struct nvme_ctrlr *ctrlr)
{
	union nvme_critical_warning_state state;
	struct nvme_async_event_request	*aer;
	unsigned int i;
	int ret;

	state.raw = 0xFF;
	state.bits.reserved = 0;

	ret =  nvme_admin_set_feature(ctrlr, false,
				      NVME_FEAT_ASYNC_EVENT_CONFIGURATION,
				      state.raw, 0, NULL);
	if (ret != 0) {
		nvme_notice("Set feature ASYNC_EVENT_CONFIGURATION failed\n");
		return ret;
	}

	/* aerl is a zero-based value, so we need to add 1 here. */
	ctrlr->num_aers = nvme_min(NVME_MAX_ASYNC_EVENTS,
				   (ctrlr->cdata.aerl + 1));

	for (i = 0; i < ctrlr->num_aers; i++) {
		aer = &ctrlr->aer[i];
		if (nvme_ctrlr_construct_and_submit_aer(ctrlr, aer)) {
			nvme_notice("Construct AER failed\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Start a controller.
 */
static int nvme_ctrlr_start(struct nvme_ctrlr *ctrlr)
{

	nvme_qpair_reset(&ctrlr->adminq);
	nvme_qpair_enable(&ctrlr->adminq);

	if (nvme_ctrlr_identify(ctrlr) != 0)
		return -1;

	if (nvme_ctrlr_set_num_qpairs(ctrlr) != 0)
		return -1;

	if (nvme_ctrlr_init_io_qpairs(ctrlr))
		return -1;

	if (nvme_ctrlr_construct_namespaces(ctrlr) != 0)
		return -1;

	if (nvme_ctrlr_configure_aer(ctrlr) != 0)
		nvme_warning("controller does not support AER!\n");

	nvme_ctrlr_set_supported_log_pages(ctrlr);
	nvme_ctrlr_set_supported_features(ctrlr);

	if (ctrlr->cdata.sgls.supported)
		ctrlr->flags |= NVME_CTRLR_SGL_SUPPORTED;

	return 0;
}

/*
 * Memory map the controller side buffer.
 */
static void nvme_ctrlr_map_cmb(struct nvme_ctrlr *ctrlr)
{
	int ret;
	void *addr;
	uint32_t bir;
	union nvme_cmbsz_register cmbsz;
	union nvme_cmbloc_register cmbloc;
	uint64_t size, unit_size, offset, bar_size, bar_phys_addr;

	cmbsz.raw = nvme_reg_mmio_read_4(ctrlr, cmbsz.raw);
	cmbloc.raw = nvme_reg_mmio_read_4(ctrlr, cmbloc.raw);
	if (!cmbsz.bits.sz)
		goto out;

	/* Values 0 2 3 4 5 are valid for BAR */
	bir = cmbloc.bits.bir;
	if (bir > 5 || bir == 1)
		goto out;

	/* unit size for 4KB/64KB/1MB/16MB/256MB/4GB/64GB */
	unit_size = (uint64_t)1 << (12 + 4 * cmbsz.bits.szu);

	/* controller memory buffer size in Bytes */
	size = unit_size * cmbsz.bits.sz;

	/* controller memory buffer offset from BAR in Bytes */
	offset = unit_size * cmbloc.bits.ofst;

	nvme_pcicfg_get_bar_addr_len(ctrlr->pci_dev, bir, &bar_phys_addr,
				     &bar_size);

	if (offset > bar_size)
		goto out;

	if (size > bar_size - offset)
		goto out;

	ret = nvme_pcicfg_map_bar_write_combine(ctrlr->pci_dev, bir, &addr);
	if ((ret != 0) || addr == NULL)
		goto out;

	ctrlr->cmb_bar_virt_addr = addr;
	ctrlr->cmb_bar_phys_addr = bar_phys_addr;
	ctrlr->cmb_size = size;
	ctrlr->cmb_current_offset = offset;

	if (!cmbsz.bits.sqs)
		ctrlr->opts.use_cmb_sqs = false;

	return;

out:
	ctrlr->cmb_bar_virt_addr = NULL;
	ctrlr->opts.use_cmb_sqs = false;

	return;
}

/*
 * Unmap the controller side buffer.
 */
static int nvme_ctrlr_unmap_cmb(struct nvme_ctrlr *ctrlr)
{
	union nvme_cmbloc_register cmbloc;
	void *addr = ctrlr->cmb_bar_virt_addr;
	int ret = 0;

	if (addr) {
		cmbloc.raw = nvme_reg_mmio_read_4(ctrlr, cmbloc.raw);
		ret = nvme_pcicfg_unmap_bar(ctrlr->pci_dev, cmbloc.bits.bir,
					    addr);
	}
	return ret;
}

/*
 * Map the controller PCI bars.
 */
static int nvme_ctrlr_map_bars(struct nvme_ctrlr *ctrlr)
{
	void *addr;
	int ret;

	ret = nvme_pcicfg_map_bar(ctrlr->pci_dev, 0, 0, &addr);
	if (ret != 0 || addr == NULL) {
		nvme_err("Map PCI device bar failed %d (%s)\n",
			 ret, strerror(ret));
		return ret;
	}

	nvme_debug("Controller BAR mapped at %p\n", addr);

	ctrlr->regs = (volatile struct nvme_registers *)addr;
	nvme_ctrlr_map_cmb(ctrlr);

	return 0;
}

/*
 * Unmap the controller PCI bars.
 */
static int nvme_ctrlr_unmap_bars(struct nvme_ctrlr *ctrlr)
{
	void *addr = (void *)ctrlr->regs;
	int ret;

	ret = nvme_ctrlr_unmap_cmb(ctrlr);
	if (ret != 0) {
		nvme_err("Unmap controller side buffer failed %d\n", ret);
		return ret;
	}

	if (addr) {
		ret = nvme_pcicfg_unmap_bar(ctrlr->pci_dev, 0, addr);
		if (ret != 0) {
			nvme_err("Unmap PCI device bar failed %d\n", ret);
			return ret;
		}
	}

	return 0;
}

/*
 * Set a controller in the failed state.
 */
static void nvme_ctrlr_fail(struct nvme_ctrlr *ctrlr)
{
	unsigned int i;

	ctrlr->failed = true;

	nvme_qpair_fail(&ctrlr->adminq);
	if (ctrlr->ioq)
		for (i = 0; i < ctrlr->io_queues; i++)
			nvme_qpair_fail(&ctrlr->ioq[i]);
}

/*
 * This function will be called repeatedly during initialization
 * until the controller is ready.
 */
static int nvme_ctrlr_init(struct nvme_ctrlr *ctrlr)
{
	unsigned int ready_timeout_in_ms = nvme_ctrlr_get_ready_to_in_ms(ctrlr);
	int ret;

	/*
	 * Check if the current initialization step is done or has timed out.
	 */
	switch (ctrlr->state) {

	case NVME_CTRLR_STATE_INIT:

		/* Begin the hardware initialization by making
		 * sure the controller is disabled. */
		if (nvme_ctrlr_enabled(ctrlr)) {
			/*
			 * Disable the controller to cause a reset.
			 */
			if (!nvme_ctrlr_ready(ctrlr)) {
				/* Wait for the controller to be ready */
				nvme_ctrlr_set_state(ctrlr,
				      NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_1,
				      ready_timeout_in_ms);
				return 0;
			}

			/*
			 * The controller is enabled and ready.
			 * It can be immediatly disabled
			 */
			nvme_ctrlr_disable(ctrlr);
			nvme_ctrlr_set_state(ctrlr,
				      NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_0,
				      ready_timeout_in_ms);

			if (ctrlr->quirks & NVME_QUIRK_DELAY_BEFORE_CHK_RDY)
				nvme_msleep(2000);

			return 0;
		}

		if (nvme_ctrlr_ready(ctrlr)) {
			/*
			 * Controller is in the process of shutting down.
			 * We need to wait for CSTS.RDY to become 0.
			 */
			nvme_ctrlr_set_state(ctrlr,
				      NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_0,
				      ready_timeout_in_ms);
			return 0;
		}

		/*
		 * Controller is currently disabled.
		 * We can jump straight to enabling it.
		 */
		ret = nvme_ctrlr_enable(ctrlr);
		if (ret)
			nvme_err("Enable controller failed\n");
		else
			nvme_ctrlr_set_state(ctrlr,
				       NVME_CTRLR_STATE_ENABLE_WAIT_FOR_READY_1,
				       ready_timeout_in_ms);
		return ret;

	case NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_1:

		if (nvme_ctrlr_ready(ctrlr)) {
			/* CC.EN = 1 && CSTS.RDY = 1,
			 * so we can disable the controller now. */
			nvme_ctrlr_disable(ctrlr);
			nvme_ctrlr_set_state(ctrlr,
				      NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_0,
				      ready_timeout_in_ms);
			return 0;
		}

		break;

	case NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_0:

		if (!nvme_ctrlr_ready(ctrlr)) {
			/* CC.EN = 0 && CSTS.RDY = 0,
			 * so we can enable the controller now. */
			ret = nvme_ctrlr_enable(ctrlr);
			if (ret)
				nvme_err("Enable controller failed\n");
			else
				nvme_ctrlr_set_state(ctrlr,
				       NVME_CTRLR_STATE_ENABLE_WAIT_FOR_READY_1,
				       ready_timeout_in_ms);
			return ret;
		}
		break;

	case NVME_CTRLR_STATE_ENABLE_WAIT_FOR_READY_1:

		if (nvme_ctrlr_ready(ctrlr)) {
			if (ctrlr->quirks & NVME_QUIRK_DELAY_AFTER_RDY)
				nvme_msleep(2000);

			ret = nvme_ctrlr_start(ctrlr);
			if (ret)
				nvme_err("Start controller failed\n");
			else
				nvme_ctrlr_set_state(ctrlr,
						     NVME_CTRLR_STATE_READY,
						     NVME_TIMEOUT_INFINITE);
			return ret;
		}
		break;

	default:
		nvme_panic("Unhandled ctrlr state %d\n", ctrlr->state);
		nvme_ctrlr_fail(ctrlr);
		return -1;
	}

	if ((ctrlr->state_timeout_ms != NVME_TIMEOUT_INFINITE) &&
	    (nvme_time_msec() > ctrlr->state_timeout_ms)) {
		nvme_err("Initialization timed out in state %d\n",
			 ctrlr->state);
		nvme_ctrlr_fail(ctrlr);
		return -1;
	}

	return 0;
}

/*
 * Reset a controller.
 */
static int nvme_ctrlr_reset(struct nvme_ctrlr *ctrlr)
{
	struct nvme_qpair *qpair;
	unsigned int i;

	if (ctrlr->resetting || ctrlr->failed)
		/*
		 * Controller is already resetting or has failed. Return
		 * immediately since there is no need to kick off another
		 * reset in these cases.
		 */
		return 0;

	ctrlr->resetting = true;

	/* Disable all queues before disabling the controller hardware. */
	nvme_qpair_disable(&ctrlr->adminq);
	for (i = 0; i < ctrlr->io_queues; i++)
		nvme_qpair_disable(&ctrlr->ioq[i]);

	/* Set the state back to INIT to cause a full hardware reset. */
	nvme_ctrlr_set_state(ctrlr, NVME_CTRLR_STATE_INIT,
			     NVME_TIMEOUT_INFINITE);

	while (ctrlr->state != NVME_CTRLR_STATE_READY) {
		if (nvme_ctrlr_init(ctrlr) != 0) {
			nvme_crit("Controller reset failed\n");
			nvme_ctrlr_fail(ctrlr);
			goto out;
		}
	}

	/* Reinitialize qpairs */
	TAILQ_FOREACH(qpair, &ctrlr->active_io_qpairs, tailq) {
		if (nvme_ctrlr_create_qpair(ctrlr, qpair) != 0)
			nvme_ctrlr_fail(ctrlr);
	}

out:
	ctrlr->resetting = false;

	return ctrlr->failed ? -1 : 0;
}

/*
 * Set a controller options.
 */
static void nvme_ctrlr_set_opts(struct nvme_ctrlr *ctrlr,
				struct nvme_ctrlr_opts *opts)
{
	if (opts)
		memcpy(&ctrlr->opts, opts, sizeof(struct nvme_ctrlr_opts));
	else
		memset(&ctrlr->opts, 0, sizeof(struct nvme_ctrlr_opts));

	if (ctrlr->opts.io_queues == 0)
		ctrlr->opts.io_queues = DEFAULT_MAX_IO_QUEUES;

	if (ctrlr->opts.io_queues > NVME_MAX_IO_QUEUES) {
		nvme_info("Limiting requested I/O queues %u to %d\n",
			  ctrlr->opts.io_queues, NVME_MAX_IO_QUEUES);
		ctrlr->opts.io_queues = NVME_MAX_IO_QUEUES;
	}
}

/*
 * Attach a PCI controller.
 */
struct nvme_ctrlr *
nvme_ctrlr_attach(struct pci_device *pci_dev,
		  struct nvme_ctrlr_opts *opts)
{
	struct nvme_ctrlr *ctrlr;
	union nvme_cap_register	cap;
	uint32_t cmd_reg;
	int ret;

	/* Get a new controller handle */
	ctrlr = malloc(sizeof(struct nvme_ctrlr));
	if (!ctrlr) {
		nvme_err("Allocate controller handle failed\n");
		return NULL;
	}

	nvme_debug("New controller handle %p\n", ctrlr);

	/* Initialize the handle */
	memset(ctrlr, 0, sizeof(struct nvme_ctrlr));
	ctrlr->pci_dev = pci_dev;
	ctrlr->resetting = false;
	ctrlr->failed = false;
	TAILQ_INIT(&ctrlr->free_io_qpairs);
	TAILQ_INIT(&ctrlr->active_io_qpairs);
	pthread_mutex_init(&ctrlr->lock, NULL);
	ctrlr->quirks = nvme_ctrlr_get_quirks(pci_dev);

	nvme_ctrlr_set_state(ctrlr,
			     NVME_CTRLR_STATE_INIT,
			     NVME_TIMEOUT_INFINITE);

	ret = nvme_ctrlr_map_bars(ctrlr);
	if (ret != 0) {
		nvme_err("Map controller BAR failed\n");
		pthread_mutex_destroy(&ctrlr->lock);
		free(ctrlr);
		return NULL;
	}

	/* Enable PCI busmaster and disable INTx */
	nvme_pcicfg_read32(pci_dev, &cmd_reg, 4);
	cmd_reg |= 0x0404;
	nvme_pcicfg_write32(pci_dev, cmd_reg, 4);

	/*
	 * Doorbell stride is 2 ^ (dstrd + 2),
	 * but we want multiples of 4, so drop the + 2.
	 */
	cap.raw = nvme_reg_mmio_read_8(ctrlr, cap.raw);
	ctrlr->doorbell_stride_u32 = 1 << cap.bits.dstrd;
	ctrlr->min_page_size = 1 << (12 + cap.bits.mpsmin);

	/* Set default transfer size */
	ctrlr->max_xfer_size = NVME_MAX_XFER_SIZE;

	/* Create the admin queue pair */
	ret = nvme_qpair_construct(ctrlr, &ctrlr->adminq, 0,
				   NVME_ADMIN_ENTRIES, NVME_ADMIN_TRACKERS);
	if (ret != 0) {
		nvme_err("Initialize admin queue pair failed\n");
		goto err;
	}

	/* Set options and then initialize */
	nvme_ctrlr_set_opts(ctrlr, opts);
	do {
		ret = nvme_ctrlr_init(ctrlr);
		if (ret)
			goto err;
	} while (ctrlr->state != NVME_CTRLR_STATE_READY);

	return ctrlr;

err:
	nvme_ctrlr_detach(ctrlr);

	return NULL;
}

/*
 * Detach a PCI controller.
 */
void nvme_ctrlr_detach(struct nvme_ctrlr *ctrlr)
{
	struct nvme_qpair *qpair;
	uint32_t i;

	while (!TAILQ_EMPTY(&ctrlr->active_io_qpairs)) {
		qpair = TAILQ_FIRST(&ctrlr->active_io_qpairs);
		nvme_ioqp_release(qpair);
	}

	nvme_ctrlr_shutdown(ctrlr);

	nvme_ctrlr_destruct_namespaces(ctrlr);
	if (ctrlr->ioq) {
		for (i = 0; i < ctrlr->io_queues; i++)
			nvme_qpair_destroy(&ctrlr->ioq[i]);
		free(ctrlr->ioq);
	}

	nvme_qpair_destroy(&ctrlr->adminq);

	nvme_ctrlr_unmap_bars(ctrlr);

	pthread_mutex_destroy(&ctrlr->lock);
	free(ctrlr);
}

/*
 * Get a controller feature.
 */
int nvme_ctrlr_get_feature(struct nvme_ctrlr *ctrlr,
			   enum nvme_feat_sel sel, enum nvme_feat feature,
			   uint32_t cdw11,
			   uint32_t *attributes)
{
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_get_feature(ctrlr, sel, feature, cdw11, attributes);
	if (ret != 0)
		nvme_notice("Get feature 0x%08x failed\n",
			    (unsigned int) feature);

	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Set a controller feature.
 */
int nvme_ctrlr_set_feature(struct nvme_ctrlr *ctrlr,
			   bool save, enum nvme_feat feature,
			   uint32_t cdw11, uint32_t cdw12,
			   uint32_t *attributes)
{
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_set_feature(ctrlr, save, feature,
				     cdw11, cdw12, attributes);
	if (ret != 0)
		nvme_notice("Set feature 0x%08x failed\n",
			    (unsigned int) feature);

	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Attach a namespace.
 */
int nvme_ctrlr_attach_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid,
			 struct nvme_ctrlr_list *clist)
{
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_attach_ns(ctrlr, nsid, clist);
	if (ret) {
		nvme_notice("Attach namespace %u failed\n", nsid);
		goto out;
	}

	ret = nvme_ctrlr_reset(ctrlr);
	if (ret != 0)
		nvme_notice("Reset controller failed\n");

out:
	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Detach a namespace.
 */
int nvme_ctrlr_detach_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid,
			 struct nvme_ctrlr_list *clist)
{
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_detach_ns(ctrlr, nsid, clist);
	if (ret != 0) {
		nvme_notice("Detach namespace %u failed\n", nsid);
		goto out;
	}

	ret = nvme_ctrlr_reset(ctrlr);
	if (ret)
		nvme_notice("Reset controller failed\n");

out:
	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Create a namespace.
 */
unsigned int nvme_ctrlr_create_ns(struct nvme_ctrlr *ctrlr,
				  struct nvme_ns_data *nsdata)
{
	unsigned int nsid;
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_create_ns(ctrlr, nsdata, &nsid);
	if (ret != 0) {
		nvme_notice("Create namespace failed\n");
		nsid = 0;
	}

	pthread_mutex_unlock(&ctrlr->lock);

	return nsid;
}

/*
 * Delete a namespace.
 */
int nvme_ctrlr_delete_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid)
{
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_delete_ns(ctrlr, nsid);
	if (ret != 0) {
		nvme_notice("Delete namespace %u failed\n", nsid);
		goto out;
	}

	ret = nvme_ctrlr_reset(ctrlr);
	if (ret)
		nvme_notice("Reset controller failed\n");

out:
	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Format NVM media.
 */
int nvme_ctrlr_format_ns(struct nvme_ctrlr *ctrlr, unsigned int nsid,
			 struct nvme_format *format)
{
	int ret;

	pthread_mutex_lock(&ctrlr->lock);

	ret = nvme_admin_format_nvm(ctrlr, nsid, format);
	if (ret != 0) {
		if (nsid == NVME_GLOBAL_NS_TAG)
			nvme_notice("Format device failed\n");
		else
			nvme_notice("Format namespace %u failed\n", nsid);
		goto out;
	}

	ret = nvme_ctrlr_reset(ctrlr);
	if (ret)
		nvme_notice("Reset controller failed\n");

out:
	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Update a device firmware.
 */
int nvme_ctrlr_update_firmware(struct nvme_ctrlr *ctrlr,
			       void *fw, size_t size, int slot)
{
	struct nvme_fw_commit fw_commit;
	unsigned int size_remaining = size, offset = 0, transfer;
	void *f = fw;
	int ret;

	if (size & 0x3) {
		nvme_err("Invalid firmware size\n");
		return EINVAL;
	}

	pthread_mutex_lock(&ctrlr->lock);

	/* Download firmware */
	while (size_remaining > 0) {

		transfer = nvme_min(size_remaining, ctrlr->min_page_size);

		ret = nvme_admin_fw_image_dl(ctrlr, f, transfer, offset);
		if (ret != 0) {
			nvme_err("Download FW (%u B at %u) failed\n",
				 transfer, offset);
			goto out;
		}

		f += transfer;
		offset += transfer;
		size_remaining -= transfer;

	}

	/* Commit firmware */
	memset(&fw_commit, 0, sizeof(struct nvme_fw_commit));
	fw_commit.fs = slot;
	fw_commit.ca = NVME_FW_COMMIT_REPLACE_IMG;

	ret = nvme_admin_fw_commit(ctrlr, &fw_commit);
	if (ret != 0) {
		nvme_err("Commit downloaded FW (%zu B) failed\n",
			 size);
		goto out;
	}

	ret = nvme_ctrlr_reset(ctrlr);
	if (ret)
		nvme_notice("Reset controller failed\n");

out:
	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Get an unused I/O queue pair.
 */
struct nvme_qpair *nvme_ioqp_get(struct nvme_ctrlr *ctrlr,
				 enum nvme_qprio qprio, unsigned int qd)
{
	struct nvme_qpair *qpair = NULL;
	union nvme_cc_register cc;
	uint32_t trackers;
	int ret;

	cc.raw = nvme_reg_mmio_read_4(ctrlr, cc.raw);

	/* Only the low 2 bits (values 0, 1, 2, 3) of QPRIO are valid. */
	if ((qprio & 3) != qprio)
		return NULL;

	/*
	 * Only value NVME_QPRIO_URGENT(0) is valid for the
	 * default round robin arbitration method.
	 */
	if ((cc.bits.ams == NVME_CC_AMS_RR) && (qprio != NVME_QPRIO_URGENT)) {
		nvme_err("Invalid queue priority for default round "
			 "robin arbitration method\n");
		return NULL;
	}

	/* I/O qpairs number of entries belong to [2, io_qpairs_max_entries] */
	if (qd == 1) {
		nvme_err("Invalid queue depth\n");
		return NULL;
	}

	if (qd == 0 || qd > ctrlr->io_qpairs_max_entries)
		qd = ctrlr->io_qpairs_max_entries;

	/*
	 * No need to have more trackers than entries in the submit queue.
	 * Note also that for a queue size of N, we can only have (N-1)
	 * commands outstanding, hence the "-1" here.
	 */
	trackers = nvme_min(NVME_IO_TRACKERS, (qd - 1));

	pthread_mutex_lock(&ctrlr->lock);

	/* Get the first available qpair structure */
	qpair = TAILQ_FIRST(&ctrlr->free_io_qpairs);
	if (qpair == NULL) {
		/* No free queue IDs */
		nvme_err("No free I/O queue pairs\n");
		goto out;
	}

	/* Construct the qpair */
	ret = nvme_qpair_construct(ctrlr, qpair, qprio, qd, trackers);
	if (ret != 0) {
		nvme_qpair_destroy(qpair);
		qpair = NULL;
		goto out;
	}

	/*
	 * At this point, qpair contains a preallocated submission
	 * and completion queue and a unique queue ID, but it is not
	 * yet created on the controller.
	 * Fill out the submission queue priority and send out the
	 * Create I/O Queue commands.
	 */
	if (nvme_ctrlr_create_qpair(ctrlr, qpair) != 0) {
		nvme_err("Create queue pair on the controller failed\n");
		nvme_qpair_destroy(qpair);
		qpair = NULL;
		goto out;
	}

	TAILQ_REMOVE(&ctrlr->free_io_qpairs, qpair, tailq);
	TAILQ_INSERT_TAIL(&ctrlr->active_io_qpairs, qpair, tailq);

out:
	pthread_mutex_unlock(&ctrlr->lock);

	return qpair;
}

/*
 * Free an I/O queue pair.
 */
int nvme_ioqp_release(struct nvme_qpair *qpair)
{
	struct nvme_ctrlr *ctrlr;
	int ret;

	if (qpair == NULL)
		return 0;

	ctrlr = qpair->ctrlr;

	pthread_mutex_lock(&ctrlr->lock);

	/* Delete the I/O submission and completion queues */
	ret = nvme_ctrlr_delete_qpair(ctrlr, qpair);
	if (ret != 0) {
		nvme_notice("Delete queue pair %u failed\n", qpair->id);
	} else {
		TAILQ_REMOVE(&ctrlr->active_io_qpairs, qpair, tailq);
		TAILQ_INSERT_HEAD(&ctrlr->free_io_qpairs, qpair, tailq);
	}

	pthread_mutex_unlock(&ctrlr->lock);

	return ret;
}

/*
 * Submit an NVMe command using the specified I/O queue pair.
 */
int nvme_ioqp_submit_cmd(struct nvme_qpair *qpair,
			 struct nvme_cmd *cmd,
			 void *buf, size_t len,
			 nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	int ret = ENOMEM;

	req = nvme_request_allocate_contig(qpair, buf, len, cb_fn, cb_arg);
	if (req) {
		memcpy(&req->cmd, cmd, sizeof(req->cmd));
		ret = nvme_qpair_submit_request(qpair, req);
	}

	return ret;
}
