/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RootInode.h"

#include <string.h>

#include "Request.h"


status_t
RootInode::ReadInfo(struct fs_info* info)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		Attribute attr[] = { FATTR4_FILES_FREE, FATTR4_FILES_TOTAL,
			FATTR4_MAXREAD, FATTR4_MAXWRITE, FATTR4_SPACE_FREE,
			FATTR4_SPACE_TOTAL };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		AttrValue* values;
		uint32 count, next = 0;
		result = reply.GetAttr(&values, &count);
		if (result != B_OK)
			return result;

		if (count >= next && values[next].fAttribute == FATTR4_FILES_FREE) {
			info->free_nodes = values[next].fData.fValue64;
			next++;
		}

		if (count >= next && values[next].fAttribute == FATTR4_FILES_TOTAL) {
			info->total_nodes = values[next].fData.fValue64;
			next++;
		}

		uint64 io_size = LONGLONG_MAX;
		if (count >= next && values[next].fAttribute == FATTR4_MAXREAD) {
			io_size = min_c(io_size, values[next].fData.fValue64);
			next++;
		}

		if (count >= next && values[next].fAttribute == FATTR4_MAXWRITE) {
			io_size = min_c(io_size, values[next].fData.fValue64);
			next++;
		}

		if (io_size == LONGLONG_MAX)
			io_size = 32768;
		info->io_size = io_size;
		info->block_size = io_size;

		if (count >= next && values[next].fAttribute == FATTR4_SPACE_FREE) {
			info->free_blocks = values[next].fData.fValue64 / io_size;
			next++;
		}

		if (count >= next && values[next].fAttribute == FATTR4_SPACE_TOTAL) {
			info->total_blocks = values[next].fData.fValue64 / io_size;
			next++;
		}

		delete[] values;

		break;
	} while (true);

	info->flags = 0;
	strncpy(info->volume_name, fInfo.fName, B_FILE_NAME_LENGTH);

	return B_OK;
}


bool
RootInode::ProbeMigration()
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Access();

		status_t result = request.Send();
		if (result != B_OK)
			continue;

		ReplyInterpreter& reply = request.Reply();

		if (reply.NFS4Error() == NFS4ERR_MOVED)
			return true;

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		return false;
	} while (true);
}



status_t
RootInode::GetLocations(AttrValue** attrv)
{
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		Attribute attr[] = { FATTR4_FS_LOCATIONS };
		req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (_HandleErrors(reply.NFS4Error(), serv))
			continue;

		reply.PutFH();

		uint32 count;
		result = reply.GetAttr(attrv, &count);
		if (result != B_OK)
			return result;
		if (count < 1)
			return B_ERROR;
		return B_OK;

	} while (true);

	return B_OK;
}

