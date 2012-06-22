/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef REPLYINTERPRETER_H
#define REPLYINTERPRETER_H


#include <SupportDefs.h>

#include "NFS4Defs.h"
#include "RPCReply.h"


struct FSLocation {
	const char*			fRootPath;
	const char**		fLocations;
	uint32				fCount;

						~FSLocation();
};

struct FSLocations {
	const char*			fRootPath;
	FSLocation*			fLocations;
	uint32				fCount;

						~FSLocations();
};

struct AttrValue {
			AttrValue();
			~AttrValue();

	uint8	fAttribute;
	bool	fFreePointer;
	union {
			uint32			fValue32;
			uint64			fValue64;
			void*			fPointer;
			FSLocations*	fLocations;
	} fData;
};

struct DirEntry {
	const char*			fName;
	AttrValue*			fAttrs;
	uint32				fAttrCount;

						DirEntry();
						~DirEntry();
};

class ReplyInterpreter {
public:
						ReplyInterpreter(RPC::Reply* reply = NULL);
						~ReplyInterpreter();

	inline	status_t	SetTo(RPC::Reply* reply);
	inline	void		Reset();

	inline	uint32		NFS4Error();

			status_t	Access(uint32* supported, uint32* allowed);
			status_t	Close();
			status_t	GetAttr(AttrValue** attrs, uint32* count);
			status_t	GetFH(Filehandle* fh);
	inline	status_t	LookUp();
	inline	status_t	LookUpUp();
			status_t	Open(uint32* id, uint32* seq, bool* confirm);
			status_t	OpenConfirm(uint32* stateSeq);
	inline	status_t	PutFH();
	inline	status_t	PutRootFH();
			status_t	Read(void* buffer, uint32* size, bool* eof);
			status_t	ReadDir(uint64* cookie, uint64* cookieVerf,
							DirEntry** dirents, uint32* count, bool* eof);
			status_t	ReadLink(void* buffer, uint32* size, uint32 maxSize);
			status_t	Remove();
			status_t	Rename();
	inline	status_t	Renew();
	inline	status_t	SaveFH();
			status_t	SetClientID(uint64* clientid, uint64* verifier);
	inline	status_t	SetClientIDConfirm();
	inline	status_t	Verify();

private:
			void		_ParseHeader();

			status_t	_DecodeAttrs(XDR::ReadStream& stream, AttrValue** attrs,
							uint32* count);
			status_t	_OperationError(Opcode op);

	static	status_t	_NFS4ErrorToHaiku(uint32 x);

			uint32		fNFS4Error;
			RPC::Reply*	fReply;
};


inline status_t
ReplyInterpreter::SetTo(RPC::Reply* _reply)
{
	if (fReply != NULL)
		return B_DONT_DO_THAT;

	fReply = _reply;

	if (fReply != NULL)
		_ParseHeader();

	return B_OK;
}


inline void
ReplyInterpreter::Reset()
{
	delete fReply;
	fReply = NULL;
}


inline uint32
ReplyInterpreter::NFS4Error()
{
	return fNFS4Error;
}


inline status_t
ReplyInterpreter::LookUp()
{
	return _OperationError(OpLookUp);
}


inline status_t
ReplyInterpreter::LookUpUp()
{
	return _OperationError(OpLookUpUp);
}


inline status_t
ReplyInterpreter::PutFH()
{
	return _OperationError(OpPutFH);
}


inline status_t
ReplyInterpreter::PutRootFH()
{
	return _OperationError(OpPutRootFH);
}


inline status_t
ReplyInterpreter::Renew()
{
	return _OperationError(OpRenew);
}


inline status_t
ReplyInterpreter::SaveFH()
{
	return _OperationError(OpSaveFH);
}


inline status_t
ReplyInterpreter::SetClientIDConfirm()
{
	return _OperationError(OpSetClientIDConfirm);
}


inline status_t
ReplyInterpreter::Verify()
{
	return _OperationError(OpVerify);
}


#endif	// REPLYINTERPRETER_H

