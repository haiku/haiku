/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageCoreInfo.h"

#include "Package.h"


PackageCoreInfo::PackageCoreInfo()
	:
	fVersion(),
	fPublisher(),
	fArchitecture(),
	fDepotName()
{
}


PackageCoreInfo::PackageCoreInfo(const PackageCoreInfo& other)
	:
	fVersion(other.fVersion),
	fPublisher(other.fPublisher),
	fArchitecture(other.fArchitecture),
	fDepotName(other.fDepotName)
{
}


PackageCoreInfo::~PackageCoreInfo()
{
}


bool
PackageCoreInfo::operator==(const PackageCoreInfo& other) const
{
	return fVersion == other.fVersion
		&& fPublisher == other.fPublisher
		&& fArchitecture == other.fArchitecture
		&& fDepotName == other.fDepotName;
}


bool
PackageCoreInfo::operator!=(const PackageCoreInfo& other) const
{
	return !(*this == other);
}


const PackageVersionRef&
PackageCoreInfo::Version() const
{
	return fVersion;
}


const PackagePublisherInfoRef&
PackageCoreInfo::Publisher() const
{
	return fPublisher;
}


const BString
PackageCoreInfo::Architecture() const
{
	return fArchitecture;
}


const BString&
PackageCoreInfo::DepotName() const
{
	return fDepotName;
}


void
PackageCoreInfo::SetVersion(PackageVersionRef value)
{
	fVersion = value;
}


void
PackageCoreInfo::SetPublisher(PackagePublisherInfoRef value)
{
	fPublisher = value;
}


void
PackageCoreInfo::SetArchitecture(const BString& value)
{
	fArchitecture = value;
}


void
PackageCoreInfo::SetDepotName(const BString& value)
{
	fDepotName = value;
}


// #pragma mark - PackageCoreInfoBuilder


PackageCoreInfoBuilder::PackageCoreInfoBuilder()
	:
	fVersion(),
	fPublisher(),
	fArchitecture(),
	fDepotName()
{
}


PackageCoreInfoBuilder::PackageCoreInfoBuilder(const PackageCoreInfoRef& other)
	:
	fVersion(),
	fPublisher(),
	fArchitecture(),
	fDepotName()
{
	fSource = other;
}


PackageCoreInfoBuilder::~PackageCoreInfoBuilder()
{
}


void
PackageCoreInfoBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource.Get());
		fSource.Unset();
	}
}


void
PackageCoreInfoBuilder::_Init(const PackageCoreInfo* other)
{
	fVersion = other->Version();
	fPublisher = other->Publisher();
	fArchitecture = other->Architecture();
	fDepotName = other->DepotName();
}


PackageCoreInfoRef
PackageCoreInfoBuilder::BuildRef()
{
	if (fSource.IsSet())
		return fSource;

	PackageCoreInfo* coreInfo = new PackageCoreInfo();
	coreInfo->SetVersion(fVersion);
	coreInfo->SetPublisher(fPublisher);
	coreInfo->SetArchitecture(fArchitecture);
	coreInfo->SetDepotName(fDepotName);

	return PackageCoreInfoRef(coreInfo, true);
}


PackageCoreInfoBuilder&
PackageCoreInfoBuilder::WithVersion(PackageVersionRef value)
{
	if (!fSource.IsSet() || fSource->Version() != value) {
		_InitFromSource();
		fVersion = value;
	}
	return *this;
}


PackageCoreInfoBuilder&
PackageCoreInfoBuilder::WithPublisher(PackagePublisherInfoRef value)
{
	if (!fSource.IsSet() || fSource->Publisher() != value) {
		_InitFromSource();
		fPublisher = value;
	}
	return *this;
}


PackageCoreInfoBuilder&
PackageCoreInfoBuilder::WithArchitecture(const BString& value)
{
	if (!fSource.IsSet() || fSource->Architecture() != value) {
		_InitFromSource();
		fArchitecture = value;
	}
	return *this;
}


PackageCoreInfoBuilder&
PackageCoreInfoBuilder::WithDepotName(const BString& value)
{
	if (!fSource.IsSet() || fSource->DepotName() != value) {
		_InitFromSource();
		fDepotName = value;
	}
	return *this;
}
