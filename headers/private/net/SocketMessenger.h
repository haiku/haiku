/*
 * Copyright 2011-2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef SOCKET_MESSENGER_H
#define SOCKET_MESSENGER_H

#include <Socket.h>

class BMessage;
class BMessenger;


class BSocketMessenger {
public:
								BSocketMessenger();
								BSocketMessenger(
									const BNetworkAddress& address,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
								// adopt an existing already connected socket.
								BSocketMessenger(const BSocket& socket);
	virtual						~BSocketMessenger();

			void				Unset();
			status_t			SetTo(const BNetworkAddress& address,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
			status_t			SetTo(const BSocketMessenger& target,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

			status_t			InitCheck() const { return fInitStatus; }

			const BNetworkAddress& Address() const { return fSocket.Peer(); }

	virtual	status_t			SendMessage(const BMessage& message);
	virtual	status_t			SendMessage(const BMessage& message,
									BMessage& _reply,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual	status_t			SendMessage(const BMessage& message,
									BMessenger& replyTarget,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual	status_t			SendReply(const BMessage& message,
									const BMessage& reply);

								// wait for unsolicited message on socket
	virtual	status_t			ReceiveMessage(BMessage& _message,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

private:
			struct Private;
private:
								BSocketMessenger(const BSocketMessenger&);
			BSocketMessenger&	operator=(const BSocketMessenger&);

			void				_Init();
			status_t			_WaitForMessage(bigtime_t timeout);
			status_t			_SendMessage(const BMessage& message);
			status_t			_ReadMessage(BMessage& _message);
			status_t			_ReadReply(int64 replyID,
									BMessage& _reply, bigtime_t timeout);

	static	status_t			_MessageReader(void* arg);

private:
			Private*			fPrivateData;
			BSocket				fSocket;
			status_t			fInitStatus;
};

#endif	// SOCKET_MESSENGER_H
