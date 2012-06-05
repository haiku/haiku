/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "RequestBuilder.h"

#include <string.h>


RequestBuilder::RequestBuilder(Procedure proc)
	:
	fOpCount(0),
	fProcedure(proc),
	fRequest(RPC::Call::Create(proc, RPC::Auth::CreateSys(),
								RPC::Auth::CreateNone()))
{
	if (fProcedure == ProcCompound) {
		fRequest->Stream().AddOpaque(NULL, 0);
		fRequest->Stream().AddUInt(0);

		fOpCountPosition = fRequest->Stream().Current();
		fRequest->Stream().AddUInt(0);
	}
}


RequestBuilder::~RequestBuilder()
{
	delete fRequest;
}


status_t
RequestBuilder::Access()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpAccess);
	fRequest->Stream().AddUInt(ACCESS4_READ | ACCESS4_LOOKUP | ACCESS4_MODIFY
								| ACCESS4_EXTEND | ACCESS4_DELETE
								| ACCESS4_EXECUTE);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::GetAttr(Attribute* attrs, uint32 count)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpGetAttr);
	_AttrBitmap(fRequest->Stream(), attrs, count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::GetFH()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpGetFH);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::LookUp(const char* name)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;
	if (name == NULL)
		return B_BAD_VALUE;

	fRequest->Stream().AddUInt(OpLookUp);
	fRequest->Stream().AddString(name, strlen(name));
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::LookUpUp()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpLookUpUp);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Open(uint32 seq, uint32 access, uint64 id, OpenCreate oc,
	const char* name)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpOpen);
	fRequest->Stream().AddUInt(seq);
	fRequest->Stream().AddUInt(access);
	fRequest->Stream().AddUInt(0);			// deny none
	fRequest->Stream().AddUHyper(id);

	char owner[128];
	int pos = 0;
	*(uint32*)(owner + pos) = time(NULL);
	pos += sizeof(uint32);

	*(uint32*)(owner + pos) = find_thread(NULL);
	pos += sizeof(uint32);

	fRequest->Stream().AddOpaque(owner, pos);

	fRequest->Stream().AddUInt(oc);
	fRequest->Stream().AddUInt(0);			// claim null
	fRequest->Stream().AddString(name, strlen(name));

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::OpenConfirm(uint32 seq, const uint32* id, uint32 stateSeq)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpOpenConfirm);
	fRequest->Stream().AddUInt(stateSeq);
	fRequest->Stream().AddUInt(id[0]);
	fRequest->Stream().AddUInt(id[1]);
	fRequest->Stream().AddUInt(id[2]);
	fRequest->Stream().AddUInt(seq);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::PutFH(const Filehandle& fh)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpPutFH);
	fRequest->Stream().AddOpaque(fh.fFH, fh.fSize);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::PutRootFH()
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpPutRootFH);
	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::Read(const uint32* id, uint32 stateSeq, uint64 pos, uint32 len)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpRead);
	fRequest->Stream().AddUInt(stateSeq);
	fRequest->Stream().AddUInt(id[0]);
	fRequest->Stream().AddUInt(id[1]);
	fRequest->Stream().AddUInt(id[2]);
	fRequest->Stream().AddUHyper(pos);
	fRequest->Stream().AddUInt(len);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::ReadDir(uint32 count, uint64* cookie, Attribute* attrs,
	uint32 attr_count)
{
	(void)count;

	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpReadDir);
	fRequest->Stream().AddUHyper(cookie[0]);
	fRequest->Stream().AddUHyper(cookie[1]);

	// consider predicting this values basing on count or buffer size
	fRequest->Stream().AddUInt(0x2000);
	fRequest->Stream().AddUInt(0x8000);
	_AttrBitmap(fRequest->Stream(), attrs, attr_count);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::SetClientID(const RPC::Server* serv)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpSetClientID);
	uint64 verifier = rand();
	verifier = verifier << 32 | rand();
	fRequest->Stream().AddUHyper(verifier);

	char id[128] = "HAIKU:kernel:";
	int pos = strlen(id);
	*(uint32*)(id + pos) = serv->ID().fAddress;
	pos += sizeof(uint32);
	*(uint16*)(id + pos) = serv->ID().fPort;
	pos += sizeof(uint16);
	*(uint16*)(id + pos) = serv->ID().fProtocol;
	pos += sizeof(uint16);

	*(uint32*)(id + pos) = serv->LocalID().fAddress;
	pos += sizeof(uint32);
	
	fRequest->Stream().AddOpaque(id, pos);

	// Callbacks are currently not supported
	fRequest->Stream().AddUInt(0);
	fRequest->Stream().AddOpaque(NULL, 0);
	fRequest->Stream().AddOpaque(NULL, 0);
	fRequest->Stream().AddUInt(0);

	fOpCount++;

	return B_OK;
}


status_t
RequestBuilder::SetClientIDConfirm(uint64 id, uint64 ver)
{
	if (fProcedure != ProcCompound)
		return B_BAD_VALUE;
	if (fRequest == NULL)
		return B_NO_MEMORY;

	fRequest->Stream().AddUInt(OpSetClientIDConfirm);
	fRequest->Stream().AddUHyper(id);
	fRequest->Stream().AddUHyper(ver);

	fOpCount++;

	return B_OK;
}


RPC::Call*
RequestBuilder::Request()
{
	if (fProcedure == ProcCompound)
		fRequest->Stream().InsertUInt(fOpCountPosition, fOpCount);

	if (fRequest == NULL || fRequest->Stream().Error() == B_OK)
		return fRequest;
	else
		return NULL;
}


void
RequestBuilder::_AttrBitmap(XDR::WriteStream& stream, Attribute* attrs,
	uint32 count)
{
	// 2 is safe in NFS4, not in NFS4.1 though
	uint32 bitmap[2];
	memset(bitmap, 0, sizeof(bitmap));
	for (uint32 i = 0; i < count; i++) {
		bitmap[attrs[i] / 32] |= 1 << attrs[i] % 32;
	}

	uint32 bcount = bitmap[1] != 0 ? 2 : 1;
	stream.AddUInt(bcount);
	for (uint32 i = 0; i < bcount; i++)
		stream.AddUInt(bitmap[i]);
}

