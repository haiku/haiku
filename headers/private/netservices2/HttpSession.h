/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_SESSION_H_
#define _B_HTTP_SESSION_H_

#include <memory>

#include <ExclusiveBorrow.h>
#include <Messenger.h>

class BUrl;


namespace BPrivate {

namespace Network {

class BHttpRequest;
class BHttpResult;


class BHttpSession
{
public:
	// Constructors & Destructor
								BHttpSession();
								BHttpSession(const BHttpSession&) noexcept;
								BHttpSession(BHttpSession&&) noexcept = delete;
								~BHttpSession() noexcept;

	// Assignment operators
			BHttpSession&		operator=(const BHttpSession&) noexcept;
			BHttpSession&		operator=(BHttpSession&&) noexcept = delete;

	// Requests
			BHttpResult			Execute(BHttpRequest&& request, BBorrow<BDataIO> target = nullptr,
									BMessenger observer = BMessenger());
			void				Cancel(int32 identifier);
			void				Cancel(const BHttpResult& request);

	// Concurrency limits
			void				SetMaxConnectionsPerHost(size_t maxConnections);
			void				SetMaxHosts(size_t maxConnections);

private:
	struct Redirect;
	class Request;
	class Impl;
			std::shared_ptr<Impl> fImpl;
};


namespace UrlEvent {
enum { HttpStatus = '_HST', HttpFields = '_HHF', CertificateError = '_CER', HttpRedirect = '_HRE' };
}


namespace UrlEventData {
extern const char* HttpStatusCode;
extern const char* SSLCertificate;
extern const char* SSLMessage;
extern const char* HttpRedirectUrl;
} // namespace UrlEventData

} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_SESSION_H_
