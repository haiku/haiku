/*
* Copyright 2017, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Adrien Destugues <pulkomandy@pulkomandy.tk>
*/
#ifndef LPSTYLDATA_H
#define LPSTYLDATA_H


#include "PrinterData.h"


class BNode;


class LpstylData : public PrinterData {
public:
					LpstylData(BNode* node)
					:
					PrinterData(node)
					{
					}

	// PrinterData overrides
	virtual	void	Save();
};

#endif // LPSTYLDATA_H

