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


void
BPackageResolvableExpression::GetAsString(BString& string) const
{
	string = fName;

	if (fVersion.InitCheck() == B_OK) {
		string << fOperator;
		fVersion.GetAsString(string);
	}
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
