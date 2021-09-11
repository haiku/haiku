/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_
#define _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_


#include <Handler.h>
#include <Message.h>
#include <UrlProtocolDispatchingListener.h>


namespace BPrivate {

namespace Network {


class BUrlProtocolAsynchronousListener : public BHandler,
	public BUrlProtocolListener {
public:
								BUrlProtocolAsynchronousListener(
									bool transparent = false);
	virtual						~BUrlProtocolAsynchronousListener();

	// Synchronous listener access
			BUrlProtocolListener* SynchronousListener();

	// BHandler interface
	virtual	void				MessageReceived(BMessage* message);

private:
			BUrlProtocolDispatchingListener*
						 		fSynchronousListener;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_URL_PROTOCOL_ASYNCHRONOUS_LISTENER_H_
