/*
 * Copyright 2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#ifndef _B_DATA_REQUEST_H_
#define _B_DATA_REQUEST_H_


#include <UrlRequest.h>


class BDataRequest: public BUrlRequest {
public:
							BDataRequest(const BUrl& url,
								BUrlProtocolListener* listener = NULL,
								BUrlContext* context = NULL);
		const BUrlResult&	Result() const;
private:
		status_t			_ProtocolLoop();	
private:
		BUrlResult			fResult;
};

#endif
