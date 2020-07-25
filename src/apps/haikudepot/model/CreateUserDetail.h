/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CREATE_USER_DETAIL_H
#define CREATE_USER_DETAIL_H


#include <Archivable.h>
#include <String.h>

/*!	The operator may choose to create a User.  In this case, he or she is
	required to supply some details such as the nickname, password, email...
	and those details are then collated and provided to the application server
	(HDS) in order that the User is created in the application server.  This
	model carries all of those details in an object so that they might be
	easily conveyed between methods.
*/

class CreateUserDetail : public BArchivable {
public:
								CreateUserDetail(BMessage* from);
								CreateUserDetail();
	virtual						~CreateUserDetail();

	const	BString&			Nickname() const;
	const	BString&			PasswordClear() const;
			bool				IsPasswordRepeated() const;
	const	BString&			Email() const;
	const	BString&			CaptchaToken() const;
	const	BString&			CaptchaResponse() const;
	const	BString&			LanguageCode() const;
	const	BString&			AgreedToUserUsageConditionsCode() const;

			void				SetNickname(const BString& value);
			void				SetPasswordClear(const BString& value);
			void				SetIsPasswordRepeated(bool value);
			void				SetEmail(const BString& value);
			void				SetCaptchaToken(const BString& value);
			void				SetCaptchaResponse(const BString& value);
			void				SetLanguageCode(const BString& value);
			void				SetAgreedToUserUsageConditionsCode(
									const BString& value);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fNickname;
			BString				fPasswordClear;
			bool				fIsPasswordRepeated;
			BString				fEmail;
			BString				fCaptchaToken;
			BString				fCaptchaResponse;
			BString				fLanguageCode;
			BString				fAgreedUserUsageConditionsCode;
};


#endif // CREATE_USER_DETAIL_H
