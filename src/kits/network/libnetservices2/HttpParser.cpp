/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpParser.h"

#include <string>
#include <stdexcept>

#include <HttpFields.h>
#include <NetServicesDefs.h>
#include <ZlibCompressionAlgorithm.h>

using namespace std::literals;
using namespace BPrivate::Network;


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
	fStatus.code = status.code; // cache the status code
	return true;
}


/*!
	\brief Parse the fields from the \a buffer and store it in \a fields.

	The fields are parsed incrementally, meaning that even if the full header is not yet in the
	\a buffer, it will still parse all complete fields and store them in the \a fields.

	After all fields have been parsed, it will determine the properties of the request body.
	This means it will determine whether there is any content compression, if there is a body,
	and if so if it has a fixed size or not.

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

	if (!fieldLine || (fieldLine && !fieldLine.value().IsEmpty())){
		// there is more to parse
		return false;
	}

	// Determine the properties for the body
	// RFC 7230 section 3.3.3 has a prioritized list of 7 rules around determining the body:
	if (fBodyType == HttpBodyType::NoContent
		|| fStatus.StatusCode() == BHttpStatusCode::NoContent
		|| fStatus.StatusCode() == BHttpStatusCode::NotModified) {
		// [1] In case of HEAD (set previously), status codes 1xx (TODO!), status code 204 or 304, no content
		// [2] NOT SUPPORTED: when doing a CONNECT request, no content
		fBodyType = HttpBodyType::NoContent;
	} else if (auto header = fields.FindField("Transfer-Encoding"sv);
		header != fields.end() && header->Value() == "chunked"sv) {
		// [3] If there is a Transfer-Encoding heading set to 'chunked'
		// TODO: support the more advanced rules in the RFC around the meaning of this field
		fBodyType = HttpBodyType::Chunked;
	} else if (fields.CountFields("Content-Length"sv) > 0) {
		// [4] When there is no Transfer-Encoding, then look for Content-Encoding:
		//	- If there are more than one, the values must match
		//	- The value must be a valid number
		// [5] If there is a valid value, then that is the expected size of the body
		try {
			auto contentLength = std::string();
			for (const auto& field: fields) {
				if (field.Name() == "Content-Length"sv) {
					if (contentLength.size() == 0)
						contentLength = field.Value();
					else if (contentLength != field.Value()) {
						throw BNetworkRequestError(__PRETTY_FUNCTION__,
							BNetworkRequestError::ProtocolError,
							"Multiple Content-Length fields with differing values");
					}
				}
			}
			auto bodyBytesTotal = std::stol(contentLength);
			if (bodyBytesTotal == 0)
				fBodyType = HttpBodyType::NoContent;
			else {
				fBodyType = HttpBodyType::FixedSize;
				fBodyBytesTotal = bodyBytesTotal;
			}
		} catch (const std::logic_error& e) {
			throw BNetworkRequestError(__PRETTY_FUNCTION__,
				BNetworkRequestError::ProtocolError,
				"Cannot parse Content-Length field value (logic_error)");
		}
	} else {
		// [6] Applies to request messages only (this is a response)
		// [7] If nothing else then the received message is all data until connection close
		// (this is the default)
	}

	// Content-Encoding
	auto header = fields.FindField("Content-Encoding"sv);
	if (header != fields.end()
		&& (header->Value() == "gzip" || header->Value() == "deflate"))
	{
		_SetGzipCompression();
	}
	return true;
}


/*!
	\brief Parse the body from the \a buffer and use \a writeToBody function to save.
*/
size_t
HttpParser::ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody)
{
	if (fBodyType == HttpBodyType::NoContent) {
		return 0;
	} else if (fBodyType == HttpBodyType::Chunked) {
		return _ParseBodyChunked(buffer, writeToBody);
	} else {
		return _ParseBodyRaw(buffer, writeToBody);
	}
}


/*!
	\brief Parse the Gzip compression from the body.

	\exception std::bad_alloc in case there is an error allocating memory.
*/
void
HttpParser::_SetGzipCompression()
{
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
}


/*!
	\brief Parse the body from the \a buffer and use \a writeToBody function to save.
*/
size_t
HttpParser::_ParseBodyRaw(HttpBuffer& buffer, HttpTransferFunction writeToBody)
{
	if (fBodyBytesTotal && (fTransferredBodySize + static_cast<off_t>(buffer.RemainingBytes()))
		> *fBodyBytesTotal)
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);

	auto bytesToRead = buffer.RemainingBytes();
	auto readEnd = fBodyBytesTotal.value()
		== (fTransferredBodySize + static_cast<off_t>(bytesToRead));

	auto bytesRead = _ReadChunk(buffer, writeToBody, bytesToRead, readEnd);
	fTransferredBodySize += bytesRead;
	return bytesRead;
}


/*!
	\brief Parse the body from the \a buffer and use \a writeToBody function to save.
*/
size_t
HttpParser::_ParseBodyChunked(HttpBuffer& buffer, HttpTransferFunction writeToBody)
{
	size_t totalBytesRead = 0;
	while (buffer.RemainingBytes() > 0) {
		switch (fBodyState) {
		case HttpBodyInputStreamState::ChunkSize:
		{
			// Read the next chunk size from the buffer; if unsuccesful wait for more data
			auto chunkSizeString = buffer.GetNextLine();
			if (!chunkSizeString)
				return totalBytesRead;
			auto chunkSizeStr = std::string(chunkSizeString.value().String());
			try {
				size_t pos = 0;
				fRemainingChunkSize = std::stoll(chunkSizeStr, &pos, 16);
				if (pos < chunkSizeStr.size() && chunkSizeStr[pos] != ';'){
					throw BNetworkRequestError(__PRETTY_FUNCTION__,
						BNetworkRequestError::ProtocolError);
				}
			} catch (const std::invalid_argument&) {
				throw BNetworkRequestError(__PRETTY_FUNCTION__,
					BNetworkRequestError::ProtocolError);
			} catch (const std::out_of_range&) {
				throw BNetworkRequestError(__PRETTY_FUNCTION__,
					BNetworkRequestError::ProtocolError);
			}

			if (fRemainingChunkSize > 0)
				fBodyState = HttpBodyInputStreamState::Chunk;
			else
				fBodyState = HttpBodyInputStreamState::Trailers;
			break;
		}

		case HttpBodyInputStreamState::Chunk:
		{
			size_t bytesToRead;
			bool readEnd = false;
			if (fRemainingChunkSize > static_cast<off_t>(buffer.RemainingBytes()))
				bytesToRead = buffer.RemainingBytes();
			else {
				readEnd = true;
				bytesToRead = fRemainingChunkSize;
			}

			auto bytesRead = _ReadChunk(buffer, writeToBody, bytesToRead, readEnd);
			fTransferredBodySize += bytesRead;
			totalBytesRead += bytesRead;
			fRemainingChunkSize -= bytesRead;
			if (fRemainingChunkSize == 0)
				fBodyState = HttpBodyInputStreamState::ChunkEnd;
			break;
		}

		case HttpBodyInputStreamState::ChunkEnd:
		{
			if (buffer.RemainingBytes() < 2) {
				// not enough data in the buffer to finish the chunk
				return totalBytesRead;
			}
			auto chunkEndString = buffer.GetNextLine();
			if (!chunkEndString || chunkEndString.value().Length() != 0) {
				// There should have been an empty chunk
				throw BNetworkRequestError(__PRETTY_FUNCTION__,
					BNetworkRequestError::ProtocolError);
			}

			fBodyState = HttpBodyInputStreamState::ChunkSize;
			break;
		}

		case HttpBodyInputStreamState::Trailers:
		{
			auto trailerString = buffer.GetNextLine();
			if (!trailerString)  {
				// More data to come
				return totalBytesRead;
			}

			if (trailerString.value().Length() > 0) {
				// Ignore empty trailers for now
				// TODO: review if the API should support trailing headers
			} else {
				fBodyState = HttpBodyInputStreamState::Done;
				return totalBytesRead;
			}
			break;
		}

		case HttpBodyInputStreamState::Done:
			return totalBytesRead;
		}
	}
	return totalBytesRead;
}


/*!
	\brief Check if the body is fully parsed.
*/
bool
HttpParser::Complete() const noexcept
{
	if (fBodyType == HttpBodyType::Chunked)
		return fBodyState == HttpBodyInputStreamState::Done;
	else if (fBodyType == HttpBodyType::FixedSize)
		return fBodyBytesTotal.value() == fTransferredBodySize;
	else if (fBodyType == HttpBodyType::NoContent)
		return true;
	else
		return false;
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
		buffer.WriteTo([this](const std::byte* buffer, size_t bufferSize){
			auto status = fDecompressingStream->WriteExactly(buffer, bufferSize);
			if (status != B_OK) {
				throw BNetworkRequestError("BDataIO::WriteExactly()",
					BNetworkRequestError::SystemError, status);
			}
			return bufferSize;
		}, size);

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
		buffer.WriteTo(writeToBody, size);
	}
	return size;
}
