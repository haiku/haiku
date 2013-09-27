/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverPackageSpecifierList.h>

#include <new>
#include <vector>

#include <package/solver/SolverPackageSpecifier.h>


namespace BPackageKit {

class BSolverPackageSpecifierList::Vector
	: public std::vector<BSolverPackageSpecifier> {
public:
	Vector()
		:
		std::vector<BSolverPackageSpecifier>()
	{
	}

	Vector(const std::vector<BSolverPackageSpecifier>& other)
		:
		std::vector<BSolverPackageSpecifier>(other)
	{
	}
};


BSolverPackageSpecifierList::BSolverPackageSpecifierList()
	:
	fSpecifiers(NULL)
{
}


BSolverPackageSpecifierList::BSolverPackageSpecifierList(
	const BSolverPackageSpecifierList& other)
	:
	fSpecifiers(NULL)
{
	*this = other;
}


BSolverPackageSpecifierList::~BSolverPackageSpecifierList()
{
	delete fSpecifiers;
}


bool
BSolverPackageSpecifierList::IsEmpty() const
{
	return fSpecifiers == NULL || fSpecifiers->empty();
}


int32
BSolverPackageSpecifierList::CountSpecifiers() const
{
	return fSpecifiers != NULL ? fSpecifiers->size() : 0;
}


const BSolverPackageSpecifier*
BSolverPackageSpecifierList::SpecifierAt(int32 index) const
{
	if (fSpecifiers == NULL || index < 0
		|| (size_t)index >= fSpecifiers->size()) {
		return NULL;
	}

	return &(*fSpecifiers)[index];
}


bool
BSolverPackageSpecifierList::AppendSpecifier(
	const BSolverPackageSpecifier& specifier)
{
	try {
		if (fSpecifiers == NULL) {
			fSpecifiers = new(std::nothrow) Vector;
			if (fSpecifiers == NULL)
				return false;
		}

		fSpecifiers->push_back(specifier);
		return true;
	} catch (std::bad_alloc&) {
		return false;
	}
}


bool
BSolverPackageSpecifierList::AppendSpecifier(BSolverPackage* package)
{
	return AppendSpecifier(BSolverPackageSpecifier(package));
}


bool
BSolverPackageSpecifierList::AppendSpecifier(const BString& selectString)
{
	return AppendSpecifier(BSolverPackageSpecifier(selectString));
}


bool
BSolverPackageSpecifierList::AppendSpecifiers(const char* const* selectStrings,
	int32 count)
{
	for (int32 i = 0; i < count; i++) {
		if (!AppendSpecifier(selectStrings[i])) {
			for (int32 k = i - 1; k >= 0; k--)
				fSpecifiers->pop_back();
			return false;
		}
	}

	return true;
}


void
BSolverPackageSpecifierList::MakeEmpty()
{
	fSpecifiers->clear();
}


BSolverPackageSpecifierList&
BSolverPackageSpecifierList::operator=(const BSolverPackageSpecifierList& other)
{
	if (this == &other)
		return *this;

	delete fSpecifiers;
	fSpecifiers = NULL;

	if (other.fSpecifiers == NULL)
		return *this;

	try {
		fSpecifiers = new(std::nothrow) Vector(*other.fSpecifiers);
	} catch (std::bad_alloc&) {
	}

	return *this;
}


}	// namespace BPackageKit
