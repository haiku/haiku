/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"

#include <dirent.h>
#include <string.h>

#include "ReplyInterpreter.h"
#include "RequestBuilder.h"


// Creating Inode object from Filehandle probably is not a good idea when
// filehandles are volatile.
Inode::Inode(Filesystem* fs, const Filehandle &fh, bool root)
	:
	fFilesystem(fs),
	fRoot(root)
{
	memcpy(&fHandle, &fh, sizeof(fh));

	RequestBuilder req(ProcCompound);
	req.PutFH(fh);

	Attribute attr[] = { FATTR4_TYPE, FATTR4_FILEID };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fs->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK || count < 1)
		return;

	if (count < 2 || values[1].fAttribute != FATTR4_FILEID) {
		// Server does not provide fileid. We need to make something up.
		fFileId = fs->GetId();
	} else
		fFileId = values[1].fData.fValue64;

	// FATTR4_TYPE is mandatory
	fType = values[0].fData.fValue32;

	delete[] values;
}


// filesystems that do not provide fileid are currently unsupported
// client will have to be able to create its own mapping between file names
// and made up IDs
status_t
Inode::LookUp(const char* name, ino_t* id)
{
	if (fType != NF4DIR)
		return B_NOT_A_DIRECTORY;

	if (!strcmp(name, ".")) {
		*id = ID();
		return B_OK;
	}

	RequestBuilder req(ProcCompound);
	req.PutFH(fHandle);

	if (!strcmp(name, ".."))
		req.LookUpUp();
	else
		req.LookUp(name);

	req.GetFH();

	Attribute attr[] = { FATTR4_FILEID };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fFilesystem->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result = reply.PutFH();
	if (result != B_OK)
		return result;

	if (!strcmp(name, ".."))
		result = reply.LookUpUp();
	else
		result = reply.LookUp();
	if (result != B_OK)
		return result;

	Filehandle fh;
	result = reply.GetFH(&fh);
	if (result != B_OK)
		return result;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	if (count < 1 || values[0].fAttribute != FATTR4_FILEID) {
		delete[] values;
		return B_UNSUPPORTED;
	}
	*id = _FileIdToInoT(values[0].fData.fValue64);
	fFilesystem->InoIdMap()->AddEntry(fh, values[0].fData.fValue64);

	delete[] values;
	return B_OK;
}


status_t
Inode::Stat(struct stat* st)
{
	RequestBuilder req(ProcCompound);
	req.PutFH(fHandle);

	Attribute attr[] = { FATTR4_SIZE, FATTR4_MODE, FATTR4_NUMLINKS };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fFilesystem->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return result;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	// FATTR4_SIZE is mandatory
	if (count < 1 || values[0].fAttribute != FATTR4_SIZE) {
		delete[] values;
		return B_BAD_VALUE;
	}
	st->st_size = values[0].fData.fValue64;

	uint32 next = 1;
	st->st_mode = Type();
	if (count >= next && values[next].fAttribute == FATTR4_MODE) {
		st->st_mode |= values[next].fData.fValue32;
		next++;
	}
	// TODO: else: get some info from ACCESS

	if (count >= next && values[next].fAttribute == FATTR4_NUMLINKS) {
		st->st_nlink = values[next].fData.fValue32;
		next++;
	} else
		st->st_nlink = 1;	// TODO: if !link_support we dont have to ask

	st->st_uid = 0;
	st->st_gid = 0;

	delete[] values;
	return B_OK;
}


status_t
Inode::OpenDir(uint64* cookie)
{
	if (fType != NF4DIR)
		return B_NOT_A_DIRECTORY;

	RequestBuilder req(ProcCompound);
	req.PutFH(fHandle);
	req.Access();

	RPC::Reply *rpl;
	fFilesystem->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return result;

	uint32 allowed;
	result = reply.Access(NULL, &allowed);
	if (result != B_OK)
		return result;

	if (allowed & ACCESS4_READ != ACCESS4_READ)
		return B_PERMISSION_DENIED;

	cookie[0] = 0;
	cookie[1] = 2;

	return B_OK;
}


status_t
Inode::_ReadDirOnce(DirEntry** dirents, uint32* count, uint64* cookie,
	bool* eof)
{
	RequestBuilder req(ProcCompound);
	req.PutFH(fHandle);

	Attribute attr[] = { FATTR4_FILEID };
	req.ReadDir(*count, cookie, attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fFilesystem->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return result;

	return reply.ReadDir(cookie, dirents, count, eof);
}


status_t
Inode::_FillDirEntry(struct dirent* de, ino_t id, const char* name, uint32 pos,
	uint32 size)
{
	uint32 nameSize = strlen(name);
	const uint32 entSize = sizeof(struct dirent);

	if (pos + entSize + nameSize > size)
		return B_BUFFER_OVERFLOW;

	de->d_dev = fFilesystem->DevId();
	de->d_ino = id;
	de->d_reclen = entSize + nameSize;
	if (de->d_reclen % 8 != 0)
		de->d_reclen += 8 - de->d_reclen % 8;

	strcpy(de->d_name, name);

	return B_OK;
}


status_t
Inode::_ReadDirUp(struct dirent* de, uint32 pos, uint32 size)
{
	RequestBuilder req(ProcCompound);
	req.PutFH(fHandle);
	req.LookUpUp();
	req.GetFH();
	Attribute attr[] = { FATTR4_FILEID };
	req.GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	RPC::Reply *rpl;
	fFilesystem->Server()->SendCall(req.Request(), &rpl);
	ReplyInterpreter reply(rpl);

	status_t result;
	result = reply.PutFH();
	if (result != B_OK)
		return result;

	result = reply.LookUpUp();
	if (result != B_OK)
		return result;

	Filehandle fh;
	result = reply.GetFH(&fh);
	if (result != B_OK)
		return result;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	uint64 fileId;
	if (count < 1 || values[0].fAttribute != FATTR4_FILEID) {
		// Server does not provide fileid. We need to make something up.
		fileId = fFilesystem->GetId();
	} else
		fileId = values[0].fData.fValue64;
	
	fFilesystem->InoIdMap()->AddEntry(fh, fileId);

	return _FillDirEntry(de, _FileIdToInoT(fileId), "..", pos, size);
}


status_t
Inode::ReadDir(void* _buffer, uint32 size, uint32* _count, uint64* cookie)
{
	uint32 count = 0;
	uint32 pos = 0;
	uint32 this_count;
	bool eof = false;

	char* buffer = reinterpret_cast<char*>(_buffer);

	if (cookie[0] == 0 && cookie[1] == 2 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

		_FillDirEntry(de, fFileId, ".", pos, size);

		pos += de->d_reclen;
		count++;
		cookie[1]--;
	}

	if (cookie[0] == 0 && cookie[1] == 1 && count < *_count) {
		struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);
		
		if (!fRoot)
			_ReadDirUp(de, pos, size);
		else
			_FillDirEntry(de, _FileIdToInoT(fFileId), "..", pos, size);

		pos += de->d_reclen;
		count++;
		cookie[1]--;
	}

	while (count < *_count && !eof) {
		this_count = *_count - count;
		DirEntry* dirents;

		status_t result = _ReadDirOnce(&dirents, &this_count, cookie, &eof);
		if (result != B_OK)
			return result;

		uint32 i;
		for (i = 0; i < min_c(this_count, *_count - count); i++) {
			struct dirent* de = reinterpret_cast<dirent*>(buffer + pos);

			ino_t id;
			if (dirents[i].fAttrCount == 1)
				id = _FileIdToInoT(dirents[i].fAttrs[0].fData.fValue64);
			else
				id = _FileIdToInoT(fFilesystem->GetId());

			const char* name = dirents[i].fName;
			if (_FillDirEntry(de, id, name, pos, size) == B_BUFFER_OVERFLOW) {
				eof = true;
				break;
			}

			pos += de->d_reclen;
		}
		delete[] dirents;
		count += i;
	}

	if (count == 0 && this_count > 0)
		return B_BUFFER_OVERFLOW;

	*_count = count;
	
	return B_OK;
}

