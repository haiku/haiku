/*
*******************************************************************************
*
*   Copyright (C) 1999-2001, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  genprops.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999dec13
*   created by: Markus W. Scherer
*
*	adapted for use under BeOS by Axel Dörfler, axeld@pinc-software.de.
*/

#ifndef __GENPROPS_H__
#define __GENPROPS_H__

#include <SupportDefs.h>
#include "utf.h"


// special casing data
struct SpecialCasing {
	uint32	code;
	bool	isComplex;
	UChar	lowerCase[32], upperCase[32], titleCase[32];
};

// case folding data
struct CaseFolding {
	uint32	code, simple;
	char	status;
	UChar	full[32];
};

// character properties
struct Props {
	uint32	code, lowerCase, upperCase, titleCase, mirrorMapping;
	int16	decimalDigitValue, digitValue; /* -1: no value */
	int32	numericValue; /* see hasNumericValue */
	uint32	denominator; /* 0: no value */
	uint8	generalCategory, canonicalCombining, bidi, isMirrored, hasNumericValue;
	SpecialCasing *specialCasing;
	CaseFolding *caseFolding;
};

// global flags
extern bool gBeVerbose;

// name tables
extern const char *const bidiNames[];
extern const char *const genCategoryNames[];

// prototypes
extern void initStore(void);
extern uint32 makeProps(Props *p);
extern void addProps(uint32 c, uint32 props);
extern void repeatProps(uint32 first, uint32 last, uint32 props);
extern void compactStage2(void);
extern void compactStage3(void);
extern void compactProps(void);
extern status_t generateData(const char *dataDir);

#endif

