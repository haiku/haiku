/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef ATTRIBUTE_VALUE_H
#define ATTRIBUTE_VALUE_H

#include "AttributeClasses.h"
#include "Types.h"


class DebugInfoEntry;


struct AttributeValue {
	union {
		target_addr_t		address;
		struct {
			const void*		data;
			off_t			length;
		}					block;
		uint64				constant;
		bool				flag;
		off_t				pointer;
		DebugInfoEntry*		reference;
		const char*			string;
	};

	uint16				attributeForm;
	uint8				attributeClass;
	bool				isSigned;

	AttributeValue()
		:
		attributeClass(ATTRIBUTE_CLASS_UNKNOWN)
	{
	}

	~AttributeValue()
	{
		Unset();
	}

	void SetToAddress(target_addr_t address)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_ADDRESS;
		this->address = address;
	}

	void SetToBlock(const void* data, off_t length)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_BLOCK;
		block.data = data;
		block.length = length;
	}

	void SetToConstant(uint64 value, bool isSigned)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_CONSTANT;
		this->constant = value;
		this->isSigned = isSigned;
	}

	void SetToFlag(bool value)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_FLAG;
		this->flag = value;
	}

	void SetToLinePointer(off_t value)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_LINEPTR;
		this->pointer = value;
	}

	void SetToLocationListPointer(off_t value)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_LOCLISTPTR;
		this->pointer = value;
	}

	void SetToMacroPointer(off_t value)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_MACPTR;
		this->pointer = value;
	}

	void SetToRangeListPointer(off_t value)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_RANGELISTPTR;
		this->pointer = value;
	}

	void SetToReference(DebugInfoEntry* entry)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_REFERENCE;
		this->reference = entry;
	}

	void SetToString(const char* string)
	{
		Unset();
		attributeClass = ATTRIBUTE_CLASS_STRING;
		this->string = string;
	}

	void Unset()
	{
		attributeClass = ATTRIBUTE_CLASS_UNKNOWN;
	}

	const char* ToString(char* buffer, size_t size);
};


struct DynamicAttributeValue {
	union {
		uint64				constant;
		DebugInfoEntry*		reference;
		struct {
			const void*		data;
			off_t			length;
		}					block;
	};
	uint8				attributeClass;

	DynamicAttributeValue()
		:
		attributeClass(ATTRIBUTE_CLASS_UNKNOWN)
	{
		this->constant = 0;
	}

	bool IsValid() const
	{
		return attributeClass != ATTRIBUTE_CLASS_UNKNOWN;
	}

	void SetTo(uint64 constant)
	{
		this->constant = constant;
		attributeClass = ATTRIBUTE_CLASS_CONSTANT;
	}

	void SetTo(DebugInfoEntry* reference)
	{
		this->reference = reference;
		attributeClass = ATTRIBUTE_CLASS_REFERENCE;
	}

	void SetTo(const void* data, off_t length)
	{
		block.data = data;
		block.length = length;
		attributeClass = ATTRIBUTE_CLASS_BLOCK;
	}
};


struct ConstantAttributeValue {
	union {
		uint64				constant;
		const char*			string;
		struct {
			const void*		data;
			off_t			length;
		}					block;
	};
	uint8				attributeClass;

	ConstantAttributeValue()
		:
		attributeClass(ATTRIBUTE_CLASS_UNKNOWN)
	{
	}

	bool IsValid() const
	{
		return attributeClass != ATTRIBUTE_CLASS_UNKNOWN;
	}

	void SetTo(uint64 constant)
	{
		this->constant = constant;
		attributeClass = ATTRIBUTE_CLASS_CONSTANT;
	}

	void SetTo(const char* string)
	{
		this->string = string;
		attributeClass = ATTRIBUTE_CLASS_STRING;
	}

	void SetTo(const void* data, off_t length)
	{
		block.data = data;
		block.length = length;
		attributeClass = ATTRIBUTE_CLASS_BLOCK;
	}
};


struct MemberLocation {
	union {
		uint64				constant;
		off_t				listOffset;
		struct {
			const void*		data;
			off_t			length;
		}					expression;
	};
	uint8				attributeClass;

	MemberLocation()
		:
		attributeClass(ATTRIBUTE_CLASS_UNKNOWN)
	{
	}

	bool IsValid() const
	{
		return attributeClass != ATTRIBUTE_CLASS_UNKNOWN;
	}

	bool IsConstant() const
	{
		return attributeClass == ATTRIBUTE_CLASS_CONSTANT;
	}

	bool IsExpression() const
	{
		return attributeClass == ATTRIBUTE_CLASS_BLOCK
			&& expression.data != NULL;
	}

	bool IsLocationList() const
	{
		return attributeClass == ATTRIBUTE_CLASS_LOCLISTPTR;
	}

	void SetToConstant(uint64 constant)
	{
		this->constant = constant;
		attributeClass = ATTRIBUTE_CLASS_CONSTANT;
	}

	void SetToExpression(const void* data, off_t length)
	{
		expression.data = data;
		expression.length = length;
		attributeClass = ATTRIBUTE_CLASS_BLOCK;
	}

	void SetToLocationList(off_t listOffset)
	{
		this->listOffset = listOffset;
		attributeClass = ATTRIBUTE_CLASS_LOCLISTPTR;
	}
};


struct LocationDescription {
	union {
		off_t			listOffset;	// location list
		struct {
			const void*	data;
			off_t		length;
		}				expression;	// location expression
	};
	uint8				attributeClass;

	LocationDescription()
		:
		attributeClass(ATTRIBUTE_CLASS_BLOCK)
	{
		expression.data = NULL;
		expression.length = 0;
	}

	bool IsExpression() const
	{
		return attributeClass == ATTRIBUTE_CLASS_BLOCK
			&& expression.data != NULL;
	}

	bool IsLocationList() const
	{
		return attributeClass == ATTRIBUTE_CLASS_LOCLISTPTR;
	}

	bool IsValid() const
	{
		return IsExpression() || IsLocationList();
	}

	void SetToLocationList(off_t offset)
	{
		listOffset = offset;
		attributeClass = ATTRIBUTE_CLASS_LOCLISTPTR;
	}

	void SetToExpression(const void* data, off_t length)
	{
		expression.data = data;
		expression.length = length;
		attributeClass = ATTRIBUTE_CLASS_BLOCK;
	}
};


struct DeclarationLocation {
	uint32	file;
	uint32	line;
	uint32	column;

	DeclarationLocation()
		:
		file(0xffffffff),
		line(0xffffffff),
		column(0xffffffff)
	{
	}

	void SetFile(uint32 file)
	{
		this->file = file;
	}

	void SetLine(uint32 line)
	{
		this->line = line;
	}

	void SetColumn(uint32 column)
	{
		this->column = column;
	}

	bool IsFileSet() const
	{
		return file != 0xffffffff;
	}

	bool IsLineSet() const
	{
		return line != 0xffffffff;
	}

	bool IsColumnSet() const
	{
		return column != 0xffffffff;
	}
};

#endif	// ATTRIBUTE_VALUE_H
