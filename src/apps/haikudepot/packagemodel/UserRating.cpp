/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "UserRating.h"


UserRating::UserRating()
	:
	fUserInfo(),
	fRating(0.0f),
	fComment(),
	fLanguageId(),
	fPackageVersion(),
	fCreateTimestamp(0)
{
}


UserRating::UserRating(const UserInfo& userInfo, float rating,
	const BString& comment, const BString& languageId,
	const BString& packageVersion, uint64 createTimestamp)
	:
	fUserInfo(userInfo),
	fRating(rating),
	fComment(comment),
	fLanguageId(languageId),
	fPackageVersion(packageVersion),
	fCreateTimestamp(createTimestamp)
{
}


UserRating::UserRating(const UserRating& other)
	:
	fUserInfo(other.fUserInfo),
	fRating(other.fRating),
	fComment(other.fComment),
	fLanguageId(other.fLanguageId),
	fPackageVersion(other.fPackageVersion),
	fCreateTimestamp(other.fCreateTimestamp)
{
}


bool
UserRating::operator==(const UserRating& other) const
{
	return fUserInfo == other.fUserInfo
		&& fRating == other.fRating
		&& fComment == other.fComment
		&& fLanguageId == other.fLanguageId
		&& fPackageVersion == other.fPackageVersion
		&& fCreateTimestamp == other.fCreateTimestamp;
}


bool
UserRating::operator!=(const UserRating& other) const
{
	return !(*this == other);
}
