/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VARIANT_H
#define _VARIANT_H


#include <Rect.h>
#include <SupportDefs.h>
#include <TypeConstants.h>

#include <Referenceable.h>


enum {
	B_VARIANT_DONT_COPY_DATA		= 0x01,
	B_VARIANT_OWNS_DATA				= 0x02,
	B_VARIANT_REFERENCEABLE_DATA	= 0x04
};


class BMessage;


class BVariant {
public:
	inline						BVariant();
	inline						BVariant(bool value);
	inline						BVariant(int8 value);
	inline						BVariant(uint8 value);
	inline						BVariant(int16 value);
	inline						BVariant(uint16 value);
	inline						BVariant(int32 value);
	inline						BVariant(uint32 value);
	inline						BVariant(int64 value);
	inline						BVariant(uint64 value);
	inline						BVariant(float value);
	inline						BVariant(double value);
	inline						BVariant(const BRect &value);
	inline						BVariant(float left, float top, float right,
									float bottom);
	inline						BVariant(const void* value);
	inline						BVariant(const char* value,
									uint32 flags = 0);
	inline						BVariant(BReferenceable* value, type_code type);
									// type must be a custom type
	inline						BVariant(const BVariant& other);
								~BVariant();

	inline	void				SetTo(const BVariant& other);
	inline	void				SetTo(bool value);
	inline	void				SetTo(int8 value);
	inline	void				SetTo(uint8 value);
	inline	void				SetTo(int16 value);
	inline	void				SetTo(uint16 value);
	inline	void				SetTo(int32 value);
	inline	void				SetTo(uint32 value);
	inline	void				SetTo(int64 value);
	inline	void				SetTo(uint64 value);
	inline	void				SetTo(float value);
	inline	void				SetTo(double value);
	inline	void				SetTo(const BRect& value);
	inline	void				SetTo(float left, float top, float right,
									float bottom);
	inline	void				SetTo(const void* value);
	inline	void				SetTo(const char* value,
									uint32 flags = 0);
	inline	void				SetTo(BReferenceable* value, type_code type);
									// type must be a custom type
			status_t			SetToTypedData(const void* data,
									type_code type);
			void				Unset();

	inline	BVariant&			operator=(const BVariant& other);

			bool				operator==(const BVariant& other) const;
	inline	bool				operator!=(const BVariant& other) const;

	inline	type_code			Type() const		{ return fType; }
			size_t				Size() const;
			const uint8*		Bytes() const;

	inline	bool				IsNumber() const;
	inline	bool				IsInteger(bool* _isSigned = NULL) const;
	inline	bool				IsFloat() const;
									// floating point, not just float

			bool				ToBool() const;
			int8				ToInt8() const;
			uint8				ToUInt8() const;
			int16				ToInt16() const;
			uint16				ToUInt16() const;
			int32				ToInt32() const;
			uint32				ToUInt32() const;
			int64				ToInt64() const;
			uint64				ToUInt64() const;
			float				ToFloat() const;
			double				ToDouble() const;
			void*				ToPointer() const;
			const char*			ToString() const;
			BRect				ToRect() const;
			BReferenceable*		ToReferenceable() const;

			void				SwapEndianess();
									// has effect only on scalar types (pointer
									// counting as scalar, not string, though)

			status_t			AddToMessage(BMessage& message,
									const char* fieldName) const;
			status_t			SetFromMessage(const BMessage& message,
									const char* fieldName);

	static	size_t				SizeOfType(type_code type);
	static	bool				TypeIsNumber(type_code type);
	static	bool				TypeIsInteger(type_code type,
									bool* _isSigned = NULL);
	static	bool				TypeIsFloat(type_code type);

private:
			void				_SetTo(const BVariant& other);
			void				_SetTo(bool value);
			void				_SetTo(int8 value);
			void				_SetTo(uint8 value);
			void				_SetTo(int16 value);
			void				_SetTo(uint16 value);
			void				_SetTo(int32 value);
			void				_SetTo(uint32 value);
			void				_SetTo(int64 value);
			void				_SetTo(uint64 value);
			void				_SetTo(float value);
			void				_SetTo(double value);
			void				_SetTo(const void* value);
			void				_SetTo(float left, float top, float right,
									float bottom);
			bool				_SetTo(const char* value,
									uint32 flags);
			void				_SetTo(BReferenceable* value, type_code type);

	template<typename NumberType>
	inline	NumberType			_ToNumber() const;

private:
			type_code			fType;
			uint32				fFlags;
			union {
				bool			fBool;
				int8			fInt8;
				uint8			fUInt8;
				int16			fInt16;
				uint16			fUInt16;
				int32			fInt32;
				uint32			fUInt32;
				int64			fInt64;
				uint64			fUInt64;
				float			fFloat;
				double			fDouble;
				void*			fPointer;
				char*			fString;
				BReferenceable*	fReferenceable;
				struct {
					float		left;
					float		top;
					float		right;
					float		bottom;
				}				fRect;
				uint8			fBytes[sizeof(float) * 4];
			};
};


BVariant::BVariant()
	:
	fType(0),
	fFlags(0)
{
}


BVariant::BVariant(bool value)
{
	_SetTo(value);
}


BVariant::BVariant(int8 value)
{
	_SetTo(value);
}


BVariant::BVariant(uint8 value)
{
	_SetTo(value);
}


BVariant::BVariant(int16 value)
{
	_SetTo(value);
}


BVariant::BVariant(uint16 value)
{
	_SetTo(value);
}


BVariant::BVariant(int32 value)
{
	_SetTo(value);
}


BVariant::BVariant(uint32 value)
{
	_SetTo(value);
}


BVariant::BVariant(int64 value)
{
	_SetTo(value);
}


BVariant::BVariant(uint64 value)
{
	_SetTo(value);
}


BVariant::BVariant(float value)
{
	_SetTo(value);
}


BVariant::BVariant(double value)
{
	_SetTo(value);
}


BVariant::BVariant(const BRect& value)
{
	_SetTo(value);
}


BVariant::BVariant(float left, float top, float right, float bottom)
{
	_SetTo(left, top, right, bottom);
}


BVariant::BVariant(const void* value)
{
	_SetTo(value);
}


BVariant::BVariant(const char* value, uint32 flags)
{
	_SetTo(value, flags);
}


BVariant::BVariant(BReferenceable* value, type_code type)
{
	_SetTo(value, type);
}


BVariant::BVariant(const BVariant& other)
{
	_SetTo(other);
}


BVariant&
BVariant::operator=(const BVariant& other)
{
	Unset();
	_SetTo(other);
	return *this;
}


bool
BVariant::operator!=(const BVariant& other) const
{
	return !(*this == other);
}


void
BVariant::SetTo(const BVariant& other)
{
	Unset();
	_SetTo(other);
}


void
BVariant::SetTo(bool value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(int8 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(uint8 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(int16 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(uint16 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(int32 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(uint32 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(int64 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(uint64 value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(float value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(double value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(const BRect& value)
{
	Unset();
	_SetTo(value.left, value.top, value.right, value.bottom);
}


void
BVariant::SetTo(float left, float top, float right, float bottom)
{
	Unset();
	_SetTo(left, top, right, bottom);
}


void
BVariant::SetTo(const void* value)
{
	Unset();
	_SetTo(value);
}


void
BVariant::SetTo(const char* value, uint32 flags)
{
	Unset();
	_SetTo(value, flags);
}


void
BVariant::SetTo(BReferenceable* value, type_code type)
{
	Unset();
	_SetTo(value, type);
}


bool
BVariant::IsNumber() const
{
	return TypeIsNumber(fType);
}


bool
BVariant::IsInteger(bool* _isSigned) const
{
	return TypeIsInteger(fType, _isSigned);
}


bool
BVariant::IsFloat() const
{
	return TypeIsFloat(fType);
}


#endif	// _VARIANT_H
