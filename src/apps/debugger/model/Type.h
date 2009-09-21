/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_H
#define TYPE_H


#include <Referenceable.h>

#include "Types.h"


enum type_kind {
	TYPE_PRIMITIVE,
	TYPE_COMPOUND,
	TYPE_MODIFIED,
	TYPE_TYPEDEF,
	TYPE_ADDRESS,
	TYPE_ARRAY
};


enum address_type_kind {
	DERIVED_TYPE_POINTER,
	DERIVED_TYPE_REFERENCE
};


enum {
	TYPE_MODIFIER_CONST		= 0x01,
	TYPE_MODIFIER_VOLATILE	= 0x02,
	TYPE_MODIFIER_RESTRICT	= 0x04,
	TYPE_MODIFIER_PACKED	= 0x08,
	TYPE_MODIFIER_SHARED	= 0x10
};


class Type;


class BaseType : public Referenceable {
public:
	virtual						~BaseType();

	virtual	Type*				GetType() const = 0;
};


class DataMember : public Referenceable {
public:
	virtual						~DataMember();

	virtual	const char*			Name() const = 0;
	virtual	Type*				GetType() const = 0;
};


class Type : public Referenceable {
public:
	virtual						~Type();

	virtual	const char*			Name() const = 0;
	virtual	type_kind			Kind() const = 0;
	virtual	target_size_t		ByteSize() const = 0;
};


class PrimitiveType : public virtual Type {
public:
	virtual						~PrimitiveType();

	virtual	type_kind			Kind() const;

	virtual	uint32				TypeConstant() const = 0;
};


class CompoundType : public virtual Type {
public:
	virtual						~CompoundType();

	virtual	type_kind			Kind() const;

	virtual	int32				CountBaseTypes() const = 0;
	virtual	BaseType*			BaseTypeAt(int32 index) const = 0;

	virtual	int32				CountDataMembers() const = 0;
	virtual	DataMember*			DataMemberAt(int32 index) const = 0;
};


class ModifiedType : public virtual Type {
public:
	virtual						~ModifiedType();

	virtual	type_kind			Kind() const;

	virtual	uint32				Modifiers() const = 0;
	virtual	Type*				BaseType() const = 0;
};


class TypedefType : public virtual Type {
public:
	virtual						~TypedefType();

	virtual	type_kind			Kind() const;

	virtual	Type*				BaseType() const = 0;
};


class AddressType : public virtual Type {
public:
	virtual						~AddressType();

	virtual	type_kind			Kind() const;

	virtual	address_type_kind	AddressKind() const = 0;
	virtual	Type*				BaseType() const = 0;
};


class ArrayType : public virtual Type {
public:
	virtual						~ArrayType();

	virtual	type_kind			Kind() const;

	virtual	Type*				BaseType() const = 0;
	virtual	target_size_t		CountElements() const = 0;
		// TODO: That doesn't work. We need a list of dimensions which in turn
		// are enumeration or subrange types.
};


#endif	// TYPE_H
