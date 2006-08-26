// Generated automatically from QcNlpSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcNlpSolver.hh"
#line 1 "QcNlpSolver.ch"

#ifdef QC_USING_NLPSOLVER

#line 15 "QcNlpSolver.ch"
#include <qoca/QcSolver.hh>


#include <qoca/QcVarStow.hh>


/* TODO: For every var added, check if it is restricted; if it is, then add an
   explicit constraint. */


#line 74 "QcNlpSolver.ch"
QcNlpSolver::QcNlpSolver()
  : QcSolver(),
    fVarStow(),
    fConstraints(),
    fWnConstraints( 0),
    fIsEditMode( false)
{
}


QcNlpSolver::QcNlpSolver(unsigned hintNumConstraints, unsigned hintNumVariables)
  : QcSolver(),
    fVarStow(),
    fConstraints(),
    fWnConstraints( 0),
    fIsEditMode( false)
{
  UNUSED( hintNumConstraints);
  UNUSED( hintNumVariables);
  // effic: resize fVarStow, fConstraints.
}


#line 102 "QcNlpSolver.ch"
#ifndef NDEBUG

void
QcNlpSolver::assertInvar() const
{
  /* TODO: assert(&forall;[c &isin; constraints]
     &forall;[v &isin; c.LinPoly()]
     fVarStow.varPresent( v)
     &and; (fVarStow.getUsage(v) is correct) */
}


void
QcNlpSolver::assertConstraintsInvar() const
{
  assert( !fVarStow.constrCacheValid()
	  || (nWnConstraints() == checkedConstraints_size()));
  assert( fConstraints.size() == checkedConstraints_size());

  for (vector<QcConstraintRep *>::const_iterator ci = checkedConstraints_abegin(),
	 ciEnd = checkedConstraints_aend();
       ci != ciEnd; ci++)
    {
      QcConstraintRep *cc = *ci;
      fConstraints_T::const_iterator p = fConstraints.find( cc);
      assert( p != fConstraints.end());
      if (fVarStow.constrCacheValid())
	{
	  wn_sll el = p->second;
	  assert( el != 0);
	  wn_linear_constraint_type wn_c
	    = (wn_linear_constraint_type) el->contents;
	  assert( wn_c != 0);
	  assert( sameConstraint( wn_c, cc));
	}
    }
}


unsigned
QcNlpSolver::nWnConstraints() const
{
  unsigned n = 0;
  for (wn_sll el = fWnConstraints; el != 0; el = el->next)
    {
      if (el->contents)
	n++;
    }
  return n;
}

#endif



void
QcNlpSolver::AddVar(QcFloat &v)
{
  fVarStow.ensurePresent( v.pointer());
}


void
QcNlpSolver::addVar(QcFloatRep *v)
{
  fVarStow.ensurePresent( v);
}


bool
QcNlpSolver::RemoveVar(QcFloat &v)
{
  return fVarStow.tryRemove( v.pointer());
}


bool
QcNlpSolver::removeVar(QcFloatRep *v)
{
  return fVarStow.tryRemove( v);
}


bool
QcNlpSolver::IsRegistered(QcFloat const &v) const
{
  return fVarStow.isPresent( v.pointer());
}


bool
QcNlpSolver::isRegistered(QcFloatRep *v) const
{
  return fVarStow.isPresent( v);
}


#line 206 "QcNlpSolver.ch"
void
QcNlpSolver::BeginAddConstraint()
{
  QcSolver::BeginAddConstraint();
  fNBatchConstraints = 0;
}


bool
QcNlpSolver::EndAddConstraint()
{
  /* TODO: Actually, some of this is handled in QcSolver. */
  qcAssertExternalPre( inBatch());
  if (fNBatchConstraints == 0)
    return true;
  // fixme: make sure fConstraints is up-to-date.
  bool success = isFeasible();
  if (!success)
    {
      /* fixme: roll back, using wnsll_edel n times, and add to fInconsistent. */
      abort();
    }
  dbg( assertConstraintsInvar());
  return success;
}



bool
QcNlpSolver::AddConstraint(QcConstraint &c)
{
  return addConstraint( c.pointer());
}


bool
QcNlpSolver::AddConstraint(QcConstraint &c, QcFloat &hint)
{
  // QcNlpSolver is non-parametric, so we ignore the basic variable hint.
  UNUSED( hint);
  return addConstraint( c.pointer());
}


#line 259 "QcNlpSolver.ch"
bool
QcNlpSolver::addConstraint(QcConstraintRep *c)
{
  qcAssertExternalPre( c != 0);
  qcAssertExternalPre( fConstraints.count( c) == 0);

  /* QcConstraintRep's are user-changeable, so we make a copy. */
  c = new QcConstraintRep( *c);

  refVars( c);

  pair<QcConstraintRep *, wn_sll>
    ins( c, 0);
  if (fVarStow.constrCacheValid())
    ins.second = makeAndInsertWnConstraint( c);
  pair<fConstraints_T::iterator, bool>
    res( fConstraints.insert( ins));
  assert( res.second);

  if (inBatch()
      || c->isSatisfied()
      || isFeasible())
    {
#if qcCheckPost
      addCheckedConstraint( c);
#endif
      dbg( assertConstraintsInvar());
      return true;
    }
  else
    {
      doRemoveConstraint( res.first);
      dbg( assertConstraintsInvar());
      return false;
    }
}


bool
QcNlpSolver::RemoveConstraint(QcConstraint &c)
{
  return removeConstraint( c.pointer());
}


bool
QcNlpSolver::removeConstraint(QcConstraintRep *c)
{
  qcAssertExternalPre( c != 0);

  fConstraints_T::iterator i( fConstraints.find( c));
  if (i == fConstraints.end())
    return false;

#if qcCheckPost
  removeCheckedConstraint( i->first);
  /* impl: Reason for putting this first is only so that the c->Counter == 1
     assertion in doRemoveConstraint succeeds. */
#endif
  doRemoveConstraint( i);

  dbg( assertConstraintsInvar());
  return true;
}


bool
QcNlpSolver::ChangeConstraint( QcConstraint &c, numT rhs)
{
  return changeConstraint( c.pointer(), rhs);
}


bool
QcNlpSolver::changeConstraint( QcConstraintRep *c, numT rhs)
{
  /* TODO: handle inBatch() case. */
  assert( !inBatch());
  fConstraints_T::iterator i( fConstraints.find( c));

  qcAssertExternalPre( i != fConstraints.end());

  if (!fVarStow.constrCacheValid())
    fixWnConstraints();

  QcConstraintRep *qc_c = i->first;
  numT old_rhs( qc_c->RHS());
  qc_c->SetRHS( rhs);
  assert( i->second != 0);
  /* proof: fixWnConstraints call above */
  wn_linear_constraint_type wn_c
    = (wn_linear_constraint_type) i->second->contents;
  assert( wn_c != 0);
  wn_c->rhs = dbl_val( rhs);

  bool success = (qc_c->isSatisfied()
		  || isFeasible());
  if (success)
    {
#if qcCheckPost
      changeCheckedConstraint( qc_c, rhs);
#endif
    }
  else
    {
      qc_c->SetRHS( old_rhs);
      wn_c->rhs = dbl_val( old_rhs);
    }
  dbg( assertConstraintsInvar());
  return success;
}


void
QcNlpSolver::RestSolver()
{
  /* TODO: What's RestSolver supposed to do anyway? */
  return;
}



bool
QcNlpSolver::IsFree(QcFloat const &v) const
{
  return isFree( v.pointer());
}


bool
QcNlpSolver::isFree(QcFloatRep *v) const
{
  qcAssertExternalPre( isRegistered( v));

  return fVarStow.getUsage( v) == 0;
}


bool
QcNlpSolver::IsBasic(QcFloat const &v) const
{
  UNUSED( v);
  bad_call("QcNlpSolver::IsBasic");
}


void
QcNlpSolver::BeginEdit()
{
  qcAssertExternalPre( !fIsEditMode);
  fIsEditMode = true;
}


void
QcNlpSolver::EndEdit()
{
  // Not an assertion, as BeginEdit is optional.
  if (!fIsEditMode)
    return;
  fIsEditMode = false;

  for (QcVarStow::var_iterator i = fVarStow.vars_begin(),
	 iEnd = fVarStow.vars_end();
       i != iEnd;
       i++)
    (*i)->RestDesVal();

  QcSolver::EndEdit();
}



void
QcNlpSolver::Resolve()
{
  if (!fIsEditMode)
    BeginEdit();

  solveNoRest();
}



bool
QcNlpSolver::Reset()
{
  solveNoRest();
  restDesVals();
  return true;
}



void
QcNlpSolver::Solve()
{
  solveNoRest();
  restDesVals();
}


void
QcNlpSolver::solveNoRest()
{
  qcAssertExternalPre( !inBatch());
  /* relevance: may simplify things wrt feasibility.  We know that the problem
     is feasible. */

  unsigned nVars = fVarStow.size();
  if (nVars == 0)
    return;

  if (!fVarStow.fConstrCacheValid)
    fixWnConstraints();

  int result;
  {
    int const offset_iterations = 15;
    wn_nonlinear_constraint_type objective = buildObjective(),
      zeroObjective = buildZeroObjective();

    result = doSolve( objective, offset_iterations);
    doSolve( zeroObjective, 1);

    wn_free( zeroObjective);
    wn_free( objective);
  }
  if (result == WN_SUBOPTIMAL)
    cerr << "Warning: returning suboptimal solution.\n";

  commitSolution();

#if qcCheckPost
  checkSatisfied();
  assertConstraintsInvar();
#endif
}






void
QcNlpSolver::commitSolution()
{
  double *s = fVarStow.getSolvedValArray();
  for (QcVarStow::var_iterator vi = fVarStow.vars_begin(),
	 viEnd = fVarStow.vars_end();
       vi != viEnd;
       vi++, s++)
    {
      QcFloatRep *v = *vi;
      v->SetValue( numT( *s));
    }
}


void
QcNlpSolver::restDesVals()
{
  for (QcVarStow::var_iterator vi = fVarStow.vars_begin(),
	 viEnd = fVarStow.vars_end();
       vi != viEnd;
       vi++)
    {
      QcFloatRep *v = *vi;
      v->RestDesVal();
    }
}



bool
QcNlpSolver::isFeasible()
{
  static wn_nonlinear_constraint_type objective = buildZeroObjective();

  if (!fVarStow.constrCacheValid())
    fixWnConstraints();

  int result = doSolve( objective, 1);
  if (result != WN_SUCCESS)
    cerr << "DBG: isFeasible: doSolve returned " << result << '\n';
  return wnConstraintsSatisfied();
}

#ifndef NDEBUG

bool
QcNlpSolver::wnConstraintsSatisfied() const
{
  qcAssertPre( fVarStow.constrCacheValid());
  double const *solvedVals = fVarStow.getSolvedValArray();

  for (wn_sll i = fWnConstraints; i != NULL; i = i->next)
    {
      wn_linear_constraint_type wn_c = (wn_linear_constraint_type) i->contents;
      if (wn_c == 0)
	continue;

      double balance = wn_c->rhs;
      double *w = wn_c->weights;
      for (int *vi = wn_c->vars, *viEnd = wn_c->vars + wn_c->size;
	   vi != viEnd;
	   vi++, w++)
	{
	  assert( unsigned( *vi) < fVarStow.size());
	  balance -= *w * solvedVals[*vi];
	}

      double eps = (1.0 / (1<<27));
      double tst;
      switch (wn_c->comparison_type)
	{
	case WN_EQ_COMPARISON: tst = fabs( balance); break;
	case WN_LT_COMPARISON: tst = -balance; break;
	case WN_GT_COMPARISON: tst = balance; break;
	default:
	  qcDurchfall( "wn comparison_type");
	}
      if (!(tst <= eps)) // i.e. if tst > eps || isNaN(tst)
	return false;
    }
  return true;
}
#endif



#line 595 "QcNlpSolver.ch"
int
QcNlpSolver::doSolve( wn_nonlinear_constraint_type objective, int offset_iterations)
{
  qcAssertPre( fVarStow.constrCacheValid());

  double solved_val;
  int result;
  unsigned nVars = fVarStow.size();
  if (nVars == 0)
    return WN_SUCCESS;

  trimWnConstraints();
  wn_nlp_conj_method( &result, &solved_val, fVarStow.getSolvedValArray(),
		      0, objective,
		      fWnConstraints, nVars,
		      0x7fffffff /* conj_iterations */,
		      offset_iterations,
		      1.0);
  /* Increasing conj_iterations reduces the chance of having WN_SUBOPTIMAL
     returned. */
  assert( result != WN_UNBOUNDED);
  /* proof (assumes wn_nlp behaves like an exact solver): goal fn is positive
     semi-definite, and &forall;[i] there are no first-order terms in x_i unless
     there is also a second-order term in x_i. */

  return result;
}



static int
qc2wn_relation(QcConstraintRep const *c)
{
  switch(c->getRelation())
    {
    case QcConstraintRep::coEQ:
      return WN_EQ_COMPARISON;

    case QcConstraintRep::coLE:
    case QcConstraintRep::coLT:
      return WN_LT_COMPARISON;

    case QcConstraintRep::coGE:
    case QcConstraintRep::coGT:
      return WN_GT_COMPARISON;
    }

  qcDurchfall( "constraint relation");
}



typedef hash_map<QcConstraintRep *, wn_sll,
  QcConstraintRep_hash, QcConstraintRep_equal> fConstraints_T;



void
QcNlpSolver::doRemoveConstraint( QcNlpSolver::fConstraints_T::iterator i)
{
  /* The user could have changed c since it was added; we must use the version
     that we added. */
  QcConstraintRep *c = i->first;
  wn_sll el = i->second;
  if (el != 0)
    {
      assert( el->contents != 0);
      wn_free( el->contents);
      el->contents = 0;
      /* impl: We don't yet actually delete the sll element because we don't
         have the address of what points to this element (whether the previous
         `next' pointer or fWnConstraints).  And we can't just copy in the next
         element, because we'd need to update the hash table entry that points
         to that next element.  So instead the element continues to exist until
         the next trimWnConstraints call. */
    }

  derefVars( c);

  fConstraints.erase( i);
  assert( c->Counter() == 1);
  delete( c);
}



void
QcNlpSolver::refVars(QcConstraintRep const *c)
{
  QcLinPoly::TTermsVector const &terms = c->LinPoly().GetTerms();
  for (QcLinPoly::TTermsVector::const_iterator ti = terms.abegin(),
	 tiEnd = terms.aend();
       ti != tiEnd;
       ti++)
    {
      QcFloatRep *v = (*ti)->getVariable();
      fVarStow.addReference( v);
    }
}



void
QcNlpSolver::derefVars(QcConstraintRep const *c)
{
  QcLinPoly::TTermsVector const &terms = c->LinPoly().GetTerms();
  for (QcLinPoly::TTermsVector::const_iterator ti = terms.abegin(),
	 tiEnd = terms.aend();
       ti != tiEnd;
       ti++)
    {
      QcFloatRep *v = (*ti)->getVariable();
      fVarStow.decUsage( v);
    }
}


void
QcNlpSolver::fixWnConstraints()
{
  for (fConstraints_T::iterator
	 i( fConstraints.begin()), iEnd( fConstraints.end());
       i != iEnd; i++)
    {
      QcConstraintRep *c = i->first;
      wn_sll el = i->second;
      if (el == 0)
	i->second = makeAndInsertWnConstraint( c);
      else
	fixWnConstraint( c, (wn_linear_constraint_type) el->contents);
    }
  fVarStow.fConstrCacheValid = true;
}


wn_sll
QcNlpSolver::makeAndInsertWnConstraint(QcConstraintRep *c)
{
  wn_linear_constraint_type wn_c;
  unsigned cNVars = c->getNVars();
  wn_make_linear_constraint( &wn_c, cNVars, c->RHS(),
			     qc2wn_relation( c));
  assert( wn_c != 0);

  {
    int *wn_v = wn_c->vars;
    double *wn_w = wn_c->weights;
    QcLinPoly::TTermsVector const &terms = c->LinPoly().GetTerms();
    for (QcLinPoly::TTermsVector::const_iterator ti = terms.abegin(),
	   tiEnd = terms.aend();
	 ti != tiEnd;
	 ti++, wn_v++, wn_w++)
      {
	assert( unsigned( wn_v - wn_c->vars) < unsigned( wn_c->size));
	*wn_v = fVarStow.getVarIx( (*ti)->getVariable());

	assert( wn_w - wn_c->weights
		== wn_v - wn_c->vars);
	*wn_w = dbl_val( (*ti)->GetCoeff());
      }
  }

  wn_sllins( &fWnConstraints, wn_c);

  return fWnConstraints;
}


void
QcNlpSolver::fixWnConstraint(QcConstraintRep *c, wn_linear_constraint_type wn_c)
{
  qcAssertPre( wn_c != 0);

  int *wn_v = wn_c->vars;
  QcLinPoly::TTermsVector const &terms = c->LinPoly().GetTerms();
  for (QcLinPoly::TTermsVector::const_iterator ti = terms.abegin(),
	 tiEnd = terms.aend();
       ti != tiEnd;
       ti++, wn_v++)
    {
      assert( unsigned( wn_v - wn_c->vars) < unsigned( wn_c->size));
      *wn_v = fVarStow.getVarIx( (*ti)->getVariable());
    }
}

#ifndef NDEBUG

bool
QcNlpSolver::sameConstraint(wn_linear_constraint_type wn_c, QcConstraintRep const *c) const
{
  qcAssertPre( wn_c != 0);
  qcAssertPre( c != 0);

  unsigned cNVars = c->getNVars();
  if ((unsigned( wn_c->size) != cNVars)
      || (wn_c->comparison_type != qc2wn_relation( c))
      || (wn_c->rhs != dbl_val( c->RHS())))
    return false;


  {
    int const *wn_v = wn_c->vars;
    double const *wn_w = wn_c->weights;
    QcLinPoly::TTermsVector const &terms = c->LinPoly().GetTerms();
    for (QcLinPoly::TTermsVector::const_iterator ti = terms.abegin(),
	   tiEnd = terms.aend();
	 ti != tiEnd;
	 ti++, wn_v++, wn_w++)
      {
	assert( unsigned( wn_v - wn_c->vars) < unsigned( wn_c->size));
	if (unsigned( *wn_v) != fVarStow.getVarIx( (*ti)->getVariable()))
	  return false;

	assert( wn_w - wn_c->weights
		== wn_v - wn_c->vars);
	if (*wn_w != dbl_val( (*ti)->GetCoeff()))
	  return false;
      }
  }
  return true;
}
#endif


void
QcNlpSolver::trimWnConstraints()
{
  wn_sll *pptr = &fWnConstraints;
  for (wn_sll el = fWnConstraints; el != 0; el = el->next)
    {
      if (el->contents)
	{
	  assert( *pptr == el);
	  pptr = &el->next;
	}
      else
	*pptr = el->next;
    }

#ifdef qcCheckInternalPost
  for (wn_sll el = fWnConstraints; el != 0; el = el->next)
    assert( el->contents);
#endif
}


static double const scale = 1.0 / (1L << 30);

double
goalFnVal(int size, double *values, ptr varStowPtr)
{
  QcVarStow *varStow = (QcVarStow *) varStowPtr;
  double ret = 0.0;
  /* effic: Check if this function's return value actually matters.
     It's probably safe to return zero. */
  QcVarStow::var_const_iterator vi = varStow->vars_begin(),
    viEnd = varStow->vars_end();
  assert( viEnd - vi == size);
  for(; vi != viEnd; vi++, values++)
    {
      QcFloatRep *v = *vi;
      double err = *values - dbl_val( v->DesireValue());
      ret += dbl_val( v->Weight()) * err * err;
    }

  assert( !(ret < 0.0));
  // not the same as `ret >= 0.0': ret may be NaN
  // (unless SuggestValue asserts against that).

  return ret * scale;
}

void
goalFnGradient(double *grad, int size, double *values, ptr varStowPtr)
{
  QcVarStow *varStow = (QcVarStow *) varStowPtr;
  QcVarStow::var_const_iterator vi = varStow->vars_begin(),
    viEnd = varStow->vars_end();
  assert( viEnd - vi == size);
  for(; vi != viEnd;
      vi++, values++, grad++)
    {
      QcFloatRep *v = *vi;
      double err = *values - dbl_val( v->DesireValue());
      *grad = 2.0 * dbl_val( v->Weight()) * err * scale;
    }
}



wn_nonlinear_constraint_type
QcNlpSolver::buildObjective()
{
  wn_nonlinear_constraint_type obj;

  wn_make_nonlinear_constraint( &obj, fVarStow.size(), WN_EQ_COMPARISON);
  for (int i = fVarStow.size(); --i >= 0;)
    obj->vars[i] = i;
  obj->offset = 0.0; // make sure not NaN or infinite.
  obj->client_data = (ptr) &fVarStow;
  obj->pfunction = goalFnVal;
  obj->pgradient = goalFnGradient;
  return obj;
}


wn_nonlinear_constraint_type
QcNlpSolver::buildZeroObjective()
{
  wn_linear_constraint_type obj;

  wn_make_linear_constraint( &obj, 0, 0.0, WN_EQ_COMPARISON);
  return (wn_nonlinear_constraint_type) obj;
}


#line 920 "QcNlpSolver.ch"
#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.


void
QcNlpSolver_dummy()
{
}

#endif /* !QC_USING_NLPSOLVER */

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
