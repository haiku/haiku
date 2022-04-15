/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_RESULT_H_
#define _B_HTTP_RESULT_H_

#include <memory>

#include <String.h>

class BDataIO;


namespace BPrivate {

namespace Network {

class BHttpFields;
struct HttpResultPrivate;


struct BHttpBody
{
	std::unique_ptr<BDataIO>			target = nullptr;
	BString								text;
};


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
	const	BHttpFields&				Fields() const;
			BHttpBody&					Body() const;

	// Check if data is available yet
			bool						HasStatus() const;
			bool						HasFields() const;
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
