/*
 * Copyright 2021 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <NetServicesDefs.h>

using namespace BPrivate::Network;


BUnsupportedProtocol::BUnsupportedProtocol(const char* origin,
		BUrl url, BStringList supportedProtocols)
	:
	BError(origin),
	fUrl(std::move(url)),
	fSupportedProtocols(std::move(supportedProtocols))
{

}


BUnsupportedProtocol::BUnsupportedProtocol(BString origin,
		BUrl url, BStringList supportedProtocols)
	:
	BError(std::move(origin)),
	fUrl(std::move(url)),
	fSupportedProtocols(std::move(supportedProtocols))
{

}


const char*
BUnsupportedProtocol::Message() const noexcept
{
	return "Unsupported Protocol";
}


const BUrl&
BUnsupportedProtocol::Url() const
{
	return fUrl;
}


const BStringList&
BUnsupportedProtocol::SupportedProtocols() const
{
	return fSupportedProtocols;
}


BInvalidUrl::BInvalidUrl(const char* origin, BUrl url)
	:
	BError(origin),
	fUrl(std::move(url))
{

}


BInvalidUrl::BInvalidUrl(BString origin, BUrl url)
	:
	BError(std::move(origin)),
	fUrl(std::move(origin))
{

}


const char*
BInvalidUrl::Message() const noexcept
{
	return "Invalid URL";
}


const BUrl&
BInvalidUrl::Url() const
{
	return fUrl;
}
