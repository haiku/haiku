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

#ifndef __NVME_INTERNAL_H__
#define __NVME_INTERNAL_H__

#include "nvme_common.h"
#include "nvme_pci.h"
#include "nvme_intel.h"
#include "nvme_mem.h"

#ifndef __HAIKU__
#include <pthread.h>
#include <sys/user.h> /* PAGE_SIZE */
#else
#include "nvme_platform.h"
#endif

/*
 * List functions.
 */
#define	LIST_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = LIST_FIRST((head));				\
	     (var) && ((tvar) = LIST_NEXT((var), field), 1);		\
	     (var) = (tvar))

/*
 * Tail queue functions.
 */
#define	TAILQ_FOREACH_SAFE(var, head, field, tvar)			\
	for ((var) = TAILQ_FIRST((head));				\
	     (var) && ((tvar) = TAILQ_NEXT((var), field), 1);		\
	     (var) = (tvar))

#define INTEL_DC_P3X00_DEVID	0x09538086

#define NVME_TIMEOUT_INFINITE	UINT64_MAX

/*
 * Some Intel devices support vendor-unique read latency log page even
 * though the log page directory says otherwise.
 */
#define NVME_INTEL_QUIRK_READ_LATENCY   0x1

/*
 * Some Intel devices support vendor-unique write latency log page even
 * though the log page directory says otherwise.
 */
#define NVME_INTEL_QUIRK_WRITE_LATENCY  0x2

/*
 * Some controllers need a delay before starting to check the device
 * readiness, which is done by reading the controller status register rdy bit.
 */
#define NVME_QUIRK_DELAY_BEFORE_CHK_RDY	0x4

/*
 * Some controllers need a delay once the controller status register rdy bit
 * switches from 0 to 1.
 */
#define NVME_QUIRK_DELAY_AFTER_RDY	0x8

/*
 * Queues may consist of a contiguous block of physical
 * memory or optionally a non-contiguous set of physical
 * memory pages (defined by a Physical Region Pages List)
 */
#define NVME_MAX_PRP_LIST_ENTRIES       (506)

/*
 * For commands requiring more than 2 PRP entries, one PRP will be
 * embedded in the command (prp1), and the rest of the PRP entries
 * will be in a list pointed to by the command (prp2).  This means
 * that real max number of PRP entries we support is 506+1, which
 * results in a max xfer size of 506*PAGE_SIZE.
 */
#define NVME_MAX_XFER_SIZE	NVME_MAX_PRP_LIST_ENTRIES * PAGE_SIZE

#define NVME_ADMIN_TRACKERS	        (16)
#define NVME_ADMIN_ENTRIES	        (128)

/*
 * NVME_IO_ENTRIES defines the size of an I/O qpair's submission and completion
 * queues, while NVME_IO_TRACKERS defines the maximum number of I/O that we
 * will allow outstanding on an I/O qpair at any time. The only advantage in
 * having IO_ENTRIES > IO_TRACKERS is for debugging purposes - when dumping
 * the contents of the submission and completion queues, it will show a longer
 * history of data.
 */
#define NVME_IO_ENTRIES		        (1024U)
#define NVME_IO_TRACKERS	        (128U)
#define NVME_IO_ENTRIES_VS_TRACKERS_RATIO (NVME_IO_ENTRIES / NVME_IO_TRACKERS)

/*
 * NVME_MAX_SGL_DESCRIPTORS defines the maximum number of descriptors in one SGL
 * segment.
 */
#define NVME_MAX_SGL_DESCRIPTORS	(253)

/*
 * NVME_MAX_IO_ENTRIES is not defined, since it is specified in CC.MQES
 * for each controller.
 */

#define NVME_MAX_ASYNC_EVENTS	        (8)

/*
 * NVME_MAX_IO_QUEUES in nvme_spec.h defines the 64K spec-limit, but this
 * define specifies the maximum number of queues this driver will actually
 * try to configure, if available.
 */
#define DEFAULT_MAX_IO_QUEUES		(1024)

/*
 * Maximum of times a failed command can be retried.
 */
#define NVME_MAX_RETRY_COUNT		(3)

/*
 * I/O queue type.
 */
enum nvme_io_queue_type {

	NVME_IO_QTYPE_INVALID = 0,
	NVME_IO_SUBMISSION_QUEUE,
	NVME_IO_COMPLETION_QUEUE,
};

enum nvme_payload_type {

	NVME_PAYLOAD_TYPE_INVALID = 0,

	/*
	 * nvme_request::u.payload.contig_buffer is valid for this request.
	 */
	NVME_PAYLOAD_TYPE_CONTIG,

	/*
	 * nvme_request::u.sgl is valid for this request
	 */
	NVME_PAYLOAD_TYPE_SGL,
};

/*
 * Controller support flags.
 */
enum nvme_ctrlr_flags {

	/*
	 * The SGL is supported.
	 */
	NVME_CTRLR_SGL_SUPPORTED = 0x1,

};

/*
 * Descriptor for a request data payload.
 *
 * This struct is arranged so that it fits nicely in struct nvme_request.
 */
struct __attribute__((packed)) nvme_payload {

	union {
		/*
		 * Virtual memory address of a single
		 * physically contiguous buffer
		 */
		void *contig;

		/*
		 * Call back functions for retrieving physical
		 * addresses for scattered payloads.
		 */
		struct {
			nvme_req_reset_sgl_cb reset_sgl_fn;
			nvme_req_next_sge_cb next_sge_fn;
			void *cb_arg;
		} sgl;
	} u;

	/*
	 * Virtual memory address of a single physically
	 * contiguous metadata buffer
	 */
	void *md;

	/*
	 * Payload type.
	 */
	uint8_t type;

};

struct nvme_request {

	/*
	 * NVMe command: must be aligned on 64B.
	 */
	struct nvme_cmd		         cmd;

	/*
	 * Data payload for this request's command.
	 */
	struct nvme_payload              payload;

	uint8_t			         retries;

	/*
	 * Number of child requests still outstanding for this
	 * request which was split into multiple child requests.
	 */
	uint8_t			         child_reqs;
	uint32_t		         payload_size;

	/*
	 * Offset in bytes from the beginning of payload for this request.
	 * This is used for I/O commands that are split into multiple requests.
	 */
	uint32_t	                 payload_offset;
	uint32_t		         md_offset;

	nvme_cmd_cb		         cb_fn;
	void			         *cb_arg;

	/*
	 * The following members should not be reordered with members
	 * above.  These members are only needed when splitting
	 * requests which is done rarely, and the driver is careful
	 * to not touch the following fields until a split operation is
	 * needed, to avoid touching an extra cacheline.
	 */

	/*
	 * Points to the outstanding child requests for a parent request.
	 * Only valid if a request was split into multiple child
	 * requests, and is not initialized for non-split requests.
	 */
	TAILQ_HEAD(, nvme_request)	 children;

	/*
	 * Linked-list pointers for a child request in its parent's list.
	 */
	TAILQ_ENTRY(nvme_request)	 child_tailq;

	/*
	 * For queueing in qpair queued_req or free_req.
	 */
	struct nvme_qpair		 *qpair;
	STAILQ_ENTRY(nvme_request)	 stailq;

	/*
	 * Points to a parent request if part of a split request,
	 * NULL otherwise.
	 */
	struct nvme_request		 *parent;

	/*
	 * Completion status for a parent request.  Initialized to all 0's
	 * (SUCCESS) before child requests are submitted.  If a child
	 * request completes with error, the error status is copied here,
	 * to ensure that the parent request is also completed with error
	 * status once all child requests are completed.
	 */
	struct nvme_cpl		         parent_status;

} __attribute__((aligned(64)));

struct nvme_completion_poll_status {
	struct nvme_cpl		cpl;
	bool			done;
};

struct nvme_async_event_request {
	struct nvme_ctrlr	*ctrlr;
	struct nvme_request	*req;
	struct nvme_cpl		cpl;
};

struct nvme_tracker {

	LIST_ENTRY(nvme_tracker)	list;

	struct nvme_request		*req;
	uint16_t			cid;

	uint16_t			rsvd1: 15;
	uint16_t			active: 1;

	uint32_t			rsvd2;

	uint64_t			prp_sgl_bus_addr;

	union {
		uint64_t			prp[NVME_MAX_PRP_LIST_ENTRIES];
		struct nvme_sgl_descriptor	sgl[NVME_MAX_SGL_DESCRIPTORS];
	} u;

	uint64_t			rsvd3;
};

/*
 * struct nvme_tracker must be exactly 4K so that the prp[] array does not
 * cross a page boundery and so that there is no padding required to meet
 * alignment requirements.
 */
nvme_static_assert(sizeof(struct nvme_tracker) == 4096,
		   "nvme_tracker is not 4K");
nvme_static_assert((offsetof(struct nvme_tracker, u.sgl) & 7) == 0,
		   "SGL must be Qword aligned");

struct nvme_qpair {
	/*
	 * Guards access to this structure.
	 */
	pthread_mutex_t				lock;

	volatile uint32_t	        *sq_tdbl;
	volatile uint32_t	        *cq_hdbl;

	/*
	 * Submission queue
	 */
	struct nvme_cmd		        *cmd;

	/*
	 * Completion queue
	 */
	struct nvme_cpl		        *cpl;

	LIST_HEAD(, nvme_tracker)	free_tr;
	LIST_HEAD(, nvme_tracker)	outstanding_tr;

	/*
	 * Array of trackers indexed by command ID.
	 */
	uint16_t			trackers;
	struct nvme_tracker		*tr;

	struct nvme_request		*reqs;
	unsigned int			num_reqs;
	STAILQ_HEAD(, nvme_request)	free_req;
	STAILQ_HEAD(, nvme_request)	queued_req;

	uint16_t			id;

	uint16_t			entries;
	uint16_t			sq_tail;
	uint16_t			cq_head;

	uint8_t				phase;

	bool				enabled;
	bool				sq_in_cmb;

	/*
	 * Fields below this point should not be touched on the
	 * normal I/O happy path.
	 */

	uint8_t				qprio;

	struct nvme_ctrlr		*ctrlr;

	/* List entry for nvme_ctrlr::free_io_qpairs and active_io_qpairs */
	TAILQ_ENTRY(nvme_qpair)		tailq;

	phys_addr_t			cmd_bus_addr;
	phys_addr_t			cpl_bus_addr;
};

struct nvme_ns {

	struct nvme_ctrlr		*ctrlr;

	uint32_t			stripe_size;
	uint32_t			sector_size;

	uint32_t			md_size;
	uint32_t			pi_type;

	uint32_t			sectors_per_max_io;
	uint32_t			sectors_per_stripe;

	uint16_t			id;
	uint16_t			flags;

	int				open_count;

};

/*
 * State of struct nvme_ctrlr (in particular, during initialization).
 */
enum nvme_ctrlr_state {

	/*
	 * Controller has not been initialized yet.
	 */
	NVME_CTRLR_STATE_INIT = 0,

	/*
	 * Waiting for CSTS.RDY to transition from 0 to 1
	 * so that CC.EN may be set to 0.
	 */
	NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_1,

	/*
	 * Waiting for CSTS.RDY to transition from 1 to 0
	 * so that CC.EN may be set to 1.
	 */
	NVME_CTRLR_STATE_DISABLE_WAIT_FOR_READY_0,

	/*
	 * Waiting for CSTS.RDY to transition from 0 to 1
	 * after enabling the controller.
	 */
	NVME_CTRLR_STATE_ENABLE_WAIT_FOR_READY_1,

	/*
	 * Controller initialization has completed and
	 * the controller is ready.
	 */
	NVME_CTRLR_STATE_READY
};

/*
 * One of these per allocated PCI device.
 */
struct nvme_ctrlr {

	/*
	 * NVMe MMIO register space.
	 */
	volatile struct nvme_registers	*regs;

	/*
	 * Array of I/O queue pairs.
	 */
	struct nvme_qpair		*ioq;

	/*
	 * Size of the array of I/O queue pairs.
	 */
	unsigned int			io_queues;

	/*
	 * Maximum I/O queue pairs.
	 */
	unsigned int			max_io_queues;

	/*
	 * Number of I/O queue pairs enabled
	 */
	unsigned int			enabled_io_qpairs;

	/*
	 * Maximum entries for I/O qpairs
	 */
	unsigned int			io_qpairs_max_entries;

	/*
	 * Array of namespace IDs.
	 */
	unsigned int			nr_ns;
	struct nvme_ns		        *ns;

	/*
	 * Controller state.
	 */
	bool				resetting;
	bool				failed;

	/*
	 * Controller support flags.
	 */
	uint64_t			flags;

	/*
	 * Cold data (not accessed in normal I/O path) is after this point.
	 */
	enum nvme_ctrlr_state		state;
	uint64_t			state_timeout_ms;

	/*
	 * All the log pages supported.
	 */
	bool				log_page_supported[256];

	/*
	 * All the features supported.
	 */
	bool				feature_supported[256];

	/*
	 * Associated PCI device information.
	 */
	struct pci_device		*pci_dev;

	/*
	 * Maximum i/o size in bytes.
	 */
	uint32_t			max_xfer_size;

	/*
	 * Minimum page size supported by this controller in bytes.
	 */
	uint32_t			min_page_size;

	/*
	 * Stride in uint32_t units between doorbell registers
	 * (1 = 4 bytes, 2 = 8 bytes, ...).
	 */
	uint32_t			doorbell_stride_u32;

	uint32_t			num_aers;
	struct nvme_async_event_request	aer[NVME_MAX_ASYNC_EVENTS];
	nvme_aer_cb		        aer_cb_fn;
	void				*aer_cb_arg;

	/*
	 * Admin queue pair.
	 */
	struct nvme_qpair		adminq;

	/*
	 * Guards access to the controller itself.
	 */
	pthread_mutex_t			lock;

	/*
	 * Identify Controller data.
	 */
	struct nvme_ctrlr_data		cdata;

	/*
	 * Array of Identify Namespace data.
	 * Stored separately from ns since nsdata should
	 * not normally be accessed during I/O.
	 */
	struct nvme_ns_data	        *nsdata;

	TAILQ_HEAD(, nvme_qpair)	free_io_qpairs;
	TAILQ_HEAD(, nvme_qpair)	active_io_qpairs;

	/*
	 * Controller option set on open.
	 */
	struct nvme_ctrlr_opts		opts;

	/*
	 * BAR mapping address which contains controller memory buffer.
	 */
	void				*cmb_bar_virt_addr;

	/*
	 * BAR physical address which contains controller memory buffer.
	 */
	uint64_t			cmb_bar_phys_addr;

	/*
	 * Controller memory buffer size in Bytes.
	 */
	uint64_t			cmb_size;

	/*
	 * Current offset of controller memory buffer.
	 */
	uint64_t			cmb_current_offset;

	/*
	 * Quirks flags.
	 */
	unsigned int			quirks;

	/*
	 * For controller list.
	 */
	LIST_ENTRY(nvme_ctrlr)		link;

} __attribute__((aligned(PAGE_SIZE)));

/*
 * Admin functions.
 */
extern int nvme_admin_identify_ctrlr(struct nvme_ctrlr *ctrlr,
				     struct nvme_ctrlr_data *cdata);

extern int nvme_admin_get_feature(struct nvme_ctrlr *ctrlr,
				  enum nvme_feat_sel sel,
				  enum nvme_feat feature,
				  uint32_t cdw11, uint32_t *attributes);

extern int nvme_admin_set_feature(struct nvme_ctrlr *ctrlr,
				  bool save,
				  enum nvme_feat feature,
				  uint32_t cdw11, uint32_t cdw12,
				  uint32_t *attributes);

extern int nvme_admin_format_nvm(struct nvme_ctrlr *ctrlr,
				 unsigned int nsid,
				 struct nvme_format *format);

extern int nvme_admin_get_log_page(struct nvme_ctrlr *ctrlr,
				   uint8_t log_page, uint32_t nsid,
				   void *payload, uint32_t payload_size);

extern int nvme_admin_abort_cmd(struct nvme_ctrlr *ctrlr,
				uint16_t cid, uint16_t sqid);

extern int nvme_admin_create_ioq(struct nvme_ctrlr *ctrlr,
				 struct nvme_qpair *io_que,
				 enum nvme_io_queue_type io_qtype);

extern int nvme_admin_delete_ioq(struct nvme_ctrlr *ctrlr,
				 struct nvme_qpair *qpair,
				 enum nvme_io_queue_type io_qtype);

extern int nvme_admin_identify_ns(struct nvme_ctrlr *ctrlr,
				  uint16_t nsid,
				  struct nvme_ns_data *nsdata);

extern int nvme_admin_attach_ns(struct nvme_ctrlr *ctrlr,
				uint32_t nsid,
				struct nvme_ctrlr_list *clist);

extern int nvme_admin_detach_ns(struct nvme_ctrlr *ctrlr,
				uint32_t nsid,
				struct nvme_ctrlr_list *clist);

extern int nvme_admin_create_ns(struct nvme_ctrlr *ctrlr,
				struct nvme_ns_data *nsdata,
				unsigned int *nsid);

extern int nvme_admin_delete_ns(struct nvme_ctrlr *ctrlr,
				unsigned int nsid);

extern int nvme_admin_fw_commit(struct nvme_ctrlr *ctrlr,
				const struct nvme_fw_commit *fw_commit);

extern int nvme_admin_fw_image_dl(struct nvme_ctrlr *ctrlr,
				  void *fw, uint32_t size, uint32_t offset);

extern void nvme_request_completion_poll_cb(void *arg,
					    const struct nvme_cpl *cpl);

extern struct nvme_ctrlr *nvme_ctrlr_attach(struct pci_device *pci_dev,
					    struct nvme_ctrlr_opts *opts);

extern void nvme_ctrlr_detach(struct nvme_ctrlr *ctrlr);

extern int nvme_qpair_construct(struct nvme_ctrlr *ctrlr,
				struct nvme_qpair *qpair, enum nvme_qprio qprio,
				uint16_t entries, uint16_t trackers);

extern void nvme_qpair_destroy(struct nvme_qpair *qpair);
extern void nvme_qpair_enable(struct nvme_qpair *qpair);
extern void nvme_qpair_disable(struct nvme_qpair *qpair);
extern int  nvme_qpair_submit_request(struct nvme_qpair *qpair,
				      struct nvme_request *req);
extern void nvme_qpair_reset(struct nvme_qpair *qpair);
extern void nvme_qpair_fail(struct nvme_qpair *qpair);

extern int nvme_request_pool_construct(struct nvme_qpair *qpair);

extern void nvme_request_pool_destroy(struct nvme_qpair *qpair);

extern struct nvme_request *nvme_request_allocate(struct nvme_qpair *qpair,
		      const struct nvme_payload *payload, uint32_t payload_size,
		      nvme_cmd_cb cb_fn, void *cb_arg);

extern struct nvme_request *nvme_request_allocate_null(struct nvme_qpair *qpair,
						       nvme_cmd_cb cb_fn,
						       void *cb_arg);

extern struct nvme_request *
nvme_request_allocate_contig(struct nvme_qpair *qpair,
			     void *buffer, uint32_t payload_size,
			     nvme_cmd_cb cb_fn, void *cb_arg);

extern void nvme_request_free(struct nvme_request *req);
extern void nvme_request_free_locked(struct nvme_request *req);

extern void nvme_request_add_child(struct nvme_request *parent,
				   struct nvme_request *child);

extern void nvme_request_remove_child(struct nvme_request *parent,
				      struct nvme_request *child);

extern unsigned int nvme_ctrlr_get_quirks(struct pci_device *pdev);

extern int nvme_ns_construct(struct nvme_ctrlr *ctrlr,
			     struct nvme_ns *ns, unsigned int id);

/*
 * Registers mmio access.
 */
#define nvme_reg_mmio_read_4(sc, reg)		\
	nvme_mmio_read_4((__u32 *)&(sc)->regs->reg)

#define nvme_reg_mmio_read_8(sc, reg)		\
	nvme_mmio_read_8((__u64 *)&(sc)->regs->reg)

#define nvme_reg_mmio_write_4(sc, reg, val)	\
	nvme_mmio_write_4((__u32 *)&(sc)->regs->reg, val)

#define nvme_reg_mmio_write_8(sc, reg, val)	\
	nvme_mmio_write_8((__u64 *)&(sc)->regs->reg, val)

#endif /* __NVME_INTERNAL_H__ */
