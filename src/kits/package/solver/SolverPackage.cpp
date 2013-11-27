/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/SolverPackage.h>


namespace BPackageKit {


BSolverPackage::BSolverPackage(BSolverRepository* repository,
	const BPackageInfo& packageInfo)
	:
	fRepository(repository),
	fInfo(packageInfo)
{
}


BSolverPackage::BSolverPackage(const BSolverPackage& other)
	:
	fRepository(other.fRepository),
	fInfo(other.fInfo)
{
}


BSolverPackage::~BSolverPackage()
{
}


BSolverRepository*
BSolverPackage::Repository() const
{
	return fRepository;
}


const BPackageInfo&
BSolverPackage::Info() const
{
	return fInfo;
}


BString
BSolverPackage::Name() const
{
	return fInfo.Name();
}


BString
BSolverPackage::VersionedName() const
{
	if (fInfo.Version().InitCheck() != B_OK)
		return Name();
	BString result = Name();
	return result << '-' << fInfo.Version().ToString();
}


const BPackageVersion&
BSolverPackage::Version() const
{
	return fInfo.Version();
}


BSolverPackage&
BSolverPackage::operator=(const BSolverPackage& other)
{
	fRepository = other.fRepository;
	fInfo = other.fInfo;
	return *this;
}


}	// namespace BPackageKit
