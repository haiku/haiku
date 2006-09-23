// Generated automatically from QcSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcSolverDCL
#define QcSolverDCL
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

#include <vector>
#include <algorithm>
#include "qoca/QcConstraint.hh"


class QcSolver
{

public:

//-----------------------------------------------------------------------//
// Constructor.                                                          //
//-----------------------------------------------------------------------//
inline QcSolver();
#line 46 "QcSolver.ch"
virtual ~QcSolver()
{
}



#if qcCheckInternalInvar
inline void
assertInvar() const;
#line 59 "QcSolver.ch"
void
assertDeepInvar() const;
#line 74 "QcSolver.ch"
virtual void
vAssertDeepInvar() const;
#line 79 "QcSolver.ch"
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


void
addVar (QcFloatRep *v);
#line 109 "QcSolver.ch"
static void bad_call (char const *method) __attribute__ ((__noreturn__));

#line 118 "QcSolver.ch"
/** Removes variable from solver and returns true provided <tt>v</tt> is
    free.  Does nothing and returns false if <tt>v</tt> is not free.

    <p>If <tt>!IsRegistered(v)</tt> then the current behavior is to return
    <tt>false</tt>; maybe presence should be precondition.

**/
virtual bool
RemoveVar (QcFloat &v);
#line 133 "QcSolver.ch"
bool
removeVar (QcFloatRep *v)
{
  QcFloat v1 (v);
  return RemoveVar (v1);
}


/** Sets the desired value of <tt>v</tt> to <tt>desval</tt>.  This is
    the same doing <tt>v.SuggestValue(desval)</tt>, except it checks
    that <tt>v</tt> is registered in the solver.
**/
virtual inline void
SuggestValue(QcFloat &v, numT desval);
#line 152 "QcSolver.ch"
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
**/
virtual bool IsFree (QcFloat const &v) const;
#line 176 "QcSolver.ch"
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
**/
bool
IsEditVar(QcFloat const &v) const;
#line 207 "QcSolver.ch"
bool isEditVar (QcFloatRep const *v) const
{
  QcFloat v1(v);
  return IsEditVar (v1);
}


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

virtual void
AddEditVar(QcFloat &v);
#line 235 "QcSolver.ch"
void addEditVar (QcFloatRep *v)
{
  QcFloat v1 (v);
  AddEditVar (v1);
}


/** Prepare for <tt>resolve</tt> call.  The first call to
    <tt>Resolve</tt> after changing the edit vars or the constraints
    will call <tt>BeginEdit</tt> and incur a slight delay.  An explicit
    <tt>BeginEdit</tt> call avoids that delay by allowing the
    preprocessing to be done at a more convenient time. */
virtual void BeginEdit() = 0;


/** Synonym of <tt>BeginEdit</tt>. */
void beginEdit()
{
  BeginEdit();
}


/** For every variable: make it non-edit (i.e.&nbsp;undoes
    <tt>addEditVar</tt>), set it to stay weight, and (if not a
    QcFixedFloat) set its desired value to the current value.

    <p>If both fixed and nomadic variables are in this solver, and not
    (each fixed variable weight is well outside of each nomadic edit
    variable's stay--edit weight range), then <tt>endEdit</tt> may cause
    the solution to be no longer optimal (i.e. with respect to the stay
    weights).
**/

virtual void
EndEdit();
#line 280 "QcSolver.ch"
/** Synonym of <tt>EndEdit</tt>. */
void endEdit()
{ EndEdit(); }


//-----------------------------------------------------------------------//
// Constraint management methods                                         //
//-----------------------------------------------------------------------//

/** Returns constraint handles for all constraints rejected as
    inconsistant since the last call to <tt>ClearInconsistant</tt>.
    The handles are stored in the order in which they were encountered.
**/
inline vector<QcConstraint> const &Inconsistant() const;
#line 299 "QcSolver.ch"
virtual void ClearInconsistant()
{ fInconsistant.resize(0); }


/** Add to this solver all the constraints of <tt>other</tt>.  If each
    of the addConstraint operations is successful, then return
    <tt>true</tt>; otherwise, <tt>false</tt> is returned, and this
    solver is left in an undefined state.

    @precondition <tt>other != null
    @precondition <tt>other != this
**/
bool swallow (QcSolver *other);

#line 333 "QcSolver.ch"
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


bool addConstraint (QcConstraintRep *c)
{
  QcConstraint c1 (c);
  return AddConstraint (c1);
}



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

void
AddWeightedConstraint(QcConstraint &c, numT weight);
#line 443 "QcSolver.ch"
/** Marks the beginning of a batch of AddConstraint operation.

    @precondition <tt>!inBatch()</tt>
**/
inline virtual void
BeginAddConstraint();
#line 457 "QcSolver.ch"
/** Returns <tt>true</tt> iff batch constraint mode is active, i.e.&nbsp;there
    has been a call to <tt>BeginAddConstraint</tt> not followed by an
    <tt>EndAddConstraint</tt> call.
**/
inline bool
inBatch();
#line 467 "QcSolver.ch"
inline bool
ChangeRHS (QcConstraint &c, numT rhs);
#line 473 "QcSolver.ch"
inline bool
changeRHS (QcConstraintRep *c, numT rhs);
#line 479 "QcSolver.ch"
/** Change the rhs of a constraint. */
virtual bool ChangeConstraint(QcConstraint &c, numT rhs) = 0;


bool
changeConstraint(QcConstraintRep *c, numT rhs)
{
  QcConstraint c1 (c);
  return ChangeConstraint (c1, rhs);
}



/** Marks the end of a batch of <tt>AddConstraint</tt> operations.

    @precondition <tt>inBatch()</tt>
**/

virtual bool
EndAddConstraint();
#line 517 "QcSolver.ch"
/** If <tt>c</tt> is in this solver, then remove it and return
    <tt>true</tt>.  Does not modify the value of any variable in
    <tt>c</tt>.  Returns false if the constraint is not present and
    also issues a warning if safety checks are enabled.
**/
virtual bool RemoveConstraint(QcConstraint &c);
#line 530 "QcSolver.ch"
bool removeConstraint (QcConstraintRep *c)
{
  QcConstraint c1 (c);
  return RemoveConstraint (c1);
}



/** Solve all the original constraints again from scratch.  Returns
    false if any of the original constraints are found to be
    inconsistant.  The inconsistant constraints can be identified by
    calling Inconsistant after Reset.
**/
virtual bool Reset() = 0;


//-----------------------------------------------------------------------//
// Constraint Solving methods                                            //
//-----------------------------------------------------------------------//

bool GetAutoSolve() const
{
  return fAutoSolve;
}

void SetAutoSolve (bool a)
{
  fAutoSolve = a;
}


/** <tt>Solve</tt> uses the desired values of the parametric variables to
    calculate values for all the variables. */
virtual void Solve() = 0;


/** Synonym of <tt>Solve</tt>. */
void solve()
{ Solve(); }


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
RegisterEditVar(QcFloat &v);
#line 597 "QcSolver.ch"
/** Remove <tt>v</tt> from <tt>fEditVars</tt>. */

virtual void RemoveEditVar (QcFloat &v);
#line 617 "QcSolver.ch"
virtual void ClearEditVars()				// make fEditVars empty.
{
  fEditVars.resize(0);
  fEditVarsSetup = false;
}



//-----------------------------------------------------------------------//
// Utility functions.                                                    //
//-----------------------------------------------------------------------//


virtual void Print (ostream &os) const;
#line 642 "QcSolver.ch"
/** Erase everything ready to start afresh. */

virtual void
Restart();
#line 653 "QcSolver.ch"
#if qcCheckPost
public:
inline bool
hasConstraint(QcConstraintRep const *c) const;
#line 664 "QcSolver.ch"
inline bool
hasConstraint(QcConstraint const &c) const;
#line 671 "QcSolver.ch"
void checkSatisfied() const;
#line 680 "QcSolver.ch"
protected:
inline void
addCheckedConstraint(QcConstraintRep *c);
#line 689 "QcSolver.ch"
void
removeCheckedConstraint(QcConstraintRep *c);
#line 723 "QcSolver.ch"
inline void
changeCheckedConstraint(QcConstraintRep *oldc, numT rhs);
#line 729 "QcSolver.ch"
inline vector<QcConstraintRep *>::const_iterator
checkedConstraints_abegin() const;
#line 735 "QcSolver.ch"
inline vector<QcConstraintRep *>::const_iterator
checkedConstraints_aend() const;
#line 741 "QcSolver.ch"
inline vector<QcConstraintRep *>::size_type
checkedConstraints_size() const;
#line 747 "QcSolver.ch"
#endif


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


};



#ifndef qcNoStream
inline ostream &operator<< (ostream &os, const QcSolver &s);
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

#endif /* !QcSolverDCL */
