/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Cookie.h"
#include "FileSystem.h"
#include "NFS4Object.h"
#include "OpenState.h"
#include "Request.h"


static inline bigtime_t
RetryDelay(uint32 attempt, uint32 leaseTime = 0)
{
	attempt = min_c(attempt, sizeof(bigtime_t) * 8);

	bigtime_t delay = (1 << (attempt - 1)) * 100000;
	if (leaseTime != 0)
		delay = min_c(delay, sSecToBigTime(leaseTime));
	return delay;
}


bool
NFS4Object::HandleErrors(uint32& attempt, uint32 nfs4Error, RPC::Server* server,
	OpenStateCookie* cookie, OpenState* state, uint32* sequence)
{
	// No request send by the client should cause any of the following errors.
	ASSERT(nfs4Error != NFS4ERR_CLID_INUSE);
	ASSERT(nfs4Error != NFS4ERR_NOFILEHANDLE);
	ASSERT(nfs4Error != NFS4ERR_BAD_STATEID);
	ASSERT(nfs4Error != NFS4ERR_RESTOREFH);
	ASSERT(nfs4Error != NFS4ERR_LOCKS_HELD);
	ASSERT(nfs4Error != NFS4ERR_OP_ILLEGAL);

	attempt++;

	if (cookie != NULL)
		state = cookie->fOpenState;

	uint32 leaseTime;
	status_t result;
	switch (nfs4Error) {
		case NFS4_OK:
			return false;

		// retransmission of CLOSE caused seqid to fall back
		case NFS4ERR_BAD_SEQID:
			if (attempt == 1) {
				ASSERT(sequence != NULL);
				(*sequence)++;
				return true;
			}
			return false;

		// resource is locked, we need to wait
		case NFS4ERR_DENIED:
			if (sequence != NULL)
				fFileSystem->OpenOwnerSequenceUnlock(*sequence);

			result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
				B_RELATIVE_TIMEOUT, RetryDelay(attempt));

			if (sequence != NULL)
				*sequence = fFileSystem->OpenOwnerSequenceLock();

			if (result != B_TIMED_OUT) {
				if (result == B_OK)
					release_sem(cookie->fSnoozeCancel);
				return false;
			}
			return true;

		// server needs more time, we need to wait
		case NFS4ERR_LOCKED:
		case NFS4ERR_DELAY:
			if (sequence != NULL)
				fFileSystem->OpenOwnerSequenceUnlock(*sequence);

			if (cookie == NULL) {
				snooze_etc(RetryDelay(attempt), B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				

				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();

				return true;
			}

			if ((cookie->fMode & O_NONBLOCK) == 0) {
				result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, RetryDelay(attempt));

				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();

				if (result != B_TIMED_OUT) {
					if (result == B_OK)
						release_sem(cookie->fSnoozeCancel);
					return false;
				}
				return true;
			}

			if (sequence != NULL)
				*sequence = fFileSystem->OpenOwnerSequenceLock();
			return false;

		// server is in grace period, we need to wait
		case NFS4ERR_GRACE:
			leaseTime = fFileSystem->NFSServer()->LeaseTime();
			if (sequence != NULL)
				fFileSystem->OpenOwnerSequenceUnlock(*sequence);

			if (cookie == NULL) {
				snooze_etc(RetryDelay(attempt, leaseTime), B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();
				return true;
			}

			if ((cookie->fMode & O_NONBLOCK) == 0) {
				result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, RetryDelay(attempt, leaseTime));

				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();

				if (result != B_TIMED_OUT) {
					if (result == B_OK)
						release_sem(cookie->fSnoozeCancel);
					return false;
				}
				return true;
			}

			if (sequence != NULL)
				*sequence = fFileSystem->OpenOwnerSequenceLock();
			return false;

		// server has rebooted, reclaim share and try again
		case NFS4ERR_STALE_CLIENTID:
		case NFS4ERR_STALE_STATEID:
			if (state != NULL) {
				if (sequence != NULL)
					fFileSystem->OpenOwnerSequenceUnlock(*sequence);

				fFileSystem->NFSServer()->ServerRebooted(state->fClientID);
				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();

				return true;
			}
			return false;

		// File Handle has expired, is invalid or the node has been deleted
		case NFS4ERR_BADHANDLE:
		case NFS4ERR_FHEXPIRED:
		case NFS4ERR_STALE:
			if (fInfo.UpdateFileHandles(fFileSystem) == B_OK)
				return true;
			return false;

		// filesystem has been moved
		case NFS4ERR_LEASE_MOVED:
		case NFS4ERR_MOVED:
			fFileSystem->Migrate(server);
			return true;

		// lease has expired
		case NFS4ERR_EXPIRED:
			if (state != NULL) {
				fFileSystem->NFSServer()->ClientId(state->fClientID, true);
				return true;
			}
			return false;

		default:
			return false;
	}
}


status_t
NFS4Object::ConfirmOpen(const FileHandle& fh, OpenState* state,
	uint32* sequence)
{
	ASSERT(state != NULL);
	ASSERT(sequence != NULL);

	uint32 attempt = 0;
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);

		RequestBuilder& req = request.Builder();

		req.PutFH(fh);
		req.OpenConfirm(*sequence, state->fStateID, state->fStateSeq);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		result = reply.PutFH();
		if (result == B_OK)
			*sequence += IncrementSequence(reply.NFS4Error());

		if (HandleErrors(attempt, reply.NFS4Error(), serv, NULL, state))
			continue;

		result = reply.OpenConfirm(&state->fStateSeq);
		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}


uint32
NFS4Object::IncrementSequence(uint32 error)
{
	if (error != NFS4ERR_STALE_CLIENTID && error != NFS4ERR_STALE_STATEID
		&& error != NFS4ERR_BAD_STATEID && error != NFS4ERR_BAD_SEQID
		&& error != NFS4ERR_BADXDR && error != NFS4ERR_RESOURCE
		&& error != NFS4ERR_NOFILEHANDLE)
		return 1;

	return 0;
}

