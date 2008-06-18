/*
 * Copyright 2008, Haiku.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Michael Pfeiffer <laplace@users.sourceforge.net>
 */

#ifndef _PPD_H
#define _PPD_H

#include "Statement.h"

// PostScript Printer Definiton
class PPD : public StatementList {
private:
	StatementList fSymbols;

public:
	PPD();
	virtual ~PPD();
	
	// Prints the PPD to stdout in XML		
	void Print();
};

#endif

