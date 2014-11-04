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

	int			Version();

	time_t		StartDate();
	time_t		ExpirationDate();

	bool		IsValidAuthority();
	bool		IsSelfSigned();

	BString		Issuer();
	BString		Subject();
	BString		SignatureAlgorithm();

	BString		String();

private:
	friend class BSecureSocket::Private;
	class Private;
				BCertificate(Private* data);

				BCertificate(const BCertificate& other);
					// copy-construction not allowed

	Private*	fPrivate;
};

#endif
