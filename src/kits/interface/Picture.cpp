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

// System Includes -------------------------------------------------------------
#include <Picture.h>
#include <Message.h>
#include <ByteOrder.h>
#include <TPicture.h>
#include <Errors.h>
#include <malloc.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

struct _BPictureExtent_ {
	void	*fNewData;
	int32	fNewSize;
	void	*fOldData;
	int32	fOldSize;	
};

//------------------------------------------------------------------------------
BPicture::BPicture()
	:	token(-1),
		extent(NULL),
		usurped(NULL)
{
}
//------------------------------------------------------------------------------
BPicture::BPicture(const BPicture &picture)
	:	token(-1),
		extent(NULL),
		usurped(NULL)
{
	init_data();

	if (picture.extent->fNewData == NULL)
	{
/*		_BAppServerLink_ link;
		int32 err;

		link.swrite_l(0xed2);
		link.swrite_l(picture->server_token);
		link.sync();
		link.sread(4, &server_token);*/
	}
	else
	{
		extent->fNewSize = picture.extent->fNewSize;
		extent->fNewData = malloc(extent->fNewSize);
		memcpy(extent->fNewData, picture.extent->fNewData, extent->fNewSize);
	}
}
//------------------------------------------------------------------------------
BPicture::BPicture(BMessage *archive)
	:	token(-1),
		extent(NULL),
		usurped(NULL)
{
	// TODO:
	init_data();
}
//------------------------------------------------------------------------------
BPicture::~BPicture()
{
	delete extent;
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
	if (!((BPicture*)this)->assert_local_copy())
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

	//do_playback(extent->fData, extent->fSize, ?BArray<BPicture*>&?,
	//	callBackTable, tableEntries, user);

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BPicture::Flatten(BDataIO *stream)
{
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BPicture::Unflatten(BDataIO *stream)
{
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

}
//------------------------------------------------------------------------------
void BPicture::import_old_data(const void *data, int32 size)
{
	// TODO: do we need to support old data, what is old data?
	/*convert_old_to_new(data, size, &extent->fNewData, &extent->fNewSize);

	_BAppServerLink_ link;
	int32 err;

	link.swrite_l(0xed1);
	link.swrite_l(0);
	link.swrite_l(extent->fNewSize);
	link.swrite_l(extent->fNewSize, extent->fNewData);
	link.sync();
	link.sread(4, &token);*/
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

/*	_BAppServerLink_ link;
	int32 err;

	link.swrite_l(0xed5);
	link.swrite_l(server_token);
	link.sync();
	link.sread(4, &err);

	// Read sub pictures
	while (err > 0)
	{
	}

	link.sread(4, &extent->fNewSize);
	extent->fNewData = malloc(extent->fNewSize);
	link.sread(extent->fNewSize, &extent->fNewData);*/

	return true;
}
//------------------------------------------------------------------------------
bool BPicture::assert_old_local_copy()
{
	// TODO: do we need to support old copies, what are old copies?
	return true;
}
//------------------------------------------------------------------------------
bool BPicture::assert_server_copy()
{
	if (token != -1)
		return true;

	if (extent->fNewData == NULL)
		return false;

/*	BPicture *ptr = usurped;
	for (BPicture *ptr = usurped; ptr != NULL; ptr++)
		ptr->assert_server_copy();

	_BAppServerLink_ link;
	int32 err;

	link.swrite_l(0xed1);
	for (sub picture)
		link.swrite_l(picture->token);
	link.swrite_l(extent->fNewSize);
	link.swrite(extent->fNewSize, extent->fNewData);
	link.sync();
	link.sread(4, &err);

	return err == B_OK;*/
	return true;
}
//------------------------------------------------------------------------------
BPicture::BPicture(const void *data, int32 size)
{
	// TODO:
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
/*	_BAppServerLink_ link;
	int32 err;

	link.swrite_l(0xed3);
	link.swrite_l(token);*/

	init_data();
}
//------------------------------------------------------------------------------
BPicture *BPicture::step_down()
{
	// TODO: called from EndPicture
	return this;
}
//------------------------------------------------------------------------------
status_t do_playback(void * data, int32 size, /*BArray<BPicture*>& pictures,*/
	void **callBackTable, int32 tableEntries, void *user)
{
	TPicture pic(data, size/*, pictures*/);

	return pic.Play(callBackTable, tableEntries, user);
}
//------------------------------------------------------------------------------


