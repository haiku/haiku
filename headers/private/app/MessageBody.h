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
//	File Name:		MessageBody.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	BMessageBody handles data storage and retrieval for
//					BMessage.
//------------------------------------------------------------------------------

#ifndef MESSAGEBODY_H
#define MESSAGEBODY_H
#include <MessageField.h>
// Standard Includes -----------------------------------------------------------
#include <map>
#include <stdio.h>
#include <string>

// System Includes -------------------------------------------------------------
#include <Entry.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
//#include <StreamIO.h>
#include <String.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

enum { B_FLATTENABLE_TYPE = 'FLAT' };

namespace BPrivate {

class BMessageBody
{
public:
		BMessageBody();
		BMessageBody(const BMessageBody& rhs);
		~BMessageBody();

		BMessageBody&	operator=(const BMessageBody& rhs);

// Statistics and misc info
		status_t	GetInfo(type_code typeRequested, int32 which, char **name,
							type_code *typeReturned, int32 *count = NULL) const;

		status_t	GetInfo(const char *name, type_code *type, int32 *c = 0) const;
		status_t	GetInfo(const char *name, type_code *type, bool *fixed_size) const;

		int32		CountNames(type_code type) const;
		bool		IsEmpty() const;
		bool		IsSystem() const;
		bool		IsReply() const;
		void		PrintToStream() const;

		status_t	Rename(const char *old_entry, const char *new_entry);

// Flattening data
		ssize_t		FlattenedSize() const;
		status_t	Flatten(BDataIO *stream) const;
		status_t	Unflatten(const char *flat_buffer);
		status_t	Unflatten(BDataIO *stream);

// Removing data
		status_t	RemoveData(const char *name, int32 index = 0);
		status_t	RemoveName(const char *name);
		status_t	MakeEmpty();

		bool		HasData(const char* name, type_code t, int32 n) const;

		template<class T1>
		status_t	AddData(const char* name, const T1& data, type_code type);
		status_t	AddData(const char* name, type_code type, const void* data, ssize_t numBytes, bool is_fixed_size, int32);
		status_t	FindData(const char* name, type_code type, int32 index,
							 const void** data, ssize_t* numBytes) const;
		template<class T1>
		status_t	FindData(const char* name, int32 index, T1* data, type_code type);
		template<class T1>
		status_t	ReplaceData(const char* name, int32 index, const T1& data, type_code type);

#if 0
		void		*operator new(size_t size);
		void		operator delete(void *ptr, size_t size);
#endif

private:
		BMessageField*	FindData(const char *name, type_code type,
								 status_t& err) const;

		typedef std::map<std::string, BMessageField*>	TMsgDataMap;
		TMsgDataMap	fData;
		ssize_t		fFlattenedSize;
};
//------------------------------------------------------------------------------
template<class T1>
status_t BMessageBody::AddData(const char *name, const T1 &data, type_code type)
{
	// The flattened message format in R5 only allows 1 byte
	// for the length of field names
	if (!name || (strlen(name) > 255)) return B_BAD_VALUE;

	status_t err = B_OK;
	BMessageField* BMF = FindData(name, type, err);

	if (err)
	{
		if (err == B_NAME_NOT_FOUND)
		{
			// Reset err; we'll create the field
			err = B_OK;
		}
		else
		{
			// Looking for B_BAD_TYPE here in particular, which would indicate
			// that we tried to add data of type X when we already had data of
			// type Y with the same name
			return err;
		}
	}

	if (!BMF)
	{
		BMF = new(nothrow) BMessageFieldImpl<T1>(name, type);
		if (BMF)
		{
			fData[name] = BMF;
		}
		else
		{
			err = B_NO_MEMORY;
		}
	}

	if (!err)
	{
		BMessageFieldImpl<T1>* RItem =
			dynamic_cast<BMessageFieldImpl<T1>*>(BMF);
		if (!RItem)
		{
			debugger("\n\n\tyou \033[44;1;37mB\033[41;1;37me\033[m screwed\n\n");
		}
		RItem->AddItem(data);
	}

	return err;
}
//------------------------------------------------------------------------------
template<class T1>
status_t BMessageBody::FindData(const char *name, int32 index, T1 *data,
								type_code type)
{
	if (!name)
	{
		return B_BAD_VALUE;
	}
	if (index < 0)
	{
		return B_BAD_INDEX;
	}

	status_t err = B_OK;
	BMessageField* Item = FindData(name, type, err);
	if (Item)
	{
		BMessageFieldImpl<T1>* RItem =
			dynamic_cast<BMessageFieldImpl<T1>*>(Item);
		if (!RItem)
		{
			debugger("\n\n\tyou \033[44;1;37mB\033[41;1;37me"
					 "\033[m screwed\n\n");
		}
		if (index < RItem->CountItems())
		{
			*data = RItem->Data()[index];
		}
		else
		{
			err = B_BAD_INDEX;
		}
	}

	return err;
}
//------------------------------------------------------------------------------
template<class T1>
status_t BMessageBody::ReplaceData(const char *name, int32 index,
								   const T1 &data, type_code type)
{
	if (!name)
	{
		return B_BAD_VALUE;
	}
	if (index < 0)
	{
		return B_BAD_INDEX;
	}

	status_t err = B_OK;
	BMessageField* Item = FindData(name, type, err);
	if (Item)
	{
		BMessageFieldImpl<T1>* RItem =
			dynamic_cast<BMessageFieldImpl<T1>*>(Item);
		if (!RItem)
		{
			debugger("\n\n\tyou \033[44;1;37mB\033[41;1;37me\033[m screwed\n\n");
		}
		if (index < (int32)RItem->Data().Size())
		{
			RItem->Data()[index] = data;
		}
		else
		{
			err = B_BAD_INDEX;
		}
	}

	return err;
}
//------------------------------------------------------------------------------

}	// namespace BPrivate

#endif	//MESSAGEBODY_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

