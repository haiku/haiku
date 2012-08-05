/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TYPE_COMPONENT_PATH_H
#define TYPE_COMPONENT_PATH_H


#include <String.h>

#include <ObjectList.h>
#include <Referenceable.h>

#include "ArrayIndexPath.h"
#include "Type.h"


enum type_component_kind {
	TYPE_COMPONENT_UNDEFINED,
	TYPE_COMPONENT_BASE_TYPE,
	TYPE_COMPONENT_DATA_MEMBER,
	TYPE_COMPONENT_ARRAY_ELEMENT
};


struct TypeComponent {
	uint64				index;
	BString				name;
	type_component_kind	componentKind;
	type_kind			typeKind;

	TypeComponent()
		:
		componentKind(TYPE_COMPONENT_UNDEFINED)

	{
	}

	TypeComponent(const TypeComponent& other)
		:
		index(other.index),
		name(other.name),
		componentKind(other.componentKind),
		typeKind(other.typeKind)

	{
	}

	bool IsValid() const
	{
		return componentKind != TYPE_COMPONENT_UNDEFINED;
	}

	void SetToBaseType(type_kind typeKind, uint64 index = 0,
		const BString& name = BString())
	{
		this->componentKind = TYPE_COMPONENT_BASE_TYPE;
		this->typeKind = typeKind;
		this->index = index;
		this->name = name;
	}

	void SetToDataMember(type_kind typeKind, uint64 index,
		const BString& name)
	{
		this->componentKind = TYPE_COMPONENT_DATA_MEMBER;
		this->typeKind = typeKind;
		this->index = index;
		this->name = name;
	}

	void SetToArrayElement(type_kind typeKind, const BString& indexPath)
	{
		this->componentKind = TYPE_COMPONENT_ARRAY_ELEMENT;
		this->typeKind = typeKind;
		this->index = 0;
		this->name = indexPath;
	}

	bool SetToArrayElement(type_kind typeKind, const ArrayIndexPath& indexPath)
	{
		this->componentKind = TYPE_COMPONENT_ARRAY_ELEMENT;
		this->typeKind = typeKind;
		this->index = 0;
		return indexPath.GetPathString(this->name);
	}

	bool HasPrefix(const TypeComponent& other) const;

	uint32 HashValue() const;

	void Dump() const;

	TypeComponent& operator=(const TypeComponent& other)

	{
		index = other.index;
		name = other.name;
		componentKind = other.componentKind;
		typeKind = other.typeKind;
		return *this;
	}

	bool operator==(const TypeComponent& other) const;

	bool operator!=(const TypeComponent& other) const
	{
		return !(*this == other);
	}
};


class TypeComponentPath : public BReferenceable {
public:
								TypeComponentPath();
								TypeComponentPath(
									const TypeComponentPath& other);
	virtual						~TypeComponentPath();

			int32				CountComponents() const;
			TypeComponent		ComponentAt(int32 index) const;

			bool				AddComponent(const TypeComponent& component);
			void				Clear();

			TypeComponentPath*	CreateSubPath(int32 componentCount) const;
									// returns a new object (or NULL when out
									// of memory)

			uint32				HashValue() const;

			void				Dump() const;

			TypeComponentPath&	operator=(const TypeComponentPath& other);

			bool operator==(const TypeComponentPath& other) const;
			bool operator!=(const TypeComponentPath& other) const
									{ return !(*this == other); }

private:
			typedef BObjectList<TypeComponent> ComponentList;

private:
			ComponentList		fComponents;
};


#endif	// TYPE_COMPONENT_PATH_H
