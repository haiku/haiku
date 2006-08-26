// $Id: QcConstraintRep.cc,v 1.7 2000/12/06 05:32:56 pmoulder Exp $

//============================================================================//
// Written by Alan Finlay and Sitt Sen Chok                                   //
//----------------------------------------------------------------------------//
// The QOCA implementation is free software, but it is Copyright (C)          //
// 1994-1999 Monash University.  It is distributed under the terms of the GNU //
// General Public License.  See the file COPYING for copying permission.      //
//                                                                            //
// The QOCA toolkit and runtime are distributed under the terms of the GNU    //
// Library General Public License.  See the file COPYING.LIB for copying      //
// permissions for those files.                                               //
//                                                                            //
// If those licencing arrangements are not satisfactory, please contact us!   //
// We are willing to offer alternative arrangements, if the need should arise.//
//                                                                            //
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#include "qoca/QcDefines.hh"
#include "qoca/QcConstraintRep.hh"

QcConstraintRep::TId QcConstraintRep::fNextValidId = 1;
const QcConstraintRep::TId QcConstraintRep::fInvalidId = 0;

bool QcConstraintRep::FreeName()
{
	bool ok = true;

	if (fName != 0) {	// check for magic4
		char const *magic2 = qcConstraintMagic2;

		for (int i = 0; i < qcConstraintMagic2Len; i++)
			ok &= (fName[i] == magic2[i]);

		if (ok) {
			delete[] fName;
			fName = 0;
		}
	}

	return ok;
}

void QcConstraintRep::Initialize(const char *n, const QcLinPoly &lp, TOp op)
{
	fMagic = qcConstraintMagic1;
	fLinPoly = lp;
	qcAssertExternalPre(VALID_OP(op));
	fOperator = op;
	Reset();
	fName = 0;
	SetName(n);
}

#if qcCheckInternalInvar
void QcConstraintRep::assertInvar() const
{
	qcAssertInvar(fMagic == qcConstraintMagic1);
	qcAssertInvar(fCounter >= 0);
	qcAssertInvar(fId > 0);
	fLinPoly.assertInvar();
	qcAssertInvar(VALID_OP(fOperator));
	qcAssertInvar(fName == 0 || memcmp(fName, qcConstraintMagic2, qcConstraintMagic2Len) == 0);
}
#endif

void QcConstraintRep::Print(ostream &os) const
{
	if (fName != 0)
    		os << Name();

	os << "#" << fId
		<< "(" << fLinPoly << ", ";

	switch (fOperator) {
		case coEQ: os << "=="; 	break;
		  //case coLT: os << "<"; 	break;
		case coLE: os << "<="; 	break;
		  //case coGT: os << ">"; 	break;
		case coGE: os << ">="; 	break;
	}

	os << ", " << fRHS << ")";
}

void QcConstraintRep::SetName(const char *n)
	// Set name sets the field fRep->Name to point to a heap allocated
	// string with a hidden magic number field at the front.
	// The magic number is qcConstraintMagic2 without the '\0' terminator.
	// N.B. Although <strings.h> is pretty standard, for portability
	// we try to avoid using it here as we have no other need for it.
{
	int len = 0;
	char const *magic2 = qcConstraintMagic2;

	// find length of source string
	while (n[len] != '\0')
    		len++;

	qcAssertPost(n[len] == '\0');
	#ifdef qcSafetyChecks
	if (len > 1000)
    		throw QcWarning("QcConstraintRep::SetName name over 1000 chars long");
	#endif

	if (!FreeName()) {	// Deallocate existing name string
		// If FreeName fails, leave old string and allocate a new one
		throw QcWarning("QcConstraintRep::SetName: failed to free old name string");
	}

	fName = new char[len + qcConstraintMagic2Len + 1];
    	// allocate for magic+string

	for (int i = 0; i < qcConstraintMagic2Len; i++)
		fName[i] = magic2[i];

	for (int i = 0; i <= len; i++)
    		fName[i + qcConstraintMagic2Len] = n[i];

	qcAssertPost(fName[len + qcConstraintMagic2Len] == '\0');
}
