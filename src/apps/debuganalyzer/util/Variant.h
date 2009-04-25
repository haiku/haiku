/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef VARIANT_H
#define VARIANT_H


#include <SupportDefs.h>
#include <TypeConstants.h>


enum {
	VARIANT_DONT_COPY_DATA	= 0x01,
	VARIANT_OWNS_DATA		= 0x02
};


class Variant {
public:
	inline						Variant();
	inline						Variant(int8 value);
	inline						Variant(uint8 value);
	inline						Variant(int16 value);
	inline						Variant(uint16 value);
	inline						Variant(int32 value);
	inline						Variant(uint32 value);
	inline						Variant(int64 value);
	inline						Variant(uint64 value);
	inline						Variant(float value);
	inline						Variant(double value);
	inline						Variant(const void* value);
	inline						Variant(const char* value,
									uint32 flags = 0);
	inline						Variant(const Variant& other);
								~Variant();

	inline	void				SetTo(const Variant& other);
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
	inline	void				SetTo(const void* value);
	inline	void				SetTo(const char* value,
									uint32 flags = 0);
			void				Unset();

	inline	Variant&			operator=(const Variant& other);

			type_code			Type() const		{ return fType; }

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

private:
			void				_SetTo(const Variant& other);
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
			void				_SetTo(const char* value,
									uint32 flags);

	template<typename NumberType>
	inline	NumberType			_ToNumber() const;

private:
			type_code			fType;
			uint32				fFlags;
			union {
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
			};
};


Variant::Variant()
	:
	fType(0),
	fFlags(0)
{
}


Variant::Variant(int8 value)
{
	_SetTo(value);
}


Variant::Variant(uint8 value)
{
	_SetTo(value);
}


Variant::Variant(int16 value)
{
	_SetTo(value);
}


Variant::Variant(uint16 value)
{
	_SetTo(value);
}


Variant::Variant(int32 value)
{
	_SetTo(value);
}


Variant::Variant(uint32 value)
{
	_SetTo(value);
}


Variant::Variant(int64 value)
{
	_SetTo(value);
}


Variant::Variant(uint64 value)
{
	_SetTo(value);
}


Variant::Variant(float value)
{
	_SetTo(value);
}


Variant::Variant(double value)
{
	_SetTo(value);
}


Variant::Variant(const void* value)
{
	_SetTo(value);
}


Variant::Variant(const char* value, uint32 flags)
{
	_SetTo(value, flags);
}


Variant::Variant(const Variant& other)
{
	_SetTo(other);
}


Variant&
Variant::operator=(const Variant& other)
{
	Unset();
	_SetTo(other);
	return *this;
}


void
Variant::SetTo(const Variant& other)
{
	Unset();
	_SetTo(other);
}


void
Variant::SetTo(int8 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(uint8 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(int16 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(uint16 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(int32 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(uint32 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(int64 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(uint64 value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(float value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(double value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(const void* value)
{
	Unset();
	_SetTo(value);
}


void
Variant::SetTo(const char* value, uint32 flags)
{
	Unset();
	_SetTo(value, flags);
}


#endif	// VARIANT_H
