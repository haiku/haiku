/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REQUESTBUILDER_H
#define REQUESTBUILDER_H


#include <SupportDefs.h>

#include "FileInfo.h"
#include "NFS4Defs.h"
#include "ReplyInterpreter.h"
#include "RPCCall.h"
#include "RPCServer.h"
#include "XDR.h"


class OpenState;
class LockInfo;
class LockOwner;

class RequestBuilder {
public:
									RequestBuilder(Procedure p = ProcCompound);
									~RequestBuilder();

	inline	void					Reset(Procedure proc = ProcCompound);

			status_t				Access();
			status_t				Close(uint32 seq, const uint32* id,
										uint32 stateSeq);
			status_t				Commit(uint64 offset, uint32 count);
			status_t				Create(FileType type, const char* name,
										AttrValue* attr, uint32 count,
										const char* path = NULL);
			status_t				DelegReturn(const uint32* id, uint32 seq);
			status_t				GetAttr(Attribute* attrs, uint32 count);
			status_t				GetFH();
			status_t				Link(const char* name);
			status_t				Lock(OpenState* state, LockInfo* lock,
										uint32 sequence, bool reclaim = false);
			status_t				LockT(LockType type, uint64 pos,
										uint64 len, OpenState* state);
			status_t				LockU(LockInfo* lock);
			status_t				LookUp(const char* name);
			status_t				LookUpUp();
			status_t				Nverify(AttrValue* attr, uint32 count);
			status_t				Open(OpenClaim claim, uint32 seq,
										uint32 access, uint64 id, OpenCreate oc,
										uint64 ownerId, const char* name,
										AttrValue* attr = NULL,
										uint32 count = 0, bool excl = false,
										OpenDelegation delegType
											= OPEN_DELEGATE_NONE);
			status_t				OpenAttrDir(bool create);
			status_t				OpenConfirm(uint32 seq, const uint32* id,
										uint32 stateSeq);
			status_t				PutFH(const FileHandle& fh);
			status_t				PutRootFH();
			status_t				Read(const uint32* id, uint32 stateSeq,
										uint64 pos, uint32 len);
			status_t				ReadDir(uint32 count, uint64 cookie,
										uint64 cookieVerf, Attribute* attrs,
										uint32 attrCount);
			status_t				ReadLink();
			status_t				Remove(const char* file);
			status_t				Rename(const char* from, const char* to);
			status_t				Renew(uint64 clientId);
			status_t				SaveFH();
			status_t				SetAttr(const uint32* id, uint32 stateSeq,
										AttrValue* attr, uint32 count);
			status_t				SetClientID(RPC::Server* server);
			status_t				SetClientIDConfirm(uint64 id, uint64 ver);
			status_t				Verify(AttrValue* attr, uint32 count);
			status_t				Write(const uint32* id, uint32 stateSeq,
										const void* buffer, uint64 pos,
										uint32 len);
			status_t				ReleaseLockOwner(OpenState* state,
										LockOwner* owner);

			RPC::Call*				Request();

private:
			void					_InitHeader();

			void					_GenerateLockOwner(XDR::WriteStream& stream,
										OpenState* state, LockOwner* owner);
			status_t				_GenerateClientId(XDR::WriteStream& stream,
										const RPC::Server* server);

			void					_EncodeAttrs(XDR::WriteStream& stream,
										AttrValue* attr, uint32 count);
			void					_AttrBitmap(XDR::WriteStream& stream,
										Attribute* attrs, uint32 count);

			uint32					fOpCount;
			XDR::Stream::Position	fOpCountPosition;

			Procedure				fProcedure;

			RPC::Call*				fRequest;
};


inline void
RequestBuilder::Reset(Procedure proc)
{
	fRequest->Stream().Clear();
	fOpCount = 0;
	fProcedure = proc;
	delete fRequest;

	_InitHeader();
}


#endif	// REQUESTBUILDER_H

