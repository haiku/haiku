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

#include <iostream>
const BHttpStatus&
BHttpResult::Status() const
{
	if (!fData)
		throw BRuntimeError(__PRETTY_FUNCTION__, "The BHttpResult object is no longer valid");
	status_t status = B_OK;
	while (status == B_INTERRUPTED || status == B_OK) {
		auto dataStatus = fData->GetStatusAtomic();
		std::cout << "BHttpResult::Status() dataStatus " << dataStatus << std::endl;
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
