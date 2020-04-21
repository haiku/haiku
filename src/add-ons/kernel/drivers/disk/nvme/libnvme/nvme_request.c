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
 * Allocate a request descriptor from the queue pair free list.
 */
static struct nvme_request *nvme_alloc_request(struct nvme_qpair *qpair)
{
	struct nvme_request *req;

	pthread_mutex_lock(&qpair->lock);

	req = STAILQ_FIRST(&qpair->free_req);
	if (req) {
		STAILQ_REMOVE_HEAD(&qpair->free_req, stailq);
		memset(&req->cmd, 0, sizeof(struct nvme_cmd));
	}

	pthread_mutex_unlock(&qpair->lock);

	return req;
}

static void nvme_request_cb_complete_child(void *child_arg,
					   const struct nvme_cpl *cpl)
{
	struct nvme_request *child = child_arg;
	struct nvme_request *parent = child->parent;

	nvme_request_remove_child(parent, child);

	if (nvme_cpl_is_error(cpl))
		memcpy(&parent->parent_status, cpl, sizeof(*cpl));

	if (parent->child_reqs == 0) {
		if (parent->cb_fn)
			parent->cb_fn(parent->cb_arg, &parent->parent_status);
		nvme_request_free_locked(parent);
	}
}

void nvme_request_completion_poll_cb(void *arg, const struct nvme_cpl *cpl)
{
	struct nvme_completion_poll_status *status = arg;

	memcpy(&status->cpl, cpl, sizeof(*cpl));
	status->done = true;
}

int nvme_request_pool_construct(struct nvme_qpair *qpair)
{
	struct nvme_request *req;
	unsigned int i;

	qpair->num_reqs = qpair->trackers * NVME_IO_ENTRIES_VS_TRACKERS_RATIO;
	qpair->reqs = calloc(qpair->num_reqs, sizeof(struct nvme_request));
	if (!qpair->reqs) {
		nvme_err("QPair %d: allocate %u requests failed\n",
			 (int)qpair->id, qpair->num_reqs);
		return ENOMEM;
	}

	nvme_info("QPair %d: %d requests in pool\n",
		  (int)qpair->id,
		  (int)qpair->num_reqs);

	for(i = 0; i < qpair->num_reqs; i++) {
		req = &qpair->reqs[i];
		req->qpair = qpair;
		STAILQ_INSERT_TAIL(&qpair->free_req, req, stailq);
		req++;
	}

	return 0;
}

void nvme_request_pool_destroy(struct nvme_qpair *qpair)
{
	struct nvme_request *req;
	unsigned int n = 0;

	while ((req = STAILQ_FIRST(&qpair->free_req))) {
		STAILQ_REMOVE_HEAD(&qpair->free_req, stailq);
		n++;
	}

	if (n != qpair->num_reqs)
		nvme_err("QPair %d: Freed %d/%d requests\n",
			 (int)qpair->id, n, (int)qpair->num_reqs);

	free(qpair->reqs);
}

struct nvme_request *nvme_request_allocate(struct nvme_qpair *qpair,
					   const struct nvme_payload *payload,
					   uint32_t payload_size,
					   nvme_cmd_cb cb_fn,
					   void *cb_arg)
{
	struct nvme_request *req;

	req = nvme_alloc_request(qpair);
	if (req == NULL)
		return NULL;

	/*
	 * Only memset up to (but not including) the children TAILQ_ENTRY.
	 * Children, and following members, are only used as part of I/O
	 * splitting so we avoid memsetting them until it is actually needed.
	 * They will be initialized in nvme_request_add_child()
	 * if the request is split.
	 */
	memset(req, 0, offsetof(struct nvme_request, children));
	req->cb_fn = cb_fn;
	req->cb_arg = cb_arg;
	req->payload = *payload;
	req->payload_size = payload_size;

	return req;
}

struct nvme_request *nvme_request_allocate_contig(struct nvme_qpair *qpair,
						  void *buffer,
						  uint32_t payload_size,
						  nvme_cmd_cb cb_fn,
						  void *cb_arg)
{
	struct nvme_payload payload;

	payload.type = NVME_PAYLOAD_TYPE_CONTIG;
	payload.u.contig = buffer;
	payload.md = NULL;

	return nvme_request_allocate(qpair, &payload, payload_size,
				     cb_fn, cb_arg);
}

struct nvme_request *nvme_request_allocate_null(struct nvme_qpair *qpair,
						nvme_cmd_cb cb_fn, void *cb_arg)
{
	return nvme_request_allocate_contig(qpair, NULL, 0, cb_fn, cb_arg);
}

void nvme_request_free_locked(struct nvme_request *req)
{
	nvme_assert(req->child_reqs == 0, "Number of child request not 0\n");

	STAILQ_INSERT_HEAD(&req->qpair->free_req, req, stailq);
}

void nvme_request_free(struct nvme_request *req)
{
	pthread_mutex_lock(&req->qpair->lock);

	nvme_request_free_locked(req);

	pthread_mutex_unlock(&req->qpair->lock);
}

void nvme_request_add_child(struct nvme_request *parent,
			    struct nvme_request *child)
{
	if (parent->child_reqs == 0) {
		/*
		 * Defer initialization of the children TAILQ since it falls
		 * on a separate cacheline.  This ensures we do not touch this
		 * cacheline except on request splitting cases, which are
		 * relatively rare.
		 */
		TAILQ_INIT(&parent->children);
		parent->parent = NULL;
		memset(&parent->parent_status, 0, sizeof(struct nvme_cpl));
	}

	parent->child_reqs++;
	TAILQ_INSERT_TAIL(&parent->children, child, child_tailq);
	child->parent = parent;
	child->cb_fn = nvme_request_cb_complete_child;
	child->cb_arg = child;
}

void nvme_request_remove_child(struct nvme_request *parent,
			       struct nvme_request *child)
{
	nvme_assert(child->parent == parent, "child->parent != parent\n");
	nvme_assert(parent->child_reqs != 0, "child_reqs is 0\n");

	parent->child_reqs--;
	TAILQ_REMOVE(&parent->children, child, child_tailq);
}
