/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <ErrorsExt.h>
#include <HttpFields.h>
#include <HttpResult.h>

#include "HttpResultPrivate.h"

using namespace BPrivate::Network;


// #pragma mark -- BHttpStatus


BHttpStatusClass
BHttpStatus::StatusClass() const noexcept
{
	switch (code / 100) {
		case 1:
			return BHttpStatusClass::Informational;
		case 2:
			return BHttpStatusClass::Success;
		case 3:
			return BHttpStatusClass::Redirection;
		case 4:
			return BHttpStatusClass::ClientError;
		case 5:
			return BHttpStatusClass::ServerError;
		default:
			break;
	}
	return BHttpStatusClass::Invalid;
}


BHttpStatusCode
BHttpStatus::StatusCode() const noexcept
{
	switch (static_cast<BHttpStatusCode>(code)) {
		// 1xx
		case BHttpStatusCode::Continue:
			[[fallthrough]];
		case BHttpStatusCode::SwitchingProtocols:
			[[fallthrough]];

		// 2xx
		case BHttpStatusCode::Ok:
			[[fallthrough]];
		case BHttpStatusCode::Created:
			[[fallthrough]];
		case BHttpStatusCode::Accepted:
			[[fallthrough]];
		case BHttpStatusCode::NonAuthoritativeInformation:
			[[fallthrough]];
		case BHttpStatusCode::NoContent:
			[[fallthrough]];
		case BHttpStatusCode::ResetContent:
			[[fallthrough]];
		case BHttpStatusCode::PartialContent:
			[[fallthrough]];

		// 3xx
		case BHttpStatusCode::MultipleChoice:
			[[fallthrough]];
		case BHttpStatusCode::MovedPermanently:
			[[fallthrough]];
		case BHttpStatusCode::Found:
			[[fallthrough]];
		case BHttpStatusCode::SeeOther:
			[[fallthrough]];
		case BHttpStatusCode::NotModified:
			[[fallthrough]];
		case BHttpStatusCode::UseProxy:
			[[fallthrough]];
		case BHttpStatusCode::TemporaryRedirect:
			[[fallthrough]];
		case BHttpStatusCode::PermanentRedirect:
			[[fallthrough]];

		// 4xx
		case BHttpStatusCode::BadRequest:
			[[fallthrough]];
		case BHttpStatusCode::Unauthorized:
			[[fallthrough]];
		case BHttpStatusCode::PaymentRequired:
			[[fallthrough]];
		case BHttpStatusCode::Forbidden:
			[[fallthrough]];
		case BHttpStatusCode::NotFound:
			[[fallthrough]];
		case BHttpStatusCode::MethodNotAllowed:
			[[fallthrough]];
		case BHttpStatusCode::NotAcceptable:
			[[fallthrough]];
		case BHttpStatusCode::ProxyAuthenticationRequired:
			[[fallthrough]];
		case BHttpStatusCode::RequestTimeout:
			[[fallthrough]];
		case BHttpStatusCode::Conflict:
			[[fallthrough]];
		case BHttpStatusCode::Gone:
			[[fallthrough]];
		case BHttpStatusCode::LengthRequired:
			[[fallthrough]];
		case BHttpStatusCode::PreconditionFailed:
			[[fallthrough]];
		case BHttpStatusCode::RequestEntityTooLarge:
			[[fallthrough]];
		case BHttpStatusCode::RequestUriTooLarge:
			[[fallthrough]];
		case BHttpStatusCode::UnsupportedMediaType:
			[[fallthrough]];
		case BHttpStatusCode::RequestedRangeNotSatisfiable:
			[[fallthrough]];
		case BHttpStatusCode::ExpectationFailed:
			[[fallthrough]];

		// 5xx
		case BHttpStatusCode::InternalServerError:
			[[fallthrough]];
		case BHttpStatusCode::NotImplemented:
			[[fallthrough]];
		case BHttpStatusCode::BadGateway:
			[[fallthrough]];
		case BHttpStatusCode::ServiceUnavailable:
			[[fallthrough]];
		case BHttpStatusCode::GatewayTimeout:
			return static_cast<BHttpStatusCode>(code);

		default:
			break;
	}

	return BHttpStatusCode::Unknown;
}


// #pragma mark -- BHttpResult


/*private*/
BHttpResult::BHttpResult(std::shared_ptr<HttpResultPrivate> data)
	:
	fData(data)
{
}


BHttpResult::BHttpResult(BHttpResult&& other) noexcept = default;


BHttpResult::~BHttpResult()
{
	if (fData)
		fData->SetCancel();
}


BHttpResult& BHttpResult::operator=(BHttpResult&& other) noexcept = default;


const BHttpStatus&
BHttpResult::Status() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		if (dataStatus == HttpResultPrivate::kError)
			std::rethrow_exception(*(fData->error));

		if (dataStatus >= HttpResultPrivate::kStatusReady)
			return fData->status.value();

		status = acquire_sem(fData->data_wait);
	}
	throw BRuntimeError(__PRETTY_FUNCTION__, "Unexpected error waiting for status!");
}


const BHttpFields&
BHttpResult::Fields() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		if (dataStatus == HttpResultPrivate::kError)
			std::rethrow_exception(*(fData->error));

		if (dataStatus >= HttpResultPrivate::kHeadersReady)
			return *(fData->fields);

		status = acquire_sem(fData->data_wait);
	}
	throw BRuntimeError(__PRETTY_FUNCTION__, "Unexpected error waiting for fields!");
}


BHttpBody&
BHttpResult::Body() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		if (dataStatus == HttpResultPrivate::kError)
			std::rethrow_exception(*(fData->error));

		if (dataStatus >= HttpResultPrivate::kBodyReady)
			return *(fData->body);

		status = acquire_sem(fData->data_wait);
	}
	throw BRuntimeError(__PRETTY_FUNCTION__, "Unexpected error waiting for the body!");
}


bool
BHttpResult::HasStatus() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kStatusReady;
}


bool
BHttpResult::HasFields() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kHeadersReady;
}


bool
BHttpResult::HasBody() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kBodyReady;
}


bool
BHttpResult::IsCompleted() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	return HasBody();
}


int32
BHttpResult::Identity() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	return fData->id;
}
