/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _B_HTTP_PARSER_H_
#define _B_HTTP_PARSER_H_


#include <functional>
#include <optional>

#include <HttpResult.h>

#include "HttpBuffer.h"

class BMallocIO;

namespace BPrivate {

namespace Network {

using HttpTransferFunction = std::function<size_t (const std::byte*, size_t)>;


enum class HttpBodyInputStreamState {
	ChunkSize,
	ChunkEnd,
	Chunk,
	Trailers,
	Done
};


enum class HttpBodyType {
	NoContent,
	Chunked,
	FixedSize,
	VariableSize
};


class HttpParser {
public:
							HttpParser() {};

	// Explicitly mark request as having no content
	void					SetNoContent() { fBodyType = HttpBodyType::NoContent; };

	// HTTP Header
	bool					ParseStatus(HttpBuffer& buffer, BHttpStatus& status);
	bool					ParseFields(HttpBuffer& buffer, BHttpFields& fields);

	// HTTP Body
	size_t					ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody);
	void					SetConnectionClosed();

	// Details on the body status
	bool					HasContent() const noexcept { return fBodyType != HttpBodyType::NoContent; };
	std::optional<off_t>	BodyBytesTotal() const noexcept { return fBodyBytesTotal; };
	off_t					BodyBytesTransferred() const noexcept { return fTransferredBodySize; };
	bool					Complete() const noexcept;

private:
	void					_SetGzipCompression();
	size_t					_ParseBodyRaw(HttpBuffer& buffer, HttpTransferFunction writeToBody);
	size_t					_ParseBodyChunked(HttpBuffer& buffer, HttpTransferFunction writeToBody);
	size_t					_ReadChunk(HttpBuffer& buffer, HttpTransferFunction writeToBody,
								size_t maxSize, bool flush);

private:
	off_t					fHeaderBytes = 0;
	BHttpStatus				fStatus;

	// Body type
	HttpBodyType			fBodyType = HttpBodyType::VariableSize;

	// Support for chunked transfers
	HttpBodyInputStreamState fBodyState = HttpBodyInputStreamState::ChunkSize;
	off_t					fRemainingChunkSize = 0;
	bool					fLastChunk = false;

	// Receive stats
	std::optional<off_t>	fBodyBytesTotal = 0;
	off_t					fTransferredBodySize = 0;

	// Optional decompression
	std::unique_ptr<BMallocIO>		fDecompressorStorage = nullptr;
	std::unique_ptr<BDataIO>		fDecompressingStream = nullptr;

};


} // namespace Network

} // namespace BPrivate

#endif // _B_HTTP_PARSER_H_
