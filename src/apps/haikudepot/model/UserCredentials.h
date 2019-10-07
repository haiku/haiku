/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef USER_CREDENTIALS_H
#define USER_CREDENTIALS_H


#include <Archivable.h>
#include <String.h>


/*!	This object represents the tuple of the user's nickname (username) and
	password.  It also carries a boolean that indicates if an authentication
	with these credentials was successful or failed.
*/

class UserCredentials : public BArchivable {
public:
								UserCredentials(BMessage* from);
								UserCredentials(const BString& nickname,
									const BString& passwordClear);
								UserCredentials(const UserCredentials& other);
								UserCredentials();
	virtual						~UserCredentials();

	const	BString&			Nickname() const;
	const	BString&			PasswordClear() const;
	const	bool				IsSuccessful() const;
	const	bool				IsValid() const;

			void				SetNickname(const BString& value);
			void				SetPasswordClear(const BString& value);
			void				SetIsSuccessful(bool value);

			UserCredentials&	operator=(const UserCredentials& other);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fNickname;
			BString				fPasswordClear;
			bool				fIsSuccessful;
};


#endif // USER_CREDENTIALS_H
