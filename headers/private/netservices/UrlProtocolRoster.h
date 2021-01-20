/*
 * Copyright 2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_URL_ROSTER_H_
#define _B_URL_ROSTER_H_


#include <stdlib.h>


class BUrl;
class BUrlContext;
class BUrlProtocolListener;
class BUrlRequest;

class BUrlProtocolRoster {
public:
    static  BUrlRequest*    MakeRequest(const BUrl& url,
		                        BUrlProtocolListener* listener = NULL,
                                BUrlContext* context = NULL);
};

#endif
