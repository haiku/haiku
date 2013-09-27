/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageVersion.h>

#include <NaturalCompare.h>

#include <package/PackageInfo.h>
#include <package/hpkg/PackageInfoAttributeValue.h>


using BPrivate::NaturalCompare;


namespace BPackageKit {


BPackageVersion::BPackageVersion()
	:
	fRevision(0)
{
}


BPackageVersion::BPackageVersion(const BPackageVersionData& data)
{
	SetTo(data.major, data.minor, data.micro, data.preRelease, data.revision);
}


BPackageVersion::BPackageVersion(const BString& versionString,
	bool revisionIsOptional)
{
	SetTo(versionString, revisionIsOptional);
}


BPackageVersion::BPackageVersion(const BString& major, const BString& minor,
	const BString& micro, const BString& preRelease, uint32 revision)
{
	SetTo(major, minor, micro, preRelease, revision);
}


status_t
BPackageVersion::InitCheck() const
{
	return fMajor.Length() > 0 ? B_OK : B_NO_INIT;
}


const BString&
BPackageVersion::Major() const
{
	return fMajor;
}


const BString&
BPackageVersion::Minor() const
{
	return fMinor;
}


const BString&
BPackageVersion::Micro() const
{
	return fMicro;
}


const BString&
BPackageVersion::PreRelease() const
{
	return fPreRelease;
}


uint32
BPackageVersion::Revision() const
{
	return fRevision;
}


int
BPackageVersion::Compare(const BPackageVersion& other) const
{
	int diff = NaturalCompare(fMajor.String(), other.fMajor.String());
	if (diff != 0)
		return diff;

	diff = NaturalCompare(fMinor.String(), other.fMinor.String());
	if (diff != 0)
		return diff;

	diff = NaturalCompare(fMicro.String(), other.fMicro.String());
	if (diff != 0)
		return diff;

	// The pre-version works differently: The empty string is greater than any
	// non-empty string (e.g. "R1" is newer than "R1-rc2"). So we catch the
	// empty string cases first.
	if (fPreRelease.IsEmpty()) {
		if (!other.fPreRelease.IsEmpty())
			return 1;
	} else if (other.fPreRelease.IsEmpty()) {
		return -1;
	} else {
		// both are non-null -- compare normally
		diff = NaturalCompare(fPreRelease.String(), other.fPreRelease.String());
		if (diff != 0)
			return diff;
	}

	return (int)fRevision - (int)other.fRevision;
}


BString
BPackageVersion::ToString() const
{
	if (InitCheck() != B_OK)
		return BString();

	BString string = fMajor;

	if (fMinor.Length() > 0) {
		string << '.' << fMinor;
		if (fMicro.Length() > 0)
			string << '.' << fMicro;
	}

	if (!fPreRelease.IsEmpty())
		string << '~' << fPreRelease;

	if (fRevision > 0)
		string << '-' << fRevision;

	return string;
}


void
BPackageVersion::SetTo(const BString& major, const BString& minor,
	const BString& micro, const BString& preRelease, uint32 revision)
{
	fMajor = major;
	fMinor = minor;
	fMicro = micro;
	fPreRelease = preRelease;
	fRevision = revision;

	fMajor.ToLower();
	fMinor.ToLower();
	fMicro.ToLower();
	fPreRelease.ToLower();
}


status_t
BPackageVersion::SetTo(const BString& versionString, bool revisionIsOptional)
{
	Clear();
	return BPackageInfo::ParseVersionString(versionString, revisionIsOptional,
		*this);
}


void
BPackageVersion::Clear()
{
	fMajor.Truncate(0);
	fMinor.Truncate(0);
	fMicro.Truncate(0);
	fPreRelease.Truncate(0);
	fRevision = 0;
}


}	// namespace BPackageKit
