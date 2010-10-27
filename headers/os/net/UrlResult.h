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


class BUrlProtocol;


class BUrlResult {
			friend class 		BUrlProtocol;
			
public:
								BUrlResult(const BUrl& url);
								BUrlResult(const BUrlResult& other);
	
	// Result parameters modifications
			void				SetUrl(const BUrl& url);
		
	// Result parameters access
			const BUrl&			Url() const;
			const BMallocIO&	RawData() const;			
			const BHttpHeaders&	Headers() const;
			const BString&		StatusText() const;
			int32				StatusCode() const;
	
	// Result tests
			bool				HasHeaders() const;
			
	// Overloaded members
			BUrlResult&			operator=(const BUrlResult& other);
	friend 	std::ostream& 		operator<<(std::ostream& out,
									const BUrlResult& result);
									
private:
			BUrl				fUrl;
			BMallocIO			fRawData;
			BHttpHeaders 		fHeaders;
				// TODO: HTTP specific stuff should not live here.

			int32				fStatusCode;
			BString				fStatusString;
};


#endif // _B_URL_RESULT_H_
