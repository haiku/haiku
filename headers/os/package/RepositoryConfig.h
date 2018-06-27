/*
 * Copyright 2011-2018, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__REPOSITORY_CONFIG_H_
#define _PACKAGE__REPOSITORY_CONFIG_H_


#include <Entry.h>
#include <String.h>


namespace BPackageKit {


class BRepositoryConfig {
public:
								BRepositoryConfig();
								BRepositoryConfig(const BString& name,
									const BString& url, uint8 priority);
								BRepositoryConfig(const BEntry& entry);
	virtual						~BRepositoryConfig();

			status_t			Store(const BEntry& entry) const;

			status_t			SetTo(const BEntry& entry);
			status_t			InitCheck() const;

			const BString&		Name() const;
			const BString&		BaseURL() const;
			const BString&		URL() const;
			uint8				Priority() const;
			bool				IsUserSpecific() const;

			const BEntry&		Entry() const;

			BString				PackagesURL() const;

			void				SetName(const BString& name);
			void				SetBaseURL(const BString& url);
			void				SetURL(const BString& url);
			void				SetPriority(uint8 priority);
			void				SetIsUserSpecific(bool isUserSpecific);

public:
	static	const uint8			kUnsetPriority = 0;

private:
			status_t			fInitStatus;

			BString				fName;
			BString				fBaseURL;
				// this URL is the URL that can be used to access the data of
				// the repository - it points to a single mirror.
			BString				fURL;
				// this URL is actually an identifier for the repository
				// that is consistent across mirrors.
			uint8				fPriority;
			bool				fIsUserSpecific;

			BEntry				fEntry;
};


}	// namespace BPackageKit


#endif // _PACKAGE__REPOSITORY_CONFIG_H_
