// PortConnection.h

#ifndef NET_FS_PORT_CONNECTION_H
#define NET_FS_PORT_CONNECTION_H

#include <OS.h>

#include "AbstractConnection.h"
#include "PortChannel.h"

// PortConnection
class PortConnection : public AbstractConnection {
public:
								PortConnection();
	virtual						~PortConnection();

			status_t			Init(Channel* channel, int32 upStreamChannels,
									int32 downStreamChannels);	// server side
	virtual	status_t			Init(const char* parameters);	// client side

			status_t			FinishInitialization();

private:
	static	status_t			_CreateChannel(PortChannel** channel,
									PortChannel::Info* info = NULL,
									bool inverse = false);

private:
			int32				fUpStreamChannels;
			int32				fDownStreamChannels;
};

// PortConnectionDefs
namespace PortConnectionDefs {

	// ConnectRequest
	struct ConnectRequest {
		int32				protocolVersion;
		int32				upStreamChannels;
		int32				downStreamChannels;
		PortChannel::Info	channelInfo;
	};

	// ConnectReply
	struct ConnectReply {
		int32	error;
		int32	upStreamChannels;
		int32	downStreamChannels;
	};

	extern const int32 kProtocolVersion;
	extern const char* kPortConnectionPortName;

	// number of client up/down stream channels
	extern const int32 kMinUpStreamChannels;
	extern const int32 kMaxUpStreamChannels;
	extern const int32 kDefaultUpStreamChannels;
	extern const int32 kMinDownStreamChannels;
	extern const int32 kMaxDownStreamChannels;
	extern const int32 kDefaultDownStreamChannels;
}

#endif	// NET_FS_PORT_CONNECTION_H
