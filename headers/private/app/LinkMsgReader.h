/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Pahtz <pahtz@yahoo.com.au>
 */
#ifndef _LINKMSGREADER_H
#define _LINKMSGREADER_H

#include <OS.h>


//namespace BPrivate {

class LinkMsgReader {
	public:
		LinkMsgReader(port_id port);
		virtual ~LinkMsgReader(void);

		void SetPort(port_id port);
		port_id	Port(void) { return fReceivePort; }

		status_t GetNextMessage(int32 &code, bigtime_t timeout = B_INFINITE_TIMEOUT);
		status_t Read(void *data, ssize_t size);
		status_t ReadString(char **string);
		template <class Type> status_t Read(Type *data)
		{
			return Read(data, sizeof(Type));
		}

	protected:
		virtual status_t ReadFromPort(bigtime_t timeout);
		virtual status_t AdjustReplyBuffer(bigtime_t timeout);
		void ResetBuffer();

		port_id fReceivePort;

		char	*fRecvBuffer;

		int32	fRecvPosition;	//current read position

		int32	fRecvStart;	//start of current message

		int32	fRecvBufferSize;

		int32	fDataSize;	//size of data in recv buffer
		int32	fReplySize;	//size of current reply message

		status_t fReadError;	//Read failed for current message
};

//}	// namespace BPrivate

#endif
