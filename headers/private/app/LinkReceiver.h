/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Pahtz <pahtz@yahoo.com.au>
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef _LINK_RECEIVER_H
#define _LINK_RECEIVER_H


#include <OS.h>


class BGradient;
class BString;
class BRegion;


namespace BPrivate {

class LinkReceiver {
	public:
		LinkReceiver(port_id port);
		virtual ~LinkReceiver(void);

		void SetPort(port_id port);
		port_id	Port(void) const { return fReceivePort; }

		status_t GetNextMessage(int32& code, bigtime_t timeout = B_INFINITE_TIMEOUT);
		bool HasMessages() const;
		bool NeedsReply() const;
		int32 Code() const;

		virtual status_t Read(void* data, ssize_t size);
		status_t ReadString(char** _string, size_t* _length = NULL);
		status_t ReadString(BString& string, size_t* _length = NULL);
		status_t ReadString(char* buffer, size_t bufferSize);
		status_t ReadRegion(BRegion* region);
		status_t ReadGradient(BGradient** gradient);

		template <class Type> status_t Read(Type *data)
			{ return Read(data, sizeof(Type)); }

	protected:
		virtual status_t ReadFromPort(bigtime_t timeout);
		virtual status_t AdjustReplyBuffer(bigtime_t timeout);
		void ResetBuffer();

		port_id fReceivePort;

		char*	fRecvBuffer;
		int32	fRecvPosition;	//current read position
		int32	fRecvStart;	//start of current message
		int32	fRecvBufferSize;

		int32	fDataSize;	//size of data in recv buffer
		int32	fReplySize;	//size of current reply message

		status_t fReadError;	//Read failed for current message
};

}	// namespace BPrivate

#endif	// _LINK_RECEIVER_H
