// Channel.h

#ifndef NET_FS_CHANNEL_H
#define NET_FS_CHANNEL_H

#include <SupportDefs.h>

class Channel {
protected:
								Channel();

public:
	virtual						~Channel();

	virtual	void				Close() = 0;

	virtual	status_t			Send(const void* buffer, int32 size) = 0;
	virtual	status_t			Receive(void* buffer, int32 size) = 0;
};

#endif	// NET_FS_CHANNEL_H
