// AbstractConnection.h

#ifndef NET_FS_ABSTRACT_CONNECTION_H
#define NET_FS_ABSTRACT_CONNECTION_H

#include "Connection.h"
#include "Locker.h"
#include "Vector.h"

class AbstractConnection : public Connection {
protected:
								AbstractConnection();

public:
	virtual						~AbstractConnection();

	virtual	status_t			Init(const char* parameters) = 0;
			status_t			Init();
	virtual	void				Close();

//	virtual	User*				GetAuthorizedUser();

			status_t			AddDownStreamChannel(Channel* channel);
			status_t			AddUpStreamChannel(Channel* channel);

	virtual	int32				CountDownStreamChannels() const;
	virtual	Channel*			DownStreamChannelAt(int32 index) const;

	virtual	status_t			GetUpStreamChannel(Channel** channel,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual	status_t			PutUpStreamChannel(Channel* channel);

protected:
			typedef Vector<Channel*>	ChannelVector;

			status_t			fInitStatus;
			ChannelVector		fDownStreamChannels;
			sem_id				fUpStreamChannelSemaphore;
			Locker				fUpStreamChannelLock;
			ChannelVector		fUpStreamChannels;
			int32				fFreeUpStreamChannels;
};

#endif	// NET_FS_ABSTRACT_CONNECTION_H
