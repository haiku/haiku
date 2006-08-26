// Generated automatically from QcNlpSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#include "QcNlpSolver.H"
#ifndef QcNlpSolverIFN
#define QcNlpSolverIFN
#line 1 "QcNlpSolver.ch"

#ifdef QC_USING_NLPSOLVER

#line 15 "QcNlpSolver.ch"
#include <qoca/QcSolver.hh>


#line 21 "QcNlpSolver.ch"
/* TODO: For every var added, check if it is restricted; if it is, then add an
   explicit constraint. */


#line 102 "QcNlpSolver.ch"
#ifndef NDEBUG

#line 153 "QcNlpSolver.ch"
#endif



#line 547 "QcNlpSolver.ch"
#ifndef NDEBUG

#line 586 "QcNlpSolver.ch"
#endif



#line 647 "QcNlpSolver.ch"
typedef hash_map<QcConstraintRep *, wn_sll,
  QcConstraintRep_hash, QcConstraintRep_equal> fConstraints_T;



#line 780 "QcNlpSolver.ch"
#ifndef NDEBUG

#line 816 "QcNlpSolver.ch"
#endif


#line 920 "QcNlpSolver.ch"
#else /* !QC_USING_NLPSOLVER */
// Make the linker happy.


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

#endif /* !QcNlpSolverIFN */
