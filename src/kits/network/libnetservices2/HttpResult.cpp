/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */


#include <ErrorsExt.h>
#include <HttpResult.h>

#include "HttpResultPrivate.h"

using namespace BPrivate::Network;


/*private*/
BHttpResult::BHttpResult(std::shared_ptr<HttpResultPrivate> data)
	: fData(data)
{

}


BHttpResult::BHttpResult(BHttpResult&& other) noexcept = default;


BHttpResult::~BHttpResult()
{
	if (fData)
		fData->SetCancel();
}


BHttpResult&
BHttpResult::operator=(BHttpResult&& other) noexcept = default;


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
			return *(fData->status);

		status = acquire_sem(fData->data_wait);
	}
	throw BRuntimeError(__PRETTY_FUNCTION__, "Unexpected error waiting for status!");
}


bool
BHttpResult::HasStatus() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	return fData->GetStatusAtomic() >= HttpResultPrivate::kStatusReady;
}


bool
BHttpResult::HasHeaders() const
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
