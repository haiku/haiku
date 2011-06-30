/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_H
#define TYPE_H


#include <image.h>

#include <Referenceable.h>
#include <Variant.h>

#include "Types.h"


enum type_kind {
	TYPE_PRIMITIVE,
	TYPE_COMPOUND,
	TYPE_MODIFIED,
	TYPE_TYPEDEF,
	TYPE_ADDRESS,
	TYPE_ENUMERATION,
	TYPE_SUBRANGE,
	TYPE_ARRAY,
	TYPE_UNSPECIFIED,
	TYPE_FUNCTION,
	TYPE_POINTER_TO_MEMBER
};


enum compound_type_kind {
	COMPOUND_TYPE_CLASS,
	COMPOUND_TYPE_STRUCT,
	COMPOUND_TYPE_UNION,
	COMPOUND_TYPE_INTERFACE
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


class ArrayIndexPath;
class BString;
class Type;
class ValueLocation;


class BaseType : public BReferenceable {
public:
	virtual						~BaseType();

	virtual	Type*				GetType() const = 0;
};


class DataMember : public BReferenceable {
public:
	virtual						~DataMember();

	virtual	const char*			Name() const = 0;
	virtual	Type*				GetType() const = 0;
};


class EnumeratorValue : public BReferenceable {
public:
	virtual						~EnumeratorValue();

	virtual	const char*			Name() const = 0;
	virtual	BVariant			Value() const = 0;
};


class ArrayDimension : public BReferenceable {
public:
	virtual						~ArrayDimension();

	virtual	Type*				GetType() const = 0;
									// subrange or enumeration
	virtual	uint64				CountElements() const;
									// returns 0, if unknown
};


class FunctionParameter : public BReferenceable {
public:
	virtual						~FunctionParameter();

	virtual	const char*			Name() const = 0;
	virtual	Type*				GetType() const = 0;
};


class Type : public BReferenceable {
public:
	virtual						~Type();

	virtual	image_id			ImageID() const = 0;
	virtual	const BString&		ID() const = 0;
	virtual	const BString&		Name() const = 0;
	virtual	type_kind			Kind() const = 0;
	virtual	target_size_t		ByteSize() const = 0;
	virtual	Type*				ResolveRawType(bool nextOneOnly) const;
									// strips modifiers and typedefs (only one,
									// if requested)

	virtual	status_t			ResolveObjectDataLocation(
									const ValueLocation& objectLocation,
									ValueLocation*& _location) = 0;
									// returns a reference
	virtual	status_t			ResolveObjectDataLocation(
									target_addr_t objectAddress,
									ValueLocation*& _location) = 0;
									// returns a reference
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

	virtual	status_t			ResolveBaseTypeLocation(BaseType* baseType,
									const ValueLocation& parentLocation,
									ValueLocation*& _location) = 0;
									// returns a reference
	virtual	status_t			ResolveDataMemberLocation(DataMember* member,
									const ValueLocation& parentLocation,
									ValueLocation*& _location) = 0;
									// returns a reference
};


class ModifiedType : public virtual Type {
public:
	virtual						~ModifiedType();

	virtual	type_kind			Kind() const;

	virtual	uint32				Modifiers() const = 0;
	virtual	Type*				BaseType() const = 0;
	virtual	Type*				ResolveRawType(bool nextOneOnly) const;
};


class TypedefType : public virtual Type {
public:
	virtual						~TypedefType();

	virtual	type_kind			Kind() const;

	virtual	Type*				BaseType() const = 0;
	virtual	Type*				ResolveRawType(bool nextOneOnly) const;
};


class AddressType : public virtual Type {
public:
	virtual						~AddressType();

	virtual	type_kind			Kind() const;

	virtual	address_type_kind	AddressKind() const = 0;
	virtual	Type*				BaseType() const = 0;
};


class EnumerationType : public virtual Type {
public:
	virtual						~EnumerationType();

	virtual	type_kind			Kind() const;

	virtual	Type*				BaseType() const = 0;
									// may return NULL

	virtual	int32				CountValues() const = 0;
	virtual	EnumeratorValue*	ValueAt(int32 index) const = 0;
	virtual	EnumeratorValue*	ValueFor(const BVariant& value) const;
};


class SubrangeType : public virtual Type {
public:
	virtual						~SubrangeType();

	virtual	type_kind			Kind() const;

	virtual	Type*				BaseType() const = 0;

	virtual	BVariant			LowerBound() const = 0;
	virtual	BVariant			UpperBound() const = 0;
};


class ArrayType : public virtual Type {
public:
	virtual						~ArrayType();

	virtual	type_kind			Kind() const;

	virtual	Type*				BaseType() const = 0;

	virtual	int32				CountDimensions() const = 0;
	virtual	ArrayDimension*		DimensionAt(int32 index) const = 0;

	virtual	status_t			ResolveElementLocation(
									const ArrayIndexPath& indexPath,
									const ValueLocation& parentLocation,
									ValueLocation*& _location) = 0;
									// returns a reference
};


class UnspecifiedType : public virtual Type {
public:
	virtual						~UnspecifiedType();

	virtual	type_kind			Kind() const;
};


class FunctionType : public virtual Type {
public:
	virtual						~FunctionType();

	virtual	type_kind			Kind() const;

	virtual	Type*				ReturnType() const = 0;

	virtual	int32				CountParameters() const = 0;
	virtual	FunctionParameter*	ParameterAt(int32 index) const = 0;

	virtual	bool				HasVariableArguments() const = 0;
};


class PointerToMemberType : public virtual Type {
public:
	virtual						~PointerToMemberType();

	virtual	type_kind			Kind() const;

	virtual	CompoundType*		ContainingType() const = 0;
	virtual	Type*				BaseType() const = 0;
};


#endif	// TYPE_H
