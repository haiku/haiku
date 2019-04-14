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

static inline struct nvme_ns_data *nvme_ns_get_data(struct nvme_ns *ns)
{
	return &ns->ctrlr->nsdata[ns->id - 1];
}

static int nvme_ns_identify_update(struct nvme_ns *ns)
{
	struct nvme_ctrlr *ctrlr = ns->ctrlr;
	struct nvme_ns_data *nsdata = nvme_ns_get_data(ns);
	uint32_t sector_size;
	int ret;

	ret = nvme_admin_identify_ns(ctrlr, ns->id, nsdata);
	if (ret != 0) {
		nvme_err("nvme_identify_namespace failed\n");
		return ret;
	}

	sector_size = 1 << nsdata->lbaf[nsdata->flbas.format].lbads;

	ns->sector_size = sector_size;
	ns->sectors_per_max_io = ctrlr->max_xfer_size / sector_size;
	ns->sectors_per_stripe = ns->stripe_size / sector_size;

	ns->flags = 0x0000;

	if (ctrlr->cdata.oncs.dsm)
		ns->flags |= NVME_NS_DEALLOCATE_SUPPORTED;

	if (ctrlr->cdata.vwc.present)
		ns->flags |= NVME_NS_FLUSH_SUPPORTED;

	if (ctrlr->cdata.oncs.write_zeroes)
		ns->flags |= NVME_NS_WRITE_ZEROES_SUPPORTED;

	if (nsdata->nsrescap.raw)
		ns->flags |= NVME_NS_RESERVATION_SUPPORTED;

	ns->md_size = nsdata->lbaf[nsdata->flbas.format].ms;
	ns->pi_type = NVME_FMT_NVM_PROTECTION_DISABLE;

	if (nsdata->lbaf[nsdata->flbas.format].ms && nsdata->dps.pit) {
		ns->flags |= NVME_NS_DPS_PI_SUPPORTED;
		ns->pi_type = nsdata->dps.pit;
		if (nsdata->flbas.extended)
			ns->flags |= NVME_NS_EXTENDED_LBA_SUPPORTED;
	}

	return 0;
}

/*
 * Initialize a namespace.
 */
int nvme_ns_construct(struct nvme_ctrlr *ctrlr, struct nvme_ns *ns,
		      unsigned int id)
{
	uint32_t pci_devid;

	ns->ctrlr = ctrlr;
	ns->id = id;
	ns->stripe_size = 0;

	nvme_pcicfg_read32(ctrlr->pci_dev, &pci_devid, 0);
	if (pci_devid == INTEL_DC_P3X00_DEVID && ctrlr->cdata.vs[3] != 0)
		ns->stripe_size = (1 << ctrlr->cdata.vs[3])
			* ctrlr->min_page_size;

	return nvme_ns_identify_update(ns);
}

/*
 * Open a namespace.
 */
struct nvme_ns *nvme_ns_open(struct nvme_ctrlr *ctrlr, unsigned int ns_id)
{
	struct nvme_ns *ns = NULL;

	pthread_mutex_lock(&ctrlr->lock);

	if (ns_id >= 1 && ns_id <= ctrlr->nr_ns) {
		ns = &ctrlr->ns[ns_id - 1];
		ns->open_count++;
	}

	pthread_mutex_unlock(&ctrlr->lock);

	return ns;
}

/*
 * Get the controller of an open name space and lock it,
 * making sure in the process that the ns handle is valid.
 */
static struct nvme_ctrlr *nvme_ns_ctrlr_lock(struct nvme_ns *ns)
{
	struct nvme_ctrlr *ctrlr;

	if (!ns)
		return NULL;

	ctrlr = ns->ctrlr;
	if (ns->id < 1 ||
	    ns->id > ctrlr->nr_ns ||
	    ns != &ctrlr->ns[ns->id - 1])
		return NULL;

	pthread_mutex_lock(&ctrlr->lock);

	/*
	 * Between the check and lock, the ns may have gone away.
	 * So check again, and make sure that the name space is open.
	 */
	if (ns->id > ctrlr->nr_ns ||
	    ns != &ctrlr->ns[ns->id - 1] ||
	    ns->open_count == 0) {
		pthread_mutex_unlock(&ctrlr->lock);
		return NULL;
	}

	return ctrlr;
}

/*
 * Close an open namespace.
 */
int nvme_ns_close(struct nvme_ns *ns)
{
	struct nvme_ctrlr *ctrlr;

	ctrlr = nvme_ns_ctrlr_lock(ns);
	if (!ctrlr) {
		nvme_err("Invalid name space handle\n");
		return EINVAL;
	}

	ns->open_count--;

	pthread_mutex_unlock(&ctrlr->lock);

	return 0;
}

/*
 * Get namespace information
 */
int nvme_ns_stat(struct nvme_ns *ns, struct nvme_ns_stat *ns_stat)
{
	struct nvme_ctrlr *ctrlr;

	ctrlr = nvme_ns_ctrlr_lock(ns);
	if (!ctrlr) {
		nvme_err("Invalid name space handle\n");
		return EINVAL;
	}

	ns_stat->id = ns->id;
	ns_stat->sector_size = ns->sector_size;
	ns_stat->sectors = nvme_ns_get_data(ns)->nsze;
	ns_stat->flags = ns->flags;
	ns_stat->pi_type = ns->pi_type;
	ns_stat->md_size = ns->md_size;

	pthread_mutex_unlock(&ctrlr->lock);

	return 0;
}

/*
 * Get namespace data
 */
int nvme_ns_data(struct nvme_ns *ns, struct nvme_ns_data *nsdata)
{
	struct nvme_ctrlr *ctrlr;

	ctrlr = nvme_ns_ctrlr_lock(ns);
	if (!ctrlr) {
		nvme_err("Invalid name space handle\n");
		return EINVAL;
	}

	memcpy(nsdata, nvme_ns_get_data(ns), sizeof(struct nvme_ns_data));

	pthread_mutex_unlock(&ctrlr->lock);

	return 0;
}

static struct nvme_request *_nvme_ns_rw(struct nvme_ns *ns,
			struct nvme_qpair *qpair,
			const struct nvme_payload *payload, uint64_t lba,
			uint32_t lba_count, nvme_cmd_cb cb_fn,
			void *cb_arg, uint32_t opc, uint32_t io_flags,
			uint16_t apptag_mask, uint16_t apptag);

static struct nvme_request *
_nvme_ns_split_request(struct nvme_ns *ns,
		       struct nvme_qpair *qpair,
		       const struct nvme_payload *payload,
		       uint64_t lba, uint32_t lba_count,
		       nvme_cmd_cb cb_fn, void *cb_arg,
		       uint32_t opc,
		       uint32_t io_flags,
		       struct nvme_request *req,
		       uint32_t sectors_per_max_io,
		       uint32_t sector_mask,
		       uint16_t apptag_mask,
		       uint16_t apptag)
{
	uint32_t sector_size = ns->sector_size;
	uint32_t md_size = ns->md_size;
	uint32_t remaining_lba_count = lba_count;
	uint32_t offset = 0;
	uint32_t md_offset = 0;
	struct nvme_request *child, *tmp;

	if (ns->flags & NVME_NS_DPS_PI_SUPPORTED) {
		/* for extended LBA only */
		if ((ns->flags & NVME_NS_EXTENDED_LBA_SUPPORTED)
		    && !(io_flags & NVME_IO_FLAGS_PRACT))
			sector_size += ns->md_size;
	}

	while (remaining_lba_count > 0) {

		lba_count = sectors_per_max_io - (lba & sector_mask);
		lba_count = nvme_min(remaining_lba_count, lba_count);

		child = _nvme_ns_rw(ns, qpair, payload, lba, lba_count, cb_fn,
				    cb_arg, opc, io_flags, apptag_mask, apptag);
		if (child == NULL) {
			if (req->child_reqs) {
				/* free all child nvme_request  */
				TAILQ_FOREACH_SAFE(child, &req->children,
						   child_tailq, tmp) {
					nvme_request_remove_child(req, child);
					nvme_request_free(child);
				}
			}
			return NULL;
		}

		child->payload_offset = offset;

		/* for separate metadata buffer only */
		if (payload->md)
			child->md_offset = md_offset;

		nvme_request_add_child(req, child);

		remaining_lba_count -= lba_count;
		lba += lba_count;
		offset += lba_count * sector_size;
		md_offset += lba_count * md_size;

	}

	return req;
}

static struct nvme_request *_nvme_ns_rw(struct nvme_ns *ns,
					struct nvme_qpair *qpair,
					const struct nvme_payload *payload,
					uint64_t lba, uint32_t lba_count,
					nvme_cmd_cb cb_fn, void *cb_arg,
					uint32_t opc,
					uint32_t io_flags,
					uint16_t apptag_mask,
					uint16_t apptag)
{
	struct nvme_request *req;
	struct nvme_cmd	*cmd;
	uint64_t *tmp_lba;
	uint32_t sector_size;
	uint32_t sectors_per_max_io;
	uint32_t sectors_per_stripe;

	/* The bottom 16 bits must be empty */
	if (io_flags & 0xFFFF)
		return NULL;

	sector_size = ns->sector_size;
	sectors_per_max_io = ns->sectors_per_max_io;
	sectors_per_stripe = ns->sectors_per_stripe;

	if (ns->flags & NVME_NS_DPS_PI_SUPPORTED)
		/* for extended LBA only */
		if ((ns->flags & NVME_NS_EXTENDED_LBA_SUPPORTED) &&
		    !(io_flags & NVME_IO_FLAGS_PRACT))
			sector_size += ns->md_size;

	req = nvme_request_allocate(qpair, payload,
				    lba_count * sector_size, cb_fn, cb_arg);
	if (req == NULL)
		return NULL;

	/*
	 * Intel DC P3*00 NVMe controllers benefit from driver-assisted striping.
	 * If this controller defines a stripe boundary and this I/O spans
	 * a stripe boundary, split the request into multiple requests and
	 * submit each separately to hardware.
	 */
	if (sectors_per_stripe > 0 &&
	    (((lba & (sectors_per_stripe - 1)) + lba_count) > sectors_per_stripe))
		return _nvme_ns_split_request(ns, qpair, payload, lba,
					      lba_count, cb_fn, cb_arg, opc,
					      io_flags, req, sectors_per_stripe,
					      sectors_per_stripe - 1,
					      apptag_mask, apptag);

	if (lba_count > sectors_per_max_io)
		return _nvme_ns_split_request(ns, qpair, payload, lba,
					      lba_count, cb_fn, cb_arg, opc,
					      io_flags, req, sectors_per_max_io,
					      0, apptag_mask, apptag);

	cmd = &req->cmd;
	cmd->opc = opc;
	cmd->nsid = ns->id;

	tmp_lba = (uint64_t *)&cmd->cdw10;
	*tmp_lba = lba;

	if (ns->flags & NVME_NS_DPS_PI_SUPPORTED) {
		switch (ns->pi_type) {
		case NVME_FMT_NVM_PROTECTION_TYPE1:
		case NVME_FMT_NVM_PROTECTION_TYPE2:
			cmd->cdw14 = (uint32_t)lba;
			break;
		}
	}

	cmd->cdw12 = lba_count - 1;
	cmd->cdw12 |= io_flags;

	cmd->cdw15 = apptag_mask;
	cmd->cdw15 = (cmd->cdw15 << 16 | apptag);

	return req;
}

int nvme_ns_read(struct nvme_ns *ns, struct nvme_qpair *qpair,
		 void *buffer,
		 uint64_t lba, uint32_t lba_count,
		 nvme_cmd_cb cb_fn, void *cb_arg,
		 unsigned int io_flags)
{
	struct nvme_request *req;
	struct nvme_payload payload;

	payload.type = NVME_PAYLOAD_TYPE_CONTIG;
	payload.u.contig = buffer;
	payload.md = NULL;

	req = _nvme_ns_rw(ns, qpair, &payload, lba, lba_count, cb_fn, cb_arg,
			  NVME_OPC_READ, io_flags, 0, 0);
	if (req != NULL)
		return nvme_qpair_submit_request(qpair, req);

	return ENOMEM;
}

int nvme_ns_read_with_md(struct nvme_ns *ns, struct nvme_qpair *qpair,
			 void *buffer, void *metadata,
			 uint64_t lba, uint32_t lba_count,
			 nvme_cmd_cb cb_fn, void *cb_arg,
			 unsigned int io_flags,
			 uint16_t apptag_mask, uint16_t apptag)
{
	struct nvme_request *req;
	struct nvme_payload payload;

	payload.type = NVME_PAYLOAD_TYPE_CONTIG;
	payload.u.contig = buffer;
	payload.md = metadata;

	req = _nvme_ns_rw(ns, qpair, &payload, lba, lba_count, cb_fn, cb_arg,
			  NVME_OPC_READ, io_flags, apptag_mask, apptag);
	if (req != NULL)
		return nvme_qpair_submit_request(qpair, req);

	return ENOMEM;
}

int nvme_ns_readv(struct nvme_ns *ns, struct nvme_qpair *qpair,
		  uint64_t lba, uint32_t lba_count,
		  nvme_cmd_cb cb_fn, void *cb_arg,
		  unsigned int io_flags,
		  nvme_req_reset_sgl_cb reset_sgl_fn,
		  nvme_req_next_sge_cb next_sge_fn)
{
	struct nvme_request *req;
	struct nvme_payload payload;

	if (reset_sgl_fn == NULL || next_sge_fn == NULL)
		return EINVAL;

	payload.type = NVME_PAYLOAD_TYPE_SGL;
	payload.md = NULL;
	payload.u.sgl.reset_sgl_fn = reset_sgl_fn;
	payload.u.sgl.next_sge_fn = next_sge_fn;
	payload.u.sgl.cb_arg = cb_arg;

	req = _nvme_ns_rw(ns, qpair, &payload, lba, lba_count, cb_fn, cb_arg,
			  NVME_OPC_READ, io_flags, 0, 0);
	if (req != NULL)
		return nvme_qpair_submit_request(qpair, req);

	return ENOMEM;
}

int nvme_ns_write(struct nvme_ns *ns, struct nvme_qpair *qpair,
		  void *buffer,
		  uint64_t lba, uint32_t lba_count,
		  nvme_cmd_cb cb_fn, void *cb_arg,
		  unsigned int io_flags)
{
	struct nvme_request *req;
	struct nvme_payload payload;

	payload.type = NVME_PAYLOAD_TYPE_CONTIG;
	payload.u.contig = buffer;
	payload.md = NULL;

	req = _nvme_ns_rw(ns, qpair, &payload, lba, lba_count, cb_fn, cb_arg,
			  NVME_OPC_WRITE, io_flags, 0, 0);
	if (req != NULL)
		return nvme_qpair_submit_request(qpair, req);

	return ENOMEM;
}

int nvme_ns_write_with_md(struct nvme_ns *ns, struct nvme_qpair *qpair,
			  void *buffer, void *metadata,
			  uint64_t lba, uint32_t lba_count,
			  nvme_cmd_cb cb_fn, void *cb_arg,
			  unsigned int io_flags,
			  uint16_t apptag_mask, uint16_t apptag)
{
	struct nvme_request *req;
	struct nvme_payload payload;

	payload.type = NVME_PAYLOAD_TYPE_CONTIG;
	payload.u.contig = buffer;
	payload.md = metadata;

	req = _nvme_ns_rw(ns, qpair, &payload, lba, lba_count, cb_fn, cb_arg,
			  NVME_OPC_WRITE, io_flags, apptag_mask, apptag);
	if (req != NULL)
		return nvme_qpair_submit_request(qpair, req);

	return ENOMEM;
}

int nvme_ns_writev(struct nvme_ns *ns, struct nvme_qpair *qpair,
		   uint64_t lba, uint32_t lba_count,
		   nvme_cmd_cb cb_fn, void *cb_arg,
		   unsigned int io_flags,
		   nvme_req_reset_sgl_cb reset_sgl_fn,
		   nvme_req_next_sge_cb next_sge_fn)
{
	struct nvme_request *req;
	struct nvme_payload payload;

	if (reset_sgl_fn == NULL || next_sge_fn == NULL)
		return EINVAL;

	payload.type = NVME_PAYLOAD_TYPE_SGL;
	payload.md = NULL;
	payload.u.sgl.reset_sgl_fn = reset_sgl_fn;
	payload.u.sgl.next_sge_fn = next_sge_fn;
	payload.u.sgl.cb_arg = cb_arg;

	req = _nvme_ns_rw(ns, qpair, &payload, lba, lba_count, cb_fn, cb_arg,
			  NVME_OPC_WRITE, io_flags, 0, 0);
	if (req != NULL)
		return nvme_qpair_submit_request(qpair, req);

	return ENOMEM;
}

int nvme_ns_write_zeroes(struct nvme_ns *ns, struct nvme_qpair *qpair,
			 uint64_t lba, uint32_t lba_count,
			 nvme_cmd_cb cb_fn, void *cb_arg,
			 unsigned int io_flags)
{
	struct nvme_request *req;
	struct nvme_cmd	*cmd;
	uint64_t *tmp_lba;

	if (lba_count == 0)
		return EINVAL;

	req = nvme_request_allocate_null(qpair, cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_WRITE_ZEROES;
	cmd->nsid = ns->id;

	tmp_lba = (uint64_t *)&cmd->cdw10;
	*tmp_lba = lba;
	cmd->cdw12 = lba_count - 1;
	cmd->cdw12 |= io_flags;

	return nvme_qpair_submit_request(qpair, req);
}

int nvme_ns_deallocate(struct nvme_ns *ns, struct nvme_qpair *qpair,
		       void *payload, uint16_t ranges,
		       nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct nvme_cmd	*cmd;

	if (ranges == 0 || ranges > NVME_DATASET_MANAGEMENT_MAX_RANGES)
		return EINVAL;

	req = nvme_request_allocate_contig(qpair, payload,
				   ranges * sizeof(struct nvme_dsm_range),
				   cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_DATASET_MANAGEMENT;
	cmd->nsid = ns->id;

	/* TODO: create a delete command data structure */
	cmd->cdw10 = ranges - 1;
	cmd->cdw11 = NVME_DSM_ATTR_DEALLOCATE;

	return nvme_qpair_submit_request(qpair, req);
}

int nvme_ns_flush(struct nvme_ns *ns, struct nvme_qpair *qpair,
		  nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct nvme_cmd	*cmd;

	req = nvme_request_allocate_null(qpair, cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_FLUSH;
	cmd->nsid = ns->id;

	return nvme_qpair_submit_request(qpair, req);
}

int nvme_ns_reservation_register(struct nvme_ns *ns, struct nvme_qpair *qpair,
				 struct nvme_reservation_register_data *payload,
				 bool ignore_key,
				 enum nvme_reservation_register_action action,
				 enum nvme_reservation_register_cptpl cptpl,
				 nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct nvme_cmd	*cmd;

	req = nvme_request_allocate_contig(qpair, payload,
					   sizeof(struct nvme_reservation_register_data),
					   cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_RESERVATION_REGISTER;
	cmd->nsid = ns->id;

	/* Bits 0-2 */
	cmd->cdw10 = action;
	/* Bit 3 */
	cmd->cdw10 |= ignore_key ? 1 << 3 : 0;
	/* Bits 30-31 */
	cmd->cdw10 |= (uint32_t)cptpl << 30;

	return nvme_qpair_submit_request(qpair, req);
}

int nvme_ns_reservation_release(struct nvme_ns *ns, struct nvme_qpair *qpair,
				struct nvme_reservation_key_data *payload,
				bool ignore_key,
				enum nvme_reservation_release_action action,
				enum nvme_reservation_type type,
				nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;
	struct nvme_cmd	*cmd;

	req = nvme_request_allocate_contig(qpair, payload,
					   sizeof(struct nvme_reservation_key_data),
					   cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_RESERVATION_RELEASE;
	cmd->nsid = ns->id;

	/* Bits 0-2 */
	cmd->cdw10 = action;
	/* Bit 3 */
	cmd->cdw10 |= ignore_key ? 1 << 3 : 0;
	/* Bits 8-15 */
	cmd->cdw10 |= (uint32_t)type << 8;

	return nvme_qpair_submit_request(qpair, req);
}

int nvme_ns_reservation_acquire(struct nvme_ns *ns, struct nvme_qpair *qpair,
				struct nvme_reservation_acquire_data *payload,
				bool ignore_key,
				enum nvme_reservation_acquire_action action,
				enum nvme_reservation_type type,
				nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request	*req;
	struct nvme_cmd	*cmd;

	req = nvme_request_allocate_contig(qpair, payload,
					   sizeof(struct nvme_reservation_acquire_data),
					   cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_RESERVATION_ACQUIRE;
	cmd->nsid = ns->id;

	/* Bits 0-2 */
	cmd->cdw10 = action;
	/* Bit 3 */
	cmd->cdw10 |= ignore_key ? 1 << 3 : 0;
	/* Bits 8-15 */
	cmd->cdw10 |= (uint32_t)type << 8;

	return nvme_qpair_submit_request(qpair, req);
}

int nvme_ns_reservation_report(struct nvme_ns *ns, struct nvme_qpair *qpair,
			       void *payload, size_t len,
			       nvme_cmd_cb cb_fn, void *cb_arg)
{
	uint32_t num_dwords;
	struct nvme_request *req;
	struct nvme_cmd	*cmd;

	if (len % 4)
		return EINVAL;
	num_dwords = len / 4;

	req = nvme_request_allocate_contig(qpair, payload, len, cb_fn, cb_arg);
	if (req == NULL)
		return ENOMEM;

	cmd = &req->cmd;
	cmd->opc = NVME_OPC_RESERVATION_REPORT;
	cmd->nsid = ns->id;

	cmd->cdw10 = num_dwords;

	return nvme_qpair_submit_request(qpair, req);
}
