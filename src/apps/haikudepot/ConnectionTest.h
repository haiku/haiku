/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_TEST_H
#define CONNECTION_TEST_H


#include <Application.h>


class ConnectionTest : public BApplication {
public:
								ConnectionTest();
	virtual						~ConnectionTest();

	virtual	void				ReadyToRun();
};


#endif // CONNECTION_TEST_H
