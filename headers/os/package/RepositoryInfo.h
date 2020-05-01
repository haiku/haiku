/*
 * Copyright 2011-2018, Haiku, Inc.
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
			const BString&		BaseURL() const;
			const BString&		Identifier() const;
			const BString&		Vendor() const;
			const BString&		Summary() const;
			uint8				Priority() const;
			BPackageArchitecture	Architecture() const;
			const BStringList&	LicenseNames() const;
			const BStringList&	LicenseTexts() const;

			void				SetName(const BString& name);
			void				SetBaseURL(const BString& url);
			void				SetIdentifier(const BString& url);
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

	static	const char* const	kNameField;
	static	const char* const	kURLField;
	static	const char* const	kIdentifierField;
	static	const char* const	kBaseURLField;
	static	const char* const	kVendorField;
	static	const char*	const	kSummaryField;
	static	const char*	const	kPriorityField;
	static	const char*	const	kArchitectureField;
	static	const char*	const	kLicenseNameField;
	static	const char*	const	kLicenseTextField;

private:
			status_t			_SetTo(const BMessage* data);
			status_t			_SetTo(const BEntry& entry);

private:
			status_t			fInitStatus;

			BString				fName;
			BString				fBaseURL;
				// This is the URL to a single mirror.  This field is optional.
			BString				fIdentifier;
				// This is an identifier in the form of an URI for the
				// repository, that applies across mirrors. Good choices of
				// URI schemes are tag: and uuid:, for example.
			BString				fVendor;
			BString				fSummary;
			uint8				fPriority;
			BPackageArchitecture	fArchitecture;
			BStringList			fLicenseNames;
			BStringList			fLicenseTexts;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_INFO_H_
