/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverResult.h>


namespace BPackageKit {


// #pragma mark - BSolverResultElement


BSolverResultElement::BSolverResultElement(BType type, BSolverPackage* package)
	:
	fType(type),
	fPackage(package)
{
}


BSolverResultElement::BSolverResultElement(const BSolverResultElement& other)
	:
	fType(other.fType),
	fPackage(other.fPackage)
{
}


BSolverResultElement::~BSolverResultElement()
{
}


BSolverResultElement::BType
BSolverResultElement::Type() const
{
	return fType;
}


BSolverPackage*
BSolverResultElement::Package() const
{
	return fPackage;
}


BSolverResultElement&
BSolverResultElement::operator=(const BSolverResultElement& other)
{
	fType = other.fType;
	fPackage = other.fPackage;
	return *this;
}


// #pragma mark - BSolverResult


BSolverResult::BSolverResult()
	:
	fElements(20, true)
{
}


BSolverResult::~BSolverResult()
{
}


bool
BSolverResult::IsEmpty() const
{
	return fElements.IsEmpty();
}


int32
BSolverResult::CountElements() const
{
	return fElements.CountItems();
}


const BSolverResultElement*
BSolverResult::ElementAt(int32 index) const
{
	return fElements.ItemAt(index);
}


void
BSolverResult::MakeEmpty()
{
	fElements.MakeEmpty();
}


bool
BSolverResult::AppendElement(const BSolverResultElement& element)
{
	BSolverResultElement* newElement
		= new(std::nothrow) BSolverResultElement(element);
	if (newElement == NULL || !fElements.AddItem(newElement)) {
		delete newElement;
		return false;
	}

	return true;
}


}	// namespace BPackageKit
