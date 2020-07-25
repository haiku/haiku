/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CAPTCHA_H
#define CAPTCHA_H


#include <Archivable.h>
#include <String.h>

class BPositionIO;

/*!	When a user has to perform some sensitive operation, it is necessary to make
	sure that it is not a 'robot' or software system that is acting as if it
	were a person.  It is necessary to know that a real person is acting.  In
	this case a graphical puzzle is presented to the user that presumably only
	a human operator could solve.  This is called a Captcha.
*/

class Captcha : public BArchivable {
public:
								Captcha(BMessage* from);
								Captcha();
	virtual						~Captcha();

	const	BString&			Token() const;
			BPositionIO*		PngImageData() const;

			void				SetToken(const BString& value);
			void				SetPngImageData(const void* data, size_t len);

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fToken;
			BMallocIO*			fPngImageData;
};


#endif // CAPTCHA_H
