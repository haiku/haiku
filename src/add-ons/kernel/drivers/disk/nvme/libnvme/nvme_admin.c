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
 * Allocate a request, set its command and submit it
 * to the controller admin queue.
 */
static int nvme_admin_submit_cmd(struct nvme_ctrlr *ctrlr,
				 struct nvme_cmd *cmd,
				 void *buf, uint32_t len,
				 nvme_cmd_cb cb_fn, void *cb_arg)
{
	struct nvme_request *req;

	if (buf)
		req = nvme_request_allocate_contig(&ctrlr->adminq, buf, len,
						   cb_fn, cb_arg);
	else
		req = nvme_request_allocate_null(&ctrlr->adminq, cb_fn, cb_arg);
	if (!req)
		return ENOMEM;

	memcpy(&req->cmd, cmd, sizeof(req->cmd));

	return nvme_qpair_submit_request(&ctrlr->adminq, req);
}

/*
 * Poll the controller admin queue waiting for a
 * command completion.
 */
static int nvme_admin_wait_cmd(struct nvme_ctrlr *ctrlr,
			       struct nvme_completion_poll_status *status)
{

	/* Wait for completion and check result */
	while (status->done == false)
		nvme_qpair_poll(&ctrlr->adminq, 0);

	if (nvme_cpl_is_error(&status->cpl)) {
		nvme_notice("Admin command failed\n");
		return ENXIO;
	}

	return 0;
}

/*
 * Execute an admin command.
 */
static int nvme_admin_exec_cmd(struct nvme_ctrlr *ctrlr,
			       struct nvme_cmd *cmd,
			       void *buf, uint32_t len)
{
	struct nvme_completion_poll_status status;
	int ret;

	/* Submit the command */
	status.done = false;
	ret = nvme_admin_submit_cmd(ctrlr, cmd, buf, len,
				    nvme_request_completion_poll_cb,
				    &status);
	if (ret != 0)
		return ret;

	/* Wait for the command completion and check result */
	return nvme_admin_wait_cmd(ctrlr, &status);
}

/*
 * Get a controller information.
 */
int nvme_admin_identify_ctrlr(struct nvme_ctrlr *ctrlr,
			      struct nvme_ctrlr_data *cdata)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_IDENTIFY;
	cmd.cdw10 = NVME_IDENTIFY_CTRLR;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd,
				   cdata, sizeof(struct nvme_ctrlr_data));
}

/*
 * Get a controller feature.
 */
int nvme_admin_get_feature(struct nvme_ctrlr *ctrlr,
			   enum nvme_feat_sel sel,
			   enum nvme_feat feature,
			   uint32_t cdw11,
			   uint32_t *attributes)
{
	struct nvme_completion_poll_status status;
	struct nvme_cmd cmd;
	int ret;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_GET_FEATURES;
	cmd.cdw10 = (sel << 8) | feature;
	cmd.cdw11 = cdw11;

	/* Submit the command */
	status.done = false;
	ret = nvme_admin_submit_cmd(ctrlr, &cmd, NULL, 0,
				    nvme_request_completion_poll_cb,
				    &status);
	if (ret == 0) {
		/* Wait for the command completion and check result */
		ret = nvme_admin_wait_cmd(ctrlr, &status);
		if (ret == 0 && attributes)
			*attributes = status.cpl.cdw0;
	}

	return ret;
}

/*
 * Set a feature.
 */
int nvme_admin_set_feature(struct nvme_ctrlr *ctrlr,
			   bool save,
			   enum nvme_feat feature,
			   uint32_t cdw11,
			   uint32_t cdw12,
			   uint32_t *attributes)
{
	struct nvme_completion_poll_status status;
	struct nvme_cmd cmd;
	int ret;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_SET_FEATURES;
	cmd.cdw10 = feature;
	if (save)
		cmd.cdw10 |= (1 << 31);
	cmd.cdw11 = cdw11;
	cmd.cdw12 = cdw12;

	/* Submit the command */
	status.done = false;
	ret = nvme_admin_submit_cmd(ctrlr, &cmd, NULL, 0,
				    nvme_request_completion_poll_cb,
				    &status);
	if (ret == 0) {
		/* Wait for the command completion and check result */
		ret = nvme_admin_wait_cmd(ctrlr, &status);
		if (ret == 0 && attributes)
			*attributes = status.cpl.cdw0;
	}

	return ret;
}

/*
 * Create an I/O queue.
 */
int nvme_admin_create_ioq(struct nvme_ctrlr *ctrlr,
			  struct nvme_qpair *qpair,
			  enum nvme_io_queue_type io_qtype)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	switch(io_qtype) {
	case NVME_IO_SUBMISSION_QUEUE:
		cmd.opc = NVME_OPC_CREATE_IO_SQ;
		cmd.cdw11 = (qpair->id << 16) | (qpair->qprio << 1) | 0x1;
		cmd.dptr.prp.prp1 = qpair->cmd_bus_addr;
		break;
	case NVME_IO_COMPLETION_QUEUE:
		cmd.opc = NVME_OPC_CREATE_IO_CQ;
#ifdef __HAIKU__ // TODO: Option!
		cmd.cdw11 = 0x1 | 0x2; /* enable interrupts */
#else
		cmd.cdw11 = 0x1;
#endif
		cmd.dptr.prp.prp1 = qpair->cpl_bus_addr;
		break;
	default:
		return EINVAL;
	}

	cmd.cdw10 = ((qpair->entries - 1) << 16) | qpair->id;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, NULL, 0);
}

/*
 * Delete an I/O queue.
 */
int nvme_admin_delete_ioq(struct nvme_ctrlr *ctrlr,
			  struct nvme_qpair *qpair,
			  enum nvme_io_queue_type io_qtype)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	switch(io_qtype) {
	case NVME_IO_SUBMISSION_QUEUE:
		cmd.opc = NVME_OPC_DELETE_IO_SQ;
		break;
	case NVME_IO_COMPLETION_QUEUE:
		cmd.opc = NVME_OPC_DELETE_IO_CQ;
		break;
	default:
		return EINVAL;
	}
	cmd.cdw10 = qpair->id;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, NULL, 0);
}

/*
 * Get a namespace information.
 */
int nvme_admin_identify_ns(struct nvme_ctrlr *ctrlr,
			   uint16_t nsid,
			   struct nvme_ns_data *nsdata)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_IDENTIFY;
	cmd.cdw10 = NVME_IDENTIFY_NS;
	cmd.nsid = nsid;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd,
				   nsdata, sizeof(struct nvme_ns_data));
}

/*
 * Attach a namespace.
 */
int nvme_admin_attach_ns(struct nvme_ctrlr *ctrlr,
			 uint32_t nsid,
			 struct nvme_ctrlr_list *clist)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_NS_ATTACHMENT;
	cmd.nsid = nsid;
	cmd.cdw10 = NVME_NS_CTRLR_ATTACH;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd,
				   clist, sizeof(struct nvme_ctrlr_list));
}

/*
 * Detach a namespace.
 */
int nvme_admin_detach_ns(struct nvme_ctrlr *ctrlr,
			 uint32_t nsid,
			 struct nvme_ctrlr_list *clist)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_NS_ATTACHMENT;
	cmd.nsid = nsid;
	cmd.cdw10 = NVME_NS_CTRLR_DETACH;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd,
				   clist, sizeof(struct nvme_ctrlr_list));
}

/*
 * Create a namespace.
 */
int nvme_admin_create_ns(struct nvme_ctrlr *ctrlr,
			 struct nvme_ns_data *nsdata,
			 unsigned int *nsid)
{
	struct nvme_completion_poll_status status;
	struct nvme_cmd cmd;
	int ret;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_NS_MANAGEMENT;
	cmd.cdw10 = NVME_NS_MANAGEMENT_CREATE;

	/* Submit the command */
	status.done = false;
	ret = nvme_admin_submit_cmd(ctrlr, &cmd,
				    nsdata, sizeof(struct nvme_ns_data),
				    nvme_request_completion_poll_cb,
				    &status);
	if (ret == 0)
		/* Wait for the command completion and check result */
		ret = nvme_admin_wait_cmd(ctrlr, &status);

	if (ret != 0)
		return ret;

	*nsid = status.cpl.cdw0;

	return 0;
}

/*
 * Delete a namespace.
 */
int nvme_admin_delete_ns(struct nvme_ctrlr *ctrlr,
			 unsigned int nsid)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_NS_MANAGEMENT;
	cmd.cdw10 = NVME_NS_MANAGEMENT_DELETE;
	cmd.nsid = nsid;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, NULL, 0);
}

/*
 * Format media.
 * (entire device or just the specified namespace)
 */
int nvme_admin_format_nvm(struct nvme_ctrlr *ctrlr,
			  unsigned int nsid,
			  struct nvme_format *format)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_FORMAT_NVM;
	cmd.nsid = nsid;
	memcpy(&cmd.cdw10, format, sizeof(uint32_t));

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, NULL, 0);
}

/*
 * Get a log page.
 */
int nvme_admin_get_log_page(struct nvme_ctrlr *ctrlr,
			    uint8_t log_page,
			    uint32_t nsid,
			    void *payload,
			    uint32_t payload_size)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_GET_LOG_PAGE;
	cmd.nsid = nsid;
	cmd.cdw10 = ((payload_size / sizeof(uint32_t)) - 1) << 16;
	cmd.cdw10 |= log_page;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, payload, payload_size);
}

/*
 * Abort an admin or an I/O command.
 */
int nvme_admin_abort_cmd(struct nvme_ctrlr *ctrlr,
			 uint16_t cid, uint16_t sqid)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_ABORT;
	cmd.cdw10 = (cid << 16) | sqid;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, NULL, 0);
}

/*
 * Validate a FW.
 */
int nvme_admin_fw_commit(struct nvme_ctrlr *ctrlr,
			 const struct nvme_fw_commit *fw_commit)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_FIRMWARE_COMMIT;
	memcpy(&cmd.cdw10, fw_commit, sizeof(uint32_t));

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, NULL, 0);
}

/*
 * Download to the device a firmware.
 */
int nvme_admin_fw_image_dl(struct nvme_ctrlr *ctrlr,
			   void *fw, uint32_t size,
			   uint32_t offset)
{
	struct nvme_cmd cmd;

	/* Setup the command */
	memset(&cmd, 0, sizeof(struct nvme_cmd));
	cmd.opc = NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD;
	cmd.cdw10 = (size >> 2) - 1;
	cmd.cdw11 = offset >> 2;

	/* Execute the command */
	return nvme_admin_exec_cmd(ctrlr, &cmd, fw, size);
}
