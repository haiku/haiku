/*
 * Copyright 2022 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _HTTP_STREAM_H_
#define _HTTP_STREAM_H_

#include <memory>

class BDataIO;
class BMallocIO;
class BString;


namespace BPrivate {

namespace Network {

class BHttpRequest;


class BAbstractDataStream {
public:
	struct TransferInfo {
		off_t currentBytesWritten;
		off_t totalBytesWritten;
		off_t totalSize;
		bool complete;
	};

	virtual	TransferInfo	Transfer(BDataIO*) = 0;
};


class BHttpRequestStream : public BAbstractDataStream {
public:
							BHttpRequestStream(const BHttpRequest& request);
							~BHttpRequestStream();

	virtual	TransferInfo	Transfer(BDataIO* target) override;

private:
	std::unique_ptr<BMallocIO>		fHeader;
	BDataIO*						fBody;
	off_t							fTotalSize = 0;
	off_t							fBodyOffset = 0;
	off_t							fCurrentPos = 0;
};


} // namespace Network

} // namespace BPrivate

#endif // _HTTP_STREAM_H_
