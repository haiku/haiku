/*
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_UNIT_H
#define TYPE_UNIT_H


#include <ObjectList.h>
#include <String.h>

#include "BaseUnit.h"


class DIETypeUnit;


class TypeUnit : public BaseUnit {
public:
								TypeUnit(off_t headerOffset,
									off_t contentOffset,
									off_t totalSize,
									off_t abbreviationOffset,
									off_t typeOffset,
									uint8 addressSize,
									uint64 typeSignature,
									bool isDwarf64);
								~TypeUnit();

			uint64				Signature() const	{ return fSignature; }

			off_t				TypeOffset() const	{ return fTypeOffset; }


			DIETypeUnit*		UnitEntry() const	{ return fUnitEntry; }
			void				SetUnitEntry(DIETypeUnit* entry);

			DebugInfoEntry*		TypeEntry() const;
			void				SetTypeEntry(DebugInfoEntry* entry);

	virtual	dwarf_unit_kind		Kind() const;

private:
			DIETypeUnit*		fUnitEntry;
			DebugInfoEntry*		fTypeEntry;
			uint64				fSignature;
			off_t				fTypeOffset;
};


struct TypeUnitTableEntry {
	uint64					signature;
	TypeUnit*				unit;
	TypeUnitTableEntry*		next;

	TypeUnitTableEntry(uint64 signature, TypeUnit* unit)
		:
		signature(signature),
		unit(unit)
	{
	}
};


struct TypeUnitTableHashDefinition {
	typedef uint64					KeyType;
	typedef	TypeUnitTableEntry		ValueType;

	size_t HashKey(uint64 key) const
	{
		return (size_t)key;
	}

	size_t Hash(TypeUnitTableEntry* value) const
	{
		return HashKey(value->signature);
	}

	bool Compare(uint64 key, TypeUnitTableEntry* value) const
	{
		return value->signature == key;
	}

	TypeUnitTableEntry*& GetLink(TypeUnitTableEntry* value) const
	{
		return value->next;
	}
};


#endif	// TYPE_UNIT_H
