/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/


#include "PSData.h"

#include <Node.h>

#define PD_PPD_PATH	"ps:ppd_path"


void
PSData::Load()
{
	PrinterData::Load();
	fNode->ReadAttrString(PD_PPD_PATH, &fPPD);
}


void
PSData::Save()
{
	PrinterData::Save();
	fNode->WriteAttrString(PD_PPD_PATH, &fPPD);
}
