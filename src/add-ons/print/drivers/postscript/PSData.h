/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*/
#ifndef PSDATA_H
#define PSDATA_H


#include "PrinterData.h"

#include <String.h>


class BNode;


class PSData : public PrinterData {
public:
					PSData(BNode* node)
					:
					PrinterData(node)
					{
					}

	// PrinterData overrides
	virtual	void	Load();
	virtual	void	Save();

	BString fPPD;
};

#endif // PSDATA_H
