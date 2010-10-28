/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef _PRINTERS_H
#define _PRINTERS_H


#include <Application.h>
#include <Catalog.h>


#define PRINTERS_SIGNATURE	"application/x-vnd.Be-PRNT"


class PrintersApp : public BApplication {
			typedef BApplication Inherited;
public:
								PrintersApp();
			void				ReadyToRun();
			void				MessageReceived(BMessage* msg);
			void				ArgvReceived(int32 argc, char** argv);
};

#endif // _PRINTERS_H
