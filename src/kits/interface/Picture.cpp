/*
 * Copyright 2001-2007, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

//! BPicture records a series of drawing instructions that can be "replayed" later.


#include <Picture.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define DEBUG 1
#include <ByteOrder.h>
#include <Debug.h>
#include <List.h>
#include <Message.h>

#include <AppServerLink.h>
#include <Autolock.h>
#include <ObjectList.h>
#include <PicturePlayer.h>
#include <ServerProtocol.h>

#include "PicturePrivate.h"


static BObjectList<BPicture> sPictureList;
static BLocker sPictureListLock;


void
reconnect_pictures_to_app_server()
{
	BAutolock _(sPictureListLock);
	for (int32 i = 0; i < sPictureList.CountItems(); i++) {
		BPicture::Private picture(sPictureList.ItemAt(i));
		picture.ReconnectToAppServer();
	}
}


BPicture::Private::Private(BPicture* picture)
	:
	fPicture(picture)
{
}


void
BPicture::Private::ReconnectToAppServer()
{
	fPicture->_Upload();
}


struct _BPictureExtent_ {
							_BPictureExtent_(const int32 &size = 0);
							~_BPictureExtent_();

			const void*		Data() const { return fNewData; }
			status_t 		ImportData(const void *data,
											const int32 &size);
												
			status_t		Flatten(BDataIO *stream);
			status_t 		Unflatten(BDataIO *stream);

			int32			Size() const { return fNewSize; }
			status_t		SetSize(const int32 &size);

			bool			AddPicture(BPicture *picture)
								{ return fPictures.AddItem(picture); }
			void			DeletePicture(const int32 &index)
								{ delete static_cast<BPicture *>
										(fPictures.RemoveItem(index)); }
			
			BList*			Pictures() { return &fPictures; }		
			BPicture*		PictureAt(const int32 &index)
								{ return static_cast<BPicture *>
										(fPictures.ItemAt(index)); }
	
			int32			CountPictures() const
								{ return fPictures.CountItems(); }
	

private:
			void*	fNewData;
			int32	fNewSize;

			BList	fPictures;
				// In R5 this is a BArray<BPicture*>
				// which is completely inline.
};


struct picture_header {
	int32 magic1; // version ?
	int32 magic2; // endianess ?
};


BPicture::BPicture()
	:
	fToken(-1),
	fExtent(NULL),
	fUsurped(NULL)
{
	_InitData();
}


BPicture::BPicture(const BPicture &otherPicture)
	:
	fToken(-1),
	fExtent(NULL),
	fUsurped(NULL)
{
	_InitData();

	if (otherPicture.fToken != -1) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_CLONE_PICTURE);
		link.Attach<int32>(otherPicture.fToken);

		status_t status = B_ERROR;
		if (link.FlushWithReply(status) == B_OK
			&& status == B_OK)
			link.Read<int32>(&fToken);
		if (status < B_OK)
			return;
	}

	if (otherPicture.fExtent->Size() > 0) {
		fExtent->ImportData(otherPicture.fExtent->Data(), otherPicture.fExtent->Size());

		for (int32 i = 0; i < otherPicture.fExtent->CountPictures(); i++) {
			BPicture *picture = new BPicture(*otherPicture.fExtent->PictureAt(i));
			fExtent->AddPicture(picture);
		}
	}
}


BPicture::BPicture(BMessage *archive)
	:
	fToken(-1),
	fExtent(NULL),
	fUsurped(NULL)
{
	_InitData();

	int32 version;
	if (archive->FindInt32("_ver", &version) != B_OK)
		version = 0;

	int8 endian;
	if (archive->FindInt8("_endian", &endian) != B_OK)
		endian = 0;

	const void *data;
	int32 size;
	if (archive->FindData("_data", B_RAW_TYPE, &data, (ssize_t*)&size) != B_OK)
		return;
	
	// Load sub pictures
	BMessage picMsg;
	int32 i = 0;
	while (archive->FindMessage("piclib", i++, &picMsg) == B_OK) {
		BPicture *pic = new BPicture(&picMsg);
		fExtent->AddPicture(pic);
	}

	if (version == 0) {
		// TODO: For now. We'll see if it's worth to support old style data
		debugger("old style BPicture data is not supported");
	} else if (version == 1) {
		fExtent->ImportData(data, size);

//		swap_data(fExtent->fNewData, fExtent->fNewSize);

		if (fExtent->Size() > 0)
			_AssertServerCopy();
	}

	// Do we just free the data now?
	if (fExtent->Size() > 0)
		fExtent->SetSize(0);

	// What with the sub pictures?
	for (i = fExtent->CountPictures() - 1; i >= 0; i--)
		fExtent->DeletePicture(i);
}


BPicture::BPicture(const void *data, int32 size)
{
	_InitData();
	// TODO: For now. We'll see if it's worth to support old style data
	debugger("old style BPicture data is not supported");
}


void
BPicture::_InitData()
{
	fToken = -1;
	fUsurped = NULL;

	fExtent = new (std::nothrow) _BPictureExtent_;

	BAutolock _(sPictureListLock);
	sPictureList.AddItem(this);
}


BPicture::~BPicture()
{
	BAutolock _(sPictureListLock);
	sPictureList.RemoveItem(this);
	_DisposeData();
}


void
BPicture::_DisposeData()
{
	if (fToken != -1) {
		BPrivate::AppServerLink link;

		link.StartMessage(AS_DELETE_PICTURE);
		link.Attach<int32>(fToken);
		link.Flush();
		SetToken(-1);
	}

	delete fExtent;
	fExtent = NULL;
}


BArchivable *
BPicture::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BPicture"))
		return new BPicture(archive);

	return NULL;
}


status_t
BPicture::Archive(BMessage *archive, bool deep) const
{
	if (!const_cast<BPicture*>(this)->_AssertLocalCopy())
		return B_ERROR;

	status_t err = BArchivable::Archive(archive, deep);
	if (err != B_OK)
		return err;

	err = archive->AddInt32("_ver", 1);
	if (err != B_OK)
		return err;

	err = archive->AddInt8("_endian", B_HOST_IS_BENDIAN);
	if (err != B_OK)
		return err;

	err = archive->AddData("_data", B_RAW_TYPE, fExtent->Data(), fExtent->Size());
	if (err != B_OK)
		return err;
	
	for (int32 i = 0; i < fExtent->CountPictures(); i++) {
		BMessage picMsg;

		err = fExtent->PictureAt(i)->Archive(&picMsg, deep);
		if (err != B_OK)
			break;
			
		err = archive->AddMessage("piclib", &picMsg);
		if (err != B_OK)
			break;
	}

	return err;
}


status_t
BPicture::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}


status_t
BPicture::Play(void **callBackTable, int32 tableEntries, void *user)
{
	if (!_AssertLocalCopy())
		return B_ERROR;

	BPrivate::PicturePlayer player(fExtent->Data(), fExtent->Size(), fExtent->Pictures());

	return player.Play(callBackTable, tableEntries, user);
}


status_t
BPicture::Flatten(BDataIO *stream)
{
	// TODO: what about endianess?

	if (!_AssertLocalCopy())
		return B_ERROR;

	const picture_header header = { 2, 0 };
	ssize_t bytesWritten = stream->Write(&header, sizeof(header));
	if (bytesWritten < B_OK)
		return bytesWritten;
	if (bytesWritten != (ssize_t)sizeof(header))
		return B_IO_ERROR;

	return fExtent->Flatten(stream);
}


status_t
BPicture::Unflatten(BDataIO *stream)
{
	// TODO: clear current picture data?

	picture_header header;
	ssize_t bytesRead = stream->Read(&header, sizeof(header));
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead != (ssize_t)sizeof(header)
		|| header.magic1 != 2 || header.magic2 != 0)
		return B_BAD_TYPE;
	
	status_t status = fExtent->Unflatten(stream);
	if (status < B_OK)
		return status;

//	swap_data(fExtent->fNewData, fExtent->fNewSize);

	if (!_AssertServerCopy())
		return B_ERROR;

	// Data is now kept server side, remove the local copy
	if (fExtent->Data() != NULL)
		fExtent->SetSize(0);

	return status;
}


void
BPicture::_ImportOldData(const void *data, int32 size)
{
	// TODO: We don't support old data for now
}


void
BPicture::SetToken(int32 token)
{
	fToken = token;
}


int32
BPicture::Token() const
{
	return fToken;
}


bool
BPicture::_AssertLocalCopy()
{
	if (fExtent->Data() != NULL)
		return true;

	if (fToken == -1)
		return false;

	return _Download() == B_OK;
}


bool
BPicture::_AssertOldLocalCopy()
{
	// TODO: We don't support old data for now

	return false;
}


bool
BPicture::_AssertServerCopy()
{
	if (fToken != -1)
		return true;

	if (fExtent->Data() == NULL)
		return false;

	for (int32 i = 0; i < fExtent->CountPictures(); i++) {
		if (!fExtent->PictureAt(i)->_AssertServerCopy())
			return false;
	}

	return _Upload() == B_OK;
}


status_t
BPicture::_Upload()
{
	if (fExtent == NULL || fExtent->Data() == NULL)
		return B_BAD_VALUE;

	BPrivate::AppServerLink link;

	link.StartMessage(AS_CREATE_PICTURE);
	link.Attach<int32>(fExtent->CountPictures());

	for (int32 i = 0; i < fExtent->CountPictures(); i++) {
		BPicture *picture = fExtent->PictureAt(i);
		if (picture)
			link.Attach<int32>(picture->fToken);
	}
	link.Attach<int32>(fExtent->Size());
	link.Attach(fExtent->Data(), fExtent->Size());

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK
		&& status == B_OK)
		link.Read<int32>(&fToken);

	return status;
}


status_t
BPicture::_Download()
{
	ASSERT(fExtent->Data() == NULL);
	ASSERT(fToken != -1);

	BPrivate::AppServerLink link;
	
	link.StartMessage(AS_DOWNLOAD_PICTURE);
	link.Attach<int32>(fToken);
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		int32 count = 0;
		link.Read<int32>(&count);
		
		// Read sub picture tokens
		for (int32 i = 0; i < count; i++) {
			BPicture *pic = new BPicture;
			link.Read<int32>(&pic->fToken);
			fExtent->AddPicture(pic);
		}
	
		int32 size;
		link.Read<int32>(&size);
		status = fExtent->SetSize(size);
		if (status == B_OK)
			link.Read(const_cast<void *>(fExtent->Data()), size);
	}

	return status;
}


const void *
BPicture::Data() const
{
	if (fExtent->Data() == NULL)
		const_cast<BPicture*>(this)->_AssertLocalCopy();

	return fExtent->Data();
}


int32
BPicture::DataSize() const
{
	if (fExtent->Data() == NULL)
		const_cast<BPicture*>(this)->_AssertLocalCopy();

	return fExtent->Size();
}


void
BPicture::Usurp(BPicture *lameDuck)
{
	_DisposeData();

	// Reinitializes the BPicture
	_InitData();

	// Do the Usurping
	fUsurped = lameDuck;
}


BPicture *
BPicture::StepDown()
{
	BPicture *lameDuck = fUsurped;
	fUsurped = NULL;

	return lameDuck;
}


void BPicture::_ReservedPicture1() {}
void BPicture::_ReservedPicture2() {}
void BPicture::_ReservedPicture3() {}


BPicture &
BPicture::operator=(const BPicture &)
{
	return *this;
}


// _BPictureExtent_
_BPictureExtent_::_BPictureExtent_(const int32 &size)
	:
	fNewData(NULL),
	fNewSize(0)
{
	SetSize(size);
}


_BPictureExtent_::~_BPictureExtent_()
{
	free(fNewData);
	for (int32 i = 0; i < fPictures.CountItems(); i++)
		delete static_cast<BPicture *>(fPictures.ItemAtFast(i));
}


status_t
_BPictureExtent_::ImportData(const void *data, const int32 &size)
{
	if (data == NULL)
		return B_BAD_VALUE;
	
	status_t status = B_OK;
	if (Size() != size)
		status = SetSize(size);
	
	if (status == B_OK)
		memcpy(fNewData, data, size);

	return status;
}


status_t
_BPictureExtent_::Unflatten(BDataIO *stream)
{
	if (stream == NULL)
		return B_BAD_VALUE;
	
	int32 count = 0;
	ssize_t bytesRead = stream->Read(&count, sizeof(count));
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead != (ssize_t)sizeof(count))
		return B_BAD_DATA;
		
	for (int32 i = 0; i < count; i++) {
		BPicture* picture = new BPicture;
		status_t status = picture->Unflatten(stream);
		if (status < B_OK) {
			delete picture;
			return status;
		}

		AddPicture(picture);
	}
	
	int32 size;
	bytesRead = stream->Read(&size, sizeof(size));
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead != (ssize_t)sizeof(size))
		return B_IO_ERROR;

	status_t status = B_OK;
	if (Size() != size)
		status = SetSize(size);
	
	if (status < B_OK)
		return status;
	
	bytesRead = stream->Read(fNewData, size);
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead != (ssize_t)size)
		return B_IO_ERROR;

	return B_OK;
}


status_t
_BPictureExtent_::Flatten(BDataIO *stream)
{
	int32 count = fPictures.CountItems();
	ssize_t bytesWritten = stream->Write(&count, sizeof(count));
	if (bytesWritten < B_OK)
		return bytesWritten;
	if (bytesWritten != (ssize_t)sizeof(count))
		return B_IO_ERROR;

	for (int32 i = 0; i < count; i++) {
		status_t status = PictureAt(i)->Flatten(stream);
		if (status < B_OK)
			return status;
	}

	bytesWritten = stream->Write(&fNewSize, sizeof(fNewSize));
	if (bytesWritten < B_OK)
		return bytesWritten;
	if (bytesWritten != (ssize_t)sizeof(fNewSize))
		return B_IO_ERROR;

	bytesWritten = stream->Write(fNewData, fNewSize);
	if (bytesWritten < B_OK)
		return bytesWritten;
	if (bytesWritten != fNewSize)
		return B_IO_ERROR;

	return B_OK;
}


status_t
_BPictureExtent_::SetSize(const int32 &size)
{
	if (size < 0)
		return B_BAD_VALUE;

	if (size == fNewSize)
		return B_OK;

	if (size == 0) {
		free(fNewData);
		fNewData = NULL;
	} else {
		void *data = realloc(fNewData, size);
		if (data == NULL)
			return B_NO_MEMORY;
		fNewData = data;
	}

	fNewSize = size;
	return B_OK;
}
