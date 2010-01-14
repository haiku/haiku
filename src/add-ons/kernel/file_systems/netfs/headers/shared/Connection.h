// Connection.h

#ifndef NET_FS_CONNECTION_H
#define NET_FS_CONNECTION_H

#include <OS.h>

class Channel;
class SecurityContext;
class User;

// Connection
class Connection {
protected:
								Connection();

public:
	virtual						~Connection();

	virtual	status_t			Init(const char* parameters) = 0;
	virtual	void				Close() = 0;

	virtual	int32				CountDownStreamChannels() const = 0;
	virtual	Channel*			DownStreamChannelAt(int32 index) const = 0;

	virtual	status_t			GetUpStreamChannel(Channel** channel,
									bigtime_t timeout = B_INFINITE_TIMEOUT) = 0;
	virtual	status_t			PutUpStreamChannel(Channel* channel) = 0;
};

#endif	// NET_FS_CONNECTION_H
