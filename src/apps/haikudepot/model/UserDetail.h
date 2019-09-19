/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_DETAIL_H
#define USER_DETAIL_H

#include <stdio.h>

#include <Archivable.h>
#include <String.h>

#include "DateTime.h"


class UserUsageConditionsAgreement : public BArchivable {
public:
								UserUsageConditionsAgreement(BMessage* from);
								UserUsageConditionsAgreement();
	virtual						~UserUsageConditionsAgreement();

	const	BString&			Code() const;
	const	uint64				TimestampAgreed() const;
	const	bool				IsLatest() const;

			void				SetCode(const BString& value);
			void				SetTimestampAgreed(uint64 value);
			void				SetIsLatest(const bool value);

			UserUsageConditionsAgreement&
								operator=(
									const UserUsageConditionsAgreement& other);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fCode;
			uint64				fTimestampAgreed;
				// milliseconds since epoc
			bool				fIsLatest;
};


/*! This objects represents a user in the HaikuDepotServer system.
 */

class UserDetail : public BArchivable {
public:
								UserDetail(BMessage* from);
								UserDetail();
	virtual						~UserDetail();

	const	BString&			Nickname() const;
	const	UserUsageConditionsAgreement&
								Agreement() const;

			void				SetNickname(const BString& value);
			void				SetAgreement(
									const UserUsageConditionsAgreement& value);

			UserDetail&			operator=(const UserDetail& other);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fNickname;
			UserUsageConditionsAgreement
								fAgreement;
};


#endif // USER_DETAIL_H
