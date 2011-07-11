/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageResolvableExpression.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


const char*
BPackageResolvableExpression
::kOperatorNames[B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT] = {
	"<",
	"<=",
	"==",
	"!=",
	">=",
	">",
};


BPackageResolvableExpression::BPackageResolvableExpression()
	:
	fOperator(B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT)
{
}


BPackageResolvableExpression::BPackageResolvableExpression(
	const BPackageResolvableExpressionData& data)
	:
	fName(data.name),
	fOperator(data.op),
	fVersion(data.version)
{
	fName.ToLower();
}


BPackageResolvableExpression::BPackageResolvableExpression(const BString& name,
	BPackageResolvableOperator _operator, const BPackageVersion& version)
	:
	fName(name),
	fOperator(_operator),
	fVersion(version)
{
	fName.ToLower();
}


status_t
BPackageResolvableExpression::InitCheck() const
{
	if (fName.Length() == 0)
		return B_NO_INIT;

	// either both or none of operator and version must be set
	if ((fOperator >= 0 && fOperator < B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT)
		!= (fVersion.InitCheck() == B_OK))
		return B_NO_INIT;

	return B_OK;
}


const BString&
BPackageResolvableExpression::Name() const
{
	return fName;
}


BPackageResolvableOperator
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
BPackageResolvableExpression::ToString() const
{
	BString string = fName;

	if (fVersion.InitCheck() == B_OK)
		string << fOperator << fVersion.ToString();

	return string;
}


void
BPackageResolvableExpression::SetTo(const BString& name,
	BPackageResolvableOperator _operator, const BPackageVersion& version)
{
	fName = name;
	fOperator = _operator;
	fVersion = version;

	fName.ToLower();
}


void
BPackageResolvableExpression::Clear()
{
	fName.Truncate(0);
	fOperator = B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT;
	fVersion.Clear();
}


}	// namespace BPackageKit
