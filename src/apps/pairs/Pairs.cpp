/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		John Scipione, jscipione@gmail.com
 */


#include "Pairs.h"

#include <stdlib.h>

#include <Application.h>
#include <Catalog.h>

#include "PairsWindow.h"


const char* kSignature = "application/x-vnd.Haiku-Pairs";


//	#pragma mark - Pairs


Pairs::Pairs()
	:
	BApplication(kSignature),
	fWindow(NULL)
{
}


Pairs::~Pairs()
{
}


void
Pairs::ReadyToRun()
{
	fWindow = new PairsWindow();
	fWindow->Show();
}


void
Pairs::RefsReceived(BMessage* message)
{
	fWindow->PostMessage(message);
}


void
Pairs::MessageReceived(BMessage* message)
{
	BApplication::MessageReceived(message);
}


//	#pragma mark - main


int
main(void)
{
	Pairs pairs;
	pairs.Run();

	return 0;
}
