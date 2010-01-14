// RequestChannel.h

#ifndef NET_FS_REQUEST_CHANNEL_H
#define NET_FS_REQUEST_CHANNEL_H

#include <SupportDefs.h>

class Channel;
class Request;

class RequestChannel {
public:
								RequestChannel(Channel* channel);
								~RequestChannel();

			status_t			SendRequest(Request* request);
			status_t			ReceiveRequest(Request** request);

private:
			status_t			_GetRequestSize(Request* request, int32* size);

private:
			class ChannelWriter;
			class MemoryReader;
			struct RequestHeader;

			Channel*			fChannel;
			void*				fBuffer;
			int32				fBufferSize;
};

#endif	// NET_FS_REQUEST_CHANNEL_H
