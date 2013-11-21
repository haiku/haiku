/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageResolvableExpression.h>

#include <package/hpkg/PackageInfoAttributeValue.h>
#include <package/PackageInfo.h>
#include <package/PackageResolvable.h>


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


BPackageResolvableExpression::BPackageResolvableExpression(
	const BString& expressionString)
	:
	fName(),
	fOperator(B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT),
	fVersion()
{
	SetTo(expressionString);
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
		string << kOperatorNames[fOperator] << fVersion.ToString();

	return string;
}


status_t
BPackageResolvableExpression::SetTo(const BString& expressionString)
{
	fName.Truncate(0);
	fOperator = B_PACKAGE_RESOLVABLE_OP_ENUM_COUNT;
	fVersion.Clear();

	return BPackageInfo::ParseResolvableExpressionString(expressionString,
		*this);
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


bool
BPackageResolvableExpression::Matches(const BPackageVersion& version,
	const BPackageVersion& compatibleVersion) const
{
	// If no particular version is required, we always match.
	if (fVersion.InitCheck() != B_OK)
		return true;

	if (version.InitCheck() != B_OK)
		return false;

	int compare = version.Compare(fVersion);
	bool matches = false;
	switch (fOperator) {
		case B_PACKAGE_RESOLVABLE_OP_LESS:
			matches = compare < 0;
			break;
		case B_PACKAGE_RESOLVABLE_OP_LESS_EQUAL:
			matches = compare <= 0;
			break;
		case B_PACKAGE_RESOLVABLE_OP_EQUAL:
			matches = compare == 0;
			break;
		case B_PACKAGE_RESOLVABLE_OP_NOT_EQUAL:
			matches = compare != 0;
			break;
		case B_PACKAGE_RESOLVABLE_OP_GREATER_EQUAL:
			matches = compare >= 0;
			break;
		case B_PACKAGE_RESOLVABLE_OP_GREATER:
			matches = compare > 0;
			break;
		default:
			break;
	}
	if (!matches)
		return false;

	// Check compatible version. If not specified, the match must be exact.
	// Otherwise fVersion must be >= compatibleVersion.
	if (compatibleVersion.InitCheck() != B_OK)
		return compare == 0;

	// Since compatibleVersion <= version, we can save the comparison, if
	// version <= fVersion.
	return compare <= 0 || compatibleVersion.Compare(fVersion) <= 0;
}


bool
BPackageResolvableExpression::Matches(const BPackageResolvable& provides) const
{
	if (provides.Name() != fName)
		return false;

	return Matches(provides.Version(), provides.CompatibleVersion());
}


}	// namespace BPackageKit
