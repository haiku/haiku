/*
 * PrinterData.cpp
 * Copyright 1999-2000 Y.Takagi All Rights Reserved.
 */

#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <StorageDefs.h>

#include "PrinterData.h"

const char *PD_DRIVER_NAME      = "Driver Name";
const char *PD_PRINTER_NAME     = "Printer Name";
const char *PD_COMMENTS         = "Comments";
const char *PD_TRANSPORT        = "transport";

PrinterData::PrinterData(BNode *node)
{
	if (node) {
		load(node);
	}
}

PrinterData::~PrinterData()
{
}

/*
PrinterData::PrinterData(const PrinterData &printer_data)
{
	fDriverName  = printer_data.fDriverName;
	fPrinterName = printer_data.fPrinterName;
	fComments     = printer_data.fComments;
	fTransport    = printer_data.fTransport;
	fNode         = printer_data.fNode;
}

PrinterData &PrinterData::operator = (const PrinterData &printer_data)
{
	fDriverName  = printer_data.fDriverName;
	fPrinterName = printer_data.fPrinterName;
	fComments     = printer_data.fComments;
	fTransport    = printer_data.fTransport;
	fNode         = printer_data.fNode;
	return *this;
}
*/

void PrinterData::load(BNode *node)
{
	char buffer[512];

	fNode = node;

	fNode->ReadAttr(PD_DRIVER_NAME,  B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fDriverName = buffer;
	fNode->ReadAttr(PD_PRINTER_NAME, B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fPrinterName = buffer;
	fNode->ReadAttr(PD_COMMENTS,     B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fComments = buffer;
	fNode->ReadAttr(PD_TRANSPORT,    B_STRING_TYPE, 0, buffer, sizeof(buffer));
	fTransport = buffer;
}

/*
void PrinterData::save(BNode *node)
{
	BDirectory dir;
	if (node == NULL) {
		BPath path;
		::find_directory(B_USER_PRINTERS_DIRECTORY, &path, false);
		path.Append(fPrinterName.c_str());
		BDirectory base_dir;
		base_dir.CreateDirectory(path.Path(), &dir);
		node = &dir;
	}
	node->WriteAttr(PD_DRIVER_NAME,  B_STRING_TYPE, 0, driver_name_.c_str(),  driver_name_.length()  + 1);
	node->WriteAttr(PD_PRINTER_NAME, B_STRING_TYPE, 0, printer_name_.c_str(), printer_name_.length() + 1);
	node->WriteAttr(PD_COMMENTS,     B_STRING_TYPE, 0, comments_.c_str(),     comments_.length()     + 1);
}
*/

bool PrinterData::getPath(char *buf) const
{
	if (fNode) {
		node_ref nref;
		fNode->GetNodeRef(&nref);
		BDirectory dir;
		dir.SetTo(&nref);
		BPath path(&dir, NULL);
		strcpy(buf, path.Path());
		return true;
	}
	*buf = '\0';
	return false;
}
