/*
 * Copyright 2013-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Exception.h"


using namespace BPackageKit;


Exception::Exception(BTransactionError error)
	:
	fError(error),
	fSystemError(B_ERROR),
	fPackageName(),
	fPath1(),
	fPath2(),
	fString1(),
	fString2()
{
}

Exception&
Exception::SetSystemError(status_t error)
{
	fSystemError = error;
	return *this;
}


Exception&
Exception::SetPackageName(const BString& packageName)
{
	fPackageName = packageName;
	return *this;
}


Exception&
Exception::SetPath1(const BString& path)
{
	fPath1 = path;
	return *this;
}


Exception&
Exception::SetPath2(const BString& path)
{
	fPath2 = path;
	return *this;
}


Exception&
Exception::SetString1(const BString& string)
{
	fString1 = string;
	return *this;
}


Exception&
Exception::SetString2(const BString& string)
{
	fString2 = string;
	return *this;
}


void
Exception::SetOnResult(BCommitTransactionResult& result)
{
	result.SetError(fError);
	result.SetSystemError(fSystemError);
	result.SetErrorPackage(fPackageName);
	result.SetPath1(fPath1);
	result.SetPath2(fPath2);
	result.SetString1(fString1);
	result.SetString2(fString2);
}
