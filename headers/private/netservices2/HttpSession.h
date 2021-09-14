/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_SESSION_H_
#define _B_HTTP_SESSION_H_

#include <memory>

class BUrl;

namespace BPrivate {

namespace Network {


class BHttpSession {
public:
	// Constructors & Destructor
							BHttpSession();
							BHttpSession(const BHttpSession&) noexcept;
							BHttpSession(BHttpSession&&) noexcept = delete;
							~BHttpSession() noexcept;

	// Assignment operators
	BHttpSession&			operator=(const BHttpSession&) noexcept;
	BHttpSession&			operator=(BHttpSession&&) noexcept = delete;

private:
	class Request;
	class Impl;
	std::shared_ptr<Impl>	fImpl;
};


}

}

#endif // _B_HTTP_REQUEST_H_
