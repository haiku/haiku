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
//	File Name:		MessageField.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	BMessageField contains and manages the data for indiviual
//					named field in BMessageBody
//------------------------------------------------------------------------------

#ifndef MESSAGEFIELD_H
#define MESSAGEFIELD_H

// Standard Includes -----------------------------------------------------------
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <string>

// System Includes -------------------------------------------------------------
#include <Entry.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------
#include <DataBuffer.h>

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
// flags for each entry (the bitfield is 1 byte)
#define MSG_FLAG_VALID			0x01
#define MSG_FLAG_MINI_DATA		0x02
#define MSG_FLAG_FIXED_SIZE		0x04
#define MSG_FLAG_SINGLE_ITEM	0x08
#define MSG_FLAG_ALL			0x0F

#define MSG_LAST_ENTRY			0x0

// Globals ---------------------------------------------------------------------
namespace BPrivate {

class BMessageField
{
	public:
		static const char*			sNullData;

		BMessageField(const std::string& name, type_code t)
			:	fName(name), fType(t)
		{;}
		virtual const std::string&	Name() const { return fName; }
		virtual type_code			Type() const { return fType; }
		virtual bool				FixedSize() const { return true; }
		virtual	ssize_t 			FlattenedSize() const { return 0; }
		virtual status_t			Flatten(BDataIO& stream) const = 0;
		virtual ssize_t				CountItems() const { return 0; }
		virtual void				RemoveItem(ssize_t index) = 0;
		virtual void				PrintToStream(const char* name) const;
		virtual const void*			DataAt(int32 index, ssize_t* size) const = 0;

	protected:
		virtual void				PrintDataItem(int32 index) const = 0;

	private:
		type_code	fType;
		std::string	fName;
};

//------------------------------------------------------------------------------
template<class T>
struct BMessageFieldStoragePolicy
{
	class Store
	{
		public:
		// Compiler-generated versions should be just fine
		#if 0
			Store();
			Store(const Store& rhs);
			~Store();
			Store& operator=(const Store& rhs);
		#endif
			inline size_t Size() const { return fData.size(); }
			inline T& operator[](uint index) { return fData[index]; }
			inline const T& operator[](uint index) const { return fData[index]; }
			inline void Add(const T& data) { fData.push_back(data); }
			inline void Remove(uint index) { fData.erase(fData.begin() + index); }

		private:
			std::vector<T>	fData;
	};
};
//------------------------------------------------------------------------------
template<class T>
struct BMessageFieldSizePolicy
{
	typedef typename BMessageFieldStoragePolicy<T>::Store Store;

	inline static size_t	Size(const T&) { return sizeof (T); }
	inline static size_t	Size(const Store& data)
	{
		return sizeof (T) * data.Size();
	}
	inline static bool		Fixed() { return true; }
};
//------------------------------------------------------------------------------
template<class T>
struct BMessageFieldPrintPolicy
{
	static void		PrintData(const T& data) {;}
};
//------------------------------------------------------------------------------
template<class T>
struct BMessageFieldFlattenPolicy
{
	typedef BMessageFieldSizePolicy<T> SizePolicy;

	inline static status_t	Flatten(BDataIO& stream, const T& data)
	{
		return stream.Write((const void*)&data, SizePolicy::Size(data));
	}
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
template
<
	class T1,
	class StoragePolicy = BMessageFieldStoragePolicy<T1>,
	class SizePolicy = BMessageFieldSizePolicy<T1>,
	class PrintPolicy = BMessageFieldPrintPolicy<T1>,
	class FlattenPolicy = BMessageFieldFlattenPolicy<T1>
>
class BMessageFieldImpl : public BMessageField
{
	public:
		typedef typename StoragePolicy::Store StorageType;

		BMessageFieldImpl(const std::string& name, type_code type)
			:	BMessageField(name, type),
				fMaxSize(0),
				fFlags(MSG_FLAG_ALL)
		{;}

		virtual bool		FixedSize() const
			{ return fFlags & MSG_FLAG_FIXED_SIZE; }
		virtual	ssize_t		FlattenedSize() const;
		virtual status_t	Flatten(BDataIO& stream) const;
		virtual ssize_t		CountItems() const { return fData.Size(); }
		virtual void		AddItem(const T1& data);
		virtual void		RemoveItem(ssize_t index)
			{ fData.Remove(index); };

		virtual const void*		DataAt(int32 index, ssize_t* size) const;
				StorageType&	Data() { return fData; }

	protected:
		virtual void		PrintDataItem(int32 index) const;

	private:
		StorageType	fData;
		size_t		fMaxSize;
		uchar		fFlags;
};
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
template
<
	class T1,
	class StoragePolicy,
	class SizePolicy,
	class PrintPolicy,
	class FlattenPolicy
>
ssize_t 
BMessageFieldImpl<T1, StoragePolicy, SizePolicy, PrintPolicy, FlattenPolicy>::
FlattenedSize() const
{
	ssize_t size = 0;

	// Mandatory stuff
	size += 1;								// field flags byte
	size += sizeof (type_code);				// field type bytes
	if (!(fFlags & MSG_FLAG_SINGLE_ITEM))	// item count byte
		++size;
	++size;									// data length byte
	++size;									// name length byte
	size += Name().length();				// name length

	size += SizePolicy::Size(fData);		// data length and item size bytes

	return size;
}
//------------------------------------------------------------------------------
template
<
	class T1,
	class StoragePolicy,
	class SizePolicy,
	class PrintPolicy,
	class FlattenPolicy
>
status_t 
BMessageFieldImpl<T1, StoragePolicy, SizePolicy, PrintPolicy, FlattenPolicy>::
Flatten(BDataIO& stream) const
{
	status_t	err = B_OK;
	type_code	type = Type();
	uint8		count = fData.Size();
	uint8		size = SizePolicy::Size(fData);
	uint8		nameLen = Name().length();
	
	err = stream.Write(&fFlags, 1);

	// Field type_code
	if (err >= 0)
		err = stream.Write(&type, sizeof (type_code));

	// Item count, if more than one
	if (err >= 0 && !(fFlags & MSG_FLAG_SINGLE_ITEM))
		err = stream.Write(&count, 1);

	// Data length
	if (err >= 0)
		err = stream.Write(&size, 1);

	// Name length
	if (err >= 0)
		err = stream.Write(&nameLen, 1);

	// Name
	if (err >= 0)
		err = stream.Write(Name().c_str(), Name().length());

	// Actual data items
	for (uint32 i = 0; i < fData.Size() && err >= 0; ++i)
	{
		err = FlattenPolicy::Flatten(stream, fData[i]);
	}

	if (err >= 0)
		err = B_OK;
	return err;
}
//------------------------------------------------------------------------------
template
<
	class T1,
	class StoragePolicy,
	class SizePolicy,
	class PrintPolicy,
	class FlattenPolicy
>
void 
BMessageFieldImpl<T1, StoragePolicy, SizePolicy, PrintPolicy, FlattenPolicy>::
AddItem(const T1& data)
{
	fData.Add(data);

	if (fMaxSize)
	{
		if ((fFlags & MSG_FLAG_FIXED_SIZE) &&
			(!SizePolicy::Fixed() ||
			fMaxSize != SizePolicy::Size(data)))
		{
			fMaxSize = max(SizePolicy::Size(data), fMaxSize);
			fFlags &= ~MSG_FLAG_FIXED_SIZE;
		}
	}
	else
	{
		if (!SizePolicy::Fixed())
		{
			fFlags &= ~MSG_FLAG_FIXED_SIZE;
		}
		fMaxSize = SizePolicy::Size(data);
	}

	if (fData.Size() > 256 || fMaxSize > 256)
		fFlags &= ~MSG_FLAG_MINI_DATA;

	if (fData.Size() > 1)
		fFlags &= ~MSG_FLAG_SINGLE_ITEM;
}
//------------------------------------------------------------------------------
template
<
	class T1,
	class StoragePolicy,
	class SizePolicy,
	class PrintPolicy,
	class FlattenPolicy
>
const void*
BMessageFieldImpl<T1, StoragePolicy, SizePolicy, PrintPolicy, FlattenPolicy>::
DataAt(int32 index, ssize_t* size) const
{
	if (index > CountItems())
		return NULL;

	*size = SizePolicy::Size(fData[index]);
	const T1& ref = fData[index];
	const T1* data = &ref;
	
	return (const void*)data;
}
//------------------------------------------------------------------------------
template
<
	class T1,
	class StoragePolicy,
	class SizePolicy,
	class PrintPolicy,
	class FlattenPolicy
>
void 
BMessageFieldImpl<T1, StoragePolicy, SizePolicy, PrintPolicy, FlattenPolicy>::
PrintDataItem(int32 index) const
{
	if (index && FixedSize())
	{
		std::printf("         ");
	}
	else
	{
		std::printf("size=%2ld, ", SizePolicy::Size(fData[index]));
	}
	std::printf("data[%ld]: ", index);
	PrintPolicy::PrintData(fData[index]);
}
//------------------------------------------------------------------------------


// Print policy specializations ------------------------------------------------
template<> struct BMessageFieldPrintPolicy<bool>
{
	static void	PrintData(const bool& b) { std::printf("%d", int(b)); }
};
template<> struct BMessageFieldPrintPolicy<int8>
{
	static void	PrintData(const int8& i)
		{ std::printf("0x%X (%d, '%c')", int(i), int(i), char(i)); }
};
template<> struct BMessageFieldPrintPolicy<int16>
{
	static void	PrintData(const int16& i)
		{ std::printf("0x%X (%d, '%c')", i, i, char(i)); }
};
template<> struct BMessageFieldPrintPolicy<int32>
{
	static void	PrintData(const int32& i)
		{ std::printf("0x%lX (%ld, '%c')", i, i, char(i)); }
};
template<> struct BMessageFieldPrintPolicy<int64>
{
	static void	PrintData(const int64& i)
		{ std::printf("0x%LX (%Ld, '%c')", i, i, char(i)); }
};
template<> struct BMessageFieldPrintPolicy<float>
{
	static void	PrintData(const float& f) { std::printf("%.4f", f); }
};
template<> struct BMessageFieldPrintPolicy<double>
{
	static void PrintData(const double& d) { std::printf("%.8lf", d); }
};
template<> struct BMessageFieldPrintPolicy<BString>
{
	static void PrintData(const BString& s) { std::printf("\"%s\"", s.String()); }
};
template<> struct BMessageFieldPrintPolicy<BPoint>
{
	static void PrintData(const BPoint& p) { std::printf("BPoint(x:%.1f, y:%.1f)", p.x, p.y); }
};
template<> struct BMessageFieldPrintPolicy<BRect>
{
	static void	PrintData(const BRect& r)
		{ std::printf("BRect(l:%.1f, t:%.1f, r:%.1f, b:%.1f)", r.left, r.top, r.right, r.bottom); }
};
template<> struct BMessageFieldPrintPolicy<entry_ref>
{
	static void	PrintData(const entry_ref& ref)
	{
		std::printf("device=%ld, directory=%Ld, name=\"%s\", path=\"%s\"",
			   ref.device, ref.directory, ref.name, BPath(&ref).Path());
	}
};
// Storage policy specializations ----------------------------------------------
template<>
struct BMessageFieldStoragePolicy<bool>
{
	class Store
	{
		private:
			struct Boolean
			{
				Boolean() : data(bool()) {;}
				Boolean(bool b) : data(b) {;}
				bool data;
			};

		public:
			inline size_t Size() const
				{ return fData.size(); }
			inline bool& operator[](uint index)
				{ return fData[index].data; }
			inline const bool& operator[](uint index) const
				{ return fData[index].data; }
			void Add(const bool& data)
			{
				fData.push_back(data);
			}
			void Remove(uint index)
			{
				fData.erase(fData.begin() + index);
			}

		private:
			std::vector<Boolean> fData;
	};
};
// Size policy specializations -------------------------------------------------
template<> struct BMessageFieldSizePolicy<BString>
{
	typedef BMessageFieldStoragePolicy<BString>::Store Store;

	inline static size_t	Size(const BString& s)	{ return s.Length() + 1; }
	inline static size_t	Size(const Store& data)
	{
		size_t size = 0;
		size_t temp;
		for (uint32 i = 0; i < data.Size(); ++i)
		{
			temp = Size(data[i]) + 4;	// Four extra bytes for size info
			// Calculate padding
			temp += 8 - (temp % 8);
			size += temp;
		}

		return size;
	}
	inline static bool		Fixed()	{ return false; }
};
//------------------------------------------------------------------------------
template<> struct BMessageFieldSizePolicy<BDataBuffer>
{
	typedef BMessageFieldStoragePolicy<BDataBuffer>::Store Store;

	inline static size_t	Size(const BDataBuffer& db)
								{ return db.BufferSize(); }
	inline static size_t	Size(const Store& data)
	{
		size_t size = 0;
		size_t temp;
		for (uint32 i = 0; i < data.Size(); ++i)
		{
			temp = Size(data[i]) + 4;	// Four extra bytes for size info
			// Calculate padding
			temp += 8 - (temp % 8);
			size += temp;
		}

		return size;
	}
	inline static bool		Fixed()	{ return false; }
};
//------------------------------------------------------------------------------
template<> struct BMessageFieldSizePolicy<BMessage*>
{
	typedef BMessageFieldStoragePolicy<BMessage*>::Store	Store;

	inline static size_t	Size(const BMessage* msg)
								{ return msg->FlattenedSize(); }
	inline static size_t	Size(const Store& data)
	{
		size_t size = 0;
		size_t temp;
		for (uint32 i = 0; i < data.Size(); ++i)
		{
			temp = Size(data[i]) + 4;	// Four extra bytes for size info
			// Calculate padding
			temp += 8 - (temp % 8);
			size += temp;
		}

		return size;
	}
	inline static bool		Fixed() { return false; }
};
// Flatten policy specializations ----------------------------------------------
template<>
struct BMessageFieldFlattenPolicy<BString>
{
	typedef BMessageFieldSizePolicy<BString> SizePolicy;

	static status_t	Flatten(BDataIO& stream, const BString& data)
	{
		size_t size = SizePolicy::Size(data);
		status_t err = stream.Write(&size, 4);
		
		if (err >= 0)
			err = stream.Write((const void*)data.String(), size);

		if (err >= 0)
		{
			// Pad data to 8-byte boundary
			size += 4;	// Account for the stored size of size
			size %= 8;
			if (size)
			{
				err = stream.Write(&BMessageField::sNullData[size], 8 - (size));
			}
		}

		if (err >= 0)
			err = B_OK;
		return err;
	}
};
//------------------------------------------------------------------------------
template<>
struct BMessageFieldFlattenPolicy<BMessage*>
{
	typedef BMessageFieldSizePolicy<BMessage*> SizePolicy;

	static status_t Flatten(BDataIO& stream, const BMessage* data)
	{
		size_t size = SizePolicy::Size(data);
		status_t err = stream.Write(&size, 4);

		if (err >= 0)
			err = data->Flatten(&stream);

		if (err >= 0)
		{
			// Pad data to 8-byte boundary
			size += 4;	// Account for the stored size of size
			size %= 8;
			if (size)
			{
				err = stream.Write(&BMessageField::sNullData[size], 8 - (size));
			}
		}

		if (err >= 0)
			err = B_OK;
		return err;
	}
};
//------------------------------------------------------------------------------

}	// namespace BPrivate

#endif	// MESSAGEFIELD_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

