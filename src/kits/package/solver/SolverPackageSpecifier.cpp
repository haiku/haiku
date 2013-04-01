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
	fRepository(NULL),
	fExpression()
{
}


BSolverPackageSpecifier::BSolverPackageSpecifier(
	const BPackageResolvableExpression& expression)
	:
	fRepository(NULL),
	fExpression(expression)
{
}


BSolverPackageSpecifier::BSolverPackageSpecifier(BSolverRepository* repository,
	const BPackageResolvableExpression& expression)
	:
	fRepository(repository),
	fExpression(expression)
{
}


BSolverPackageSpecifier::BSolverPackageSpecifier(
	const BSolverPackageSpecifier& other)
	:
	fRepository(other.fRepository),
	fExpression(other.fExpression)
{
}


BSolverPackageSpecifier::~BSolverPackageSpecifier()
{
}


BSolverRepository*
BSolverPackageSpecifier::Repository() const
{
	return fRepository;
}


const BPackageResolvableExpression&
BSolverPackageSpecifier::Expression() const
{
	return fExpression;
}


BSolverPackageSpecifier&
BSolverPackageSpecifier::operator=(const BSolverPackageSpecifier& other)
{
	fRepository = other.fRepository;
	fExpression = other.fExpression;
	return *this;
}


}	// namespace BPackageKit
