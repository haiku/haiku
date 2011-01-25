/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_HEADER_H_
#define _PACKAGE__REPOSITORY_HEADER_H_


#include <Archivable.h>
#include <String.h>


namespace BPackageKit {


class BRepositoryHeader : public BArchivable {
	typedef	BArchivable			inherited;

public:
								BRepositoryHeader();
								BRepositoryHeader(BMessage* data);
	virtual						~BRepositoryHeader();

	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

			status_t			SetTo(const BMessage* data);
			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		OriginalBaseURL() const;
			const BString&		Vendor() const;
			const BString&		ShortDescription() const;
			const BString&		LongDescription() const;
			uint8				Priority() const;

			void				SetName(const BString& name);
			void				SetOriginalBaseURL(const BString& url);
			void				SetVendor(const BString& vendor);
			void				SetShortDescription(const BString& description);
			void				SetLongDescription(const BString& description);
			void				SetPriority(uint8 priority);

public:
	static	BRepositoryHeader*	Instantiate(BMessage* data);

	static	const uint8			kDefaultPriority;

	static	const char*			kNameField;
	static	const char*			kURLField;
	static	const char*			kVendorField;
	static	const char*			kShortDescriptionField;
	static	const char*			kLongDescriptionField;
	static	const char*			kPriorityField;

private:
			status_t			fInitStatus;

			BString				fName;
			BString				fOriginalBaseURL;
			BString				fVendor;
			BString				fShortDescription;
			BString				fLongDescription;
			uint8				fPriority;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_HEADER_H_
