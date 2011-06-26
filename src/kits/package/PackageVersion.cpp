/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/PackageVersion.h>

#include <NaturalCompare.h>

#include <package/hpkg/PackageInfoAttributeValue.h>


using BPrivate::NaturalCompare;


namespace BPackageKit {


BPackageVersion::BPackageVersion()
	:
	fRelease(0)
{
}


BPackageVersion::BPackageVersion(const BPackageVersionData& data)
	:
	fMajor(data.major),
	fMinor(data.minor),
	fMicro(data.micro),
	fPreRelease(data.preRelease),
	fRelease(data.release)
{
}


BPackageVersion::BPackageVersion(const BString& major, const BString& minor,
	const BString& micro, const BString& preRelease, uint8 release)
	:
	fMajor(major),
	fMinor(minor),
	fMicro(micro),
	fPreRelease(preRelease),
	fRelease(release)
{
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


uint8
BPackageVersion::Release() const
{
	return fRelease;
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

	return (int)fRelease - (int)other.fRelease;
}


BString
BPackageVersion::ToString() const
{
	BString string = fMajor;

	if (fMinor.Length() > 0) {
		string << '.' << fMinor;
		if (fMicro.Length() > 0)
			string << '.' << fMicro;
	}

	if (!fPreRelease.IsEmpty())
		string << '-' << fPreRelease;

	if (fRelease > 0)
		string << '-' << fRelease;

	return string;
}


void
BPackageVersion::SetTo(const BString& major, const BString& minor,
	const BString& micro, const BString& preRelease, uint8 release)
{
	fMajor = major;
	fMinor = minor;
	fMicro = micro;
	fPreRelease = preRelease;
	fRelease = release;
}


void
BPackageVersion::Clear()
{
	fMajor.Truncate(0);
	fMinor.Truncate(0);
	fMicro.Truncate(0);
	fPreRelease.Truncate(0);
	fRelease = 0;
}


}	// namespace BPackageKit
