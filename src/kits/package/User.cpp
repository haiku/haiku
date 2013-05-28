/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/User.h>

#include <ctype.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


BUser::BUser()
	:
	fName(),
	fRealName(),
	fHome(),
	fShell(),
	fGroups()
{
}


BUser::BUser(const BHPKG::BUserData& userData)
	:
	fName(userData.name),
	fRealName(userData.realName),
	fHome(userData.home),
	fShell(userData.shell),
	fGroups()
{
	for (size_t i =	0; i < userData.groupCount; i++)
		fGroups.Add(userData.groups[i]);
}


BUser::BUser(const BString& name, const BString& realName, const BString& home,
	const BString& shell, const BStringList& groups)
	:
	fName(name),
	fRealName(realName),
	fHome(home),
	fShell(shell),
	fGroups(groups)
{
}


BUser::~BUser()
{
}


status_t
BUser::InitCheck() const
{
	if (fName.IsEmpty())
		return B_NO_INIT;
	if (!IsValidUserName(fName))
		return B_BAD_VALUE;
	return B_OK;
}


const BString&
BUser::Name() const
{
	return fName;
}


const BString&
BUser::RealName() const
{
	return fRealName;
}


const BString&
BUser::Home() const
{
	return fHome;
}


const BString&
BUser::Shell() const
{
	return fShell;
}


const BStringList&
BUser::Groups() const
{
	return fGroups;
}


status_t
BUser::SetTo(const BString& name, const BString& realName, const BString& home,
	const BString& shell, const BStringList& groups)
{
	fName = name;
	fRealName = realName;
	fHome = home;
	fShell = shell;
	fGroups = groups;

	return fGroups.CountStrings() == groups.CountStrings() ? B_OK : B_NO_MEMORY;
}


/*static*/ bool
BUser::IsValidUserName(const char* name)
{
	if (name[0] == '\0')
		return false;

	for (; name[0] != '\0'; name++) {
		if (!isalnum(name[0]) && name[0] != '_')
			return false;
	}

	return true;
}


}	// namespace BPackageKit
