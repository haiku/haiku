/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _NETSERVICES_DEFS_H_
#define _NETSERVICES_DEFS_H_


#include <ErrorsExt.h>
#include <StringList.h>
#include <Url.h>


namespace BPrivate {

namespace Network {


// Standard exceptions
class BUnsupportedProtocol : public BError {
public:
							BUnsupportedProtocol(const char* origin, BUrl url,
								BStringList supportedProtocols);
							BUnsupportedProtocol(BString origin, BUrl url,
								BStringList supportedProtocols);

	virtual	const char*		Message() const noexcept override;

	const	BUrl&			Url() const;
	const	BStringList&	SupportedProtocols() const;

private:
			BUrl			fUrl;
			BStringList		fSupportedProtocols;
};


class BInvalidUrl : public BError {
public:
							BInvalidUrl(const char* origin, BUrl url);
							BInvalidUrl(BString origin, BUrl url);

	virtual	const char*		Message() const noexcept override;

	const	BUrl&			Url() const;

private:
			BUrl			fUrl;
};


}

}

#endif
