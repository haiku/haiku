/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DATA_H
#define PACKAGE_DATA_H


#include <package/hpkg/PackageData.h>

#include <string.h>


typedef BPackageKit::BHPKG::BPackageData PackageDataV2;


class PackageData {
public:
	explicit					PackageData(const PackageDataV2& data);

			uint8				Version() const	{ return fVersion; }
			const PackageDataV2& DataV2() const;

			uint64				UncompressedSize() const;

			bool				IsEncodedInline() const;
			const uint8*		InlineData() const;

private:
	static	const size_t		kDataSize = sizeof(PackageDataV2);

private:
			union {
				char			fData[kDataSize];
				uint64			fAlignmentDummy;
			};
			static const uint8	fVersion = 2;
};


inline
PackageData::PackageData(const PackageDataV2& data)
#if 0
	:
	fVersion(2)
#endif
{
	memcpy(&fData, &data, sizeof(data));
}


inline const PackageDataV2&
PackageData::DataV2() const
{
	return *(PackageDataV2*)&fData;
}


inline uint64
PackageData::UncompressedSize() const
{
	return DataV2().Size();
}


inline bool
PackageData::IsEncodedInline() const
{
	return DataV2().IsEncodedInline();
}


inline const uint8*
PackageData::InlineData() const
{
	return DataV2().InlineData();
}


#endif	// PACKAGE_DATA_H
