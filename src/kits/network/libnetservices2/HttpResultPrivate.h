/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#ifndef _HTTP_RESULT_PRIVATE_H_
#define _HTTP_RESULT_PRIVATE_H_


#include <memory>
#include <optional>
#include <string>

#include <DataIO.h>
#include <ExclusiveBorrow.h>
#include <OS.h>
#include <String.h>


namespace BPrivate {

namespace Network {

struct HttpResultPrivate {
	// Read-only properties (multi-thread safe)
	const	int32				id;

	// Locking and atomic variables
	enum { kNoData = 0, kStatusReady, kHeadersReady, kBodyReady, kError };
			int32				requestStatus = kNoData;
			int32				canCancel = 0;
			sem_id				data_wait;

	// Data
			std::optional<BHttpStatus> status;
			std::optional<BHttpFields> fields;
			std::optional<BHttpBody> body;
			std::optional<std::exception_ptr> error;

	// Interim body storage (used while the request is running)
			BString				bodyString;
			BBorrow<BDataIO>	bodyTarget;

	// Utility functions
								HttpResultPrivate(int32 identifier);
			int32				GetStatusAtomic();
			bool				CanCancel();
			void				SetCancel();
			void				SetError(std::exception_ptr e);
			void				SetStatus(BHttpStatus&& s);
			void				SetFields(BHttpFields&& f);
			void				SetBody();
			size_t				WriteToBody(const void* buffer, size_t size);
};


inline HttpResultPrivate::HttpResultPrivate(int32 identifier)
	:
	id(identifier)
{
	std::string name = "httpresult:" + std::to_string(identifier);
	data_wait = create_sem(1, name.c_str());
	if (data_wait < B_OK)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Cannot create internal sem for httpresult");
}


inline int32
HttpResultPrivate::GetStatusAtomic()
{
	return atomic_get(&requestStatus);
}


inline bool
HttpResultPrivate::CanCancel()
{
	return atomic_get(&canCancel) == 1;
}


inline void
HttpResultPrivate::SetCancel()
{
	atomic_set(&canCancel, 1);
}


inline void
HttpResultPrivate::SetError(std::exception_ptr e)
{
	// Release any held body target borrow
	bodyTarget.Return();

	error = e;
	atomic_set(&requestStatus, kError);
	release_sem(data_wait);
}


inline void
HttpResultPrivate::SetStatus(BHttpStatus&& s)
{
	status = std::move(s);
	atomic_set(&requestStatus, kStatusReady);
	release_sem(data_wait);
}


inline void
HttpResultPrivate::SetFields(BHttpFields&& f)
{
	fields = std::move(f);
	atomic_set(&requestStatus, kHeadersReady);
	release_sem(data_wait);
}


inline void
HttpResultPrivate::SetBody()
{
	if (bodyTarget.HasValue()) {
		body = BHttpBody{};
		bodyTarget.Return();
	} else
		body = BHttpBody{std::move(bodyString)};

	atomic_set(&requestStatus, kBodyReady);
	release_sem(data_wait);
}


inline size_t
HttpResultPrivate::WriteToBody(const void* buffer, size_t size)
{
	// TODO: when the support for a shared BMemoryRingIO is here, choose
	// between one or the other depending on which one is available.
	if (bodyTarget.HasValue()) {
		auto result = bodyTarget->Write(buffer, size);
		if (result < 0)
			throw BSystemError("BDataIO::Write()", result);
		return result;
	} else {
		bodyString.Append(reinterpret_cast<const char*>(buffer), size);
		return size;
	}
}


} // namespace Network

} // namespace BPrivate

#endif // _HTTP_RESULT_PRIVATE_H_
