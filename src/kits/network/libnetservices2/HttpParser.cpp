/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include "HttpParser.h"

#include <stdexcept>
#include <string>

#include <HttpFields.h>
#include <NetServicesDefs.h>
#include <ZlibCompressionAlgorithm.h>

using namespace std::literals;
using namespace BPrivate::Network;


// #pragma mark -- HttpParser


/*!
	\brief Explicitly mark the response as having no content.

	This is done in cases where the request was a HEAD request. Setting it to no content, will
	instruct the parser to move to completion after all the header fields have been parsed.
*/
void
HttpParser::SetNoContent() noexcept
{
	if (fStreamState > HttpInputStreamState::Fields)
		debugger("Cannot set the parser to no content after parsing of the body has started");
	fBodyType = HttpBodyType::NoContent;
};


/*!
	\brief Parse the status from the \a buffer and store it in \a status.

	\retval true The status was succesfully parsed
	\retval false There is not enough data in the buffer for a full status.

	\exception BNetworkRequestException The status does not conform to the HTTP spec.
*/
bool
HttpParser::ParseStatus(HttpBuffer& buffer, BHttpStatus& status)
{
	if (fStreamState != HttpInputStreamState::StatusLine)
		debugger("The Status line has already been parsed");

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
	fStreamState = HttpInputStreamState::Fields;
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
	if (fStreamState != HttpInputStreamState::Fields)
		debugger("The parser is not expecting header fields at this point");

	auto fieldLine = buffer.GetNextLine();

	while (fieldLine && !fieldLine.value().IsEmpty()) {
		// Parse next header line
		fields.AddField(fieldLine.value());
		fieldLine = buffer.GetNextLine();
	}

	if (!fieldLine || (fieldLine && !fieldLine.value().IsEmpty())) {
		// there is more to parse
		return false;
	}

	// Determine the properties for the body
	// RFC 7230 section 3.3.3 has a prioritized list of 7 rules around determining the body:
	std::optional<off_t> bodyBytesTotal = std::nullopt;
	if (fBodyType == HttpBodyType::NoContent || fStatus.StatusCode() == BHttpStatusCode::NoContent
		|| fStatus.StatusCode() == BHttpStatusCode::NotModified) {
		// [1] In case of HEAD (set previously), status codes 1xx (TODO!), status code 204 or 304,
		// no content [2] NOT SUPPORTED: when doing a CONNECT request, no content
		fBodyType = HttpBodyType::NoContent;
		fStreamState = HttpInputStreamState::Done;
	} else if (auto header = fields.FindField("Transfer-Encoding"sv);
			   header != fields.end() && header->Value() == "chunked"sv) {
		// [3] If there is a Transfer-Encoding heading set to 'chunked'
		// TODO: support the more advanced rules in the RFC around the meaning of this field
		fBodyType = HttpBodyType::Chunked;
		fStreamState = HttpInputStreamState::Body;
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
			bodyBytesTotal = std::stol(contentLength);
			if (*bodyBytesTotal == 0) {
				fBodyType = HttpBodyType::NoContent;
				fStreamState = HttpInputStreamState::Done;
			} else {
				fBodyType = HttpBodyType::FixedSize;
				fStreamState = HttpInputStreamState::Body;
			}
		} catch (const std::logic_error& e) {
			throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError,
				"Cannot parse Content-Length field value (logic_error)");
		}
	} else {
		// [6] Applies to request messages only (this is a response)
		// [7] If nothing else then the received message is all data until connection close
		// (this is the default)
		fStreamState = HttpInputStreamState::Body;
	}

	// Set up the body parser based on the logic above.
	switch (fBodyType) {
		case HttpBodyType::VariableSize:
			fBodyParser = std::make_unique<HttpRawBodyParser>();
			break;
		case HttpBodyType::FixedSize:
			fBodyParser = std::make_unique<HttpRawBodyParser>(*bodyBytesTotal);
			break;
		case HttpBodyType::Chunked:
			fBodyParser = std::make_unique<HttpChunkedBodyParser>();
			break;
		case HttpBodyType::NoContent:
		default:
			return true;
	}

	// Check Content-Encoding for compression
	auto header = fields.FindField("Content-Encoding"sv);
	if (header != fields.end() && (header->Value() == "gzip" || header->Value() == "deflate")) {
		fBodyParser = std::make_unique<HttpBodyDecompression>(std::move(fBodyParser));
	}

	return true;
}


/*!
	\brief Parse the body from the \a buffer and use \a writeToBody function to save.

	The \a readEnd parameter indicates to the parser that the buffer currently contains all the
	expected data for this request.
*/
size_t
HttpParser::ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody, bool readEnd)
{
	if (fStreamState < HttpInputStreamState::Body || fStreamState == HttpInputStreamState::Done)
		debugger("The parser is not in the correct state to parse a body");

	auto parseResult = fBodyParser->ParseBody(buffer, writeToBody, readEnd);

	if (parseResult.complete)
		fStreamState = HttpInputStreamState::Done;

	return parseResult.bytesParsed;
}


/*!
	\brief Return if the body is currently expecting to having content.

	This may change if the header fields have not yet been parsed, as these may contain
	instructions about the body having no content.
*/
bool
HttpParser::HasContent() const noexcept
{
	return fBodyType != HttpBodyType::NoContent;
}


/*!
	\brief Return the total size of the body, if known.
*/
std::optional<off_t>
HttpParser::BodyBytesTotal() const noexcept
{
	if (fBodyParser)
		return fBodyParser->TotalBodySize();
	return std::nullopt;
}


/*!
	\brief Return the number of body bytes transferred from the response.
*/
off_t
HttpParser::BodyBytesTransferred() const noexcept
{
	if (fBodyParser)
		return fBodyParser->TransferredBodySize();
	return 0;
}


/*!
	\brief Check if the body is fully parsed.
*/
bool
HttpParser::Complete() const noexcept
{
	return fStreamState == HttpInputStreamState::Done;
}


// #pragma mark -- HttpBodyParser


/*!
	\brief Default implementation to return std::nullopt.
*/
std::optional<off_t>
HttpBodyParser::TotalBodySize() const noexcept
{
	return std::nullopt;
}


/*!
	\brief Return the number of body bytes read from the stream so far.

	For chunked transfers, this excludes the chunk headers and other metadata.
*/
off_t
HttpBodyParser::TransferredBodySize() const noexcept
{
	return fTransferredBodySize;
}


// #pragma mark -- HttpRawBodyParser
/*!
	\brief Construct a HttpRawBodyParser with an unknown content size.
*/
HttpRawBodyParser::HttpRawBodyParser()
{
}


/*!
	\brief Construct a HttpRawBodyParser with expected \a bodyBytesTotal size.
*/
HttpRawBodyParser::HttpRawBodyParser(off_t bodyBytesTotal)
	:
	fBodyBytesTotal(bodyBytesTotal)
{
}


/*!
	\brief Parse a regular (non-chunked) body from a buffer.

	The buffer is parsed into a target using the \a writeToBody function.

	The \a readEnd argument indicates whether the current \a buffer contains all the expected data.
	In case the total body size is known, and the remaining bytes in the buffer are smaller than
	the expected remainder, a ProtocolError will be raised. The data in the buffer will *not* be
	copied to the target.

	Also, if the body size is known, and the data in the \a buffer is larger than the expected
	expected length, then it will only read the bytes needed and leave the remainder in the buffer.

	It is required that the \a writeToBody function writes all the bytes it is asked to; this
	method does not support partial writes and throws an exception when it fails.

	\exception BNetworkRequestError In case the buffer contains too little or invalid data.

	\returns The number of bytes parsed from the \a buffer.
*/
BodyParseResult
HttpRawBodyParser::ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody, bool readEnd)
{
	auto bytesToRead = buffer.RemainingBytes();
	if (fBodyBytesTotal) {
		auto expectedRemainingBytes = *fBodyBytesTotal - fTransferredBodySize;
		if (expectedRemainingBytes < static_cast<off_t>(buffer.RemainingBytes()))
			bytesToRead = expectedRemainingBytes;
		else if (readEnd && expectedRemainingBytes > static_cast<off_t>(buffer.RemainingBytes())) {
			throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError,
				"Message body is incomplete; less data received than expected");
		}
	}

	// Copy the data
	auto bytesRead = buffer.WriteTo(writeToBody, bytesToRead);
	fTransferredBodySize += bytesRead;

	if (bytesRead != bytesToRead) {
		// Fail if not all expected bytes are written.
		throw BNetworkRequestError(__PRETTY_FUNCTION__, BNetworkRequestError::SystemError,
			"Could not write all available body bytes to the target.");
	}

	if (fBodyBytesTotal) {
		if (*fBodyBytesTotal == fTransferredBodySize)
			return {bytesRead, bytesRead, true};
		else
			return {bytesRead, bytesRead, false};
	} else
		return {bytesRead, bytesRead, readEnd};
}


/*!
	\brief Override default implementation and return known body size (or std::nullopt)
*/
std::optional<off_t>
HttpRawBodyParser::TotalBodySize() const noexcept
{
	return fBodyBytesTotal;
}


// #pragma mark -- HttpChunkedBodyParser
/*!
	\brief Parse a chunked body from a buffer.

	The contents of the cunks are copied into a target using the \a writeToBody function.

	The \a readEnd argument indicates whether the current \a buffer contains all the expected data.
	In case the chunk argument indicates that more data was to come, an exception is thrown.

	It is required that the \a writeToBody function writes all the bytes it is asked to; this
	method does not support partial writes and throws an exception when it fails.

	\exception BNetworkRequestError In case there is an error parsing the buffer, or there is too
		little data.

	\returns The number of bytes parsed from the \a buffer.
*/
BodyParseResult
HttpChunkedBodyParser::ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody, bool readEnd)
{
	size_t totalBytesRead = 0;
	while (buffer.RemainingBytes() > 0) {
		switch (fChunkParserState) {
			case ChunkSize:
			{
				// Read the next chunk size from the buffer; if unsuccesful wait for more data
				auto chunkSizeString = buffer.GetNextLine();
				if (!chunkSizeString)
					return {totalBytesRead, totalBytesRead, false};
				auto chunkSizeStr = std::string(chunkSizeString.value().String());
				try {
					size_t pos = 0;
					fRemainingChunkSize = std::stoll(chunkSizeStr, &pos, 16);
					if (pos < chunkSizeStr.size() && chunkSizeStr[pos] != ';') {
						throw BNetworkRequestError(
							__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);
					}
				} catch (const std::invalid_argument&) {
					throw BNetworkRequestError(
						__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);
				} catch (const std::out_of_range&) {
					throw BNetworkRequestError(
						__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);
				}

				if (fRemainingChunkSize > 0)
					fChunkParserState = Chunk;
				else
					fChunkParserState = Trailers;
				break;
			}

			case Chunk:
			{
				size_t bytesToRead;
				if (fRemainingChunkSize > static_cast<off_t>(buffer.RemainingBytes()))
					bytesToRead = buffer.RemainingBytes();
				else
					bytesToRead = fRemainingChunkSize;

				auto bytesRead = buffer.WriteTo(writeToBody, bytesToRead);
				if (bytesRead != bytesToRead) {
					// Fail if not all expected bytes are written.
					throw BNetworkRequestError(__PRETTY_FUNCTION__,
						BNetworkRequestError::SystemError,
						"Could not write all available body bytes to the target.");
				}

				fTransferredBodySize += bytesRead;
				totalBytesRead += bytesRead;
				fRemainingChunkSize -= bytesRead;
				if (fRemainingChunkSize == 0)
					fChunkParserState = ChunkEnd;
				break;
			}

			case ChunkEnd:
			{
				if (buffer.RemainingBytes() < 2) {
					// not enough data in the buffer to finish the chunk
					return {totalBytesRead, totalBytesRead, false};
				}
				auto chunkEndString = buffer.GetNextLine();
				if (!chunkEndString || chunkEndString.value().Length() != 0) {
					// There should have been an empty chunk
					throw BNetworkRequestError(
						__PRETTY_FUNCTION__, BNetworkRequestError::ProtocolError);
				}

				fChunkParserState = ChunkSize;
				break;
			}

			case Trailers:
			{
				auto trailerString = buffer.GetNextLine();
				if (!trailerString) {
					// More data to come
					return {totalBytesRead, totalBytesRead, false};
				}

				if (trailerString.value().Length() > 0) {
					// Ignore empty trailers for now
					// TODO: review if the API should support trailing headers
				} else {
					fChunkParserState = Complete;
					return {totalBytesRead, totalBytesRead, true};
				}
				break;
			}

			case Complete:
				return {totalBytesRead, totalBytesRead, true};
		}
	}
	return {totalBytesRead, totalBytesRead, false};
}


// #pragma mark -- HttpBodyDecompression
/*!
	\brief Set up a decompression stream that decompresses the data read by \a bodyParser.
*/
HttpBodyDecompression::HttpBodyDecompression(std::unique_ptr<HttpBodyParser> bodyParser)
{
	fDecompressorStorage = std::make_unique<BMallocIO>();

	BDataIO* stream = nullptr;
	auto result = BZlibCompressionAlgorithm().CreateDecompressingOutputStream(
		fDecompressorStorage.get(), nullptr, stream);

	if (result != B_OK) {
		throw BNetworkRequestError("BZlibCompressionAlgorithm().CreateCompressingOutputStream",
			BNetworkRequestError::SystemError, result);
	}

	fDecompressingStream = std::unique_ptr<BDataIO>(stream);
	fBodyParser = std::move(bodyParser);
}


/*!
	\brief Read a compressed body into a target..

	The stream captures chunked or raw data, and decompresses it. The decompressed data is then
	copied into a target using the \a writeToBody function.

	The \a readEnd argument indicates whether the current \a buffer contains all the expected data.
	It is up for the underlying parser to determine if more data was expected, and therefore, if
	there is an error.

	It is required that the \a writeToBody function writes all the bytes it is asked to; this
	method does not support partial writes and throws an exception when it fails.

	\exception BNetworkRequestError In case there is an error parsing the buffer, or there is too
		little data.

	\returns The number of bytes parsed from the \a buffer.
*/
BodyParseResult
HttpBodyDecompression::ParseBody(HttpBuffer& buffer, HttpTransferFunction writeToBody, bool readEnd)
{
	// Get the underlying raw or chunked parser to write data to our decompressionstream
	auto parseResults = fBodyParser->ParseBody(
		buffer,
		[this](const std::byte* buffer, size_t bufferSize) {
			auto status = fDecompressingStream->WriteExactly(buffer, bufferSize);
			if (status != B_OK) {
				throw BNetworkRequestError(
					"BDataIO::WriteExactly()", BNetworkRequestError::SystemError, status);
			}
			return bufferSize;
		},
		readEnd);
	fTransferredBodySize += parseResults.bytesParsed;

	if (readEnd || parseResults.complete) {
		// No more bytes expected so flush out the final bytes
		if (auto status = fDecompressingStream->Flush(); status != B_OK) {
			throw BNetworkRequestError(
				"BZlibDecompressionStream::Flush()", BNetworkRequestError::SystemError, status);
		}
	}

	size_t bytesWritten = 0;
	if (auto bodySize = fDecompressorStorage->Position(); bodySize > 0) {
		bytesWritten
			= writeToBody(static_cast<const std::byte*>(fDecompressorStorage->Buffer()), bodySize);
		if (static_cast<off_t>(bytesWritten) != bodySize) {
			throw BNetworkRequestError(
				__PRETTY_FUNCTION__, BNetworkRequestError::SystemError, B_PARTIAL_WRITE);
		}
		fDecompressorStorage->Seek(0, SEEK_SET);
	}
	return {parseResults.bytesParsed, bytesWritten, parseResults.complete};
}


/*!
	\brief Return the TotalBodySize() from the underlying chunked or raw parser.
*/
std::optional<off_t>
HttpBodyDecompression::TotalBodySize() const noexcept
{
	return fBodyParser->TotalBodySize();
}
