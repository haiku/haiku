//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Picture.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BPicture records a series of drawing instructions that can
//                  be "replayed" later.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// System Includes -------------------------------------------------------------
#include <Picture.h>
#include <Message.h>
#include <ByteOrder.h>
#include <TPicture.h>
#include <Errors.h>
#include <List.h>
#include <AppServerLink.h>
#include <PortMessage.h>
#include <ServerProtocol.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

struct _BPictureExtent_ {
	void	*fNewData;
	int32	fNewSize;
	void	*fOldData;
	int32	fOldSize;
	BList	fPictures;	// In R5 this is a BArray<BPicture*> which is completely
						// inline.
};

status_t do_playback(void * data, int32 size, BList& pictures,
	void **callBackTable, int32 tableEntries, void *user);

//------------------------------------------------------------------------------
BPicture::BPicture()
	:	token(-1),
		extent(NULL),
		usurped(NULL)
{
	extent = new _BPictureExtent_;
	extent->fNewData = NULL;
	extent->fNewSize = 0;
	extent->fOldData = NULL;
	extent->fOldSize = 0;
}
//------------------------------------------------------------------------------
BPicture::BPicture(const BPicture &picture)
	:	token(-1),
		extent(NULL),
		usurped(NULL)
{
	init_data();

	if (picture.token != -1)
	{

		BPrivate::BAppServerLink link;
		PortMessage msg;

		link.Attach<int32>(AS_CLONE_PICTURE);
		link.Attach<int32>(picture.token);
		link.FlushWithReply(&msg);
		msg.Read<int32>(&token);

	}
	if (picture.extent->fNewData != NULL)
	{
		extent->fNewSize = picture.extent->fNewSize;
		extent->fNewData = malloc(extent->fNewSize);
		memcpy(extent->fNewData, picture.extent->fNewData, extent->fNewSize);

		BPicture *pic;

		for (int32 i = 0; i < picture.extent->fPictures.CountItems(); i++)
		{
			pic = new BPicture(*(BPicture*)picture.extent->fPictures.ItemAt(i));
			extent->fPictures.AddItem(pic);
		}
	}
	else if (picture.extent->fOldData != NULL)
	{
		extent->fOldSize = picture.extent->fOldSize;
		extent->fOldData = malloc(extent->fOldSize);
		memcpy(extent->fOldData, picture.extent->fOldData, extent->fOldSize);

		// In old data the sub pictures are inside the data
	}
}
//------------------------------------------------------------------------------
BPicture::BPicture(BMessage *archive)
	:	token(-1),
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

	while (archive->FindMessage("piclib", i++, &picMsg) == B_OK)
	{
		BPicture *pic = new BPicture(&picMsg);
		extent->fPictures.AddItem(pic);
	}

	if (version == 0)
		import_old_data(data, size);
	else if (version == 1)
	{
		extent->fNewSize = size;
		extent->fNewData = malloc(extent->fNewSize);
		memcpy(extent->fNewData, data, extent->fNewSize);

//		swap_data(extent->fNewData, extent->fNewSize);

		if (extent->fNewSize != 0 && extent->fNewData != 0)
		{
			BPrivate::BAppServerLink link;
		PortMessage msg;
			BPicture *pic;
			
			link.Attach<int32>(AS_CREATE_PICTURE);
			link.Attach<int32>(extent->fPictures.CountItems());
			for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
			{
				pic=(BPicture*)extent->fPictures.ItemAt(i);
				if(pic)
					link.Attach<int32>(pic->token);
			}
			link.Attach<int32>(extent->fNewSize);
			link.Attach(extent->fNewData,extent->fNewSize);
			link.FlushWithReply(&msg);
			msg.Read<int32>(&token);
		}
	}

	// Do we just free the data now?
	if (extent->fNewData)
	{
		free(extent->fNewData);
		extent->fNewData = NULL;
		extent->fNewSize = 0;
	}

	if (extent->fOldData)
	{
		free(extent->fOldData);
		extent->fOldData = NULL;
		extent->fOldSize = 0;
	}

	// What with the sub pictures?
	for (i = 0; i < extent->fPictures.CountItems(); i++)
		delete (BPicture *)extent->fPictures.ItemAt(i);
	extent->fPictures.MakeEmpty();
}
//------------------------------------------------------------------------------
BPicture::~BPicture()
{
	if (token != -1)
	{
		BPrivate::BAppServerLink link;

		link.Attach<int32>(AS_DELETE_PICTURE);
		link.Attach<int32>(token);
		link.Flush();
	}

	if (extent->fNewData != NULL)
	{
		free(extent->fNewData);
		extent->fNewData = NULL;
		extent->fNewSize = 0;
	}

	if (extent->fOldData != NULL)
	{
		free(extent->fOldData);
		extent->fOldData = NULL;
		extent->fOldSize = 0;
	}

	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
		delete (BPicture *)extent->fPictures.ItemAt(i);
	extent->fPictures.MakeEmpty();

	free(extent);
}
//------------------------------------------------------------------------------
BArchivable *BPicture::Instantiate(BMessage *archive)
{
	if (validate_instantiation(archive, "BPicture"))
		return new BPicture(archive);
	else
		return NULL;
}
//------------------------------------------------------------------------------
status_t BPicture::Archive(BMessage *archive, bool deep) const
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
	
	err = archive->AddData("_data", B_RAW_TYPE, extent->fNewData,
		extent->fNewSize);

	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
	{
		BMessage picMsg;
		
		((BPicture*)extent->fPictures.ItemAt(i))->Archive(&picMsg, deep);
		
		archive->AddMessage("piclib", &picMsg);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BPicture::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}
//------------------------------------------------------------------------------
status_t BPicture::Play(void **callBackTable, int32 tableEntries, void *user)
{
	if (!assert_local_copy())
		return B_ERROR;

	return do_playback(extent->fNewData, extent->fNewSize, extent->fPictures,
		callBackTable, tableEntries, user);
}
//------------------------------------------------------------------------------
status_t BPicture::Flatten(BDataIO *stream)
{
	if (!assert_local_copy())
		return B_ERROR;

	// TODO check the header
	int32 bla1 = 2;
	int32 bla2 = 0;
	int32 count = 0;

	stream->Write(&bla1, 4);
	stream->Write(&bla2, 4);

	count = extent->fPictures.CountItems();
	stream->Write(&count, 4);

	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
		((BPicture*)extent->fPictures.ItemAt(i))->Flatten(stream);

	stream->Write(&extent->fNewSize, 4);
	stream->Write(extent->fNewData, extent->fNewSize);

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BPicture::Unflatten(BDataIO *stream)
{
	// TODO check the header
	int32 bla1 = 2;
	int32 bla2 = 0;
	int32 count = 0;

	stream->Read(&bla1, 4);
	stream->Read(&bla2, 4);

	stream->Read(&count, 4);

	for (int32 i = 0; i < count; i++)
	{
		BPicture *pic = new BPicture;

		pic->Unflatten(stream);

		extent->fPictures.AddItem(pic);
	}

	stream->Read(&extent->fNewSize, 4);
	extent->fNewData = malloc(extent->fNewSize);
	stream->Read(extent->fNewData, extent->fNewSize);

//	swap_data(extent->fNewData, extent->fNewSize);

	BPrivate::BAppServerLink link;
	PortMessage msg;
	BPicture *pic;

	link.Attach<int32>(AS_CREATE_PICTURE);
	link.Attach<int32>(extent->fPictures.CountItems());
	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
	{
		pic=(BPicture*)extent->fPictures.ItemAt(i);
		if(pic)
			link.Attach<int32>(pic->token);
	}
	link.Attach<int32>(extent->fNewSize);
	link.Attach(extent->fNewData, extent->fNewSize);
	link.FlushWithReply(&msg);
	msg.Read<int32>(&token);

	if (extent->fNewData)
	{
		free(extent->fNewData);
		extent->fNewData = NULL;
		extent->fNewSize = 0;
	}

	return B_OK;
}
//------------------------------------------------------------------------------
void BPicture::_ReservedPicture1() {}
void BPicture::_ReservedPicture2() {}
void BPicture::_ReservedPicture3() {}
//------------------------------------------------------------------------------
BPicture &BPicture::operator=(const BPicture &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BPicture::init_data()
{
	token = -1;
	usurped = NULL;

	extent = new _BPictureExtent_;
	extent->fNewData = NULL;
	extent->fNewSize = 0;
	extent->fOldData = NULL;
	extent->fOldSize = 0;
}
//------------------------------------------------------------------------------
void BPicture::import_data(const void *data, int32 size, BPicture **subs,
						   int32 subCount)
{
	if (data == NULL || size == 0)
		return;

	BPrivate::BAppServerLink link;
	PortMessage msg;
	
	link.Attach<int32>(AS_CREATE_PICTURE);
	link.Attach<int32>(subCount);

	for (int32 i = 0; i < subCount; i++)
		link.Attach<int32>(subs[i]->token);

	link.Attach<int32>(size);
	link.Attach(data, size);
	link.FlushWithReply(&msg);
	msg.Read<int32>(&token);
}
//------------------------------------------------------------------------------
void BPicture::import_old_data(const void *data, int32 size)
{
	// TODO: do we need to support old data, what is old data?
	/*if (data == NULL)
		return;

	if (size == 0)
		return;
		
	convert_old_to_new(data, size, &extent->fNewData, &extent->fNewSize);

	BPrivate::BAppServerLink link;
	link.Attach<int32>(AS_CREATE_PICTURE);
	link.Attach<int32>(0L);
	link.Attach<int32>(extent->fNewSize);
	link.Attach(extent->fNewData,extent->fNewSize);
	link.FlushWithReply(&msg);
	msg.Read<int32>(&token)
	
	// Do we free all data now?
	free(extent->fNewData);
	extent->fNewData = 0;
	extent->fNewSize = 0;
	free(extent->fOldData);
	extent->fOldData = 0;
	extent->fOldSize = 0;*/
}
//------------------------------------------------------------------------------
void BPicture::set_token(int32 _token)
{
	token = _token;
}
//------------------------------------------------------------------------------
bool BPicture::assert_local_copy()
{
	if (extent->fNewData != NULL)
		return true;

	if (token == -1)
		return false;

/*	BPrivate::BAppServerLink link;
	int32 count;

	link.Attach<int32>(AS_DOWNLOAD_PICTURE);
	link.Attach<int32>(token);
	link.FlushWithReply(&msg);
	count=*((int32*)replydata.buffer);

	// Read sub picture tokens
	for (int32 i = 0; i < count; i++)
	{
		BPicture *pic = new BPicture;

		link.sread(4, &pic->token);
		
		extent->fPictures.AddItem(pic);
	}

	link.sread(4, &extent->fNewSize);
	extent->fNewData = malloc(extent->fNewSize);
	link.sread(extent->fNewSize, &extent->fNewData);*/

	return true;
}
//------------------------------------------------------------------------------
bool BPicture::assert_old_local_copy()
{
	if (extent->fOldData != NULL)
		return true;

	if (!assert_local_copy())
		return false;

//	convert_new_to_old(extent->fNewData, extent->fNewSize, extent->fOldData,
//		extent->fOldSize);

	return true;
}
//------------------------------------------------------------------------------
bool BPicture::assert_server_copy()
{
	if (token != -1)
		return true;

	if (extent->fNewData == NULL)
		return false;

/*	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
		extent->fPictures.ItemAt(i)->assert_server_copy();

	BPrivate::BAppServerLink link;
	link.Attach<int32>(AS_CREATE_PICTURE);
	link.Attach<int32>(extent->fPictures.CountItems());
	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
		link.Attach<int32>(extent->fPictures.ItemAt(i)->token);
	link.Attach<int32>(extent->fNewSize);
	link.Attach(extent->fNewData,extent->fNewSize);
	link.FlushWithReply(&msg);
	msg.Read<int32>(&token);

	return token != -1;*/
	return true;
}
//------------------------------------------------------------------------------
BPicture::BPicture(const void *data, int32 size)
{
	init_data();
	import_old_data(data, size);
}
//------------------------------------------------------------------------------
const void *BPicture::Data() const
{
	if (extent->fNewData == NULL)
	{
		const_cast<BPicture*>(this)->assert_local_copy();
		//convert_new_to_old(void *, long, void **, long *);
	}

	return extent->fNewData;
}
//------------------------------------------------------------------------------
int32 BPicture::DataSize() const
{
	if (extent->fNewData == NULL)
	{
		const_cast<BPicture*>(this)->assert_local_copy();
		//convert_new_to_old(void *, long, void **, long *);
	}

	return extent->fNewSize;
}
//------------------------------------------------------------------------------
void BPicture::usurp(BPicture *lameDuck)
{
	if (token != -1)
	{
		BPrivate::BAppServerLink link;

		link.Attach<int32>(AS_DELETE_PICTURE);
		link.Attach<int32>(token);
		link.Flush();
	}

	if (extent->fNewData != NULL)
	{
		free(extent->fNewData);
		extent->fNewData = NULL;
		extent->fNewSize = 0;
	}

	if (extent->fOldData != NULL)
	{
		free(extent->fOldData);
		extent->fOldData = NULL;
		extent->fOldSize = 0;
	}

	for (int32 i = 0; i < extent->fPictures.CountItems(); i++)
		delete (BPicture *)extent->fPictures.ItemAt(i);
	extent->fPictures.MakeEmpty();

	free(extent);

	init_data();

	// Do the usurping
	usurped = lameDuck;
}
//------------------------------------------------------------------------------
BPicture *BPicture::step_down()
{
	BPicture *lameDuck = usurped;
	
	usurped = NULL;

	return lameDuck;
}
//------------------------------------------------------------------------------
status_t do_playback(void * data, int32 size, BList& pictures,
	void **callBackTable, int32 tableEntries, void *user)
{
	TPicture pic(data, size, pictures);

	return pic.Play(callBackTable, tableEntries, user);
}
//------------------------------------------------------------------------------


