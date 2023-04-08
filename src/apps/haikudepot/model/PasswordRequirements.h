/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PASSWORD_REQUIREMENTS_H
#define PASSWORD_REQUIREMENTS_H


#include <Archivable.h>
#include <String.h>


/*!	When a user enters their password there are requirements around that
    password such as the length of the password in characters as well as
    how many digits it must contain.  This class models those
    requirements so that they can be conveyed to the user in the UI.
*/

class PasswordRequirements : public BArchivable {
public:
								PasswordRequirements(BMessage* from);
								PasswordRequirements();
	virtual						~PasswordRequirements();

	const	uint32				MinPasswordLength() const
									{ return fMinPasswordLength; }
	const	uint32				MinPasswordUppercaseChar() const
									{ return fMinPasswordUppercaseChar; }
	const	uint32				MinPasswordDigitsChar() const
									{ return fMinPasswordDigitsChar; }

			void				SetMinPasswordLength(uint32 value);
			void				SetMinPasswordUppercaseChar(uint32 value);
			void				SetMinPasswordDigitsChar(uint32 value);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			uint32				fMinPasswordLength;
			uint32				fMinPasswordUppercaseChar;
			uint32				fMinPasswordDigitsChar;
};

#endif // PASSWORD_REQUIREMENTS_H
