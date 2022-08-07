/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_SERIALIZER_H_
#define _B_HTTP_SERIALIZER_H_


#include <functional>
#include <optional>

class BDataIO;

namespace BPrivate {

namespace Network {

class BHttpRequest;
class HttpBuffer;

using HttpTransferFunction = std::function<size_t (const std::byte*, size_t)>;


enum class HttpSerializerState {
	Uninitialized,
	Header,
	ChunkHeader,
	Body,
	Done
};


class HttpSerializer {
public:
							HttpSerializer() {};

	void					SetTo(HttpBuffer& buffer, const BHttpRequest& request);
	bool					IsInitialized() const noexcept { return fState != HttpSerializerState::Uninitialized; }

	size_t					Serialize(HttpBuffer& buffer, BDataIO* target);

	std::optional<off_t>	BodyBytesTotal() const noexcept { return fBodySize; };
	off_t					BodyBytesTransferred() const noexcept { return fTransferredBodySize; };
	bool					Complete() const noexcept { return fState == HttpSerializerState::Done; };

private:
	bool					_IsChunked() const noexcept;
	size_t					_WriteToTarget(HttpBuffer& buffer, BDataIO* target) const;

private:
	HttpSerializerState		fState = HttpSerializerState::Uninitialized;
	BDataIO*				fBody = nullptr;
	off_t					fTransferredBodySize = 0;
	std::optional<off_t>	fBodySize;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_SERIALIZER_H_
