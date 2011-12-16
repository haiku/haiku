/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_INFO_H_
#define _PACKAGE__REPOSITORY_INFO_H_


#include <Archivable.h>
#include <Entry.h>
#include <StringList.h>
#include <String.h>

#include <package/PackageArchitecture.h>


namespace BPackageKit {


class BRepositoryInfo : public BArchivable {
	typedef	BArchivable			inherited;

public:
								BRepositoryInfo();
								BRepositoryInfo(BMessage* data);
								BRepositoryInfo(const BEntry& entry);
	virtual						~BRepositoryInfo();

	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

			status_t			SetTo(const BMessage* data);
			status_t			SetTo(const BEntry& entry);
			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		OriginalBaseURL() const;
			const BString&		Vendor() const;
			const BString&		Summary() const;
			uint8				Priority() const;
			BPackageArchitecture	Architecture() const;
			const BStringList&	LicenseNames() const;
			const BStringList&	LicenseTexts() const;

			void				SetName(const BString& name);
			void				SetOriginalBaseURL(const BString& url);
			void				SetVendor(const BString& vendor);
			void				SetSummary(const BString& summary);
			void				SetPriority(uint8 priority);
			void				SetArchitecture(BPackageArchitecture arch);

			status_t			AddLicense(const BString& licenseName,
									const BString& licenseText);
			void				ClearLicenses();

public:
	static	BRepositoryInfo*	Instantiate(BMessage* data);

	static	const uint8			kDefaultPriority;

	static	const char*			kNameField;
	static	const char*			kURLField;
	static	const char*			kVendorField;
	static	const char*			kSummaryField;
	static	const char*			kPriorityField;
	static	const char*			kArchitectureField;
	static	const char*			kLicenseNameField;
	static	const char*			kLicenseTextField;

private:
			status_t			fInitStatus;

			BString				fName;
			BString				fOriginalBaseURL;
			BString				fVendor;
			BString				fSummary;
			uint8				fPriority;
			BPackageArchitecture	fArchitecture;
			BStringList			fLicenseNames;
			BStringList			fLicenseTexts;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_INFO_H_
