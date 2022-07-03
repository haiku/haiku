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
#include <OS.h> 
#include <String.h>


namespace BPrivate {

namespace Network {

struct HttpResultPrivate {
	// Read-only properties (multi-thread safe)
	const	int32						id;

	// Locking and atomic variables
			sem_id						data_wait;
			enum {
				kNoData = 0,
				kStatusReady,
				kHeadersReady,
				kBodyReady,
				kError
			};
			int32						requestStatus = kNoData;
			int32						canCancel = 0;

	// Data
			std::optional<BHttpStatus>	status;
			std::optional<BHttpFields>	fields;
			std::optional<BHttpBody>	body;
			std::optional<std::exception_ptr>	error;

	// Body storage
			std::unique_ptr<BDataIO>	ownedBody = nullptr;
	//		std::shared_ptr<BMemoryRingIO>	shared_body = nullptr;
			BString						bodyText;

	// Utility functions
										HttpResultPrivate(int32 identifier);
			int32						GetStatusAtomic();
			bool						CanCancel();
			void						SetCancel();
			void						SetError(std::exception_ptr e);
			void						SetStatus(BHttpStatus&& s);
			void						SetFields(BHttpFields&& f);
			void						SetBody();
			size_t						WriteToBody(const void* buffer, size_t size);
};


inline
HttpResultPrivate::HttpResultPrivate(int32 identifier)
	: id(identifier)
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
	body = BHttpBody{std::move(ownedBody), std::move(bodyText)};
	atomic_set(&requestStatus, kBodyReady);
	release_sem(data_wait);
}


inline size_t
HttpResultPrivate::WriteToBody(const void* buffer, size_t size)
{
	// TODO: when the support for a shared BMemoryRingIO is here, choose
	// between one or the other depending on which one is available.
	if (ownedBody == nullptr) {
		bodyText.Append(static_cast<const char*>(buffer), size);
		return size;
	}
	auto result = ownedBody->Write(buffer, size);
	if (result < 0)
		throw BSystemError("BDataIO::Write()", result);
	return result;
}


} // namespace Network

} // namespace BPrivate

#endif // _HTTP_RESULT_PRIVATE_H_
