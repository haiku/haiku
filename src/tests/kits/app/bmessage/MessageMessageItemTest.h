//------------------------------------------------------------------------------
//	MessageMessageItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEMESSAGEITEMTEST_H
#define MESSAGEMESSAGEITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Debug.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
struct TMessageFuncPolicy
{
	static status_t Add(BMessage& msg, const char* name, BMessage& val);
	static status_t AddData(BMessage& msg, const char* name, type_code type,
							BMessage* data, ssize_t size, bool);
	static status_t Find(BMessage& msg, const char* name, int32 index,
						 BMessage* val);
	static status_t ShortFind(BMessage& msg, const char* name, BMessage* val);
	static BMessage QuickFind(BMessage& msg, const char* name, int32 index);
	static bool Has(BMessage& msg, const char* name, int32 index);
	static status_t Replace(BMessage& msg, const char* name, int32 index,
							BMessage& val);
	static status_t FindData(BMessage& msg, const char* name, type_code type,
							 int32 index, const void** data, ssize_t* size);

	private:
		static BMessage sMsg;
};
BMessage TMessageFuncPolicy::sMsg;
//------------------------------------------------------------------------------
status_t TMessageFuncPolicy::Add(BMessage& msg, const char* name,
								 BMessage& val)
{
	return msg.AddMessage(name, &val);
}
//------------------------------------------------------------------------------
status_t TMessageFuncPolicy::AddData(BMessage& msg, const char* name,
									 type_code type, BMessage* data,
									 ssize_t size, bool)
{
	char* buf = new char[size];
	status_t err = data->Flatten(buf, size);
	if (!err)
	{
		err = msg.AddData(name, type, buf, size, false);
	}
	delete[] buf;
	return err;
}
//------------------------------------------------------------------------------
inline status_t TMessageFuncPolicy::Find(BMessage& msg, const char* name,
										 int32 index, BMessage* val)
{
	return msg.FindMessage(name, index, val);
}
//------------------------------------------------------------------------------
inline status_t TMessageFuncPolicy::ShortFind(BMessage& msg, const char* name,
											  BMessage* val)
{
	return msg.FindMessage(name, val);
}
//------------------------------------------------------------------------------
BMessage TMessageFuncPolicy::QuickFind(BMessage& msg, const char* name,
											  int32 index)
{
	BMessage val;
	msg.FindMessage(name, index, &val);
	return val;
}
//------------------------------------------------------------------------------
inline bool TMessageFuncPolicy::Has(BMessage& msg, const char* name,
									int32 index)
{
	return msg.HasMessage(name, index);
}
//------------------------------------------------------------------------------
inline status_t TMessageFuncPolicy::Replace(BMessage& msg, const char* name,
											int32 index, BMessage& val)
{
	return msg.ReplaceMessage(name, index, &val);
}
//------------------------------------------------------------------------------
inline status_t TMessageFuncPolicy::FindData(BMessage& msg, const char* name,
											 type_code type, int32 index,
											 const void** data, ssize_t* size)
{
	*data = NULL;
	char* ptr;
	status_t err = msg.FindData(name, type, index, (const void**)&ptr, size);
	if (!err)
	{
		err = sMsg.Unflatten(ptr);
		if (!err)
		{
			*(BMessage**)data = &sMsg;
		}
	}
	return err;
//	return (msg.*FindDataFunc)(name, type, index, data, size);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
template<>
struct ArrayTypeBase<BMessage>
{
	class ArrayType
	{
		public:
			ArrayType() : array(NULL), size(0) {;}
			ArrayType(const ArrayType& rhs) : array(NULL), size(0)
				{ *this = rhs; }
			ArrayType(BMessage* data, uint32 len) : array(NULL), size(0)
				{ Assign(data, len); }
			~ArrayType() { if (array) delete[] array; }

			ArrayType& operator=(const ArrayType& rhs)
			{
				if (this != &rhs)
					Assign(rhs.array, rhs.size);
				return *this;
			};

			uint32 Size() { return size; }
			BMessage& operator[](int index)
			{
				// We're just gonna let a segfault happen
				return array[index];
			}

		private:
			void Assign(BMessage* data, uint32 len)
			{
				size = len;

				if (array)
					delete[] array;
				array = new BMessage[Size()];
				for (uint32 i = 0; i < Size(); ++i)
					array[i] = data[i];
			}
			BMessage*	array;
			uint32		size;
	};

	typedef uint32 SizeType;
	static SizeType Size(ArrayType& array) { return array.Size(); }
};
//------------------------------------------------------------------------------
struct TMessageInitPolicy : public ArrayTypeBase<BMessage>
{
	inline static BMessage Zero()	{ return BMessage(); }
	static BMessage Test1()
	{
		BMessage msg('1234');
		msg.AddInt32("int32", 1234);
		return msg;
	}
	static BMessage Test2()
	{
		BMessage msg('5678');
		msg.AddString("string", "5678");
		return msg;
	}
	static size_t SizeOf(const BMessage& data)
	{
		return data.FlattenedSize();
	}
	inline static ArrayType Array()
	{
		BMessage array[3];
		array[0] = Zero();
		array[1] = Test1();
		array[2] = Test2();
		return ArrayType(array, 3);
	}
};
//------------------------------------------------------------------------------
struct TMessageAssertPolicy
{
	inline static BMessage	Zero()				{ return BMessage(); }
	inline static BMessage	Invalid()			{ return BMessage(); }
	 static bool		Size(size_t size, BMessage& msg)
		;//{ return size == msg.FlattenedSize(); }
};
bool TMessageAssertPolicy::Size(size_t size, BMessage& msg)
{
	ssize_t msgSize = msg.FlattenedSize();
	return size == (size_t)msgSize;
}
//------------------------------------------------------------------------------
template<>
struct TMessageItemComparePolicy<BMessage>
{
	inline static bool Compare(const BMessage& lhs, const BMessage& rhs)
		{ return lhs.what == rhs.what; }
};
//------------------------------------------------------------------------------
typedef TMessageItemTest
<
	BMessage,
	B_MESSAGE_TYPE,
	TMessageFuncPolicy,
	TMessageInitPolicy,
	TMessageAssertPolicy
>
TMessageMessageItemTest;
//------------------------------------------------------------------------------

#endif	// MESSAGEMESSAGEITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

