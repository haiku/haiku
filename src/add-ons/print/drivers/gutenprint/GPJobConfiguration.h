/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_JOB_CONFIGURATION_H
#define GP_JOB_CONFIGURATION_H

#include<String.h>

#include <map>
#include <string>

using namespace std;

class GPJobConfiguration
{
public:
			GPJobConfiguration();

	BString	fDriver;
	BString	fPageSize;
	BString	fResolution;
	BString	fInputSlot;
	BString	fPrintingMode;

	int fXDPI;
	int fYDPI;

	map<string, string> fStringSettings;
	map<string, bool> fBooleanSettings;
	map<string, int32> fIntSettings;
	map<string, int32> fDimensionSettings;
	map<string, double> fDoubleSettings;
};

#endif
