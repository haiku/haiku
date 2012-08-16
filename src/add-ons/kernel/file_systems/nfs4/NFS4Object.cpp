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


bool
NFS4Object::HandleErrors(uint32 nfs4Error, RPC::Server* serv,
	OpenStateCookie* cookie, OpenState* state, uint32* sequence)
{
	uint32 leaseTime;

	if (cookie != NULL)
		state = cookie->fOpenState;

	switch (nfs4Error) {
		case NFS4_OK:
			return false;

		// server needs more time, we need to wait
		case NFS4ERR_LOCKED:
		case NFS4ERR_DELAY:
			if (sequence != NULL)
				fFileSystem->OpenOwnerSequenceUnlock(*sequence);

			if (cookie == NULL) {
				snooze_etc(sSecToBigTime(5), B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);

				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();

				return true;
			}

			if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(5));

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
				snooze_etc(sSecToBigTime(leaseTime) / 3, B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();
				return true;
			}

			if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(leaseTime) / 3);

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

		// FileHandle has expired or is invalid
		case NFS4ERR_NOFILEHANDLE:
		case NFS4ERR_BADHANDLE:
		case NFS4ERR_FHEXPIRED:
			if (fInfo.UpdateFileHandles(fFileSystem) == B_OK)
				return true;
			return false;

		// filesystem has been moved
		case NFS4ERR_LEASE_MOVED:
		case NFS4ERR_MOVED:
			fFileSystem->Migrate(serv);
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

		*sequence += IncrementSequence(reply.NFS4Error());

		if (HandleErrors(reply.NFS4Error(), serv, NULL, state))
			continue;

		reply.PutFH();
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

