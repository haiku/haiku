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

struct nvme_qpair_string {
	uint16_t	value;
	const char 	*str;
};

static const struct nvme_qpair_string admin_opcode[] = {
	{ NVME_OPC_DELETE_IO_SQ,	"DELETE IO SQ" },
	{ NVME_OPC_CREATE_IO_SQ,	"CREATE IO SQ" },
	{ NVME_OPC_GET_LOG_PAGE,	"GET LOG PAGE" },
	{ NVME_OPC_DELETE_IO_CQ,	"DELETE IO CQ" },
	{ NVME_OPC_CREATE_IO_CQ,	"CREATE IO CQ" },
	{ NVME_OPC_IDENTIFY, 		"IDENTIFY" },
	{ NVME_OPC_ABORT,		"ABORT" },
	{ NVME_OPC_SET_FEATURES,	"SET FEATURES" },
	{ NVME_OPC_GET_FEATURES,	"GET FEATURES" },
	{ NVME_OPC_ASYNC_EVENT_REQUEST, "ASYNC EVENT REQUEST" },
	{ NVME_OPC_NS_MANAGEMENT,	"NAMESPACE MANAGEMENT" },
	{ NVME_OPC_FIRMWARE_COMMIT,	"FIRMWARE COMMIT" },
	{ NVME_OPC_FIRMWARE_IMAGE_DOWNLOAD, "FIRMWARE IMAGE DOWNLOAD" },
	{ NVME_OPC_NS_ATTACHMENT,	"NAMESPACE ATTACHMENT" },
	{ NVME_OPC_FORMAT_NVM,		"FORMAT NVM" },
	{ NVME_OPC_SECURITY_SEND,	"SECURITY SEND" },
	{ NVME_OPC_SECURITY_RECEIVE,	"SECURITY RECEIVE" },
	{ 0xFFFF,			"ADMIN COMMAND" }
};

static const struct nvme_qpair_string io_opcode[] = {
	{ NVME_OPC_FLUSH,		"FLUSH" },
	{ NVME_OPC_WRITE,		"WRITE" },
	{ NVME_OPC_READ,		"READ" },
	{ NVME_OPC_WRITE_UNCORRECTABLE, "WRITE UNCORRECTABLE" },
	{ NVME_OPC_COMPARE,		"COMPARE" },
	{ NVME_OPC_WRITE_ZEROES,	"WRITE ZEROES" },
	{ NVME_OPC_DATASET_MANAGEMENT,	"DATASET MANAGEMENT" },
	{ NVME_OPC_RESERVATION_REGISTER, "RESERVATION REGISTER" },
	{ NVME_OPC_RESERVATION_REPORT,	"RESERVATION REPORT" },
	{ NVME_OPC_RESERVATION_ACQUIRE, "RESERVATION ACQUIRE" },
	{ NVME_OPC_RESERVATION_RELEASE, "RESERVATION RELEASE" },
	{ 0xFFFF,			"IO COMMAND" }
};

static const struct nvme_qpair_string generic_status[] = {
	{ NVME_SC_SUCCESS,			"SUCCESS" },
	{ NVME_SC_INVALID_OPCODE,		"INVALID OPCODE" },
	{ NVME_SC_INVALID_FIELD,		"INVALID FIELD" },
	{ NVME_SC_COMMAND_ID_CONFLICT,		"COMMAND ID CONFLICT" },
	{ NVME_SC_DATA_TRANSFER_ERROR,		"DATA TRANSFER ERROR" },
	{ NVME_SC_ABORTED_POWER_LOSS,		"ABORTED - POWER LOSS" },
	{ NVME_SC_INTERNAL_DEVICE_ERROR,	"INTERNAL DEVICE ERROR" },
	{ NVME_SC_ABORTED_BY_REQUEST,		"ABORTED - BY REQUEST" },
	{ NVME_SC_ABORTED_SQ_DELETION,		"ABORTED - SQ DELETION" },
	{ NVME_SC_ABORTED_FAILED_FUSED,		"ABORTED - FAILED FUSED" },
	{ NVME_SC_ABORTED_MISSING_FUSED,	"ABORTED - MISSING FUSED" },
	{ NVME_SC_INVALID_NAMESPACE_OR_FORMAT,	"INVALID NAMESPACE OR FORMAT" },
	{ NVME_SC_COMMAND_SEQUENCE_ERROR,	"COMMAND SEQUENCE ERROR" },
	{ NVME_SC_INVALID_SGL_SEG_DESCRIPTOR,	"INVALID SGL SEGMENT DESCRIPTOR" },
	{ NVME_SC_INVALID_NUM_SGL_DESCIRPTORS,	"INVALID NUMBER OF SGL DESCRIPTORS" },
	{ NVME_SC_DATA_SGL_LENGTH_INVALID,	"DATA SGL LENGTH INVALID" },
	{ NVME_SC_METADATA_SGL_LENGTH_INVALID,	"METADATA SGL LENGTH INVALID" },
	{ NVME_SC_SGL_DESCRIPTOR_TYPE_INVALID,	"SGL DESCRIPTOR TYPE INVALID" },
	{ NVME_SC_INVALID_CONTROLLER_MEM_BUF,	"INVALID CONTROLLER MEMORY BUFFER" },
	{ NVME_SC_INVALID_PRP_OFFSET,		"INVALID PRP OFFSET" },
	{ NVME_SC_ATOMIC_WRITE_UNIT_EXCEEDED,	"ATOMIC WRITE UNIT EXCEEDED" },
	{ NVME_SC_LBA_OUT_OF_RANGE,		"LBA OUT OF RANGE" },
	{ NVME_SC_CAPACITY_EXCEEDED,		"CAPACITY EXCEEDED" },
	{ NVME_SC_NAMESPACE_NOT_READY,		"NAMESPACE NOT READY" },
	{ NVME_SC_RESERVATION_CONFLICT,		"RESERVATION CONFLICT" },
	{ NVME_SC_FORMAT_IN_PROGRESS,		"FORMAT IN PROGRESS" },
	{ 0xFFFF,				"GENERIC" }
};

static const struct nvme_qpair_string command_specific_status[] = {
	{ NVME_SC_COMPLETION_QUEUE_INVALID,	"INVALID COMPLETION QUEUE" },
	{ NVME_SC_INVALID_QUEUE_IDENTIFIER,	"INVALID QUEUE IDENTIFIER" },
	{ NVME_SC_MAXIMUM_QUEUE_SIZE_EXCEEDED,	"MAX QUEUE SIZE EXCEEDED" },
	{ NVME_SC_ABORT_COMMAND_LIMIT_EXCEEDED,	"ABORT CMD LIMIT EXCEEDED" },
	{ NVME_SC_ASYNC_EVENT_REQUEST_LIMIT_EXCEEDED,"ASYNC LIMIT EXCEEDED" },
	{ NVME_SC_INVALID_FIRMWARE_SLOT,	"INVALID FIRMWARE SLOT" },
	{ NVME_SC_INVALID_FIRMWARE_IMAGE,	"INVALID FIRMWARE IMAGE" },
	{ NVME_SC_INVALID_INTERRUPT_VECTOR,	"INVALID INTERRUPT VECTOR" },
	{ NVME_SC_INVALID_LOG_PAGE,		"INVALID LOG PAGE" },
	{ NVME_SC_INVALID_FORMAT,		"INVALID FORMAT" },
	{ NVME_SC_FIRMWARE_REQ_CONVENTIONAL_RESET,"FIRMWARE REQUIRES CONVENTIONAL RESET" },
	{ NVME_SC_INVALID_QUEUE_DELETION,	"INVALID QUEUE DELETION" },
	{ NVME_SC_FEATURE_ID_NOT_SAVEABLE,	"FEATURE ID NOT SAVEABLE" },
	{ NVME_SC_FEATURE_NOT_CHANGEABLE,	"FEATURE NOT CHANGEABLE" },
	{ NVME_SC_FEATURE_NOT_NAMESPACE_SPECIFIC,"FEATURE NOT NAMESPACE SPECIFIC" },
	{ NVME_SC_FIRMWARE_REQ_NVM_RESET,	"FIRMWARE REQUIRES NVM RESET" },
	{ NVME_SC_FIRMWARE_REQ_RESET,		"FIRMWARE REQUIRES RESET" },
	{ NVME_SC_FIRMWARE_REQ_MAX_TIME_VIOLATION,"FIRMWARE REQUIRES MAX TIME VIOLATION" },
	{ NVME_SC_FIRMWARE_ACTIVATION_PROHIBITED,"FIRMWARE ACTIVATION PROHIBITED" },
	{ NVME_SC_OVERLAPPING_RANGE,		"OVERLAPPING RANGE" },
	{ NVME_SC_NAMESPACE_INSUFFICIENT_CAPACITY,"NAMESPACE INSUFFICIENT CAPACITY" },
	{ NVME_SC_NAMESPACE_ID_UNAVAILABLE,	"NAMESPACE ID UNAVAILABLE" },
	{ NVME_SC_NAMESPACE_ALREADY_ATTACHED,	"NAMESPACE ALREADY ATTACHED" },
	{ NVME_SC_NAMESPACE_IS_PRIVATE,		"NAMESPACE IS PRIVATE" },
	{ NVME_SC_NAMESPACE_NOT_ATTACHED,	"NAMESPACE NOT ATTACHED" },
	{ NVME_SC_THINPROVISIONING_NOT_SUPPORTED,"THINPROVISIONING NOT SUPPORTED" },
	{ NVME_SC_CONTROLLER_LIST_INVALID,	"CONTROLLER LIST INVALID" },
	{ NVME_SC_CONFLICTING_ATTRIBUTES,	"CONFLICTING ATTRIBUTES" },
	{ NVME_SC_INVALID_PROTECTION_INFO,	"INVALID PROTECTION INFO" },
	{ NVME_SC_ATTEMPTED_WRITE_TO_RO_PAGE,	"WRITE TO RO PAGE" },
	{ 0xFFFF,				"COMMAND SPECIFIC" }
};

static const struct nvme_qpair_string media_error_status[] = {
	{ NVME_SC_WRITE_FAULTS, 		"WRITE FAULTS" },
	{ NVME_SC_UNRECOVERED_READ_ERROR, 	"UNRECOVERED READ ERROR" },
	{ NVME_SC_GUARD_CHECK_ERROR, 		"GUARD CHECK ERROR" },
	{ NVME_SC_APPLICATION_TAG_CHECK_ERROR, 	"APPLICATION TAG CHECK ERROR" },
	{ NVME_SC_REFERENCE_TAG_CHECK_ERROR, 	"REFERENCE TAG CHECK ERROR" },
	{ NVME_SC_COMPARE_FAILURE, 		"COMPARE FAILURE" },
	{ NVME_SC_ACCESS_DENIED, 		"ACCESS DENIED" },
	{ NVME_SC_DEALLOCATED_OR_UNWRITTEN_BLOCK, "DEALLOCATED OR UNWRITTEN BLOCK" },
	{ 0xFFFF, 				"MEDIA ERROR" }
};

static inline bool nvme_qpair_is_admin_queue(struct nvme_qpair *qpair)
{
	return qpair->id == 0;
}

static inline bool nvme_qpair_is_io_queue(struct nvme_qpair *qpair)
{
	return qpair->id != 0;
}

static const char*nvme_qpair_get_string(const struct nvme_qpair_string *strings,
					uint16_t value)
{
	const struct nvme_qpair_string *entry;

	entry = strings;

	while (entry->value != 0xFFFF) {
		if (entry->value == value)
			return entry->str;
		entry++;
	}
	return entry->str;
}

static void nvme_qpair_admin_qpair_print_command(struct nvme_qpair *qpair,
						 struct nvme_cmd *cmd)
{
	nvme_info("%s (%02x) sqid:%d cid:%d nsid:%x cdw10:%08x cdw11:%08x\n",
		  nvme_qpair_get_string(admin_opcode, cmd->opc), cmd->opc,
		  qpair->id, cmd->cid,
		  cmd->nsid, cmd->cdw10, cmd->cdw11);
}

static void nvme_qpair_io_qpair_print_command(struct nvme_qpair *qpair,
					      struct nvme_cmd *cmd)
{
	nvme_assert(qpair != NULL, "print_command: qpair == NULL\n");
	nvme_assert(cmd != NULL, "print_command: cmd == NULL\n");

	switch ((int)cmd->opc) {
	case NVME_OPC_WRITE:
	case NVME_OPC_READ:
	case NVME_OPC_WRITE_UNCORRECTABLE:
	case NVME_OPC_COMPARE:
		nvme_info("%s sqid:%d cid:%d nsid:%d lba:%llu len:%d\n",
			  nvme_qpair_get_string(io_opcode, cmd->opc),
			  qpair->id, cmd->cid, cmd->nsid,
			  ((unsigned long long)cmd->cdw11 << 32) + cmd->cdw10,
			  (cmd->cdw12 & 0xFFFF) + 1);
		break;
	case NVME_OPC_FLUSH:
	case NVME_OPC_DATASET_MANAGEMENT:
		nvme_info("%s sqid:%d cid:%d nsid:%d\n",
			  nvme_qpair_get_string(io_opcode, cmd->opc),
			  qpair->id, cmd->cid, cmd->nsid);
		break;
	default:
		nvme_info("%s (%02x) sqid:%d cid:%d nsid:%d\n",
			  nvme_qpair_get_string(io_opcode, cmd->opc),
			  cmd->opc, qpair->id, cmd->cid, cmd->nsid);
		break;
	}
}

static void nvme_qpair_print_command(struct nvme_qpair *qpair,
				     struct nvme_cmd *cmd)
{
	nvme_assert(qpair != NULL, "qpair can not be NULL");
	nvme_assert(cmd != NULL, "cmd can not be NULL");

	if (nvme_qpair_is_admin_queue(qpair))
		return nvme_qpair_admin_qpair_print_command(qpair, cmd);

	return nvme_qpair_io_qpair_print_command(qpair, cmd);
}

static const char *get_status_string(uint16_t sct, uint16_t sc)
{
	const struct nvme_qpair_string *entry;

	switch (sct) {
	case NVME_SCT_GENERIC:
		entry = generic_status;
		break;
	case NVME_SCT_COMMAND_SPECIFIC:
		entry = command_specific_status;
		break;
	case NVME_SCT_MEDIA_ERROR:
		entry = media_error_status;
		break;
	case NVME_SCT_VENDOR_SPECIFIC:
		return "VENDOR SPECIFIC";
	default:
		return "RESERVED";
	}

	return nvme_qpair_get_string(entry, sc);
}

static void nvme_qpair_print_completion(struct nvme_qpair *qpair,
					struct nvme_cpl *cpl)
{
	nvme_info("Cpl: %s (%02x/%02x) sqid:%d cid:%d "
		  "cdw0:%x sqhd:%04x p:%x m:%x dnr:%x\n",
		  get_status_string(cpl->status.sct, cpl->status.sc),
		  cpl->status.sct,
		  cpl->status.sc,
		  cpl->sqid,
		  cpl->cid,
		  cpl->cdw0,
		  cpl->sqhd,
		  cpl->status.p,
		  cpl->status.m,
		  cpl->status.dnr);
}

static bool nvme_qpair_completion_retry(const struct nvme_cpl *cpl)
{
	/*
	 * TODO: spec is not clear how commands that are aborted due
	 *  to TLER will be marked.  So for now, it seems
	 *  NAMESPACE_NOT_READY is the only case where we should
	 *  look at the DNR bit.
	 */
	switch ((int)cpl->status.sct) {
	case NVME_SCT_GENERIC:
		switch ((int)cpl->status.sc) {
		case NVME_SC_NAMESPACE_NOT_READY:
		case NVME_SC_FORMAT_IN_PROGRESS:
			if (cpl->status.dnr)
				return false;
			return true;
		case NVME_SC_INVALID_OPCODE:
		case NVME_SC_INVALID_FIELD:
		case NVME_SC_COMMAND_ID_CONFLICT:
		case NVME_SC_DATA_TRANSFER_ERROR:
		case NVME_SC_ABORTED_POWER_LOSS:
		case NVME_SC_INTERNAL_DEVICE_ERROR:
		case NVME_SC_ABORTED_BY_REQUEST:
		case NVME_SC_ABORTED_SQ_DELETION:
		case NVME_SC_ABORTED_FAILED_FUSED:
		case NVME_SC_ABORTED_MISSING_FUSED:
		case NVME_SC_INVALID_NAMESPACE_OR_FORMAT:
		case NVME_SC_COMMAND_SEQUENCE_ERROR:
		case NVME_SC_LBA_OUT_OF_RANGE:
		case NVME_SC_CAPACITY_EXCEEDED:
		default:
			return false;
		}
	case NVME_SCT_COMMAND_SPECIFIC:
	case NVME_SCT_MEDIA_ERROR:
	case NVME_SCT_VENDOR_SPECIFIC:
	default:
		return false;
	}
}

static void nvme_qpair_construct_tracker(struct nvme_tracker *tr,
					 uint16_t cid, uint64_t phys_addr)
{
	tr->prp_sgl_bus_addr = phys_addr + offsetof(struct nvme_tracker, u.prp);
	tr->cid = cid;
	tr->active = false;
}

static inline void nvme_qpair_copy_command(struct nvme_cmd *dst,
					   const struct nvme_cmd *src)
{
	/* dst and src are known to be non-overlapping and 64-byte aligned. */
#if defined(__AVX__)
	__m256i *d256 = (__m256i *)dst;
	const __m256i *s256 = (const __m256i *)src;

	_mm256_store_si256(&d256[0], _mm256_load_si256(&s256[0]));
	_mm256_store_si256(&d256[1], _mm256_load_si256(&s256[1]));
#elif defined(__SSE2__)
	__m128i *d128 = (__m128i *)dst;
	const __m128i *s128 = (const __m128i *)src;

	_mm_store_si128(&d128[0], _mm_load_si128(&s128[0]));
	_mm_store_si128(&d128[1], _mm_load_si128(&s128[1]));
	_mm_store_si128(&d128[2], _mm_load_si128(&s128[2]));
	_mm_store_si128(&d128[3], _mm_load_si128(&s128[3]));
#else
	*dst = *src;
#endif
}

static void nvme_qpair_submit_tracker(struct nvme_qpair *qpair,
				      struct nvme_tracker *tr)
{
	struct nvme_request *req = tr->req;

	/*
	 * Set the tracker active and copy its command
	 * to the submission queue.
	 */
	nvme_debug("qpair %d: Submit command, tail %d, cid %d / %d\n",
		   qpair->id,
		   (int)qpair->sq_tail,
		   (int)tr->cid,
		   (int)tr->req->cmd.cid);

	qpair->tr[tr->cid].active = true;
	nvme_qpair_copy_command(&qpair->cmd[qpair->sq_tail], &req->cmd);

	if (++qpair->sq_tail == qpair->entries)
		qpair->sq_tail = 0;

	nvme_wmb();
	nvme_mmio_write_4(qpair->sq_tdbl, qpair->sq_tail);
}

static void nvme_qpair_complete_tracker(struct nvme_qpair *qpair,
					struct nvme_tracker *tr,
					struct nvme_cpl *cpl,
					bool print_on_error)
{
	struct nvme_request *req = tr->req;
	bool retry, error;

	if (!req) {
		nvme_crit("tracker has no request\n");
		qpair->tr[cpl->cid].active = false;
		goto done;
	}

	error = nvme_cpl_is_error(cpl);
	retry = error && nvme_qpair_completion_retry(cpl) &&
		(req->retries < NVME_MAX_RETRY_COUNT);
	if (error && print_on_error) {
		nvme_qpair_print_command(qpair, &req->cmd);
		nvme_qpair_print_completion(qpair, cpl);
	}

	qpair->tr[cpl->cid].active = false;

	if (cpl->cid != req->cmd.cid)
		nvme_crit("cpl and command CID mismatch (%d / %d)\n",
			  (int)cpl->cid, (int)req->cmd.cid);

	if (retry) {
		req->retries++;
		nvme_qpair_submit_tracker(qpair, tr);
		return;
	}

	if (req->cb_fn)
		req->cb_fn(req->cb_arg, cpl);

	nvme_request_free_locked(req);

done:
	tr->req = NULL;

	LIST_REMOVE(tr, list);
	LIST_INSERT_HEAD(&qpair->free_tr, tr, list);
}

static void nvme_qpair_submit_queued_requests(struct nvme_qpair *qpair)
{
	STAILQ_HEAD(, nvme_request) req_queue;
	STAILQ_INIT(&req_queue);

	pthread_mutex_lock(&qpair->lock);

	STAILQ_CONCAT(&req_queue, &qpair->queued_req);

	/*
	 * If the controller is in the middle of a reset, don't
	 * try to submit queued requests - let the reset logic
	 * handle that instead.
	 */
	while (!qpair->ctrlr->resetting && LIST_FIRST(&qpair->free_tr)
			&& !STAILQ_EMPTY(&req_queue)) {
		struct nvme_request *req = STAILQ_FIRST(&req_queue);
		STAILQ_REMOVE_HEAD(&req_queue, stailq);

		pthread_mutex_unlock(&qpair->lock);
		nvme_qpair_submit_request(qpair, req);
		pthread_mutex_lock(&qpair->lock);
	}

	STAILQ_CONCAT(&qpair->queued_req, &req_queue);

	pthread_mutex_unlock(&qpair->lock);
}

static void nvme_qpair_manual_complete_tracker(struct nvme_qpair *qpair,
					       struct nvme_tracker *tr,
					       uint32_t sct,
					       uint32_t sc,
					       uint32_t dnr,
					       bool print_on_error)
{
	struct nvme_cpl	cpl;

	memset(&cpl, 0, sizeof(cpl));
	cpl.sqid = qpair->id;
	cpl.cid = tr->cid;
	cpl.status.sct = sct;
	cpl.status.sc = sc;
	cpl.status.dnr = dnr;

	nvme_qpair_complete_tracker(qpair, tr, &cpl, print_on_error);
}

static void nvme_qpair_manual_complete_request(struct nvme_qpair *qpair,
					       struct nvme_request *req,
					       uint32_t sct, uint32_t sc,
					       bool print_on_error)
{
	struct nvme_cpl	cpl;
	bool error;

	memset(&cpl, 0, sizeof(cpl));
	cpl.sqid = qpair->id;
	cpl.status.sct = sct;
	cpl.status.sc = sc;

	error = nvme_cpl_is_error(&cpl);

	if (error && print_on_error) {
		nvme_qpair_print_command(qpair, &req->cmd);
		nvme_qpair_print_completion(qpair, &cpl);
	}

	if (req->cb_fn)
		req->cb_fn(req->cb_arg, &cpl);

	nvme_request_free_locked(req);
}

static void nvme_qpair_abort_aers(struct nvme_qpair *qpair)
{
	struct nvme_tracker *tr;

	tr = LIST_FIRST(&qpair->outstanding_tr);
	while (tr != NULL) {
		nvme_assert(tr->req != NULL,
			    "tr->req == NULL in abort_aers\n");
		if (tr->req->cmd.opc == NVME_OPC_ASYNC_EVENT_REQUEST) {
			nvme_qpair_manual_complete_tracker(qpair, tr,
					      NVME_SCT_GENERIC,
					      NVME_SC_ABORTED_SQ_DELETION,
					      0, false);
			tr = LIST_FIRST(&qpair->outstanding_tr);
			continue;
		}
		tr = LIST_NEXT(tr, list);
	}
}

static inline void _nvme_qpair_admin_qpair_destroy(struct nvme_qpair *qpair)
{
	nvme_qpair_abort_aers(qpair);
}

static inline void _nvme_qpair_req_bad_phys(struct nvme_qpair *qpair,
					    struct nvme_tracker *tr)
{
	/*
	 * Bad vtophys translation, so abort this request
	 * and return immediately, without retry.
	 */
	nvme_qpair_manual_complete_tracker(qpair, tr, NVME_SCT_GENERIC,
					   NVME_SC_INVALID_FIELD,
					   1, true);
}

/*
 * Build PRP list describing physically contiguous payload buffer.
 */
static int _nvme_qpair_build_contig_request(struct nvme_qpair *qpair,
					    struct nvme_request *req,
					    struct nvme_tracker *tr)
{
	uint64_t phys_addr;
	void *seg_addr;
	uint32_t nseg, cur_nseg, modulo, unaligned;
	void *md_payload;
	void *payload = req->payload.u.contig + req->payload_offset;

	phys_addr = nvme_mem_vtophys(payload);
	if (phys_addr == NVME_VTOPHYS_ERROR) {
		_nvme_qpair_req_bad_phys(qpair, tr);
		return -1;
	}
	nseg = req->payload_size >> PAGE_SHIFT;
	modulo = req->payload_size & (PAGE_SIZE - 1);
	unaligned = phys_addr & (PAGE_SIZE - 1);
	if (modulo || unaligned)
		nseg += 1 + ((modulo + unaligned - 1) >> PAGE_SHIFT);

	if (req->payload.md) {
		md_payload = req->payload.md + req->md_offset;
		tr->req->cmd.mptr = nvme_mem_vtophys(md_payload);
		if (tr->req->cmd.mptr == NVME_VTOPHYS_ERROR) {
			_nvme_qpair_req_bad_phys(qpair, tr);
			return -1;
		}
	}

	tr->req->cmd.psdt = NVME_PSDT_PRP;
	tr->req->cmd.dptr.prp.prp1 = phys_addr;
	if (nseg == 2) {
		seg_addr = payload + PAGE_SIZE - unaligned;
		tr->req->cmd.dptr.prp.prp2 = nvme_mem_vtophys(seg_addr);
	} else if (nseg > 2) {
		cur_nseg = 1;
		tr->req->cmd.dptr.prp.prp2 = (uint64_t)tr->prp_sgl_bus_addr;
		while (cur_nseg < nseg) {
			seg_addr = payload + cur_nseg * PAGE_SIZE - unaligned;
			phys_addr = nvme_mem_vtophys(seg_addr);
			if (phys_addr == NVME_VTOPHYS_ERROR) {
				_nvme_qpair_req_bad_phys(qpair, tr);
				return -1;
			}
			tr->u.prp[cur_nseg - 1] = phys_addr;
			cur_nseg++;
		}
	}

	return 0;
}

/*
 * Build SGL list describing scattered payload buffer.
 */
static int _nvme_qpair_build_hw_sgl_request(struct nvme_qpair *qpair,
					    struct nvme_request *req,
					    struct nvme_tracker *tr)
{
	struct nvme_sgl_descriptor *sgl;
	uint64_t phys_addr;
	uint32_t remaining_transfer_len, length, nseg = 0;
	int ret;

	/*
	 * Build scattered payloads.
	 */
	nvme_assert(req->payload_size != 0,
		    "cannot build SGL for zero-length transfer\n");
	nvme_assert(req->payload.type == NVME_PAYLOAD_TYPE_SGL,
		    "sgl payload type required\n");
	nvme_assert(req->payload.u.sgl.reset_sgl_fn != NULL,
		    "sgl reset callback required\n");
	nvme_assert(req->payload.u.sgl.next_sge_fn != NULL,
		    "sgl callback required\n");
	req->payload.u.sgl.reset_sgl_fn(req->payload.u.sgl.cb_arg,
					req->payload_offset);

	sgl = tr->u.sgl;
	req->cmd.psdt = NVME_PSDT_SGL_MPTR_SGL;
	req->cmd.dptr.sgl1.unkeyed.subtype = 0;

	remaining_transfer_len = req->payload_size;

	while (remaining_transfer_len > 0) {

		if (nseg >= NVME_MAX_SGL_DESCRIPTORS) {
			_nvme_qpair_req_bad_phys(qpair, tr);
			return -1;
		}

		ret = req->payload.u.sgl.next_sge_fn(req->payload.u.sgl.cb_arg,
						     &phys_addr, &length);
		if (ret != 0) {
			_nvme_qpair_req_bad_phys(qpair, tr);
			return ret;
		}

		length = nvme_min(remaining_transfer_len, length);
		remaining_transfer_len -= length;

		sgl->unkeyed.type = NVME_SGL_TYPE_DATA_BLOCK;
		sgl->unkeyed.length = length;
		sgl->address = phys_addr;
		sgl->unkeyed.subtype = 0;

		sgl++;
		nseg++;

	}

	if (nseg == 1) {
		/*
		 * The whole transfer can be described by a single Scatter
		 * Gather List descriptor. Use the special case described
		 * by the spec where SGL1's type is Data Block.
		 * This means the SGL in the tracker is not used at all,
		 * so copy the first (and only) SGL element into SGL1.
		 */
		req->cmd.dptr.sgl1.unkeyed.type = NVME_SGL_TYPE_DATA_BLOCK;
		req->cmd.dptr.sgl1.address = tr->u.sgl[0].address;
		req->cmd.dptr.sgl1.unkeyed.length = tr->u.sgl[0].unkeyed.length;
	} else {
		/* For now we only support 1 SGL segment in NVMe controller */
		req->cmd.dptr.sgl1.unkeyed.type = NVME_SGL_TYPE_LAST_SEGMENT;
		req->cmd.dptr.sgl1.address = tr->prp_sgl_bus_addr;
		req->cmd.dptr.sgl1.unkeyed.length =
			nseg * sizeof(struct nvme_sgl_descriptor);
	}

	return 0;
}

/*
 * Build Physical Region Page list describing scattered payload buffer.
 */
static int _nvme_qpair_build_prps_sgl_request(struct nvme_qpair *qpair,
					      struct nvme_request *req,
					      struct nvme_tracker *tr)
{
	uint64_t phys_addr, prp2 = 0;
	uint32_t data_transferred, remaining_transfer_len, length;
	uint32_t nseg, cur_nseg, total_nseg = 0, last_nseg = 0;
	uint32_t modulo, unaligned, sge_count = 0;
	int ret;

	/*
	 * Build scattered payloads.
	 */
	nvme_assert(req->payload.type == NVME_PAYLOAD_TYPE_SGL,
		    "sgl payload type required\n");
	nvme_assert(req->payload.u.sgl.reset_sgl_fn != NULL,
		    "sgl reset callback required\n");
	req->payload.u.sgl.reset_sgl_fn(req->payload.u.sgl.cb_arg,
					req->payload_offset);

	remaining_transfer_len = req->payload_size;

	while (remaining_transfer_len > 0) {

		nvme_assert(req->payload.u.sgl.next_sge_fn != NULL,
			    "sgl callback required\n");

		ret = req->payload.u.sgl.next_sge_fn(req->payload.u.sgl.cb_arg,
						    &phys_addr, &length);
		if (ret != 0) {
			_nvme_qpair_req_bad_phys(qpair, tr);
			return -1;
		}

		nvme_assert((phys_addr & 0x3) == 0, "address must be dword aligned\n");
		nvme_assert((length >= remaining_transfer_len) || ((phys_addr + length) % PAGE_SIZE) == 0,
			"All SGEs except last must end on a page boundary\n");
		nvme_assert((sge_count == 0) || (phys_addr % PAGE_SIZE) == 0,
			"All SGEs except first must start on a page boundary\n");

		data_transferred = nvme_min(remaining_transfer_len, length);

		nseg = data_transferred >> PAGE_SHIFT;
		modulo = data_transferred & (PAGE_SIZE - 1);
		unaligned = phys_addr & (PAGE_SIZE - 1);
		if (modulo || unaligned)
			nseg += 1 + ((modulo + unaligned - 1) >> PAGE_SHIFT);

		if (total_nseg == 0) {
			req->cmd.psdt = NVME_PSDT_PRP;
			req->cmd.dptr.prp.prp1 = phys_addr;
		}

		total_nseg += nseg;
		sge_count++;
		remaining_transfer_len -= data_transferred;

		if (total_nseg == 2) {
			if (sge_count == 1)
				tr->req->cmd.dptr.prp.prp2 = phys_addr +
					PAGE_SIZE - unaligned;
			else if (sge_count == 2)
				tr->req->cmd.dptr.prp.prp2 = phys_addr;
			/* save prp2 value */
			prp2 = tr->req->cmd.dptr.prp.prp2;
		} else if (total_nseg > 2) {
			if (sge_count == 1)
				cur_nseg = 1;
			else
				cur_nseg = 0;

			tr->req->cmd.dptr.prp.prp2 =
				(uint64_t)tr->prp_sgl_bus_addr;

			while (cur_nseg < nseg) {
				if (prp2) {
					tr->u.prp[0] = prp2;
					tr->u.prp[last_nseg + 1] = phys_addr +
						cur_nseg * PAGE_SIZE - unaligned;
				} else {
					tr->u.prp[last_nseg] = phys_addr +
						cur_nseg * PAGE_SIZE - unaligned;
				}
				last_nseg++;
				cur_nseg++;
			}
		}
	}

	return 0;
}

static void _nvme_qpair_admin_qpair_enable(struct nvme_qpair *qpair)
{
	struct nvme_tracker *tr, *tr_temp;

	/*
	 * Manually abort each outstanding admin command.  Do not retry
	 * admin commands found here, since they will be left over from
	 * a controller reset and its likely the context in which the
	 * command was issued no longer applies.
	 */
	LIST_FOREACH_SAFE(tr, &qpair->outstanding_tr, list, tr_temp) {
		nvme_info("Aborting outstanding admin command\n");
		nvme_qpair_manual_complete_tracker(qpair, tr, NVME_SCT_GENERIC,
						   NVME_SC_ABORTED_BY_REQUEST,
						   1 /* do not retry */, true);
	}

	qpair->enabled = true;
}

static void _nvme_qpair_io_qpair_enable(struct nvme_qpair *qpair)
{
	struct nvme_tracker *tr, *temp;
	struct nvme_request *req;

	qpair->enabled = true;

	qpair->ctrlr->enabled_io_qpairs++;

	/* Manually abort each queued I/O. */
	while (!STAILQ_EMPTY(&qpair->queued_req)) {
		req = STAILQ_FIRST(&qpair->queued_req);
		STAILQ_REMOVE_HEAD(&qpair->queued_req, stailq);
		nvme_info("Aborting queued I/O command\n");
		nvme_qpair_manual_complete_request(qpair, req, NVME_SCT_GENERIC,
						   NVME_SC_ABORTED_BY_REQUEST,
						   true);
	}

	/* Manually abort each outstanding I/O. */
	LIST_FOREACH_SAFE(tr, &qpair->outstanding_tr, list, temp) {
		nvme_info("Aborting outstanding I/O command\n");
		nvme_qpair_manual_complete_tracker(qpair, tr, NVME_SCT_GENERIC,
						   NVME_SC_ABORTED_BY_REQUEST,
						   0, true);
	}
}

static inline void _nvme_qpair_admin_qpair_disable(struct nvme_qpair *qpair)
{
	qpair->enabled = false;
	nvme_qpair_abort_aers(qpair);
}

static inline void _nvme_qpair_io_qpair_disable(struct nvme_qpair *qpair)
{
	qpair->enabled = false;

	qpair->ctrlr->enabled_io_qpairs--;
}

/*
 * Reserve room for the submission queue
 * in the controller memory buffer
 */
static int nvme_ctrlr_reserve_sq_in_cmb(struct nvme_ctrlr *ctrlr,
					uint16_t entries,
					uint64_t aligned, uint64_t *offset)
{
	uint64_t round_offset;
	const uint64_t length = entries * sizeof(struct nvme_cmd);

	round_offset = ctrlr->cmb_current_offset;
	round_offset = (round_offset + (aligned - 1)) & ~(aligned - 1);

	if (round_offset + length > ctrlr->cmb_size)
		return -1;

	*offset = round_offset;
	ctrlr->cmb_current_offset = round_offset + length;

	return 0;
}

/*
 * Initialize a queue pair on the host side.
 */
int nvme_qpair_construct(struct nvme_ctrlr *ctrlr, struct nvme_qpair *qpair,
			 enum nvme_qprio qprio,
			 uint16_t entries, uint16_t trackers)
{
	volatile uint32_t *doorbell_base;
	struct nvme_tracker *tr;
	uint64_t offset;
	unsigned long phys_addr = 0;
	uint16_t i;
	int ret;

	nvme_assert(entries != 0, "Invalid number of entries\n");
	nvme_assert(trackers != 0, "Invalid trackers\n");

	pthread_mutex_init(&qpair->lock, NULL);

	qpair->entries = entries;
	qpair->trackers = trackers;
	qpair->qprio = qprio;
	qpair->sq_in_cmb = false;
	qpair->ctrlr = ctrlr;

	if (ctrlr->opts.use_cmb_sqs) {
		/*
		 * Reserve room for the submission queue in ctrlr
		 * memory buffer.
		 */
		ret = nvme_ctrlr_reserve_sq_in_cmb(ctrlr, entries,
						   PAGE_SIZE,
						   &offset);
		if (ret == 0) {

			qpair->cmd = ctrlr->cmb_bar_virt_addr + offset;
			qpair->cmd_bus_addr = ctrlr->cmb_bar_phys_addr + offset;
			qpair->sq_in_cmb = true;

			nvme_debug("Allocated qpair %d cmd in cmb at %p / 0x%llx\n",
				   qpair->id,
				   qpair->cmd, qpair->cmd_bus_addr);

		}
	}

	if (qpair->sq_in_cmb == false) {

		qpair->cmd =
			nvme_mem_alloc_node(sizeof(struct nvme_cmd) * entries,
				    PAGE_SIZE, NVME_NODE_ID_ANY,
				    (unsigned long *) &qpair->cmd_bus_addr);
		if (!qpair->cmd) {
			nvme_err("Allocate qpair commands failed\n");
			goto fail;
		}
		memset(qpair->cmd, 0, sizeof(struct nvme_cmd) * entries);

		nvme_debug("Allocated qpair %d cmd %p / 0x%llx\n",
			   qpair->id,
			   qpair->cmd, qpair->cmd_bus_addr);
	}

	qpair->cpl = nvme_mem_alloc_node(sizeof(struct nvme_cpl) * entries,
				 PAGE_SIZE, NVME_NODE_ID_ANY,
				 (unsigned long *) &qpair->cpl_bus_addr);
	if (!qpair->cpl) {
		nvme_err("Allocate qpair completions failed\n");
		goto fail;
	}
	memset(qpair->cpl, 0, sizeof(struct nvme_cpl) * entries);

	nvme_debug("Allocated qpair %d cpl at %p / 0x%llx\n",
		   qpair->id,
		   qpair->cpl,
		   qpair->cpl_bus_addr);

	doorbell_base = &ctrlr->regs->doorbell[0].sq_tdbl;
	qpair->sq_tdbl = doorbell_base +
		(2 * qpair->id + 0) * ctrlr->doorbell_stride_u32;
	qpair->cq_hdbl = doorbell_base +
		(2 * qpair->id + 1) * ctrlr->doorbell_stride_u32;

	LIST_INIT(&qpair->free_tr);
	LIST_INIT(&qpair->outstanding_tr);
	STAILQ_INIT(&qpair->free_req);
	STAILQ_INIT(&qpair->queued_req);

	/* Request pool */
	if (nvme_request_pool_construct(qpair)) {
		nvme_err("Create request pool failed\n");
		goto fail;
	}

	/*
	 * Reserve space for all of the trackers in a single allocation.
	 * struct nvme_tracker must be padded so that its size is already
	 * a power of 2. This ensures the PRP list embedded in the nvme_tracker
	 * object will not span a 4KB boundary, while allowing access to
	 * trackers in tr[] via normal array indexing.
	 */
	qpair->tr = nvme_mem_alloc_node(sizeof(struct nvme_tracker) * trackers,
					sizeof(struct nvme_tracker),
					NVME_NODE_ID_ANY, &phys_addr);
	if (!qpair->tr) {
		nvme_err("Allocate request trackers failed\n");
		goto fail;
	}
	memset(qpair->tr, 0, sizeof(struct nvme_tracker) * trackers);

	nvme_debug("Allocated qpair %d trackers at %p / 0x%lx\n",
		   qpair->id, qpair->tr, phys_addr);

	for (i = 0; i < trackers; i++) {
		tr = &qpair->tr[i];
		nvme_qpair_construct_tracker(tr, i, phys_addr);
		LIST_INSERT_HEAD(&qpair->free_tr, tr, list);
		phys_addr += sizeof(struct nvme_tracker);
	}

	nvme_qpair_reset(qpair);

	return 0;

fail:
	nvme_qpair_destroy(qpair);

	return -1;
}

void nvme_qpair_destroy(struct nvme_qpair *qpair)
{
	if (!qpair->ctrlr)
		return; // Not initialized.

	if (nvme_qpair_is_admin_queue(qpair))
		_nvme_qpair_admin_qpair_destroy(qpair);

	if (qpair->cmd && !qpair->sq_in_cmb) {
		nvme_free(qpair->cmd);
		qpair->cmd = NULL;
	}
	if (qpair->cpl) {
		nvme_free(qpair->cpl);
		qpair->cpl = NULL;
	}
	if (qpair->tr) {
		nvme_free(qpair->tr);
		qpair->tr = NULL;
	}
	nvme_request_pool_destroy(qpair);

	qpair->ctrlr = NULL;

	pthread_mutex_destroy(&qpair->lock);
}

static bool nvme_qpair_enabled(struct nvme_qpair *qpair)
{
	if (!qpair->enabled && !qpair->ctrlr->resetting)
		nvme_qpair_enable(qpair);

	return qpair->enabled;
}

int nvme_qpair_submit_request(struct nvme_qpair *qpair,
			      struct nvme_request *req)
{
	struct nvme_tracker *tr;
	struct nvme_request *child_req, *tmp;
	struct nvme_ctrlr *ctrlr = qpair->ctrlr;
	bool child_req_failed = false;
	int ret = 0;

	if (ctrlr->failed) {
		nvme_request_free(req);
		return ENXIO;
	}

	nvme_qpair_enabled(qpair);

	if (req->child_reqs) {

		/*
		 * This is a splitted (parent) request. Submit all of the
		 * children but not the parent request itself, since the
		 * parent is the original unsplit request.
		 */
		TAILQ_FOREACH_SAFE(child_req, &req->children, child_tailq, tmp) {
			if (!child_req_failed) {
				ret = nvme_qpair_submit_request(qpair, child_req);
				if (ret != 0)
					child_req_failed = true;
			} else {
				/* free remaining child_reqs since
				 * one child_req fails */
				nvme_request_remove_child(req, child_req);
				nvme_request_free(child_req);
			}
		}

		return ret;
	}

	pthread_mutex_lock(&qpair->lock);

	tr = LIST_FIRST(&qpair->free_tr);
	if (tr == NULL || !qpair->enabled || !STAILQ_EMPTY(&qpair->queued_req)) {
		/*
		 * No tracker is available, the qpair is disabled due
		 * to an in-progress controller-level reset, or
		 * there are already queued requests.
		 *
		 * Put the request on the qpair's request queue to be
		 * processed when a tracker frees up via a command
		 * completion or when the controller reset is completed.
		 */
		STAILQ_INSERT_TAIL(&qpair->queued_req, req, stailq);
		pthread_mutex_unlock(&qpair->lock);

		if (tr)
			nvme_qpair_submit_queued_requests(qpair);
		return 0;
	}

	/* remove tr from free_tr */
	LIST_REMOVE(tr, list);
	LIST_INSERT_HEAD(&qpair->outstanding_tr, tr, list);
	tr->req = req;
	req->cmd.cid = tr->cid;

	if (req->payload_size == 0) {
		/* Null payload - leave PRP fields zeroed */
		ret = 0;
	} else if (req->payload.type == NVME_PAYLOAD_TYPE_CONTIG) {
		ret = _nvme_qpair_build_contig_request(qpair, req, tr);
	} else if (req->payload.type == NVME_PAYLOAD_TYPE_SGL) {
		if (ctrlr->flags & NVME_CTRLR_SGL_SUPPORTED)
			ret = _nvme_qpair_build_hw_sgl_request(qpair, req, tr);
		else
			ret = _nvme_qpair_build_prps_sgl_request(qpair, req, tr);
	} else {
		nvme_qpair_manual_complete_tracker(qpair, tr, NVME_SCT_GENERIC,
						   NVME_SC_INVALID_FIELD,
						   1 /* do not retry */, true);
		ret = -EINVAL;
	}

	if (ret == 0)
		nvme_qpair_submit_tracker(qpair, tr);

	pthread_mutex_unlock(&qpair->lock);

	return ret;
}

/*
 * Poll for completion of NVMe commands submitted to the
 * specified I/O queue pair.
 */
unsigned int nvme_qpair_poll(struct nvme_qpair *qpair,
			     unsigned int max_completions)
{
	struct nvme_tracker *tr;
	struct nvme_cpl	*cpl;
	uint32_t num_completions = 0;

	if (!nvme_qpair_enabled(qpair))
		/*
		 * qpair is not enabled, likely because a controller reset is
		 * is in progress.  Ignore the interrupt - any I/O that was
		 * associated with this interrupt will get retried when the
		 * reset is complete.
		 */
		return 0;

	if ((max_completions == 0) ||
	    (max_completions > (qpair->entries - 1U)))
		/*
		 * max_completions == 0 means unlimited, but complete at most
		 * one queue depth batch of I/O at a time so that the completion
		 * queue doorbells don't wrap around.
		 */
		max_completions = qpair->entries - 1;

	pthread_mutex_lock(&qpair->lock);

	while (1) {

		cpl = &qpair->cpl[qpair->cq_head];
		if (cpl->status.p != qpair->phase)
			break;

		tr = &qpair->tr[cpl->cid];
		if (tr->active) {
			nvme_qpair_complete_tracker(qpair, tr, cpl, true);
		} else {
			nvme_info("cpl does not map to outstanding cmd\n");
			nvme_qpair_print_completion(qpair, cpl);
			nvme_panic("received completion for unknown cmd\n");
		}

		if (++qpair->cq_head == qpair->entries) {
			qpair->cq_head = 0;
			qpair->phase = !qpair->phase;
		}

		if (++num_completions == max_completions)
			break;
	}

	if (num_completions > 0)
		nvme_mmio_write_4(qpair->cq_hdbl, qpair->cq_head);

	pthread_mutex_unlock(&qpair->lock);

	if (!STAILQ_EMPTY(&qpair->queued_req))
		nvme_qpair_submit_queued_requests(qpair);

	return num_completions;
}

void nvme_qpair_reset(struct nvme_qpair *qpair)
{
	pthread_mutex_lock(&qpair->lock);

	qpair->sq_tail = qpair->cq_head = 0;

	/*
	 * First time through the completion queue, HW will set phase
	 * bit on completions to 1.  So set this to 1 here, indicating
	 * we're looking for a 1 to know which entries have completed.
	 * we'll toggle the bit each time when the completion queue rolls over.
	 */
	qpair->phase = 1;

	memset(qpair->cmd, 0, qpair->entries * sizeof(struct nvme_cmd));
	memset(qpair->cpl, 0, qpair->entries * sizeof(struct nvme_cpl));

	pthread_mutex_unlock(&qpair->lock);
}

void nvme_qpair_enable(struct nvme_qpair *qpair)
{
	pthread_mutex_lock(&qpair->lock);

	if (nvme_qpair_is_io_queue(qpair))
		_nvme_qpair_io_qpair_enable(qpair);
	else
		_nvme_qpair_admin_qpair_enable(qpair);

	pthread_mutex_unlock(&qpair->lock);
}

void nvme_qpair_disable(struct nvme_qpair *qpair)
{
	pthread_mutex_lock(&qpair->lock);

	if (nvme_qpair_is_io_queue(qpair))
		_nvme_qpair_io_qpair_disable(qpair);
	else
		_nvme_qpair_admin_qpair_disable(qpair);

	pthread_mutex_unlock(&qpair->lock);
}

void nvme_qpair_fail(struct nvme_qpair *qpair)
{
	struct nvme_tracker *tr;
	struct nvme_request *req;

	pthread_mutex_lock(&qpair->lock);

	while (!STAILQ_EMPTY(&qpair->queued_req)) {

		nvme_notice("Failing queued I/O command\n");
		req = STAILQ_FIRST(&qpair->queued_req);
		STAILQ_REMOVE_HEAD(&qpair->queued_req, stailq);
		nvme_qpair_manual_complete_request(qpair, req, NVME_SCT_GENERIC,
						   NVME_SC_ABORTED_BY_REQUEST,
						   true);

	}

	/* Manually abort each outstanding I/O. */
	while (!LIST_EMPTY(&qpair->outstanding_tr)) {

		/*
		 * Do not remove the tracker. The abort_tracker path
		 * will do that for us.
		 */
		nvme_notice("Failing outstanding I/O command\n");
		tr = LIST_FIRST(&qpair->outstanding_tr);
		nvme_qpair_manual_complete_tracker(qpair, tr, NVME_SCT_GENERIC,
						   NVME_SC_ABORTED_BY_REQUEST,
						   1, true);

	}

	pthread_mutex_unlock(&qpair->lock);
}

