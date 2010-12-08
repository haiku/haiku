/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Ithamar R. Adema <ithamar.adema@team-embedded.nl>
*		Michael Pfeiffer
*/
#ifndef GP_DATA_H
#define GP_DATA_H


#include "PrinterData.h"

#include <String.h>


class BNode;


class GPData : public PrinterData {
public:
					GPData(BNode* node)
						:
						PrinterData(node)
						{
						}

	// PrinterData overrides
	virtual	void	Load();
	virtual	void	Save();

	BString fGutenprintDriverName;
};

#endif // PSDATA_H
