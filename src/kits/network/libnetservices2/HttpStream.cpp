/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels Sascha Reedijk, niels.reedijk@gmail.com
 */

#include <HttpStream.h>

#include <DataIO.h>
#include <HttpRequest.h>

using namespace BPrivate::Network;


BHttpRequestStream::BHttpRequestStream(const BHttpRequest& request)
	: fHeader(std::make_unique<BMallocIO>()), fBody(nullptr)
{
	// Serialize the header of the request to text
	fTotalSize = request.SerializeHeaderTo(fHeader.get());
	fBodyOffset = fTotalSize;
	// TODO: add size of request body to total size
}


BHttpRequestStream::~BHttpRequestStream() = default;


BHttpRequestStream::TransferInfo
BHttpRequestStream::Transfer(BDataIO* target)
{
	if (fCurrentPos == fTotalSize)
		return TransferInfo{0, fTotalSize, fTotalSize, true};

	off_t bytesWritten = 0;

	if (fCurrentPos < fBodyOffset) {
		// Writing the header
		auto remainingSize = fBodyOffset - fCurrentPos;
		bytesWritten = target->Write(
			static_cast<const char*>(fHeader->Buffer()) + fCurrentPos, remainingSize);
		if (bytesWritten == B_WOULD_BLOCK)
			return TransferInfo{0, 0, fTotalSize, false};
		else if (bytesWritten < 0)
			throw BSystemError("BDataIO::Write()", bytesWritten);

		fCurrentPos += bytesWritten;

		if (bytesWritten < remainingSize) {
			return TransferInfo{bytesWritten, fCurrentPos, fTotalSize, false};
		}
	}

	// Write the body
	if (fBody) {
		// TODO
		throw BRuntimeError(__PRETTY_FUNCTION__, "Not implemented");
	}
	return TransferInfo{bytesWritten, fTotalSize, fTotalSize, true};
}
