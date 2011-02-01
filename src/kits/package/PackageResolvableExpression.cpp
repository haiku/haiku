/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageResolvableExpression.h>


namespace BPackageKit {


BPackageResolvableExpression::BPackageResolvableExpression()
{
}


BPackageResolvableExpression::BPackageResolvableExpression(const BString& name,
	const BString& _operator, const BPackageVersion& version)
	:
	fName(name),
	fOperator(_operator),
	fVersion(version)
{
}


status_t
BPackageResolvableExpression::InitCheck() const
{
	if (fName.Length() == 0)
		return B_NO_INIT;

	// either both or none of operator and version must be set
	if (fOperator.Length() == 0 && fVersion.InitCheck() == B_OK)
		return B_NO_INIT;

	if (fOperator.Length() > 0 && fVersion.InitCheck() != B_OK)
		return B_NO_INIT;

	return B_OK;
}


const BString&
BPackageResolvableExpression::Name() const
{
	return fName;
}


const BString&
BPackageResolvableExpression::Operator() const
{
	return fOperator;
}


const BPackageVersion&
BPackageResolvableExpression::Version() const
{
	return fVersion;
}


BString
BPackageResolvableExpression::AsString() const
{
	BString string = fName;

	if (fVersion.InitCheck() == B_OK)
		string << fOperator << fVersion.AsString();

	return string;
}


void
BPackageResolvableExpression::SetTo(const BString& name,
	const BString& _operator, const BPackageVersion& version)
{
	fName = name;
	fOperator = _operator;
	fVersion = version;
}


void
BPackageResolvableExpression::Clear()
{
	fName.Truncate(0);
	fOperator.Truncate(0);
	fVersion.Clear();
}


}	// namespace BPackageKit
