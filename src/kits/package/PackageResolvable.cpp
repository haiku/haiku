/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageResolvable.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


namespace BPackageKit {


const char*
BPackageResolvable::kTypeNames[B_PACKAGE_RESOLVABLE_TYPE_ENUM_COUNT] = {
	"",
	"lib",
	"cmd",
	"app",
	"add_on",
};


BPackageResolvable::BPackageResolvable()
	:
	fType(B_PACKAGE_RESOLVABLE_TYPE_DEFAULT)
{
}


BPackageResolvable::BPackageResolvable(const BPackageResolvableData& data)
	:
	fName(data.name),
	fType(data.type),
	fVersion(data.version),
	fCompatibleVersion(data.compatibleVersion)
{
}


BPackageResolvable::BPackageResolvable(const BString& name,
	BPackageResolvableType type, const BPackageVersion& version,
	const BPackageVersion& compatibleVersion)
	:
	fName(name),
	fType(type),
	fVersion(version),
	fCompatibleVersion(compatibleVersion)
{
	fName.ToLower();
}


status_t
BPackageResolvable::InitCheck() const
{
	return fName.Length() > 0 ? B_OK : B_NO_INIT;
}


const BString&
BPackageResolvable::Name() const
{
	return fName;
}


BPackageResolvableType
BPackageResolvable::Type() const
{
	return fType;
}


const BPackageVersion&
BPackageResolvable::Version() const
{
	return fVersion;
}


const BPackageVersion&
BPackageResolvable::CompatibleVersion() const
{
	return fCompatibleVersion;
}


BString
BPackageResolvable::ToString() const
{
	// the type is part of the name
	BString string = fName;

	if (fVersion.InitCheck() == B_OK)
		string << '=' << fVersion.ToString();

	if (fCompatibleVersion.InitCheck() == B_OK)
		string << " compat>=" << fCompatibleVersion.ToString();

	return string;
}


void
BPackageResolvable::SetTo(const BString& name, BPackageResolvableType type,
	const BPackageVersion& version, const BPackageVersion& compatibleVersion)
{
	fName = name;
	fType = type;
	fVersion = version;
	fCompatibleVersion = compatibleVersion;

	fName.ToLower();
}


void
BPackageResolvable::Clear()
{
	fName.Truncate(0);
	fVersion.Clear();
	fCompatibleVersion.Clear();
}


}	// namespace BPackageKit
