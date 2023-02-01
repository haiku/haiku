/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpBuffer.h"

#include <DataIO.h>
#include <NetServicesDefs.h>
#include <String.h>

using namespace BPrivate::Network;


/*!
	\brief Newline sequence

	As per the RFC, defined as \r\n
*/
static constexpr std::array<std::byte, 2> kNewLine = {std::byte('\r'), std::byte('\n')};


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
HttpBuffer::ReadFrom(BDataIO* source, std::optional<size_t> maxSize)
{
	// Remove any unused bytes at the beginning of the buffer
	Flush();

	auto currentSize = fBuffer.size();
	auto remainingBufferSize = fBuffer.capacity() - currentSize;

	if (maxSize && maxSize.value() < remainingBufferSize)
		remainingBufferSize = maxSize.value();

	// Adjust the buffer to the maximum size
	fBuffer.resize(fBuffer.capacity());

	ssize_t bytesRead = B_INTERRUPTED;
	while (bytesRead == B_INTERRUPTED)
		bytesRead = source->Read(fBuffer.data() + currentSize, remainingBufferSize);

	if (bytesRead == B_WOULD_BLOCK || bytesRead == 0) {
		fBuffer.resize(currentSize);
		return bytesRead;
	} else if (bytesRead < 0) {
		throw BNetworkRequestError(
			"BDataIO::Read()", BNetworkRequestError::NetworkError, bytesRead);
	}

	// Adjust the buffer to the current size
	fBuffer.resize(currentSize + bytesRead);

	return bytesRead;
}


/*!
	\brief Write the contents of the buffer through the helper \a func.

	\param func Handle the actual writing. The function accepts a pointer and a size as inputs
		and should return the number of actual written bytes, which may be fewer than the number
		of available bytes.

	\returns the actual number of bytes written to the \a func.
*/
size_t
HttpBuffer::WriteTo(HttpTransferFunction func, std::optional<size_t> maxSize)
{
	if (RemainingBytes() == 0)
		return 0;

	auto size = RemainingBytes();
	if (maxSize.has_value() && *maxSize < size)
		size = *maxSize;

	auto bytesWritten = func(fBuffer.data() + fCurrentOffset, size);
	if (bytesWritten > size)
		throw BRuntimeError(__PRETTY_FUNCTION__, "More bytes written than were made available");

	fCurrentOffset += bytesWritten;

	return bytesWritten;
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

	BString line(
		reinterpret_cast<const char*>(std::addressof(*offset)), std::distance(offset, result));
	fCurrentOffset = std::distance(fBuffer.cbegin(), result) + 2;
	return line;
}


/*!
	\brief Get the number of remaining bytes in this buffer.
*/
size_t
HttpBuffer::RemainingBytes() const noexcept
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
	\brief Get a view over the current data
*/
std::string_view
HttpBuffer::Data() const noexcept
{
	if (RemainingBytes() > 0) {
		return std::string_view(
			reinterpret_cast<const char*>(fBuffer.data()) + fCurrentOffset, RemainingBytes());
	} else
		return std::string_view();
}


/*!
	\brief Load data into the buffer

	\exception BNetworkRequestError in case of a buffer overflow
*/
HttpBuffer&
HttpBuffer::operator<<(const std::string_view& data)
{
	if (data.size() > (fBuffer.capacity() - fBuffer.size())) {
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError,
			"No capacity left in buffer to append data.");
	}

	for (const auto& character: data)
		fBuffer.push_back(static_cast<const std::byte>(character));

	return *this;
}
