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

#include <DataIO.h>
#include <HttpRequest.h>
#include <NetServicesDefs.h>

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
HttpBuffer::WriteTo(WriteFunction func)
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
