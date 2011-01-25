/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_CONFIG_H_
#define _PACKAGE__REPOSITORY_CONFIG_H_


#include <Archivable.h>
#include <Entry.h>
#include <String.h>


class BEntry;


namespace BPackageKit {


class BRepositoryConfig : public BArchivable {
	typedef	BArchivable			inherited;

public:
								BRepositoryConfig();
								BRepositoryConfig(const BString& name,
									const BString& url,
									uint8 priority = kDefaultPriority);
								BRepositoryConfig(const BEntry& entry);
								BRepositoryConfig(BMessage* data);
	virtual						~BRepositoryConfig();

	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

			status_t			StoreAsConfigFile(const BEntry& entry) const;

			status_t			SetTo(const BEntry& entry);
			status_t			SetTo(const BMessage* data);
			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		URL() const;
			uint8				Priority() const;
			bool				IsUserSpecific() const;

			const BEntry&		Entry() const;

			void				SetName(const BString& name);
			void				SetURL(const BString& url);
			void				SetPriority(uint8 priority);
			void				SetIsUserSpecific(bool isUserSpecific);

public:
	static	BRepositoryConfig*	Instantiate(BMessage* data);

	static	const uint8			kDefaultPriority;
	static	const char*			kNameField;
	static	const char*			kURLField;
	static	const char*			kPriorityField;

private:
			status_t			fInitStatus;

			BString				fName;
			BString				fURL;
			uint8				fPriority;
			bool				fIsUserSpecific;

			BEntry				fEntry;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CONFIG_H_
