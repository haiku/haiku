/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_FILE_REQUEST_H_
#define _B_FILE_REQUEST_H_


#include <deque>


#include <UrlRequest.h>


class BFileRequest : public BUrlRequest {
public:
								BFileRequest(const BUrl& url,
									BUrlProtocolListener* listener = NULL,
									BUrlContext* context = NULL);
	virtual						~BFileRequest();

	const 	BUrlResult&			Result() const;
            void                SetDisableListener(bool disable);

private:
			status_t			_ProtocolLoop();
private:
			BUrlResult			fResult;
};


#endif // _B_FILE_REQUEST_H_
