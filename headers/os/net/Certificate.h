/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CERTIFICATE_H
#define _CERTIFICATE_H


#include <SecureSocket.h>
#include <String.h>


class BCertificate {
public:
				~BCertificate();

	BString		String();

	bigtime_t	StartDate();
	bigtime_t	ExpirationDate();
	BString		Issuer();
	BString		Subject();

private:
	friend class BSecureSocket::Private;
	class Private;
				BCertificate(Private* data);

	Private*	fPrivate;
};

#endif
