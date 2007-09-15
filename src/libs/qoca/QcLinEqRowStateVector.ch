// $Id: QcLinEqRowStateVector.ch,v 1.3 2000/12/13 01:44:58 pmoulder Exp $

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

//o[d] #include <qoca/QcQuasiRowStateVector.H>
//o[i] #include <qoca/QcQuasiRowStateVector.hh>
#include <qoca/QcLinEqRowState.hh>

//o[d] class QcLinEqRowStateVector
//o[d]   : public QcQuasiRowStateVector
//o[d] {
//o[d] public:

//begin o[d]
	//-----------------------------------------------------------------------//
	// Constructor.                                                          //
	//-----------------------------------------------------------------------//
	QcLinEqRowStateVector()
		: QcQuasiRowStateVector()
		{ }

	//-----------------------------------------------------------------------//
	// Query functions.                                                      //
	//-----------------------------------------------------------------------//
	int GetBasicVar(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcLinEqRowState *)fStates[i])->fBasicVar;
	}

	int GetCondition(unsigned i) const
	{
		qcAssertPre (i < fSize);
		return ((QcLinEqRowState *)fStates[i])->getCondition();
	}

	//-----------------------------------------------------------------------//
	// Set functions.                                                        //
	//-----------------------------------------------------------------------//
	void SetBasicVar(unsigned i, int vi)
	{
		qcAssertPre (i < fSize);
		((QcLinEqRowState *)fStates[i])->fBasicVar = vi;
	}

	void SetCondition(unsigned i, int c)
	{
		qcAssertPre (i < fSize);
		((QcLinEqRowState *)fStates[i])->setCondition( c);
	}
//end o[d]


//cf
virtual void
Alloc(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fCapacity);

  for (unsigned i = start - fStates;
       start != finish;
       start++, i++)
    *start = new QcLinEqRowState( i);
}

//cf
virtual void
RemoveFromList(QcState **start, QcState **finish)
{
  qcAssertPre( fStates <= start);
  qcAssertPre( start <= finish);
  qcAssertPre( finish <= fStates + fSize);

  for(; start != finish; start++)
    {
      QcLinEqRowState *state = CAST(QcLinEqRowState *, *start);

      state->setCondition( QcLinEqRowState::fInvalid);
      state->fBasicVar = QcTableau::fInvalidVariableIndex;
      QcQuasiRowStateVector::Restart( start - fStates);
    }
}

//o[d] };


/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
