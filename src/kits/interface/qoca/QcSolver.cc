// Generated automatically from QcSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcSolver.hh"
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
#include <qoca/QcFixedFloatRep.hh>






//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
#line 52 "QcSolver.ch"
#if qcCheckInternalInvar
#line 59 "QcSolver.ch"
void
QcSolver::assertDeepInvar() const
{
  assertInvar();
  for (int i = fInconsistant.size(); --i >= 0;)
    fInconsistant[i].assertInvar();
  for (int i = fEditVars.size(); --i >= 0;)
    fEditVars[i].assertInvar();
  for (int i = fBatchConstraints.size(); --i >= 0;)
    fBatchConstraints[i].assertInvar();
  for (int i = checkedConstraints.size(); --i >= 0;)
    checkedConstraints[i]->assertInvar();
}


void
QcSolver::vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif


//-----------------------------------------------------------------------//
// Variable management methods                                           //
//-----------------------------------------------------------------------//


#line 96 "QcSolver.ch"
void
QcSolver::addVar (QcFloatRep *v)
{
  QcFloat v1 (v);
  AddVar (v1);
  qcAssertPost (IsRegistered (v1));
  QcFloat v2 (v);
  assert (v2.Id() == v1.Id());
  assert (!(v2 < v1));
  assert (!(v1 < v2));
  qcAssertPost (IsRegistered (v2));
}



void QcSolver::bad_call (char const *method)
{
  cerr << "should not call " << method << endl;
  abort();
}



#line 125 "QcSolver.ch"
bool
QcSolver::RemoveVar (QcFloat &v)
{
  UNUSED(v);
  bad_call ("QcSolver::RemoveVar");
}


#line 156 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Enquiry functions for variables                                       //
//-----------------------------------------------------------------------//

#line 169 "QcSolver.ch"
bool QcSolver::IsFree (QcFloat const &v) const
{
  UNUSED(v);
  bad_call ("QcSolver::IsFree");
}



#line 194 "QcSolver.ch"
bool
QcSolver::IsEditVar(QcFloat const &v) const
{
  vector<QcFloat>::const_iterator it;

  for (it = fEditVars.begin(); it != fEditVars.end(); ++it)
    if ((*it) == v)
      return true;

  return false;
}


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



void
QcSolver::AddEditVar(QcFloat &v)
{
  v.SetToEditWeight();
  RegisterEditVar (v);
}


#line 268 "QcSolver.ch"
void
QcSolver::EndEdit()
{
  for (vector<QcFloat>::iterator evi = fEditVars.begin(), eviEnd = fEditVars.end();
       evi != eviEnd;
       ++evi)
    evi->SetToStayWeight();

  ClearEditVars();
}


#line 285 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//


#line 313 "QcSolver.ch"
#ifdef qcCheckPost /* swallow nyi unless compiled with qcCheckPost defined */
bool QcSolver::swallow (QcSolver *other)
{
  assert (other != NULL);
  assert (other != this);

  /* TODO: Consider transferring the variables too.  E.g. for v in
     other.getVariables; do addVar(v); done.  (Except that
     getVariables currently returns "internal" variables.) */
  for (vector<QcConstraintRep *>::iterator ci = other->checkedConstraints.begin(),
	 ciEnd = other->checkedConstraints.end();
       ci != ciEnd; ci++)
    if (!addConstraint (*ci))
      return false;
  return true;
}
#endif




#line 383 "QcSolver.ch"
void
QcSolver::AddWeightedConstraint(QcConstraint &c, numT weight)
{
  /* The reason that we create a new constraint object instead of adding the
     variable to the existing constraint is so that adding then removing then
     adding a given constraint won't create more and more variables, and so that
     user operations on c (such as printing it) don't show the error variable.

     OTOH, removeConstraint could get rid of the extra term from the constraint. */

  QcLinPoly &wp = const_cast<QcLinPoly &>( c.LinPoly());
  QcFloatRep *v;
  switch (c.getRelation())
    {
    case QcConstraintRep::coEQ:
      v = new QcFixedFloatRep( "cnWeight", 0, weight, false);
      wp.push( 1, v);
      goto ok;

      //case QcConstraintRep::coLT:
    case QcConstraintRep::coLE:
      v = new QcFixedFloatRep( "cnWeight", 0, weight, true);
      wp.push( -1, v);
      goto ok;

      //case QcConstraintRep::coGT:
    case QcConstraintRep::coGE:
      v = new QcFixedFloatRep( "cnWeight", 0, weight, true);
      wp.push( 1, v);
      goto ok;
    }
  qcDurchfall( "relation");

 ok:
#if 0 /* For the moment we'll use the same object, to keep the hash tables happy. */
  QcConstraintRep *wc = new QcConstraintRep( *c.pointer());
  long refcnt = wc->Counter();
  wc->SetLinPoly( wp);
  addConstraint( wc);
#else
  AddConstraint( c);
#endif
  v->Decrease();
  assert( v->Counter() > 0);
  /* proof: v->Counter is 1 at creation, it increases by 1 in addConstraint
     and/or when added to the end of wc, and the Decrease cancels out the creation
     value.

     relevance: Decrease() doesn't call delete.  And if it did, then presumably
     we'd have a garbage variable in the solver. */

#if 0
  assert( refcnt);
  while (--refcnt)
    wc->Decrease();
  wc->Drop();
#endif
}



#line 497 "QcSolver.ch"
bool
QcSolver::EndAddConstraint()
{
  qcAssertExternalPre( inBatch());
  fBatchAddConst = false;

  if (fBatchAddConstFail)
    {
      vector<QcConstraint>::iterator it;

      for (it = fBatchConstraints.begin(); it != fBatchConstraints.end(); it++)
	RemoveConstraint (*it);
    }

  fBatchConstraints.clear();

  return !fBatchAddConstFail;
}



#line 522 "QcSolver.ch"
bool QcSolver::RemoveConstraint(QcConstraint &c)
{
  UNUSED(c);
  bad_call ("QcSolver::RemoveConstraint");
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


#line 599 "QcSolver.ch"
void QcSolver::RemoveEditVar (QcFloat &v)
{
  vector<QcFloat>::iterator it;
  int last = fEditVars.size() - 1;

  for (it = fEditVars.begin(); it != fEditVars.end(); ++it)
    {
      if ((*it) == v)
	{
	  *it = fEditVars[last];
	  fEditVars.resize(last);
	  fEditVarsSetup = false;
	  break;
	}
    }
}


#line 625 "QcSolver.ch"
//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


void QcSolver::Print (ostream &os) const
{
  if (fEditVarsSetup)
    {
      os << "Edit Variables";
      unsigned size = fEditVars.size();

      for (unsigned i = 0; i < size; i++)
	fEditVars[i].Print(os);
    }
}



void
QcSolver::Restart()
{
  ClearEditVars();
  ClearInconsistant();
  fEditVarsSetup = false;
}


#if qcCheckPost

#line 671 "QcSolver.ch"
void QcSolver::checkSatisfied() const
{
  for (int i = checkedConstraints.size(); --i >= 0;)
    {
      QcConstraintRep *c = checkedConstraints[i];
      qcAssertExternalPost (c->isSatisfied());
    }
}


#line 689 "QcSolver.ch"
void
QcSolver::removeCheckedConstraint(QcConstraintRep *c)
{
  int n1 = checkedConstraints.size() - 1;
  int i = n1;

  if (n1 < 0)
    goto notHere;

  if (checkedConstraints[n1] == c)
    goto trim;

  while (i--)
    {
      if (checkedConstraints[i] == c)
	goto swap;
    }

 notHere:
  qcAssertExternalPre( 0 && "removing non-present constraint");

 swap:
  {
    QcConstraintRep *last;
    last = checkedConstraints[n1];
    checkedConstraints[i] = last;
  }

 trim:
  checkedConstraints.pop_back();
  c->Drop();
}


#line 747 "QcSolver.ch"
#endif


#line 769 "QcSolver.ch"
#ifndef qcNoStream
#line 775 "QcSolver.ch"
#endif

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
