// InsecureConnection.h

#ifndef NET_FS_INSECURE_CONNECTION_H
#define NET_FS_INSECURE_CONNECTION_H

#ifdef HAIKU_TARGET_PLATFORM_BEOS
#	include <socket.h>
#else
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

#include "AbstractConnection.h"

// InsecureConnection
class InsecureConnection : public AbstractConnection {
public:
								InsecureConnection();
	virtual						~InsecureConnection();

			status_t			Init(int socket);				// server side
	virtual	status_t			Init(const char* parameters);	// client side

			status_t			FinishInitialization();

private:
			status_t			_OpenClientChannel(in_addr serverAddr,
									uint16 port, Channel** channel);

			status_t			_SendErrorReply(Channel* channel,
									status_t error);
};

// InsecureConnectionDefs
namespace InsecureConnectionDefs {

	// ConnectRequest
	struct ConnectRequest {
		int32	protocolVersion;
		uint32	serverAddress;
		int32	upStreamChannels;
		int32	downStreamChannels;
	};
	
	// ConnectReply
	struct ConnectReply {
		int32	error;
		int32	upStreamChannels;
		int32	downStreamChannels;
		uint16	port;
	};

	extern const int32 kProtocolVersion;
	extern const bigtime_t kAcceptingTimeout;

	// number of client up/down stream channels
	extern const int32 kMinUpStreamChannels;
	extern const int32 kMaxUpStreamChannels;
	extern const int32 kDefaultUpStreamChannels;
	extern const int32 kMinDownStreamChannels;
	extern const int32 kMaxDownStreamChannels;
	extern const int32 kDefaultDownStreamChannels;
	
} // namespace InsecureConnectionDefs

#endif	// NET_FS_INSECURE_CONNECTION_H
