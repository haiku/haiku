/*
 * Copyright 2017 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brian Hill
 */


#include "Repositories.h"

#include <Catalog.h>

#include "constants.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "RepositoriesApplication"

const char* kAppSignature = "application/x-vnd.Haiku-Repositories";


RepositoriesApplication::RepositoriesApplication()
	:
	BApplication(kAppSignature)
{
	fWindow = new RepositoriesWindow();
}


int
main()
{
	RepositoriesApplication myApp;
	myApp.Run();
	return 0;
}
