//------------------------------------------------------------------------------
//	MessageBoolItemTest.h
//
//------------------------------------------------------------------------------

#ifndef MESSAGEBOOLITEMTEST_H
#define MESSAGEBOOLITEMTEST_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageItemTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

typedef TMessageItemFuncPolicy
<
	bool,
	&BMessage::AddBool,
	&BMessage::FindBool,
	&BMessage::FindBool,
	&BMessage::FindBool,
	&BMessage::HasBool,
	&BMessage::ReplaceBool
>
TBoolFuncPolicy;

template<>
struct ArrayTypeBase<bool>
{
	class ArrayType
	{
		public:
			ArrayType() : array(NULL), size(0) {;}
			ArrayType(const ArrayType& rhs) : array(NULL), size(0)
				{ *this = rhs; }
			ArrayType(bool* data, uint32 len) : array(NULL), size(0)
				{ Assign(data, len); }
			~ArrayType() { if (array) delete[] array; }

			ArrayType& operator=(const ArrayType& rhs)
			{
				if (this != &rhs)
					Assign(rhs.array, rhs.size);
				return *this;
			};

			uint32 Size() { return size; }
			bool& operator[](int index)
			{
				// We're just gonna let a segfault happen
				return array[index];
			}

		private:
			void Assign(bool* data, uint32 len)
			{
				size = len;

				if (array)
					delete[] array;
				array = new bool[Size()];
				memcpy(array, data, Size());
			}
			bool*	array;
			uint32	size;
	};

	typedef uint32 SizeType;
	static SizeType Size(ArrayType& array) { return array.Size(); }
};

struct TBoolInitPolicy : public ArrayTypeBase<bool>
{
	inline static bool Zero()	{ return false; }
	inline static bool Test1()	{ return true; }
	inline static bool Test2()	{ return false; }
	inline static size_t SizeOf(const bool&)	{ return sizeof (bool); }
	inline static ArrayType Array()
	{
		static bool array[] = { true, true, true };
		return ArrayType(array, 3);
	}
};

//struct TBoolAssertPolicy
//{
//	inline static bool Zero()		{ return false; }
//	inline static bool Invalid()	{ return false;}
//};
typedef TMessageItemAssertPolicy
<
	bool//,
//	false,
//	false
>
TBoolAssertPolicy;

typedef TMessageItemTest
<
	bool,
	B_BOOL_TYPE,
	TBoolFuncPolicy,
	TBoolInitPolicy,
	TBoolAssertPolicy
>
TMessageBoolItemTest;

#endif	// MESSAGEBOOLITEMTEST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

