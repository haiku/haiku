/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_RESULT_H_
#define _B_HTTP_RESULT_H_

#include <memory>

#include <String.h>


namespace BPrivate {

namespace Network {

struct HttpResultPrivate;


struct BHttpStatus
{ 
	int16			code = 0;
	BString			text;
};


class BHttpResult
{
public:
	// Constructors and destructor
										BHttpResult(const BHttpResult& other) = delete;
										BHttpResult(BHttpResult&& other) noexcept;
										~BHttpResult();

	// Assignment operators
			BHttpResult&				operator=(const BHttpResult& other) = delete;
			BHttpResult&				operator=(BHttpResult&& other) noexcept;

	// Blocking Access Functions
	const	BHttpStatus&				Status() const;
//	BHttpHeaders&						Headers() const;
//	BHttpBody&							Body() const;

	// Check if data is available yet
			bool						HasStatus() const;
			bool						HasHeaders() const;
			bool						HasBody() const;
			bool						IsCompleted() const;

	// Identity
			int32						Identity() const;

private:
	friend class BHttpSession;
										BHttpResult(std::shared_ptr<HttpResultPrivate> data);
	std::shared_ptr<HttpResultPrivate>	fData;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_RESPONSE_H_
