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
//	File Name:		MessageBody.cpp
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	BMessageBody handles data storage and retrieval for
//					BMessage.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <TypeConstants.h>

// Project Includes ------------------------------------------------------------
#include <MessageBody.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

//------------------------------------------------------------------------------
BMessageBody::BMessageBody()
	:	fFlattenedSize(0)
{
}
//------------------------------------------------------------------------------
BMessageBody::BMessageBody(const BMessageBody &rhs)
{
	*this = rhs;
}
//------------------------------------------------------------------------------
BMessageBody::~BMessageBody()
{
	MakeEmpty();
}
//------------------------------------------------------------------------------
BMessageBody& BMessageBody::operator=(const BMessageBody &rhs)
{
	if (this != &rhs)
	{
		MakeEmpty();
		for (TMsgDataMap::const_iterator i = rhs.fData.begin();
			 i != rhs.fData.end();
			 ++i)
		{
			BMessageField* BMF = i->second;
			fData[BMF->Name()] = BMF->Clone();
		}
		fFlattenedSize = rhs.fFlattenedSize;
	}

	return *this;
}
//------------------------------------------------------------------------------
status_t BMessageBody::GetInfo(type_code typeRequested, int32 which,
							   char** name, type_code* typeReturned,
							   int32* count) const
{
	TMsgDataMap::const_iterator i = fData.begin();
	int32 index = 0;
	for (index = 0; i != fData.end(); ++i)
	{
		if (B_ANY_TYPE == typeRequested && i->second->Type() == typeRequested)
		{
			++index;
			if (index == which)
			{
				break;
			}
		}
	}

	// We couldn't find any appropriate data
	if (i == fData.end())
	{
		// TODO: research
		// Which gets returned on an empty message with
		// typeRequested == B_ANY_TYPE?
		if (index)
		{
			// TODO: research
			// Is B_NAME_NOT_FOUND ever really returned from this function, as
			// the BeBook claims?
			return B_BAD_INDEX;
		}
		else
		{
			if (count) *count = 0;
			return B_BAD_TYPE;
		}
	}

	// TODO: BC Break
	// Change 'name' parameter to const char*
	*name = const_cast<char*>(i->first.c_str());
	*typeReturned = i->second->Type();
	if (count) *count = i->second->CountItems();

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessageBody::GetInfo(const char* name, type_code* type,
							   int32* c) const
{
	status_t err;
	BMessageField* BMF = FindData(name, B_ANY_TYPE, err);
	if (BMF)
	{
		*type = BMF->Type();
		if (c)
		{
			*c = BMF->CountItems();
		}
	}
	else
	{
		if (c)
		{
			*c = 0;
		}
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessageBody::GetInfo(const char* name, type_code* type,
							   bool* fixed_size) const
{
	status_t err;
	BMessageField* BMF = FindData(name, B_ANY_TYPE, err);
	if (BMF)
	{
		*type = BMF->Type();
		*fixed_size = BMF->FixedSize();
	}

	return err;
}
//------------------------------------------------------------------------------
int32 BMessageBody::CountNames(type_code type) const
{
	if (type == B_ANY_TYPE)
	{
		return fData.size();
	}

	int32 count = 0;
	for (TMsgDataMap::const_iterator i = fData.begin(); i != fData.end(); ++i)
	{
		if (type == B_ANY_TYPE || i->second->Type() == type)
		{
			++count;
		}
	}

	return count;
}
//------------------------------------------------------------------------------
bool BMessageBody::IsEmpty() const
{
	return fData.empty();
}
//------------------------------------------------------------------------------
void BMessageBody::PrintToStream() const
{
	// TODO: test
	for (TMsgDataMap::const_iterator i = fData.begin(); i != fData.end(); ++i)
	{
		i->second->PrintToStream(i->first.c_str());
		printf("\n");
	}
}
//------------------------------------------------------------------------------
status_t BMessageBody::Rename(const char* old_entry, const char* new_entry)
{
	TMsgDataMap::iterator i = fData.find(old_entry);
	if (i == fData.end())
	{
		return B_NAME_NOT_FOUND;
	}
	BMessageField* BMF = i->second;
	RemoveName(new_entry);
	fData[new_entry] = BMF;
	fData.erase(old_entry);

	return B_OK;
}
//------------------------------------------------------------------------------
ssize_t BMessageBody::FlattenedSize() const
{
	ssize_t size = 1;	// For MSG_LAST_ENTRY

	for (TMsgDataMap::const_iterator i = fData.begin(); i != fData.end(); ++i)
	{
		BMessageField* BMF = i->second;
		size += BMF->FlattenedSize();
	}

	return size;
}
//------------------------------------------------------------------------------
status_t BMessageBody::Flatten(BDataIO* stream) const
{
	status_t err = B_OK;

	for (TMsgDataMap::const_iterator i = fData.begin();
		 i != fData.end() && !err;
		 ++i)
	{
		BMessageField* BMF = i->second;
		err = BMF->Flatten(*stream);
	}

	if (!err)
	{
		err = stream->Write("", 1);	// For MSG_LAST_ENTRY
		if (err >= 0)
			err = B_OK;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessageBody::Unflatten(const char* flat_buffer)
{
	// TODO: implement
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessageBody::Unflatten(BDataIO* stream)
{
	// TODO: implement
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessageBody::AddData(const char* name, type_code type,
							   const void* data, ssize_t numBytes,
							   bool is_fixed_size, int32 /* count */)
{
/**
	@note	The same logic applies here are in AddFlat(): is_fixed_size and
			count aren't useful to us because dynamically adding items isn't
			an issue.  Nevertheless, we keep is_fixed_size to set flags
			appropriately when flattening the message.
 */

	// TODO: test
	// In particular, we want to see what happens if is_fixed_size == true and
	// the user attempts to add something bigger or smaller.  We may need to
	// enforce the size thing.
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessageBody::RemoveData(const char* name, int32 index)
{
	if (index < 0)
	{
		return B_BAD_VALUE;
	}

	status_t err = B_OK;
	BMessageField* BMF = FindData(name, B_ANY_TYPE, err);
	if (BMF)
	{
		if (index < BMF->CountItems())
		{
			BMF->RemoveItem(index);
			if (!BMF->CountItems())
			{
				RemoveName(name);
			}
		}
		else
		{
			err = B_BAD_INDEX;
		}
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessageBody::RemoveName(const char* name)
{
	TMsgDataMap::iterator i = fData.find(name);
	if (i == fData.end())
	{
		return B_NAME_NOT_FOUND;
	}

	delete i->second;
	fData.erase(i);

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessageBody::MakeEmpty()
{
	for (TMsgDataMap::iterator i = fData.begin();
		 i != fData.end();
		 ++i)
	{
		delete i->second;
	}

	fData.clear();
	return B_OK;
}
//------------------------------------------------------------------------------
#if 0
void* BMessageBody::operator new(size_t size)
{
	return ::new BMessageBody;
}
//------------------------------------------------------------------------------
void BMessageBody::operator delete(void* ptr, size_t size)
{
	::delete(ptr);
}
#endif
//------------------------------------------------------------------------------
bool BMessageBody::HasData(const char* name, type_code t, int32 n) const
{
	if (!name || n < 0)
	{
		return false;
	}

	status_t err;
	BMessageField* BMF = FindData(name, t, err);
	if (!BMF)
	{
		return false;
	}

	if (n >= BMF->CountItems())
	{
		return false;
	}

	return true;
}
//------------------------------------------------------------------------------
status_t BMessageBody::FindData(const char *name, type_code type, int32 index,
								const void **data, ssize_t *numBytes) const
{
	status_t err;
	*data = NULL;
	BMessageField* Field = FindData(name, type, err);
	if (!err)
	{
		*data = Field->DataAt(index, numBytes);
		// TODO: verify
		if (!*data)
		{
			err = B_BAD_INDEX;
		}
	}

	return err;
}
//------------------------------------------------------------------------------
BMessageField* BMessageBody::FindData(const char* name, type_code type,
									  status_t& err) const
{
	BMessageField* Item = NULL;
	if (!name)
	{
		err = B_BAD_VALUE;
		return Item;
	}

	err = B_OK;
	TMsgDataMap::const_iterator i = fData.find(name);
	if (i == fData.end())
	{
		err = B_NAME_NOT_FOUND;
	}
	else
	{
		Item = i->second;
		if (Item->Type() != type)
		{
			err = B_BAD_TYPE;
			Item = NULL;
		}
	}

	return Item;
}
//------------------------------------------------------------------------------

}	// namespace BPrivate

/*
 * $Log $
 *
 * $Id  $
 *
 */

