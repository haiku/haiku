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
//o[c] #include <qoca/QcFixedFloatRep.hh>

//ho class QcSolver
//ho {

//ho public:

//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
inline QcSolver()
  : fEditVarsSetup(false), fAutoSolve(false), fBatchAddConst(false)
{
}

//begin ho
virtual ~QcSolver()
{
}
//end ho


#if qcCheckInternalInvar
inline void
assertInvar() const
{
}

//cf
void
assertDeepInvar() const
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

//cf
virtual void
vAssertDeepInvar() const
{
  assertDeepInvar();
}
#endif


//-----------------------------------------------------------------------//
// Variable management methods                                           //
//-----------------------------------------------------------------------//

/** Registers a variable with a solver.  It is usually not necessary to
    do this, as variables are registered automatically when they are
    first encountered during an <tt>AddConstraint</tt> operation.
    <tt>AddVar</tt> is useful to indicate variables which are to be
    included in <tt>Solve</tt> and <tt>RestSolver</tt> operations when
    not referred to in any of the constraints added to the solver.
**/
virtual void AddVar(QcFloat &v) = 0;

//cf
void
addVar (QcFloatRep *v)
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

//ho static void bad_call (char const *method) __attribute__ ((__noreturn__));
//begin o[c]
void QcSolver::bad_call (char const *method)
{
  cerr << "should not call " << method << endl;
  abort();
}
//end o[c]

/** Removes variable from solver and returns true provided <tt>v</tt> is
    free.  Does nothing and returns false if <tt>v</tt> is not free.

    <p>If <tt>!IsRegistered(v)</tt> then the current behavior is to return
    <tt>false</tt>; maybe presence should be precondition.

**/ //cf
virtual bool
RemoveVar (QcFloat &v)
{
  UNUSED(v);
  bad_call ("QcSolver::RemoveVar");
}

//begin ho
bool
removeVar (QcFloatRep *v)
{
  QcFloat v1 (v);
  return RemoveVar (v1);
}
//end ho

/** Sets the desired value of <tt>v</tt> to <tt>desval</tt>.  This is
    the same doing <tt>v.SuggestValue(desval)</tt>, except it checks
    that <tt>v</tt> is registered in the solver.
**/ //hf
virtual inline void
SuggestValue(QcFloat &v, numT desval)
{
  v.SuggestValue (desval);
}


/** Sets the desired value of all registered variables to their current
    value.  Useful in some situations after a solve. */
virtual void RestSolver() = 0;

//-----------------------------------------------------------------------//
// Enquiry functions for variables                                       //
//-----------------------------------------------------------------------//
/** Indicates whether a variable has been introduced to the solver
    either by virtue of a constraint being added, or via <tt>AddVar</tt>
    (or <tt>AddEditVar</tt>). */
virtual bool IsRegistered (QcFloat const &v) const = 0;

/** Indicates a parametric variable which does not depend on any basic
    variables (i.e. has zero coefficients in all added constraints).  It
    is an error to call <tt>IsFree</tt> for a variable not used by the
    the solver, a warning is issued if safety checks are enabled.
**/ //cf
virtual bool IsFree (QcFloat const &v) const
{
  UNUSED(v);
  bad_call ("QcSolver::IsFree");
}


/** Indicates if a variable used by the tableau is basic.  It is an
    error to call <tt>IsBasic</tt> for a variable not used by the the
    solver; a warning is issued in this case if safety checks are
    enabled.

    <p>For some solvers (<tt>QcNlpSolver</tt>) this is not meaningful.
**/
virtual bool IsBasic(QcFloat const &v) const = 0;

/** Returns true iff <tt>v</tt> is an edit var.  Currently does linear search
    through list of edit vars.

    <p>(Fixing this performance bug requires limiting how many solvers a given
    var can be in, e.g. to 1.  Currently variables can be in more than one
    QcSolver due to the way the solvers work (e.g. Qc*System inheriting from
    QcSolver).  But maybe it's enough to guarantee that &forall;[QcFloat v] v in
    no more than N solvers, where N &le; 32.)
**/ //cf
bool
IsEditVar(QcFloat const &v) const
{
  vector<QcFloat>::const_iterator it;

  for (it = fEditVars.begin(); it != fEditVars.end(); ++it)
    if ((*it) == v)
      return true;

  return false;
}

//begin ho
bool isEditVar (QcFloatRep const *v) const
{
  QcFloat v1(v);
  return IsEditVar (v1);
}
//end ho

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

/** Add <tt>v</tt> to edit vars. */
//cf
virtual void
AddEditVar(QcFloat &v)
{
  v.SetToEditWeight();
  RegisterEditVar (v);
}

//begin ho
void addEditVar (QcFloatRep *v)
{
  QcFloat v1 (v);
  AddEditVar (v1);
}
//end ho

/** Prepare for <tt>resolve</tt> call.  The first call to
    <tt>Resolve</tt> after changing the edit vars or the constraints
    will call <tt>BeginEdit</tt> and incur a slight delay.  An explicit
    <tt>BeginEdit</tt> call avoids that delay by allowing the
    preprocessing to be done at a more convenient time. */
virtual void BeginEdit() = 0;

//begin ho
/** Synonym of <tt>BeginEdit</tt>. */
void beginEdit()
{
  BeginEdit();
}
//end ho

/** For every variable: make it non-edit (i.e.&nbsp;undoes
    <tt>addEditVar</tt>), set it to stay weight, and (if not a
    QcFixedFloat) set its desired value to the current value.

    <p>If both fixed and nomadic variables are in this solver, and not
    (each fixed variable weight is well outside of each nomadic edit
    variable's stay--edit weight range), then <tt>endEdit</tt> may cause
    the solution to be no longer optimal (i.e. with respect to the stay
    weights).
**/
//cf
virtual void
EndEdit()
{
  for (vector<QcFloat>::iterator evi = fEditVars.begin(), eviEnd = fEditVars.end();
       evi != eviEnd;
       ++evi)
    evi->SetToStayWeight();

  ClearEditVars();
}

//begin ho
/** Synonym of <tt>EndEdit</tt>. */
void endEdit()
{ EndEdit(); }
//end ho

//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//

/** Returns constraint handles for all constraints rejected as
    inconsistant since the last call to <tt>ClearInconsistant</tt>.
    The handles are stored in the order in which they were encountered.
**/
inline vector<QcConstraint> const &Inconsistant() const
{
  return fInconsistant;
}
          
//begin ho
virtual void ClearInconsistant()
{ fInconsistant.resize(0); }
//end ho

/** Add to this solver all the constraints of <tt>other</tt>.  If each
    of the addConstraint operations is successful, then return
    <tt>true</tt>; otherwise, <tt>false</tt> is returned, and this
    solver is left in an undefined state.

    @precondition <tt>other != null
    @precondition <tt>other != this
**/
//ho bool swallow (QcSolver *other);
//begin o[c]
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
//end o[c]


/** If <tt>c</tt> is consistent with the other constraints in this
    solver, then add <tt>c</tt> to this solver and return
    <tt>true</tt>; otherwise simply return <tt>false</tt>.  The
    inconsistant constraint can be identified by calling
    <tt>Inconsistant</tt> after <tt>AddConstraint</tt>.

    @precondition <tt>c</tt> (i.e.&nbsp;something returning the same
    <tt>Id()</tt> as <tt>c</tt>; values of LHS and RHS are irrelevant) is not
    already present.

**/
virtual bool AddConstraint(QcConstraint &c) = 0;

//begin ho
bool addConstraint (QcConstraintRep *c)
{
  QcConstraint c1 (c);
  return AddConstraint (c1);
}
//end ho


/** Identical to <tt>AddConstraint(c)</tt> except gives hint of which variable
    to make basic.

    <p>One would usually choose a variable that is never an edit variable (or at
    least not an edit variable for the first few edit sessions), and is not the
    hint for any other constraint in this solver.
**/
virtual bool AddConstraint(QcConstraint &c, QcFloat &hint) = 0;


/** Add weighted "constraint", implemented with an error variable.  Equivalent
    to adding a <tt>QcFixedFloat e</tt> to the left-hand side of <tt>c</tt>,
    where <tt>e</tt> has goal value zero and weight <tt>weight</tt>.

    <p>Bug: The error variable doesn't get cleaned up when <tt>c</tt> is removed.
    (Fix: use special marker, and have <tt>removeConstraint</tt> etc. remove the
    error variable from the solver and delete it.)

    <p>N.B. The interface for weighted constraints will change.  It might make
    more sense for weightedness to be a property of the constraint object rather
    than added on during addConstraint.

    <p>And/or use an explicit cookie object that is returned from
    AddConstraint,AddWeightedConstraint that is used for
    removeConstraint,changeConstraint.

**/
//cf
void
AddWeightedConstraint(QcConstraint &c, numT weight)
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


/** Marks the beginning of a batch of AddConstraint operation.

    @precondition <tt>!inBatch()</tt>
**/
inline virtual void
BeginAddConstraint()
{
  qcAssertExternalPre( !fBatchAddConst);

  fBatchConstraints.clear();
  fBatchAddConst = true;
  fBatchAddConstFail = false;
}

/** Returns <tt>true</tt> iff batch constraint mode is active, i.e.&nbsp;there
    has been a call to <tt>BeginAddConstraint</tt> not followed by an
    <tt>EndAddConstraint</tt> call.
**/
inline bool
inBatch()
{
  return fBatchAddConst;
}

inline bool
ChangeRHS (QcConstraint &c, numT rhs)
{
  return ChangeConstraint (c, rhs);
}

inline bool
changeRHS (QcConstraintRep *c, numT rhs)
{
  return changeConstraint (c, rhs);
}

/** Change the rhs of a constraint. */
virtual bool ChangeConstraint(QcConstraint &c, numT rhs) = 0;

//begin ho
bool
changeConstraint(QcConstraintRep *c, numT rhs)
{
  QcConstraint c1 (c);
  return ChangeConstraint (c1, rhs);
}
//end ho


/** Marks the end of a batch of <tt>AddConstraint</tt> operations.

    @precondition <tt>inBatch()</tt>
**/
//cf
virtual bool
EndAddConstraint()
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


/** If <tt>c</tt> is in this solver, then remove it and return
    <tt>true</tt>.  Does not modify the value of any variable in
    <tt>c</tt>.  Returns false if the constraint is not present and
    also issues a warning if safety checks are enabled.
**/ //cf
virtual bool RemoveConstraint(QcConstraint &c)
{
  UNUSED(c);
  bad_call ("QcSolver::RemoveConstraint");
}


//begin ho
bool removeConstraint (QcConstraintRep *c)
{
  QcConstraint c1 (c);
  return RemoveConstraint (c1);
}
//end ho


/** Solve all the original constraints again from scratch.  Returns
    false if any of the original constraints are found to be
    inconsistant.  The inconsistant constraints can be identified by
    calling Inconsistant after Reset.
**/
virtual bool Reset() = 0;


//-----------------------------------------------------------------------//
// Constraint Solving methods                                            //
//-----------------------------------------------------------------------//
//begin ho
bool GetAutoSolve() const
{
  return fAutoSolve;
}

void SetAutoSolve (bool a)
{
  fAutoSolve = a;
}
//end ho

/** <tt>Solve</tt> uses the desired values of the parametric variables to
    calculate values for all the variables. */
virtual void Solve() = 0;

//begin ho
/** Synonym of <tt>Solve</tt>. */
void solve()
{ Solve(); }
//end ho

/** Similar to <tt>Solve</tt>, but for "interactive" use.  Unlike
    <tt>Solve</tt>, does not call <tt>RestDesVal</tt> on anything; that is done
    by <tt>EndEdit</tt>.  Only values for the variables of interest and
    dependent (basic) variables are recalculated, so the solution may not be as
    exact as with <tt>Solve</tt>.
**/
virtual void Resolve() = 0;

//-----------------------------------------------------------------------//
// Low Level Edit Variable Interface for use by solvers. These functions //
// manipulate the set of edit variables without any adjustment of the    //
// variable weights or automatic registration on addition or RestSolver()//
// on removal.
//-----------------------------------------------------------------------//

/** Add <tt>v</tt> to fEditVars. */
inline void
RegisterEditVar(QcFloat &v)
{
  if (IsEditVar( v))
    return;

  fEditVars.push_back( v);
  fEditVarsSetup = false;
}

/** Remove <tt>v</tt> from <tt>fEditVars</tt>. */
//cf
virtual void RemoveEditVar (QcFloat &v)
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

//begin ho
virtual void ClearEditVars()				// make fEditVars empty.
{
  fEditVars.resize(0);
  fEditVarsSetup = false;
}
//end ho


//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//

//cf
virtual void Print (ostream &os) const
{
  if (fEditVarsSetup)
    {
      os << "Edit Variables";
      unsigned size = fEditVars.size();

      for (unsigned i = 0; i < size; i++)
	fEditVars[i].Print(os);
    }
}

/** Erase everything ready to start afresh. */
//cf
virtual void
Restart()
{
  ClearEditVars();
  ClearInconsistant();
  fEditVarsSetup = false;
}


#if qcCheckPost
//ho public:
inline bool
hasConstraint(QcConstraintRep const *c) const
{
  return (find (checkedConstraints.begin(),
		checkedConstraints.end(),
		c)
	  != checkedConstraints.end());
}

inline bool
hasConstraint(QcConstraint const &c) const
{
  return hasConstraint (c.pointer());
}

//cf
void checkSatisfied() const
{
  for (int i = checkedConstraints.size(); --i >= 0;)
    {
      QcConstraintRep *c = checkedConstraints[i];
      qcAssertExternalPost (c->isSatisfied());
    }
}

//ho protected:
inline void
addCheckedConstraint(QcConstraintRep *c)
{
  checkedConstraints.push_back( c);
  c->Increase();
}

//cf
void
removeCheckedConstraint(QcConstraintRep *c)
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


inline void
changeCheckedConstraint(QcConstraintRep *oldc, numT rhs)
{
  oldc->SetRHS( rhs);
}

inline vector<QcConstraintRep *>::const_iterator
checkedConstraints_abegin() const
{
  return checkedConstraints.begin();
}

inline vector<QcConstraintRep *>::const_iterator
checkedConstraints_aend() const
{
  return checkedConstraints.end();
}

inline vector<QcConstraintRep *>::size_type
checkedConstraints_size() const
{
  return checkedConstraints.size();
}

#endif

//begin ho
protected:
  vector<QcConstraint> fInconsistant; 	// Records inconsistant constraints
  vector<QcFloat> fEditVars;		// Used for Edit/Resolve
  bool fEditVarsSetup;			// Used for Edit/Resolve
  bool fAutoSolve;
  bool fBatchAddConst;
  bool fBatchAddConstFail; 	
  vector<QcConstraint> fBatchConstraints;

#if qcCheckPost
private:
  vector<QcConstraintRep *> checkedConstraints;
#endif
//end ho

//ho };

//$class_prefix = 

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
