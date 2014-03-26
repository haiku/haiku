/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CONNECTION_TEST_H
#define CONNECTION_TEST_H


#include <Application.h>
#include <String.h>


class ConnectionTest : public BApplication {
public:
								ConnectionTest(const char* username,
									const char* password);
	virtual						~ConnectionTest();

	virtual	void				ReadyToRun();

private:
			void				_TestGetPackage();
			void				_TestDownloadIcon();
private:
			BString				fUsername;
			BString				fPassword;
};


#endif // CONNECTION_TEST_H
