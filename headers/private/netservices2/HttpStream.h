/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _HTTP_STREAM_H_
#define _HTTP_STREAM_H_

#include <functional>
#include <memory>
#include <optional>
#include <vector>

class BDataIO;
class BMallocIO;
class BString;


namespace BPrivate {

namespace Network {

class BHttpFields;
class BHttpRequest;
class BHttpStatus;


class BAbstractDataStream {
public:
	struct TransferInfo {
		off_t currentBytesWritten;
		off_t totalBytesWritten;
		off_t totalSize;
		bool complete;
	};

	virtual	TransferInfo			Transfer(BDataIO*) = 0;

protected:
			ssize_t					BufferData(BDataIO* source, size_t maxSize);

			std::vector<std::byte>	fBuffer;
};


class BHttpRequestStream : public BAbstractDataStream {
public:
							BHttpRequestStream(const BHttpRequest& request);
							~BHttpRequestStream();

	virtual	TransferInfo	Transfer(BDataIO* target) override;

private:
	off_t					fRemainingHeaderSize = 0;
	BDataIO*				fBody = nullptr;
	off_t					fTotalBodySize = 0;
	off_t					fBufferedBodySize = 0;
	off_t					fTransferredBodySize = 0;
};


using HttpTransferFunction = std::function<size_t (const std::byte*, size_t)>;


class HttpBuffer {
public:
							HttpBuffer(size_t capacity = 8*1024);

	ssize_t					ReadFrom(BDataIO* source);
	void					WriteExactlyTo(BDataIO* target);
	void					WriteTo(HttpTransferFunction func);
	std::optional<BString>	GetNextLine();

	size_t					RemainingBytes() noexcept;

	void					Flush() noexcept;
	void					Clear() noexcept;

private:
	std::vector<std::byte>	fBuffer;
	size_t					fCurrentOffset = 0;
};


class HttpParser {
public:
							HttpParser() {};

	// HTTP Header
	bool					ParseStatus(HttpBuffer& buffer, BHttpStatus& status);
	bool					ParseFields(HttpBuffer& buffer, BHttpFields& fields);

	// HTTP Body
	void					SetGzipCompression(bool compression = true);
	void					SetContentLength(std::optional<off_t> contentLength) noexcept;

	size_t					ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody);
	std::optional<off_t>	BodyBytesTotal() const noexcept { return fBodyBytesTotal; };
	off_t					BodyBytesTransferred() const noexcept { return fTransferredBodySize; };
	bool					Complete() const noexcept;

private:
	size_t					_ReadChunk(HttpBuffer& buffer, HttpTransferFunction writeToBody,
								size_t maxSize, bool flush);
	bool					_IsChunked() const noexcept;

private:
	off_t					fHeaderBytes = 0;

	// Support for chunked transfers
	std::optional<off_t>	fRemainingChunkSize;
	bool					fChunkedTransferComplete = false;

	// Receive stats
	std::optional<off_t>	fBodyBytesTotal = 0;
	off_t					fTransferredBodySize = 0;

	// Optional decompression
	std::unique_ptr<BMallocIO>		fDecompressorStorage = nullptr;
	std::unique_ptr<BDataIO>		fDecompressingStream = nullptr;

};


} // namespace Network

} // namespace BPrivate

#endif // _HTTP_STREAM_H_
