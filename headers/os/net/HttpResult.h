/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_RESULT_H_
#define _B_URL_RESULT_H_


#include <iostream>

#include <DataIO.h>
#include <HttpHeaders.h>
#include <String.h>
#include <Url.h>


class BUrlRequest;


class BHttpResult {
			friend class 				BHttpRequest;
			
public:
										BHttpResult(const BUrl& url);
										BHttpResult(const BHttpResult& other);
										~BHttpResult();
	
	// Result parameters modifications
			void						SetUrl(const BUrl& url);
		
	// Result parameters access
			const BUrl&					Url() const;

	// HTTP-Specific stuff
			const BHttpHeaders&			Headers() const;
			const BString&				StatusText() const;
			int32						StatusCode() const;
	
	// Result tests
			bool						HasHeaders() const;
			
	// Overloaded members
			BHttpResult&					operator=(const BHttpResult& other);
									
private:
			BUrl						fUrl;
			
			// TODO: HTTP specific stuff should not live here.
			BHttpHeaders 				fHeaders;
			int32						fStatusCode;
			BString						fStatusString;
};


#endif // _B_URL_RESULT_H_
