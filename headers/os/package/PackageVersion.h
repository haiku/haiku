/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__PACKAGE_VERSION_H_
#define _PACKAGE__PACKAGE_VERSION_H_


#include <String.h>


namespace BPackageKit {


namespace BHPKG {
	class BPackageVersionData;
}
using BHPKG::BPackageVersionData;


class BPackageVersion {
public:
								BPackageVersion();
								BPackageVersion(
									const BPackageVersionData& data);
	explicit					BPackageVersion(const BString& versionString,
									bool revisionIsOptional = true);
								BPackageVersion(const BString& major,
									const BString& minor, const BString& micro,
									const BString& preRelease, uint32 revision);

			status_t			InitCheck() const;

			const BString&		Major() const;
			const BString&		Minor() const;
			const BString&		Micro() const;
			const BString&		PreRelease() const;
									// "alpha3", "beta2", "rc1" or "" if final
			uint32				Revision() const;

			BString				ToString() const;

			void				SetTo(const BString& major,
									const BString& minor, const BString& micro,
									const BString& preRelease, uint32 revision);
			status_t			SetTo(const BString& versionString,
									bool revisionIsOptional = true);
			void				Clear();

			int					Compare(const BPackageVersion& other) const;
									// does a natural compare over major, minor
									// and micro, finally comparing revision

	inline	bool				operator==(const BPackageVersion& other) const;
	inline	bool				operator!=(const BPackageVersion& other) const;
	inline	bool				operator<(const BPackageVersion& other) const;
	inline	bool				operator>(const BPackageVersion& other) const;
	inline	bool				operator<=(const BPackageVersion& other) const;
	inline	bool				operator>=(const BPackageVersion& other) const;

private:
			BString				fMajor;
			BString				fMinor;
			BString				fMicro;
			BString				fPreRelease;
			uint32				fRevision;
};


inline bool
BPackageVersion::operator==(const BPackageVersion& other) const
{
	return Compare(other) == 0;
}


inline bool
BPackageVersion::operator!=(const BPackageVersion& other) const
{
	return Compare(other) != 0;
}


inline bool
BPackageVersion::operator<(const BPackageVersion& other) const
{
	return Compare(other) < 0;
}


inline bool
BPackageVersion::operator>(const BPackageVersion& other) const
{
	return Compare(other) > 0;
}


inline bool
BPackageVersion::operator<=(const BPackageVersion& other) const
{
	return Compare(other) <= 0;
}


inline bool
BPackageVersion::operator>=(const BPackageVersion& other) const
{
	return Compare(other) >= 0;
}


}	// namespace BPackageKit


#endif	// _PACKAGE__PACKAGE_VERSION_H_
