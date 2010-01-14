// PortChannel.h

#ifndef NET_FS_PORT_CHANNEL_H
#define NET_FS_PORT_CHANNEL_H

#include <OS.h>

#include "Channel.h"

class PortChannel : public Channel {
public:
	struct Info {
		port_id	sendPort;
		port_id	receivePort;
	};

public:
								PortChannel();
								PortChannel(const Info* info, bool inverse);
								PortChannel(port_id sendPort,
									port_id receivePort);
	virtual						~PortChannel();

			status_t			InitCheck() const;
			void				GetInfo(Info* info) const;

	virtual	void				Close();

	virtual	status_t			Send(const void* buffer, int32 size);
	virtual	status_t			Receive(void* buffer, int32 size);

private:
			port_id				fSendPort;
			port_id				fReceivePort;
			uint8*				fBuffer;
			int32				fBufferSize;
			int32				fBufferOffset;
			int32				fBufferContentSize;
};

#endif	// NET_FS_PORT_CHANNEL_H
