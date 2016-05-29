/*
 * Copyright 2009-2012, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TypeComponentPath.h"

#include <stdio.h>

#include <new>

#include "StringUtils.h"


// #pragma mark - TypeComponent


bool
TypeComponent::HasPrefix(const TypeComponent& other) const
{
	if (*this == other)
		return true;

	return componentKind == TYPE_COMPONENT_ARRAY_ELEMENT
		&& other.componentKind == TYPE_COMPONENT_ARRAY_ELEMENT
		&& name.Compare(other.name, other.name.Length()) == 0;
}


uint32
TypeComponent::HashValue() const
{
	uint32 hash = ((uint32)index << 8) | (componentKind << 4) | typeKind;
	return StringUtils::HashValue(name) * 13 + hash;
}


void
TypeComponent::Dump() const
{
	switch (typeKind) {
		case TYPE_PRIMITIVE:
			printf("primitive");
			break;
		case TYPE_COMPOUND:
			printf("compound");
			break;
		case TYPE_MODIFIED:
			printf("modified");
			break;
		case TYPE_TYPEDEF:
			printf("typedef");
			break;
		case TYPE_ADDRESS:
			printf("address");
			break;
		case TYPE_ENUMERATION:
			printf("enum");
			break;
		case TYPE_SUBRANGE:
			printf("subrange");
			break;
		case TYPE_ARRAY:
			printf("array");
			break;
		case TYPE_UNSPECIFIED:
			printf("unspecified");
			break;
		case TYPE_FUNCTION:
			printf("function");
			break;
		case TYPE_POINTER_TO_MEMBER:
			printf("pointer to member");
			break;
	}

	printf(" ");

	switch (componentKind) {
		case TYPE_COMPONENT_UNDEFINED:
			printf("undefined");
			break;
		case TYPE_COMPONENT_BASE_TYPE:
			printf("base %" B_PRIu64 " \"%s\"", index, name.String());
			break;
		case TYPE_COMPONENT_DATA_MEMBER:
			printf("member %" B_PRIu64 " \"%s\"", index, name.String());
			break;
		case TYPE_COMPONENT_ARRAY_ELEMENT:
			printf("element %" B_PRIu64 " \"%s\"", index, name.String());
			break;
	}
}


bool
TypeComponent::operator==(const TypeComponent& other) const
{
	return componentKind == other.componentKind
		&& typeKind == other.typeKind
		&& index == other.index
		&& name == other.name;
}


// #pragma mark - TypeComponentPath


TypeComponentPath::TypeComponentPath()
	:
	fComponents(10, true)
{
}


TypeComponentPath::TypeComponentPath(const TypeComponentPath& other)
	:
	fComponents(10, true)
{
	*this = other;
}


TypeComponentPath::~TypeComponentPath()
{
}


int32
TypeComponentPath::CountComponents() const
{
	return fComponents.CountItems();
}


TypeComponent
TypeComponentPath::ComponentAt(int32 index) const
{
	TypeComponent* component = fComponents.ItemAt(index);
	return component != NULL ? *component : TypeComponent();
}


bool
TypeComponentPath::AddComponent(const TypeComponent& component)
{
	TypeComponent* myComponent = new(std::nothrow) TypeComponent(component);
	if (myComponent == NULL || !fComponents.AddItem(myComponent)) {
		delete myComponent;
		return false;
	}

	return true;
}


void
TypeComponentPath::Clear()
{
	fComponents.MakeEmpty();
}


TypeComponentPath*
TypeComponentPath::CreateSubPath(int32 componentCount) const
{
	if (componentCount < 0 || componentCount > fComponents.CountItems())
		componentCount = fComponents.CountItems();

	TypeComponentPath* path = new(std::nothrow) TypeComponentPath;
	if (path == NULL)
		return NULL;
	BReference<TypeComponentPath> pathReference(path, true);

	for (int32 i = 0; i < componentCount; i++) {
		if (!path->AddComponent(*fComponents.ItemAt(i)))
			return NULL;
	}

	return pathReference.Detach();
}


uint32
TypeComponentPath::HashValue() const
{
	int32 count = fComponents.CountItems();
	if (count == 0)
		return 0;

	uint32 hash = fComponents.ItemAt(0)->HashValue();

	for (int32 i = 1; i < count; i++)
		hash = hash * 17 + fComponents.ItemAt(i)->HashValue();

	return hash;
}


void
TypeComponentPath::Dump() const
{
	int32 count = fComponents.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (i == 0)
			printf("[");
		else
			printf(" -> [");
		fComponents.ItemAt(i)->Dump();
		printf("]");
	}
}


TypeComponentPath&
TypeComponentPath::operator=(const TypeComponentPath& other)
{
	if (this != &other) {
		fComponents.MakeEmpty();

		for (int32 i = 0;
				TypeComponent* component = other.fComponents.ItemAt(i); i++) {
			if (!AddComponent(*component))
				break;
		}
	}

	return *this;
}


bool
TypeComponentPath::operator==(const TypeComponentPath& other) const
{
	int32 count = fComponents.CountItems();
	if (count != other.fComponents.CountItems())
		return false;

	for (int32 i = 0; i < count; i++) {
		if (*fComponents.ItemAt(i) != *other.fComponents.ItemAt(i))
			return false;
	}

	return true;
}
