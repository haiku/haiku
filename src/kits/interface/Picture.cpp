/*
 * Copyright 2001-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 */

//! BPicture records a series of drawing instructions that can be "replayed" later.


#include <ByteOrder.h>
#include <List.h>
#include <Message.h>
#include <Picture.h>

#include <AppServerLink.h>
#include <ServerProtocol.h>
#include <TPicture.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct _BPictureExtent_ {
	_BPictureExtent_(const int32 &size = 0);
	~_BPictureExtent_();

	const void *Data() const { return fNewData; }
	status_t ImportData(const void *data, const int32 &size);	
	status_t ImportData(BDataIO *stream);	

	int32 Size() const { return fNewSize; }
	status_t SetSize(const int32 &size);

	bool AddPicture(BPicture *picture) { return fPictures.AddItem(picture); }
	void DeletePicture(const int32 &index)
			{ delete static_cast<BPicture *>(fPictures.RemoveItem(index)); }
	BPicture *PictureAt(const int32 &index)
			{ return static_cast<BPicture *>(fPictures.ItemAt(index)); }
	
	int32 CountPictures() const { return fPictures.CountItems(); }
	BList &Pictures() { return fPictures; }
	
private:
	void	*fNewData;
	int32	fNewSize;
	void	*fOldStyleData;
	int32	fOldStyleSize;

	BList	fPictures;	// In R5 this is a BArray<BPicture*> which is completely inline.
};


struct picture_header {
	int32 magic1; // ?
	int32 magic2; // ?
};


BPicture::BPicture()
	:
	token(-1),
	extent(NULL),
	usurped(NULL)
{
	init_data();
}


BPicture::BPicture(const BPicture &otherPicture)
	:
	token(-1),
	extent(NULL),
	usurped(NULL)
{
	init_data();

	if (otherPicture.token != -1) {
		BPrivate::AppServerLink link;
		link.StartMessage(AS_CLONE_PICTURE);
		link.Attach<int32>(otherPicture.token);

		status_t status = B_ERROR;
		if (link.FlushWithReply(status) == B_OK
			&& status == B_OK)
			link.Read<int32>(&token);
		if (status < B_OK)
			return;
	}

	if (otherPicture.extent->Size() > 0) {
		extent->ImportData(otherPicture.extent->Data(), otherPicture.extent->Size());

		for (int32 i = 0; i < otherPicture.extent->CountPictures(); i++) {
			BPicture *picture = new BPicture(*otherPicture.extent->PictureAt(i));
			extent->AddPicture(picture);
		}
	} /*else if (otherPicture.extent->fOldStyleData != NULL) {
		extent->fOldStyleSize = otherPicture.extent->fOldStyleSize;
		extent->fOldStyleData = malloc(extent->fOldStyleSize);
		memcpy(extent->fOldStyleData, otherPicture.extent->fOldStyleData, extent->fOldStyleSize);

		// In old data the sub pictures are inside the data
	}*/
}


BPicture::BPicture(BMessage *archive)
	:
	token(-1),
	extent(NULL),
	usurped(NULL)
{
	init_data();

	int32 version, size;
	int8 endian;
	const void *data;

	if (archive->FindInt32("_ver", &version) != B_OK)
		version = 0;

	if (archive->FindInt8("_endian", &endian) != B_OK)
		endian = 0;

	if (archive->FindData("_data", B_RAW_TYPE, &data, (ssize_t*)&size) != B_OK)
		return;
	
	// Load sub pictures
	BMessage picMsg;
	int32 i = 0;

	while (archive->FindMessage("piclib", i++, &picMsg) == B_OK) {
		BPicture *pic = new BPicture(&picMsg);
		extent->AddPicture(pic);
	}

	if (version == 0)
		import_old_data(data, size);
	else if (version == 1) {
		extent->ImportData(data, size);

//		swap_data(extent->fNewData, extent->fNewSize);

		if (extent->Size() > 0) {
			BPrivate::AppServerLink link;

			link.StartMessage(AS_CREATE_PICTURE);
			link.Attach<int32>(extent->CountPictures());

			for (int32 i = 0; i < extent->CountPictures(); i++) {
				BPicture *picture = extent->PictureAt(i);
				if (picture != NULL)
					link.Attach<int32>(picture->token);
			}
			link.Attach<int32>(extent->Size());
			link.Attach(extent->Data(), extent->Size());

			status_t status = B_ERROR;
			if (link.FlushWithReply(status) == B_OK
				&& status == B_OK)
				link.Read<int32>(&token);
		}
	}

	// Do we just free the data now?
	if (extent->Size() > 0)
		extent->SetSize(0);

	/*if (extent->fOldStyleData) {
		free(extent->fOldStyleData);
		extent->fOldStyleData = NULL;
		extent->fOldStyleSize = 0;
	}*/

	// What with the sub pictures?
	for (i = extent->CountPictures() - 1; i >= 0; i--)
		extent->DeletePicture(i);
}


BPicture::~BPicture()
{
	if (token != -1) {
		BPrivate::AppServerLink link;

		link.StartMessage(AS_DELETE_PICTURE);
		link.Attach<int32>(token);
		link.Flush();
	}

	delete extent;
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
	if (!const_cast<BPicture*>(this)->assert_local_copy())
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

	err = archive->AddData("_data", B_RAW_TYPE, extent->Data(), extent->Size());
	if (err != B_OK)
		return err;
	
	for (int32 i = 0; i < extent->CountPictures(); i++) {
		BMessage picMsg;

		extent->PictureAt(i)->Archive(&picMsg, deep);
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
	if (!assert_local_copy())
		return B_ERROR;

	BList &pictures = extent->Pictures();
	return do_playback(const_cast<void *>(extent->Data()), extent->Size(), &pictures,
		callBackTable, tableEntries, user);
}


status_t
BPicture::Flatten(BDataIO *stream)
{
	if (!assert_local_copy())
		return B_ERROR;

	const picture_header header = { 2, 0 };
	status_t status = stream->Write(&header, sizeof(header));
	if (status < B_OK)
		return status;

	int32 count = extent->CountPictures();
	if (count > 0) {
		status = stream->Write(&count, sizeof(count));
		if (status < B_OK)
			return status;
	
		for (int32 i = 0; i < extent->CountPictures(); i++) {
			status = extent->PictureAt(i)->Flatten(stream);
			if (status < B_OK)
				return status;
		}
	}

	int32 size = extent->Size();
	status = stream->Write(&size, sizeof(size));
	if (status == B_OK)
		status = stream->Write(extent->Data(), extent->Size());

	return status;
}


status_t
BPicture::Unflatten(BDataIO *stream)
{
	picture_header header;
	if (stream->Read(&header, sizeof(header)) < sizeof(header)
		|| header.magic1 != 2 || header.magic2 != 0)
		return B_ERROR;

	int32 count = 0;
	status_t status = stream->Read(&count, sizeof(count));
	if (status < B_OK)
		return status;
	
	for (int32 i = 0; i < count; i++) {
		BPicture *pic = new BPicture;
		status = pic->Unflatten(stream);
		if (status < B_OK)
			return status;

		extent->AddPicture(pic);
	}

	status = extent->ImportData(stream);
	if (status < B_OK)
		return status;

//	swap_data(extent->fNewData, extent->fNewSize);

	BPrivate::AppServerLink link;

	link.StartMessage(AS_CREATE_PICTURE);
	link.Attach<int32>(extent->CountPictures());

	for (int32 i = 0; i < extent->CountPictures(); i++) {
		BPicture *picture = extent->PictureAt(i);
		if (picture)
			link.Attach<int32>(picture->token);
	}
	link.Attach<int32>(extent->Size());
	link.Attach(extent->Data(), extent->Size());

	status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK
		&& status == B_OK)
		link.Read<int32>(&token);

	if (extent->Data() != NULL)
		extent->SetSize(0);

	return status;
}


void BPicture::_ReservedPicture1() {}
void BPicture::_ReservedPicture2() {}
void BPicture::_ReservedPicture3() {}


BPicture &
BPicture::operator=(const BPicture &)
{
	return *this;
}


void
BPicture::init_data()
{
	token = -1;
	usurped = NULL;

	extent = new _BPictureExtent_;
}


void
BPicture::import_data(const void *data, int32 size, BPicture **subs,
	int32 subCount)
{
	if (data == NULL || size == 0)
		return;

	BPrivate::AppServerLink link;
	
	link.StartMessage(AS_CREATE_PICTURE);
	link.Attach<int32>(subCount);

	for (int32 i = 0; i < subCount; i++)
		link.Attach<int32>(subs[i]->token);

	link.Attach<int32>(size);
	link.Attach(data, size);

	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK
		&& status == B_OK)
		link.Read<int32>(&token);
}


void
BPicture::import_old_data(const void *data, int32 size)
{
	// TODO: do we need to support old data, what is old data?
	/*if (data == NULL)
		return;

	if (size == 0)
		return;
		
	convert_old_to_new(data, size, &extent->fNewData, &extent->fNewSize);

	BPrivate::BAppServerLink link;
	link.StartMessage(AS_CREATE_PICTURE);
	link.Attach<int32>(0L);
	link.Attach<int32>(extent->fNewSize);
	link.Attach(extent->fNewData,extent->fNewSize);
	link.FlushWithReply(&code);
	if (code == B_OK)
		link.Read<int32>(&token)

	// Do we free all data now?
	free(extent->fNewData);
	extent->fNewData = 0;
	extent->fNewSize = 0;
	free(extent->fOldStyleData);
	extent->fOldStyleData = 0;
	extent->fOldStyleSize = 0;*/
}


void
BPicture::set_token(int32 _token)
{
	token = _token;
}


bool
BPicture::assert_local_copy()
{
	if (extent->Data() != NULL)
		return true;

	if (token == -1)
		return false;

	BPrivate::AppServerLink link;
	
	link.StartMessage(AS_DOWNLOAD_PICTURE);
	link.Attach<int32>(token);
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK) {
		int32 count = 0;
		link.Read<int32>(&count);

		// Read sub picture tokens
		for (int32 i = 0; i < count; i++) {
			BPicture *pic = new BPicture;
			link.Read<int32>(&pic->token);
			extent->AddPicture(pic);
		}
	
		int32 size;
		link.Read<int32>(&size);
		status = extent->SetSize(size);
		if (status == B_OK)
			link.Read(const_cast<void *>(extent->Data()), extent->Size());
	}

	return status == B_OK;
}


bool
BPicture::assert_old_local_copy()
{
	//if (extent->fOldStyleData != NULL)
	//	return true;

	if (!assert_local_copy())
		return false;

//	convert_new_to_old(extent->fNewData, extent->fNewSize, extent->fOldStyleData,
//		extent->fOldStyleSize);

	return true;
}


bool
BPicture::assert_server_copy()
{
	if (token != -1)
		return true;

	if (extent->Data() == NULL)
		return false;

	for (int32 i = 0; i < extent->CountPictures(); i++)
		extent->PictureAt(i)->assert_server_copy();

	BPrivate::AppServerLink link;
	link.StartMessage(AS_CREATE_PICTURE);
	link.Attach<int32>(extent->CountPictures());
	for (int32 i = 0; i < extent->CountPictures(); i++)
		link.Attach<int32>(extent->PictureAt(i)->token);
	link.Attach<int32>(extent->Size());
	link.Attach(extent->Data(), extent->Size());
	
	status_t status = B_ERROR;
	if (link.FlushWithReply(status) == B_OK && status == B_OK)
		link.Read<int32>(&token);

	return token != -1;
}


BPicture::BPicture(const void *data, int32 size)
{
	init_data();
	import_old_data(data, size);
}


const void *
BPicture::Data() const
{
	if (extent->Data() == NULL) {
		const_cast<BPicture*>(this)->assert_local_copy();
		//convert_new_to_old(void *, long, void **, long *);
	}

	return extent->Data();
}


int32
BPicture::DataSize() const
{
	if (extent->Data() == NULL) {
		const_cast<BPicture*>(this)->assert_local_copy();
		//convert_new_to_old(void *, long, void **, long *);
	}

	return extent->Size();
}


void
BPicture::usurp(BPicture *lameDuck)
{
	if (token != -1) {
		BPrivate::AppServerLink link;

		link.StartMessage(AS_DELETE_PICTURE);
		link.Attach<int32>(token);
		link.Flush();
	}
/*
	if (extent->fOldStyleData != NULL) {
		free(extent->fOldStyleData);
		extent->fOldStyleData = NULL;
		extent->fOldStyleSize = 0;
	}
*/
	delete extent;

	// Reinitializes the BPicture
	init_data();

	// Do the usurping
	usurped = lameDuck;
}


BPicture *
BPicture::step_down()
{
	BPicture *lameDuck = usurped;
	usurped = NULL;

	return lameDuck;
}


status_t
do_playback(void * data, int32 size, BList* pictures,
	void **callBackTable, int32 tableEntries, void *user)
{
	PicturePlayer player(data, size, pictures);

	return player.Play(callBackTable, tableEntries, user);
}



_BPictureExtent_::_BPictureExtent_(const int32 &size)
	:
	fNewData(NULL),
	fNewSize(0),
	fOldStyleData(NULL),
	fOldStyleSize(0)
{
	SetSize(size);
}


_BPictureExtent_::~_BPictureExtent_()
{
	free(fNewData);
	free(fOldStyleData);
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
_BPictureExtent_::ImportData(BDataIO *stream)
{
	if (stream == NULL)
		return B_BAD_VALUE;
	
	status_t status = B_OK;
	int32 size;
	stream->Read(&size, sizeof(size));
	if (Size() != size)
		status = SetSize(size);
	
	if (status == B_OK)
		stream->Read(fNewData, size);

	return status;
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
