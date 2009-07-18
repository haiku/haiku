/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "TypeComponentPath.h"

#include <new>

#include "StringUtils.h"


// #pragma mark - TypeComponent


uint32
TypeComponent::HashValue() const
{
	uint32 hash = ((uint32)index << 8) | (componentKind << 4) | typeKind;
	return StringUtils::HashValue(name) * 13 + hash;
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
