/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_RATING_H
#define USER_RATING_H


#include <Referenceable.h>

#include "UserInfo.h"


class UserRating : public BReferenceable {
public:
								UserRating();
								UserRating(const UserInfo& userInfo,
									float rating,
									const BString& comment,
									const BString& languageId,
									const BString& packageVersion,
									uint64 createTimestamp);
								UserRating(const UserRating& other);

			bool				operator==(const UserRating& other) const;
			bool				operator!=(const UserRating& other) const;

			const UserInfo&		User() const
									{ return fUserInfo; }
			const BString&		Comment() const
									{ return fComment; }
			const BString&		LanguageId() const
									{ return fLanguageId; }
			const float			Rating() const
									{ return fRating; }
			const BString&		PackageVersion() const
									{ return fPackageVersion; }
			const uint64		CreateTimestamp() const
									{ return fCreateTimestamp; }
private:
			UserInfo			fUserInfo;
			float				fRating;
			BString				fComment;
			BString				fLanguageId;
			BString				fPackageVersion;
			uint64				fCreateTimestamp;
				// milliseconds since epoch
};


typedef BReference<UserRating> UserRatingRef;


#endif // USER_RATING_H
