// Generated automatically from QcNlpSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcNlpSolverDCL
#define QcNlpSolverDCL
#line 1 "QcNlpSolver.ch"
#include <qoca/QcEnables.H>
#ifdef QC_USING_NLPSOLVER

#include <hash_map>
#include <qoca/QcSolver.H>
#include <qoca/QcVarStow.H>
extern "C" {
#include <wn/wnsll.h>
#include <wn/wnnlp.h>
#include <wn/wnmem.h>
}
#undef bool


#line 21 "QcNlpSolver.ch"
/* TODO: For every var added, check if it is restricted; if it is, then add an
   explicit constraint. */

/** A solver suitable for non-linear constraints.  The constraints should
    be continuous and differentiable.

    <p>Currently only linear constraints are implemented :-) but adding
    non-linear constraints should be little work; mainly just creating the
    non-linear constraint object (see <tt>buildObjective</tt> for how this is
    done) and adding it to the <tt>fWnConstraints</tt> singly-linked list;
    though there'll also be a bit of fiddling to allow <tt>removeConstraint</tt>
    and <tt>changeConstraint</tt> to work.

    <p>Improvements:
    <ul>

    <li><p>Obviously linear constraints ought to be handled differently.  At
    least linear equality constraints should be handled by elimination.
    Probably inequalities can be handled the same way, once the active set has
    been determined.

    <li><p>The line search in the underlying unconstrained problem is currently
    done with a golden section search.  Should probably solve a (sequence of)
    Taylor approximation(s) if the functions are polynomials.  (It's OK for the
    line search to consider only the section of the barrier function that
    applies at the start of the line search... hmm, need to consider the
    start-in-flat-section case.)

    <li><p>Currently <tt>doSolve</tt> makes a feasibility step (ignoring the
    goal function) immediately after solving.  This is obviously suboptimal: it
    will tend to lose optimality.  However, without that step, we often had
    infeasible "solutions" returned.  Maybe should reduce the scale of the
    [user] goal function with each iteration, as well as or instead of the
    current barrier offset method.

    <p>Could experiment with "big-M" arithmetic: use an explicit (symbolic)
    big-M to represent the ratio between the weight of the constraints and the
    weight of the user goal function.  This means that only a single call to the
    underlying unconstrained optimizer is needed, at the cost of doing numerical
    operations in a quasi-symbolic form.  The number representation might be
    <tt>{int power_of_bigM_in_first_term; double term_coeffs[];}</tt>.  One
    problem is when to decide that the high-order term should be considered
    zero.

    </ul>

**/
class QcNlpSolver
  : public QcSolver
{
public:


QcNlpSolver();
#line 84 "QcNlpSolver.ch"
QcNlpSolver(unsigned hintNumConstraints, unsigned hintNumVariables);
#line 97 "QcNlpSolver.ch"
virtual ~QcNlpSolver()
{
}


#ifndef NDEBUG

void
assertInvar() const;
#line 114 "QcNlpSolver.ch"
void
assertConstraintsInvar() const;
#line 141 "QcNlpSolver.ch"
unsigned
nWnConstraints() const;
#line 153 "QcNlpSolver.ch"
#endif



virtual void
AddVar(QcFloat &v);
#line 164 "QcNlpSolver.ch"
void
addVar(QcFloatRep *v);
#line 171 "QcNlpSolver.ch"
virtual bool
RemoveVar(QcFloat &v);
#line 178 "QcNlpSolver.ch"
bool
removeVar(QcFloatRep *v);
#line 185 "QcNlpSolver.ch"
virtual bool
IsRegistered(QcFloat const &v) const;
#line 192 "QcNlpSolver.ch"
bool
isRegistered(QcFloatRep *v) const;
#line 198 "QcNlpSolver.ch"
/** Begin a sequence of constraint operations.  The changes are not necessarily
    applied until the corresponding <tt>EndAddConstraint</tt> call.

    <p>Batches are not (currently) nestable.

    @precondition <tt>!isInBatchMode()</tt>
**/

virtual void
BeginAddConstraint();
#line 214 "QcNlpSolver.ch"
virtual bool
EndAddConstraint();
#line 234 "QcNlpSolver.ch"
virtual bool
AddConstraint(QcConstraint &c);
#line 241 "QcNlpSolver.ch"
virtual bool
AddConstraint(QcConstraint &c, QcFloat &hint);
#line 249 "QcNlpSolver.ch"
/** @precondition <tt>c != 0</tt>

    @precondition <tt>c</tt> (i.e.&nbsp;something returning the same
    <tt>Id()</tt> as <tt>c</tt>; values of LHS and RHS are irrelevant) is not
    already present.

    @postcondition All vars in <tt>c.LinPoly()</tt> are registered with this
    solver, with <tt>fVarStow.getUsage(v)</tt> incremented by 1.
**/

bool
addConstraint(QcConstraintRep *c);
#line 297 "QcNlpSolver.ch"
virtual bool
RemoveConstraint(QcConstraint &c);
#line 304 "QcNlpSolver.ch"
bool
removeConstraint(QcConstraintRep *c);
#line 325 "QcNlpSolver.ch"
virtual bool
ChangeConstraint( QcConstraint &c, numT rhs);
#line 332 "QcNlpSolver.ch"
bool
changeConstraint( QcConstraintRep *c, numT rhs);
#line 372 "QcNlpSolver.ch"
virtual void
RestSolver();
#line 381 "QcNlpSolver.ch"
virtual bool
IsFree(QcFloat const &v) const;
#line 388 "QcNlpSolver.ch"
bool
isFree(QcFloatRep *v) const;
#line 397 "QcNlpSolver.ch"
virtual bool
IsBasic(QcFloat const &v) const;
#line 405 "QcNlpSolver.ch"
virtual void
BeginEdit();
#line 413 "QcNlpSolver.ch"
virtual void
EndEdit();
#line 432 "QcNlpSolver.ch"
virtual void
Resolve();
#line 443 "QcNlpSolver.ch"
virtual bool
Reset();
#line 453 "QcNlpSolver.ch"
virtual void
Solve();
#line 461 "QcNlpSolver.ch"
void
solveNoRest();
#line 500 "QcNlpSolver.ch"
private:


void
commitSolution();
#line 518 "QcNlpSolver.ch"
void
restDesVals();
#line 533 "QcNlpSolver.ch"
bool
isFeasible();
#line 547 "QcNlpSolver.ch"
#ifndef NDEBUG

bool
wnConstraintsSatisfied() const;
#line 586 "QcNlpSolver.ch"
#endif


/** Solve <tt>fWnConstraints</tt>, storing the results only in
    <tt>fVarStow.getSolvedValArray()</tt>.  Ignores <tt>fConstraints</tt>.

    @precondition <tt>fVarStow.constrCacheValid()</tt>
**/

int
doSolve( wn_nonlinear_constraint_type objective, int offset_iterations);
#line 647 "QcNlpSolver.ch"
typedef hash_map<QcConstraintRep *, wn_sll,
  QcConstraintRep_hash, QcConstraintRep_equal> fConstraints_T;



void
doRemoveConstraint( QcNlpSolver::fConstraints_T::iterator i);
#line 681 "QcNlpSolver.ch"
void
refVars(QcConstraintRep const *c);
#line 695 "QcNlpSolver.ch"
/** Decrement variable usage counts (reversing <tt>refVars</tt>). */

void
derefVars(QcConstraintRep const *c);
#line 712 "QcNlpSolver.ch"
void
fixWnConstraints();
#line 730 "QcNlpSolver.ch"
wn_sll
makeAndInsertWnConstraint(QcConstraintRep *c);
#line 763 "QcNlpSolver.ch"
void
fixWnConstraint(QcConstraintRep *c, wn_linear_constraint_type wn_c);
#line 780 "QcNlpSolver.ch"
#ifndef NDEBUG

bool
sameConstraint(wn_linear_constraint_type wn_c, QcConstraintRep const *c) const;
#line 816 "QcNlpSolver.ch"
#endif


void
trimWnConstraints();
#line 885 "QcNlpSolver.ch"
wn_nonlinear_constraint_type
buildObjective();
#line 901 "QcNlpSolver.ch"
wn_nonlinear_constraint_type
buildZeroObjective();
#line 911 "QcNlpSolver.ch"
private:
QcVarStow fVarStow;
unsigned fNBatchConstraints;
fConstraints_T fConstraints;
wn_sll fWnConstraints;
bool fIsEditMode;

};

#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.


void
QcNlpSolver_dummy();
#line 929 "QcNlpSolver.ch"
#endif /* !QC_USING_NLPSOLVER */

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcNlpSolverDCL */
