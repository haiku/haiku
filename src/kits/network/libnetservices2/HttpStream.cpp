/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <HttpStream.h>

#include <algorithm>
#include <optional>
#include <string>

#include <DataIO.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <NetServicesDefs.h>
#include <ZlibCompressionAlgorithm.h>

using namespace BPrivate::Network;


/*!
	\brief Size of the internal buffer for reads/writes

	Curl 7.82.0 sets the default to 512 kB (524288 bytes)
		https://github.com/curl/curl/blob/64db5c575d9c5536bd273a890f50777ad1ca7c13/include/curl/curl.h#L232
	Libsoup sets it to 8 kB, though the buffer may grow beyond that if there are leftover bytes.
	The absolute maximum seems to be 64 kB (HEADER_SIZE_LIMIT)
		https://gitlab.gnome.org/GNOME/libsoup/-/blob/master/libsoup/http1/soup-client-message-io-http1.c#L58
	The previous iteration set it to 4 kB, though the input buffer would dynamically grow.
*/
static constexpr ssize_t kMaxBufferSize = 8192;


/*!
	\brief Newline sequence

	As per the RFC, defined as \r\n
*/
static constexpr std::array<std::byte, 2> kNewLine = {std::byte('\r'), std::byte('\n')};


// #pragma mark -- ByteIOHelper base class


class ByteIOHelper : public BDataIO {
public:
							ByteIOHelper(std::vector<std::byte>& buffer);

	virtual	ssize_t			Read(void* buffer, size_t size) override;
	virtual	ssize_t			Write(const void* buffer, size_t size) override;

private:
	std::vector<std::byte>&	fBuffer;
};


ByteIOHelper::ByteIOHelper(std::vector<std::byte>& buffer)
	: fBuffer(buffer)
{
	if (buffer.size() != 0)
		throw BRuntimeError(__PRETTY_FUNCTION__, "Target buffer with size > 0");
}


ssize_t
ByteIOHelper::Read(void* buffer, size_t size)
{
	throw BRuntimeError(__PRETTY_FUNCTION__, "Unexpected Read() call");
}


ssize_t
ByteIOHelper::Write(const void* buffer, size_t size)
{
	auto remainingSize = kMaxBufferSize - fBuffer.size();
	if (remainingSize < 0)
		return 0;

	if (size > remainingSize)
		size = remainingSize;

	auto bufferCast = static_cast<const std::byte*>(buffer);
	fBuffer.insert(fBuffer.end(), bufferCast, bufferCast + size);
	return size;
}


// #pragma mark -- BAbstractDataStream (helper methods)


/*!
	\brief Load data from \a source into the internal buffer.

	The buffer will be filled up to the maximum size (64kB). Partial reads are supported; it will
	not do a retry.

	\return The return value of the underlying BDataIO::Read() call.
*/
ssize_t
BAbstractDataStream::BufferData(BDataIO* source, size_t maxSize)
{
	auto currentSize = fBuffer.size();
	auto remainingSize = kMaxBufferSize - currentSize;
	if (remainingSize < 0)
		return B_OK;

	if (remainingSize > maxSize)
		remainingSize = maxSize;

	fBuffer.resize(currentSize + remainingSize);
	ssize_t readSize = B_INTERRUPTED;
	while (readSize == B_INTERRUPTED)
		readSize = source->Read(fBuffer.data() + currentSize, remainingSize);

	if (readSize <= 0) {
		fBuffer.resize(currentSize); // resize back to the original size
		return readSize;
	}

	if (readSize > 0)
		fBuffer.resize(currentSize + readSize);

	return readSize;
}


// #pragma mark -- BHttpRequestStream


BHttpRequestStream::BHttpRequestStream(const BHttpRequest& request)
	: fBody(nullptr)
{
	// Serialize the header of the request to text
	ByteIOHelper helper(fBuffer);
	fRemainingHeaderSize = request.SerializeHeaderTo(&helper);

	// Check if there is a body
	if (auto requestBody = request.RequestBody()) {
		fBody = requestBody->input.get();
		if (!requestBody->size) {
			throw BRuntimeError(__PRETTY_FUNCTION__,
				"BHttpRequestStream: chunked transfer not implemented");
		}
		fTotalBodySize += *requestBody->size;
	}
}


BHttpRequestStream::~BHttpRequestStream() = default;


BHttpRequestStream::TransferInfo
BHttpRequestStream::Transfer(BDataIO* target)
{
	if (fBuffer.size() == 0 && fTotalBodySize == fBufferedBodySize) {
		// all done; header was written and no more body left
		return TransferInfo{0, fTotalBodySize, fTotalBodySize, true};
	}

	if (fBody != nullptr && fBuffer.size() < kMaxBufferSize) {
		// buffer additional data from the body in the buffer
		auto remainingBodySize = fTotalBodySize - fBufferedBodySize;
		auto bufferedSize = BufferData(fBody, remainingBodySize);

		if (bufferedSize == B_WOULD_BLOCK) {
			// do nothing; try again next round
		} else if (bufferedSize == 0) {
			// no remaining data; throw error
			throw BRuntimeError(__PRETTY_FUNCTION__,
				"No more data in request input body while more data is expected");
		} else if (bufferedSize < 0) {
			throw BSystemError(__PRETTY_FUNCTION__, bufferedSize);
		} else {
			// update counters
			fBufferedBodySize += bufferedSize;
			if (fBufferedBodySize == fTotalBodySize) {
				// no more body to load
				fBody = nullptr;
			}
		}
	}

	if (fBuffer.size() == 0) {
		// nothing this round
		return TransferInfo{0, fTransferredBodySize, fTotalBodySize, false};
	}

	auto bytesWritten = target->Write(fBuffer.data(), fBuffer.size());
	if (bytesWritten == B_WOULD_BLOCK || bytesWritten == 0)
		return TransferInfo{0, fTransferredBodySize, fTotalBodySize, false};
	else if (bytesWritten < 0)
		throw BSystemError("BDataIO::Write()", bytesWritten);

	// Adjust the buffer
	if (static_cast<size_t>(bytesWritten) == fBuffer.size())
		fBuffer.clear();
	else
		fBuffer.erase(fBuffer.begin(), fBuffer.begin() + bytesWritten);

	// Update the stats and return
	if (fRemainingHeaderSize > 0){
		if (bytesWritten >= fRemainingHeaderSize) {
			bytesWritten -= fRemainingHeaderSize;
			fRemainingHeaderSize = 0;
		} else {
			fRemainingHeaderSize -= bytesWritten;
			bytesWritten = 0;
		}
	}

	fTransferredBodySize += bytesWritten;

	auto complete = fRemainingHeaderSize == 0 && fTransferredBodySize == fTotalBodySize;
	return TransferInfo{bytesWritten, fTransferredBodySize, fTotalBodySize, complete};
}


/*!
	\brief Create a new HTTP buffer with \a capacity.
*/
HttpBuffer::HttpBuffer(size_t capacity)
{
	fBuffer.reserve(capacity);
};


/*!
	\brief Load data from \a source into the spare capacity of this buffer.

	\exception BNetworkRequestError When BDataIO::Read() returns any error other than B_WOULD_BLOCK

	\retval B_WOULD_BLOCK The read call on the \a source was unsuccessful because it would block.
	\retval >=0 The actual number of bytes read.
*/
ssize_t
HttpBuffer::ReadFrom(BDataIO* source)
{
	// Remove any unused bytes at the beginning of the buffer
	Flush();

	auto currentSize = fBuffer.size();
	auto remainingBufferSize = fBuffer.capacity() - currentSize;

	// Adjust the buffer to the maximum size
	fBuffer.resize(fBuffer.capacity());

	ssize_t bytesRead = B_INTERRUPTED;
	while (bytesRead == B_INTERRUPTED)
		bytesRead = source->Read(fBuffer.data() + currentSize, remainingBufferSize);

	if (bytesRead == B_WOULD_BLOCK || bytesRead == 0) {
		fBuffer.resize(currentSize);
		return bytesRead;
	} else if (bytesRead < 0) {
		throw BNetworkRequestError("BDataIO::Read()", BNetworkRequestError::NetworkError,
			bytesRead);
	}

	// Adjust the buffer to the current size
	fBuffer.resize(currentSize + bytesRead);

	return bytesRead;
}


/*!
	\brief Use BDataIO::WriteExactly() on target to write the contents of the buffer.
*/
void
HttpBuffer::WriteExactlyTo(BDataIO* target)
{
	if (RemainingBytes() == 0)
		return;

	auto status = target->WriteExactly(fBuffer.data() + fCurrentOffset, RemainingBytes());
	if (status != B_OK) {
		throw BNetworkRequestError("BDataIO::WriteExactly()", BNetworkRequestError::SystemError,
			status);
	}

	// Entire buffer is written; reset internal buffer
	Clear();
}


/*!
	\brief Write the contents of the buffer through the helper \a func.

	\param func Handle the actual writing. The function accepts a pointer and a size as inputs
		and should return the number of actual written bytes, which may be fewer than the number
		of available bytes.
*/
void
HttpBuffer::WriteTo(HttpTransferFunction func)
{
	if (RemainingBytes() == 0)
		return;

	auto bytesWritten = func(fBuffer.data() + fCurrentOffset, RemainingBytes());
	if (bytesWritten > RemainingBytes())
		throw BRuntimeError(__PRETTY_FUNCTION__, "More bytes written than were remaining");

	fCurrentOffset += bytesWritten;
}


/*!
	\brief Get the next line from this buffer.

	This can be called iteratively until all lines in the current data are read. After using this
	method, you should use Flush() to make sure that the read lines are cleared from the beginning
	of the buffer.

	\retval std::nullopt There are no more lines in the buffer.
	\retval BString The next line.
*/
std::optional<BString>
HttpBuffer::GetNextLine()
{
	auto offset = fBuffer.cbegin() + fCurrentOffset;
	auto result = std::search(offset, fBuffer.cend(), kNewLine.cbegin(), kNewLine.cend());
	if (result == fBuffer.cend())
		return std::nullopt;

	BString line(reinterpret_cast<const char*>(std::addressof(*offset)), std::distance(offset, result));
	fCurrentOffset = std::distance(fBuffer.cbegin(), result) + 2;
	return line;
}


/*!
	\brief Get the number of remaining bytes in this buffer.
*/
size_t
HttpBuffer::RemainingBytes() noexcept
{
	return fBuffer.size() - fCurrentOffset;
}


/*!
	\brief Move data to the beginning of the buffer to clear at the back.

	The GetNextLine() increases the offset of the internal buffer. This call moves remaining data
	to the beginning of the buffer sets the correct size, making the remainder of the capacity
	available for further reading.
*/
void
HttpBuffer::Flush() noexcept
{
	if (fCurrentOffset > 0) {
		auto end = fBuffer.cbegin() + fCurrentOffset;
		fBuffer.erase(fBuffer.cbegin(), end);
		fCurrentOffset = 0;
	}
}


/*!
	\brief Clear the internal buffer
*/
void
HttpBuffer::Clear() noexcept
{
	fBuffer.clear();
	fCurrentOffset = 0;
}


/*!
	\brief Parse the status from the \a buffer and store it in \a status.

	\retval true The status was succesfully parsed
	\retval false There is not enough data in the buffer for a full status.

	\exception BNetworkRequestException The status does not conform to the HTTP spec.
*/
bool
HttpParser::ParseStatus(HttpBuffer& buffer, BHttpStatus& status)
{
	auto statusLine = buffer.GetNextLine();
	if (!statusLine)
		return false;

	auto codeStart = statusLine->FindFirst(' ') + 1;
	if (codeStart < 0)
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);

	auto codeEnd = statusLine->FindFirst(' ', codeStart);

	if (codeEnd < 0 || (codeEnd - codeStart) != 3)
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);

	std::string statusCodeString(statusLine->String() + codeStart, 3);

	// build the output
	try {
		status.code = std::stol(statusCodeString);
	} catch (...) {
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);
	}

	status.text = std::move(statusLine.value());
	return true;
}


/*!
	\brief Parse the fields from the \a buffer and store it in \a fields.

	The fields are parsed incrementally, meaning that even if the full header is not yet in the
	\a buffer, it will still parse all complete fields and store them in the \a fields.

	\retval true All fields were succesfully parsed
	\retval false There is not enough data in the buffer to complete parsing of fields.

	\exception BNetworkRequestException The fields not conform to the HTTP spec.
*/
bool
HttpParser::ParseFields(HttpBuffer& buffer, BHttpFields& fields)
{
	auto fieldLine = buffer.GetNextLine();
	
	while (fieldLine && !fieldLine.value().IsEmpty()){
		// Parse next header line
		fields.AddField(fieldLine.value());
		fieldLine = buffer.GetNextLine();
	}

	if (fieldLine && fieldLine.value().IsEmpty()){
		// end of the header section of the message
		return true;
	} else {
		// there is more to parse
		return false;
	}
}


/*!
	\brief Parse the Gzip compression from the body.

	\exception std::bad_alloc in case there is an error allocating memory.
*/
void
HttpParser::SetGzipCompression(bool compression)
{
	if (compression) {
		fDecompressorStorage = std::make_unique<BMallocIO>();

		BDataIO* stream = nullptr;
		auto result = BZlibCompressionAlgorithm()
			.CreateDecompressingOutputStream(fDecompressorStorage.get(), nullptr, stream);
		
		if (result != B_OK) {
			throw BNetworkRequestError(
				"BZlibCompressionAlgorithm().CreateCompressingOutputStream",
				BNetworkRequestError::SystemError, result);
		}

		fDecompressingStream = std::unique_ptr<BDataIO>(stream);
	} else {
		fDecompressingStream = nullptr;
		fDecompressorStorage = nullptr;
	}
}


/*!
	\brief Set the content length of the body.

	If a content length is set, the body will not be handled as a chunked transfer.
*/
void
HttpParser::SetContentLength(std::optional<off_t> contentLength) noexcept
{
	fBodyBytesTotal = contentLength;
}


/*!
	\brief Parse the body from the \a buffer and use \a writeToBody function to save.
*/
size_t
HttpParser::ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody)
{
	if (fBodyBytesTotal && (fTransferredBodySize + buffer.RemainingBytes()) > fBodyBytesTotal)
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);

	size_t bytesRead = 0;
	size_t bytesToRead = 0;
	bool readEnd = false;
	while (buffer.RemainingBytes() > 0) {
		if (_IsChunked()) {
			bytesToRead = 100;
			readEnd = false;
		} else {
			bytesToRead = buffer.RemainingBytes();
			readEnd = fBodyBytesTotal.value()
				== (fTransferredBodySize + static_cast<off_t>(bytesToRead));
		}

		bytesRead += _ReadChunk(buffer, writeToBody, bytesToRead, readEnd);
	}
	fTransferredBodySize += bytesRead;
	return bytesRead;
}


/*!
	\brief Check if the body is fully parsed.
*/
bool
HttpParser::Complete() const noexcept
{
	if (_IsChunked())
		return fChunkedTransferComplete;

	return fBodyBytesTotal.value() == fTransferredBodySize;
}


/*!
	\brief Read a chunk of data from the buffer and write it to the output.

	If there is a compression algorithm applied, then it passes through the compression first.

	\param buffer The buffer to read from
	\param writeToBody The function that can write data from the buffer to the body.
	\param size The maximum size to read from the buffer. When larger than the buffer size, the
		remaining bytes from the buffer are read.
	\param flush Setting this parameter will force the decompression to write out all data, if
		applicable. Set when all the data has been received.

	\exception BNetworkRequestError When there was any error with any of the system cals.
*/
size_t
HttpParser::_ReadChunk(HttpBuffer& buffer, HttpTransferFunction writeToBody, size_t size, bool flush)
{
	if (size == 0)
		return 0;

	if (size > buffer.RemainingBytes())
		size = buffer.RemainingBytes();

	if (fDecompressingStream) {
		buffer.WriteTo([this, &size](const std::byte* buffer, size_t bufferSize){
			if (size < bufferSize)
				bufferSize = size;
			auto status = fDecompressingStream->WriteExactly(buffer, bufferSize);
			if (status != B_OK) {
				throw BNetworkRequestError("BDataIO::WriteExactly()",
					BNetworkRequestError::SystemError, status);
			}
			return bufferSize;
		});

		if (flush) {
			// No more bytes expected so flush out the final bytes
			if (auto status = fDecompressingStream->Flush(); status != B_OK)
				throw BNetworkRequestError("BZlibDecompressionStream::Flush()",
					BNetworkRequestError::SystemError, status);
		}

		if (auto bodySize = fDecompressorStorage->Position(); bodySize > 0) {
			auto bytesWritten
				= writeToBody(static_cast<const std::byte*>(fDecompressorStorage->Buffer()),
					bodySize);
			if (static_cast<off_t>(bytesWritten) != bodySize) {
				throw BNetworkRequestError(__PRETTY_FUNCTION__,
					BNetworkRequestError::SystemError, B_PARTIAL_WRITE);
			}
			fDecompressorStorage->Seek(0, SEEK_SET);
		} 
	} else {
		// Write the body directly to the target
		buffer.WriteTo(writeToBody);
	}
	return size;
}


/*!
	\brief Internal helper to determine if the body is sent as a chunked transfer.
*/
bool
HttpParser::_IsChunked() const noexcept
{
	return fBodyBytesTotal == std::nullopt;
}
