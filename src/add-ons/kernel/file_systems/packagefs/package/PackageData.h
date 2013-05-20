/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DATA_H
#define PACKAGE_DATA_H


#include <package/hpkg/PackageData.h>
#include <package/hpkg/v1/PackageData.h>

#include <string.h>


typedef BPackageKit::BHPKG::BPackageData PackageDataV2;
typedef BPackageKit::BHPKG::V1::BPackageData PackageDataV1;


class PackageData {
public:
	explicit					PackageData(const PackageDataV1& data);
	explicit					PackageData(const PackageDataV2& data);

			uint8				Version() const	{ return fVersion; }
			const PackageDataV1& DataV1() const;
			const PackageDataV2& DataV2() const;

			uint64				CompressedSize() const;
			uint64				UncompressedSize() const;

			bool				IsEncodedInline() const;
			const uint8*		InlineData() const;

private:
	static	const size_t		kDataSize = sizeof(PackageDataV1)
										> sizeof(PackageDataV2)
									? sizeof(PackageDataV1)
									: sizeof(PackageDataV2);

private:
			union {
				char			fData[kDataSize];
				uint64			fAlignmentDummy;
			};
			uint8				fVersion;
};


inline
PackageData::PackageData(const PackageDataV1& data)
	:
	fVersion(1)
{
	memcpy(&fData, &data, sizeof(data));
}


inline
PackageData::PackageData(const PackageDataV2& data)
	:
	fVersion(2)
{
	memcpy(&fData, &data, sizeof(data));
}


inline const PackageDataV1&
PackageData::DataV1() const
{
	return *(PackageDataV1*)&fData;
}


inline const PackageDataV2&
PackageData::DataV2() const
{
	return *(PackageDataV2*)&fData;
}


inline uint64
PackageData::CompressedSize() const
{
	if (fVersion == 1)
		return DataV1().CompressedSize();
	return DataV2().Size();
}


inline uint64
PackageData::UncompressedSize() const
{
	if (fVersion == 1)
		return DataV1().UncompressedSize();
	return DataV2().Size();
}


inline bool
PackageData::IsEncodedInline() const
{
	if (fVersion == 1)
		return DataV1().IsEncodedInline();
	return DataV2().IsEncodedInline();
}


inline const uint8*
PackageData::InlineData() const
{
	if (fVersion == 1)
		return DataV1().InlineData();
	return DataV2().InlineData();
}


#endif	// PACKAGE_DATA_H
