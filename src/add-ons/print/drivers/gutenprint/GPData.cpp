/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/


#include "GPData.h"

#include <Node.h>

#define PD_PRINTER_DRIVER_ATTRIBUTE	"gutenprint:driver-name"


void
GPData::load()
{
	PrinterData::load();
	fNode->ReadAttrString(PD_PRINTER_DRIVER_ATTRIBUTE,
		&fGutenprintDriverName);
}


void
GPData::save()
{
	PrinterData::save();
	fNode->WriteAttrString(PD_PRINTER_DRIVER_ATTRIBUTE,
		&fGutenprintDriverName);
}
