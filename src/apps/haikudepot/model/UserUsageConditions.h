/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_USAGE_CONDITIONS_H
#define USER_USAGE_CONDITIONS_H


#include <Archivable.h>
#include <String.h>


/*!	A user in the HDS system should have agreed to user usage conditions when
	they created their user on the server.  This object represents the user
	usage conditions that either they have agreed to or that they could agree
	to.  Each set of user usage conditions has a code that uniquely identifies
	a given set of conditions.
*/

class UserUsageConditions : public BArchivable {
public:
								UserUsageConditions(BMessage* from);
								UserUsageConditions();
	virtual						~UserUsageConditions();

	const	BString&			Code() const;
	const	uint8				MinimumAge() const;
	const	BString&			CopyMarkdown() const;

			void				SetCode(const BString& code);
			void				SetMinimumAge(uint8 age);
			void				SetCopyMarkdown(const BString& copyMarkdown);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fCode;
			BString				fCopyMarkdown;
			uint8				fMinimumAge;
};


#endif // USER_USAGE_CONDITIONS_H
