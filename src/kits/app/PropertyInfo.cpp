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
//	File Name:		PropertyInfo.h
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	Utility class for maintain scripting information.
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <PropertyInfo.h>
#include <Message.h>
#include <Errors.h>
#include <string.h>
#include <stdio.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
BPropertyInfo::BPropertyInfo(property_info *p, value_info *ci,
							 bool free_on_delete)
	:	fPropInfo(p),
		fValueInfo(ci),
		fPropCount(0),
		fInHeap(free_on_delete),
		fOldInHeap(false),
		fValueCount(0),
		fOldPropInfo(NULL)
{
	if (fPropInfo)
	{
		while (fPropInfo[fPropCount].name)
			fPropCount++;
	}

	if (fValueInfo)
	{
		while (fValueInfo[fValueCount].name)
			fValueCount++;
	}
}
//------------------------------------------------------------------------------
BPropertyInfo::~BPropertyInfo()
{
	if (fInHeap)
	{
		if (fPropInfo)
			delete fPropInfo;

		if (fValueInfo)
			delete fValueInfo;
	}
}
//------------------------------------------------------------------------------
int32 BPropertyInfo::FindMatch(BMessage *msg, int32 index, BMessage *spec,
							   int32 form, const char *prop, void *data) const
{
	int32 property_index = 0;

	while (fPropInfo[property_index].name)
	{
		property_info *propInfo = fPropInfo + property_index;

		if (strcmp(propInfo->name, prop) == 0)
		{
			int32 specifier_index = 0;

			for (int32 i = 0; i < 10 && propInfo->specifiers[i] != 0; i++)
			{
				if (propInfo->specifiers[i] == form)
				{
					for (int32 j = 0; j < 10 && propInfo->commands[j] != 0; j++)
					{
						if (propInfo->commands[j] == msg->what)
						{
							if (data)
								*((uint32*)data) = propInfo->extra_data;
							return property_index;
						}
					}
				}
			}
		}

		property_index++;
	}

	return B_ERROR;
}
//------------------------------------------------------------------------------
bool BPropertyInfo::IsFixedSize() const
{
	return false;
}
//------------------------------------------------------------------------------
type_code BPropertyInfo::TypeCode() const
{
	return B_PROPERTY_INFO_TYPE;
}
//------------------------------------------------------------------------------
ssize_t BPropertyInfo::FlattenedSize() const
{
	size_t size = 2 * sizeof(int32);

	if (fPropInfo)
	{
		// Main chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++)
		{
			size += strlen(fPropInfo[pi].name) + 1;

			if (fPropInfo[pi].usage)
				size += strlen(fPropInfo[pi].usage) + 1;
			else
				size += sizeof(char);

			size += sizeof(int32);

			for (int32 i = 0; i < 10 && fPropInfo[pi].commands[i] != 0; i++)
				size += sizeof(int32);
			size += sizeof(int32);

			for (int32 i = 0; i < 10 && fPropInfo[pi].specifiers[i] != 0; i++)
				size += sizeof(int32);
			size += sizeof(int32);
		}

		// Type chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++)
		{
			for (int32 i = 0; i < 10 && fPropInfo[pi].types[i] != 0; i++)
				size += sizeof(int32);
			size += sizeof(int32);

			for (int32 i = 0; i < 3 && fPropInfo[pi].ctypes[i].pairs[0].name != 0; i++)
			{
				for (int32 j = 0; j < 5 && fPropInfo[pi].ctypes[i].pairs[j].name != 0; j++)
				{
					size += strlen(fPropInfo[pi].ctypes[i].pairs[j].name) + 1;
					size += sizeof(int32);
				}
				size += sizeof(int32);
			}
			size += sizeof(int32);
		}
	}

	if (fValueInfo)
	{
		size_t size = sizeof(int32);

		// Chunks
		for (int32 vi = 0; fValueInfo[vi].name != NULL; vi++)
		{
			size += sizeof(int16);
			size += sizeof(int32);

			size += strlen(fValueInfo[vi].name) + 1;

			if (fValueInfo[vi].usage)
				size += strlen(fValueInfo[vi].usage) + 1;
			else
				size += sizeof(char);

			size += sizeof(int32);
		}
	}

	return size;
}
//------------------------------------------------------------------------------
status_t BPropertyInfo::Flatten(void *buffer, ssize_t numBytes) const
{
	if (numBytes < FlattenedSize())
		return B_NO_MEMORY;

	if (buffer == NULL)
		return B_BAD_VALUE;

	union
	{
		char	*charPtr;
		uint32	*intPtr;
		uint16	*wrdPtr;
	};

	charPtr = (char*)buffer;

	*(intPtr++) = CountProperties();
	*(intPtr++) = (fPropInfo ? 0x1 : 0x0) | (fValueInfo ? 0x2 : 0x0);

	if (fPropInfo)
	{
		// Main chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++)
		{
			strcpy(charPtr, fPropInfo[pi].name);
			charPtr += strlen(fPropInfo[pi].name) + 1;

			if (fPropInfo[pi].usage)
			{
				strcpy(charPtr, fPropInfo[pi].usage);
				charPtr += strlen(fPropInfo[pi].usage) + 1;
			}
			else
				*(charPtr++) = 0;

			*(intPtr++) = fPropInfo[pi].extra_data;

			for (int32 i = 0; i < 10 && fPropInfo[pi].commands[i] != 0; i++)
				*(intPtr++) = fPropInfo[pi].commands[i];
			*(intPtr++) = 0;

			for (int32 i = 0; i < 10 && fPropInfo[pi].specifiers[i] != 0; i++)
				*(intPtr++) = fPropInfo[pi].specifiers[i];
			*(intPtr++) = 0;
		}

		// Type chunks
		for (int32 pi = 0; fPropInfo[pi].name != NULL; pi++)
		{
			for (int32 i = 0; i < 10 && fPropInfo[pi].types[i] != 0; i++)
				*(intPtr++) = fPropInfo[pi].types[i];
			*(intPtr++) = 0;

			for (int32 i = 0; i < 3 && fPropInfo[pi].ctypes[i].pairs[0].name != 0; i++)
			{
				for (int32 j = 0; j < 5 && fPropInfo[pi].ctypes[i].pairs[j].name != 0; j++)
				{
					strcpy(charPtr, fPropInfo[pi].ctypes[i].pairs[j].name);
					charPtr += strlen(fPropInfo[pi].ctypes[i].pairs[j].name) + 1;
					*(intPtr++) = fPropInfo[pi].ctypes[i].pairs[j].type;
				}
				*(intPtr++) = 0;
			}
			*(intPtr++) = 0;
		}
	}

	if (fValueInfo)
	{
		// Chunks
		for (int32 vi = 0; fValueInfo[vi].name != NULL; vi++)
		{
			*(wrdPtr++) = fValueInfo[vi].kind;
			*(intPtr++) = fValueInfo[vi].value;

			strcpy(charPtr, fValueInfo[vi].name);
			charPtr += strlen(fValueInfo[vi].name) + 1;

			if (fValueInfo[vi].usage)
			{
				strcpy(charPtr, fValueInfo[vi].usage);
				charPtr += strlen(fValueInfo[vi].usage) + 1;
			}
			else
				*(charPtr++) = 0;

			*(intPtr++) = fValueInfo[vi].extra_data;
		}
	}

	return B_OK;
}
//------------------------------------------------------------------------------
bool BPropertyInfo::AllowsTypeCode(type_code code) const
{
	return (code == B_PROPERTY_INFO_TYPE);
}
//------------------------------------------------------------------------------
status_t BPropertyInfo::Unflatten(type_code code, const void *buffer,
								  ssize_t numBytes)
{
	FreeMem();

	if (!AllowsTypeCode(code))
		return B_BAD_TYPE;

	if (buffer == NULL)
		return B_BAD_VALUE;

	union
	{
		char	*charPtr;
		uint32	*intPtr;
		uint16	*wrdPtr;
	};

	charPtr = (char*)buffer;

	fPropCount = *(intPtr++);
	int32 flags = *(intPtr++);

	if (flags & 1)
	{
		fPropInfo = new property_info[fPropCount + 1];
		memset(fPropInfo, 0, (fPropCount + 1) * sizeof(property_info));

		// Main chunks
		for (int32 pi = 0; pi < fPropCount; pi++)
		{
			fPropInfo[pi].name = strdup(charPtr);
			charPtr += strlen(fPropInfo[pi].name) + 1;

			if (*charPtr)
			{
				fPropInfo[pi].usage = strdup(charPtr);
				charPtr += strlen(fPropInfo[pi].usage) + 1;
			}
			else
				charPtr++;

			fPropInfo[pi].extra_data = *(intPtr++);

			for (int32 i = 0; i < 10 && *intPtr; i++)
				fPropInfo[pi].commands[i] = *(intPtr++);
			intPtr++;

			for (int32 i = 0; i < 10 && *intPtr; i++)
				fPropInfo[pi].specifiers[i] = *(intPtr++);
			intPtr++;
		}

		// Type chunks
		for (int32 pi = 0; pi < fPropCount; pi++)
		{
			for (int32 i = 0; i < 10 && *intPtr; i++)
				fPropInfo[pi].types[i] = *(intPtr++);
			intPtr++;

			for (int32 i = 0; i < 3 && *intPtr; i++)
			{
				for (int32 j = 0; j < 5 && *intPtr; j++)
				{
					fPropInfo[pi].ctypes[i].pairs[j].name = strdup(charPtr);
					charPtr += strlen(fPropInfo[pi].ctypes[i].pairs[j].name) + 1;
					fPropInfo[pi].ctypes[i].pairs[j].type = *(intPtr++);
				}
				intPtr++;
			}
			intPtr++;
		}
	}

	if (flags & 2)
	{
		fValueCount = (int16)*(intPtr++);

		fValueInfo = new value_info[fValueCount + 1];
		memset(fValueInfo, 0, (fValueCount + 1) * sizeof(value_info));

		for (int32 vi = 0; vi < fValueCount; vi++)
		{
			fValueInfo[vi].kind = (value_kind)*(wrdPtr++);
			fValueInfo[vi].value = *(intPtr++);

			fValueInfo[vi].name = strdup(charPtr);
			charPtr += strlen(fValueInfo[vi].name) + 1;

			if (*charPtr)
			{
				fValueInfo[vi].usage = strdup(charPtr);
				charPtr += strlen(fValueInfo[vi].usage) + 1;
			}
			else
				charPtr++;

			fValueInfo[vi].extra_data = *(intPtr++);
		}
	}

	return B_OK;
}
//------------------------------------------------------------------------------
const property_info *BPropertyInfo::Properties() const
{
	return fPropInfo;
}
//------------------------------------------------------------------------------
const value_info *BPropertyInfo::Values() const
{
	return fValueInfo;
}
//------------------------------------------------------------------------------
int32 BPropertyInfo::CountProperties() const
{
	return fPropCount;
}
//------------------------------------------------------------------------------
int32 BPropertyInfo::CountValues() const
{
	return fValueCount;
}
//------------------------------------------------------------------------------
void BPropertyInfo::PrintToStream() const
{
	printf("      property   commands                       types                specifiers\n");
	printf("--------------------------------------------------------------------------------\n");

	for (int32 pi = 0; fPropInfo[pi].name != 0; pi++)
	{
		// property
		printf("%14s", fPropInfo[pi].name);
		// commands
		for (int32 i = 0; i < 10 && fPropInfo[pi].commands[i] != 0; i++)
		{
			uint32 command = fPropInfo[pi].commands[i];

			printf("   %c%c%c%-28c", (command & 0xFF000000) >> 24,
				(command & 0xFF0000) >> 16, (command & 0xFF00) >> 8,
				command & 0xFF);
		}
		// types
		for (int32 i = 0; i < 10 && fPropInfo[pi].types[i] != 0; i++)
		{
			uint32 type = fPropInfo[pi].types[i];

			printf("%c%c%c%c", (type & 0xFF000000) >> 24,
				(type & 0xFF0000) >> 16, (type & 0xFF00) >> 8, type & 0xFF);
		}
		// specifiers
		for (int32 i = 0; i < 10 && fPropInfo[pi].specifiers[i] != 0; i++)
		{
			uint32 spec = fPropInfo[pi].specifiers[i];

			printf("%d", spec);
		}
		printf("\n");
	}
}
//------------------------------------------------------------------------------
bool BPropertyInfo::FindCommand(uint32 what, int32, property_info *p)
{
	//TODO: What's the second int32?
	for (int32 i = 0; i < 10 && p->commands[i] != 0; i++)
	{
		if (p->commands[i] == what)
			return true;
	}

	return false;
}
//------------------------------------------------------------------------------
bool BPropertyInfo::FindSpecifier(uint32 form, property_info *p)
{
	for (int32 i = 0; i < 10 && p->specifiers[i] != 0; i++)
	{
		if (p->specifiers[i] == form)
			return true;
	}

	return false;
}
//------------------------------------------------------------------------------
void BPropertyInfo::_ReservedPropertyInfo1() {}
void BPropertyInfo::_ReservedPropertyInfo2() {}
void BPropertyInfo::_ReservedPropertyInfo3() {}
void BPropertyInfo::_ReservedPropertyInfo4() {}
//------------------------------------------------------------------------------
BPropertyInfo::BPropertyInfo(const BPropertyInfo &)
{
}
//------------------------------------------------------------------------------
BPropertyInfo &BPropertyInfo::operator=(const BPropertyInfo &)
{
	return *this;
}
//------------------------------------------------------------------------------
void BPropertyInfo::FreeMem()
{
	//TODO: Does this free all members from the class?
	if (fInHeap && fPropInfo)
	{
		delete fPropInfo;
		fPropCount = 0;
	}

	if (fInHeap && fValueInfo)
	{
		delete fValueInfo;
		fValueCount = 0;
	}

	fInHeap = false;
}
//------------------------------------------------------------------------------
void BPropertyInfo::FreeInfoArray(property_info *p, int32)
{
	//TODO: Do we just have to delete the given ptr? What int32?
	delete p;
}
//------------------------------------------------------------------------------
void BPropertyInfo::FreeInfoArray(value_info *p, int32)
{
	//TODO: Do we just have to delete the given ptr? What int32?
	delete p;
}
//------------------------------------------------------------------------------
void BPropertyInfo::FreeInfoArray(_oproperty_info_ *p, int32)
{
}
//------------------------------------------------------------------------------
property_info *BPropertyInfo::ConvertToNew(const _oproperty_info_ *p)
{
	return NULL;
}
//------------------------------------------------------------------------------
_oproperty_info_ *BPropertyInfo::ConvertFromNew(const property_info *p)
{
	return NULL;
}
//------------------------------------------------------------------------------
const property_info *BPropertyInfo::PropertyInfo() const
{
	return fPropInfo;
}
//------------------------------------------------------------------------------
BPropertyInfo::BPropertyInfo(property_info *p, bool free_on_delete)
	:	fPropInfo(p),
		fValueInfo(NULL),
		fPropCount(0),
		fInHeap(free_on_delete),
		fOldInHeap(false),
		fValueCount(0),
		fOldPropInfo(NULL)
{
	if (fPropInfo)
	{
		while (fPropInfo[fPropCount].name)
			fPropCount++;
	}
}
//------------------------------------------------------------------------------
bool BPropertyInfo::MatchCommand(uint32 what, int32, property_info *p)
{
	//TODO: What's the second int32?
	for (int32 i = 0; i < 10 && p->commands[i] != 0; i++)
	{
		if (p->commands[i] == what)
			return true;
	}

	return false;
}
//------------------------------------------------------------------------------
bool BPropertyInfo::MatchSpecifier(uint32 form, property_info *p)
{
	for (int32 i = 0; i < 10 && p->specifiers[i] != 0; i++)
	{
		if (p->specifiers[i] == form)
			return true;
	}

	return false;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */
