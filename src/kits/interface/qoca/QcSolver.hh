// Generated automatically from QcSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcSolver.H"
#ifndef QcSolverIFN
#define QcSolverIFN
#line 1 "QcSolver.ch"
// $Id: QcSolver.ch,v 1.21 2001/01/30 01:32:08 pmoulder Exp $

//============================================================================//
// Initial version by by Alan Finlay and Sitt Sen Chok
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
// THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY	EXPRESSED OR  //
// IMPLIED.  ANY USE IS AT YOUR OWN RISK.                                     //
//                                                                            //
// Permission is hereby granted to use or copy this program for any purpose,  //
// provided the above notices are retained on all copies.  Permission to      //
// modify the code and to distribute modified code is granted, provided the   //
// above notices are retained, and a notice that the code was modified is     //
// included with the above copyright notice.                                  //
//============================================================================//

#include <vector.h>
#include <algo.h>
#include "qoca/QcConstraint.hh"







//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
inline QcSolver::QcSolver()
  : fEditVarsSetup(false), fAutoSolve(false), fBatchAddConst(false)
{
}


#line 52 "QcSolver.ch"
#if qcCheckInternalInvar
inline void
QcSolver::assertInvar() const
{
}


#line 79 "QcSolver.ch"
#endif


//-----------------------------------------------------------------------//
// Variable management methods                                           //
//-----------------------------------------------------------------------//


#line 145 "QcSolver.ch"
inline void
QcSolver::SuggestValue(QcFloat &v, numT desval)
{
  v.SuggestValue (desval);
}






//-----------------------------------------------------------------------//
// Enquiry functions for variables                                       //
//-----------------------------------------------------------------------//

#line 214 "QcSolver.ch"
/* ----------------------------------------------------------------------
   High Level Edit Variable Interface for use by solver clients. Edit
   variables are set and unset with the following interface. The
   semantics is that of a set.  Edit variables have their weight
   multiplied by the a factor which may be specified when the solver is
   constructed (else a default factor is used). EndEdit calls RestSolver
   for the respective edit variables.  It is not necessary for edit
   variables to be registered with the solver in advance, AddEditVar
   registers them automatically.
   --------------------------------------------------------------------- */



#line 285 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//


#line 293 "QcSolver.ch"
inline vector<QcConstraint> const &QcSolver::Inconsistant() const
{
  return fInconsistant;
}


#line 447 "QcSolver.ch"
inline void
QcSolver::BeginAddConstraint()
{
  qcAssertExternalPre( !fBatchAddConst);

  fBatchConstraints.clear();
  fBatchAddConst = true;
  fBatchAddConstFail = false;
}


#line 461 "QcSolver.ch"
inline bool
QcSolver::inBatch()
{
  return fBatchAddConst;
}

inline bool
QcSolver::ChangeRHS (QcConstraint &c, numT rhs)
{
  return ChangeConstraint (c, rhs);
}

inline bool
QcSolver::changeRHS (QcConstraintRep *c, numT rhs)
{
  return changeConstraint (c, rhs);
}





#line 546 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Constraint Solving methods                                            //
//-----------------------------------------------------------------------//

#line 579 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Low Level Edit Variable Interface for use by solvers. These functions //
// manipulate the set of edit variables without any adjustment of the    //
// variable weights or automatic registration on addition or RestSolver()//
// on removal.
//-----------------------------------------------------------------------//


inline void
QcSolver::RegisterEditVar(QcFloat &v)
{
  if (IsEditVar( v))
    return;

  fEditVars.push_back( v);
  fEditVarsSetup = false;
}



#line 625 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


#line 653 "QcSolver.ch"
#if qcCheckPost

inline bool
QcSolver::hasConstraint(QcConstraintRep const *c) const
{
  return (find (checkedConstraints.begin(),
		checkedConstraints.end(),
		c)
	  != checkedConstraints.end());
}

inline bool
QcSolver::hasConstraint(QcConstraint const &c) const
{
  return hasConstraint (c.pointer());
}


#line 681 "QcSolver.ch"
inline void
QcSolver::addCheckedConstraint(QcConstraintRep *c)
{
  checkedConstraints.push_back( c);
  c->Increase();
}


#line 723 "QcSolver.ch"
inline void
QcSolver::changeCheckedConstraint(QcConstraintRep *oldc, numT rhs)
{
  oldc->SetRHS( rhs);
}

inline vector<QcConstraintRep *>::const_iterator
QcSolver::checkedConstraints_abegin() const
{
  return checkedConstraints.begin();
}

inline vector<QcConstraintRep *>::const_iterator
QcSolver::checkedConstraints_aend() const
{
  return checkedConstraints.end();
}

inline vector<QcConstraintRep *>::size_type
QcSolver::checkedConstraints_size() const
{
  return checkedConstraints.size();
}

#endif


#line 769 "QcSolver.ch"
#ifndef qcNoStream
inline ostream &operator<< (ostream &os, const QcSolver &s)
{
  s.Print (os);
  return os;
}
#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcSolverIFN */
