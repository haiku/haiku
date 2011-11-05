/*
 * Copyright 2008 Ralf Schülke, ralf.schuelke@googlemail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <stdlib.h>

#include <Application.h>
#include <Catalog.h>

#include "Pairs.h"
#include "PairsWindow.h"

const char* kSignature = "application/x-vnd.Haiku-Pairs";


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


int
main(void)
{
	Pairs pairs;
	pairs.Run();

	return 0;
}
