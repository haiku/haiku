/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverPackageSpecifier.h>


namespace BPackageKit {


BSolverPackageSpecifier::BSolverPackageSpecifier()
	:
	fType(B_UNSPECIFIED),
	fPackage(NULL),
	fSelectString()
{
}


BSolverPackageSpecifier::BSolverPackageSpecifier(BSolverPackage* package)
	:
	fType(B_PACKAGE),
	fPackage(package),
	fSelectString()
{
}


BSolverPackageSpecifier::BSolverPackageSpecifier(const BString& selectString)
	:
	fType(B_SELECT_STRING),
	fPackage(NULL),
	fSelectString(selectString)
{
}


BSolverPackageSpecifier::BSolverPackageSpecifier(
	const BSolverPackageSpecifier& other)
	:
	fType(other.fType),
	fPackage(other.fPackage),
	fSelectString(other.fSelectString)
{
}


BSolverPackageSpecifier::~BSolverPackageSpecifier()
{
}


BSolverPackageSpecifier::BType
BSolverPackageSpecifier::Type() const
{
	return fType;
}


BSolverPackage*
BSolverPackageSpecifier::Package() const
{
	return fPackage;
}


const BString&
BSolverPackageSpecifier::SelectString() const
{
	return fSelectString;
}


BSolverPackageSpecifier&
BSolverPackageSpecifier::operator=(const BSolverPackageSpecifier& other)
{
	fType = other.fType;
	fPackage = other.fPackage;
	fSelectString = other.fSelectString;
	return *this;
}


}	// namespace BPackageKit
