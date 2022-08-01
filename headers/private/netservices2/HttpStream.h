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
class HttpBuffer;


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




} // namespace Network

} // namespace BPrivate

#endif // _HTTP_STREAM_H_
