/*
 * Copyright 2002-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 *		Simon Gauvin
 *		Michael Pfeiffer
 */

#include <AppKit.h>

extern "C"
{
	BMessage * take_job(BFile * spool_file, BNode * spool_dir, BMessage * msg);
	BMessage * config_page(BNode * spool_dir, BMessage * msg);
	BMessage * config_job(BNode * spool_dir, BMessage * msg);
	char * add_printer(char * printer_name);
	BMessage * default_settings(BNode * printer);
}

class PrinterDriver;

// instanciate_driver has to be implemented by the printer driver
PrinterDriver *instanciate_driver(BNode *spoolDir);
