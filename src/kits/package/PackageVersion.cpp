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
	fRelease(data.release)
{
}


BPackageVersion::BPackageVersion(const BString& major, const BString& minor,
	const BString& micro, uint8 release)
	:
	fMajor(major),
	fMinor(minor),
	fMicro(micro),
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


uint8
BPackageVersion::Release() const
{
	return fRelease;
}


int
BPackageVersion::Compare(const BPackageVersion& other) const
{
	int majorDiff = NaturalCompare(fMajor.String(), other.fMajor.String());
	if (majorDiff != 0)
		return majorDiff;

	int minorDiff = NaturalCompare(fMinor.String(), other.fMinor.String());
	if (minorDiff != 0)
		return minorDiff;

	int microDiff = NaturalCompare(fMicro.String(), other.fMicro.String());
	if (microDiff != 0)
		return microDiff;

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

	if (fRelease > 0)
		string << '-' << fRelease;

	return string;
}


void
BPackageVersion::SetTo(const BString& major, const BString& minor,
	const BString& micro, uint8 release)
{
	fMajor = major;
	fMinor = minor;
	fMicro = micro;
	fRelease = release;
}


void
BPackageVersion::Clear()
{
	fMajor.Truncate(0);
	fMinor.Truncate(0);
	fMicro.Truncate(0);
	fRelease = 0;
}


}	// namespace BPackageKit
