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
			if (cookie == NULL) {
				snooze_etc(sSecToBigTime(5), B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				return true;
			} else if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(5));
				if (result != B_TIMED_OUT) {
					release_sem(cookie->fSnoozeCancel);
					return false;
				}
				return true;
			}
			return false;

		// server is in grace period, we need to wait
		case NFS4ERR_GRACE:
			leaseTime = fFileSystem->NFSServer()->LeaseTime();
			if (cookie == NULL) {
				snooze_etc(sSecToBigTime(leaseTime) / 3, B_SYSTEM_TIMEBASE,
					B_RELATIVE_TIMEOUT);
				return true;
			} else if ((cookie->fMode & O_NONBLOCK) == 0) {
				status_t result = acquire_sem_etc(cookie->fSnoozeCancel, 1,
					B_RELATIVE_TIMEOUT, sSecToBigTime(leaseTime) / 3);
				if (result != B_TIMED_OUT) {
					release_sem(cookie->fSnoozeCancel);
					return false;
				}
				return true;
			}
			return false;

		// server has rebooted, reclaim share and try again
		case NFS4ERR_STALE_CLIENTID:
		case NFS4ERR_STALE_STATEID:
			if (state != NULL) {
				if (sequence != NULL)
					fFileSystem->OpenOwnerSequenceUnlock(false);

				fFileSystem->NFSServer()->ServerRebooted(state->fClientID);
dprintf("returned rebooted\n");
				if (sequence != NULL)
					*sequence = fFileSystem->OpenOwnerSequenceLock();
dprintf("locked again\n");
				return true;
			}
			return false;

		// FileHandle has expired
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
NFS4Object::ConfirmOpen(const FileHandle& fh, OpenState* state)
{
	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);

		RequestBuilder& req = request.Builder();

		req.PutFH(fh);
		req.OpenConfirm(sequence, state->fStateID, state->fStateSeq);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(false);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv, NULL, state))
			continue;

		fFileSystem->OpenOwnerSequenceUnlock();

		reply.PutFH();
		result = reply.OpenConfirm(&state->fStateSeq);
		if (result != B_OK)
			return result;

		return B_OK;
	} while (true);
}

