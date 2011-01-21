/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HAIKU__PACKAGE__REPOSITORY_CONFIG_H_
#define _HAIKU__PACKAGE__REPOSITORY_CONFIG_H_


#include <Archivable.h>
#include <String.h>


class BEntry;


namespace Haiku {

namespace Package {


class RepositoryConfig : public BArchivable {
	typedef	BArchivable			inherited;

public:
								RepositoryConfig();
								RepositoryConfig(const BString& name,
									const BString& url,
									uint8 priority = kDefaultPriority);
								RepositoryConfig(const BEntry& entry);
								RepositoryConfig(BMessage* data);
	virtual						~RepositoryConfig();

	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

			status_t			StoreAsConfigFile(const BEntry& entry) const;

			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		URL() const;
			uint8				Priority() const;
			bool				IsUserSpecific() const;

			void				SetName(const BString& name);
			void				SetURL(const BString& url);
			void				SetPriority(uint8 priority);
			void				SetIsUserSpecific(bool isUserSpecific);

public:
	static	RepositoryConfig*	Instantiate(BMessage* data);

	static	const uint8			kDefaultPriority = 50;
	static	const char*			kNameField;
	static	const char*			kURLField;
	static	const char*			kPriorityField;

private:
			status_t			_InitFrom(const BEntry& entry);
			status_t			_InitFrom(const BMessage* data);

			status_t			fInitStatus;

			BString				fName;
			BString				fURL;
			uint8				fPriority;
			bool				fIsUserSpecific;
};


}	// namespace Package

}	// namespace Haiku


#endif // _HAIKU__PACKAGE__REPOSITORY_CONFIG_H_
