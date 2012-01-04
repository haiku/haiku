/*
 * Copyright 2012, Michael Lotz, mmlr@mlotz.ch. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEY_STORE_SERVER_H
#define _KEY_STORE_SERVER_H


#include <Application.h>


class KeyStoreServer : public BApplication {
public:
									KeyStoreServer();
virtual								~KeyStoreServer();

virtual	void						MessageReceived(BMessage* message);

private:
		status_t					_InitKeyStoreDatabase();
};


#endif // _KEY_STORE_SERVER_H
