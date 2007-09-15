// Generated automatically from QcASetSolver.ch by /home/pmoulder/usr/local/bin/ch2xx.
#ifndef QcASetSolverDCL
#define QcASetSolverDCL
#line 1 "QcASetSolver.ch"
#include <qoca/QcSolver.H>


class QcASetSolver
  : public QcSolver
{
public:

inline
QcASetSolver();
#line 15 "QcASetSolver.ch"
inline
QcASetSolver(unsigned hintNumConstraints, unsigned hintNumVariables);
#line 22 "QcASetSolver.ch"
virtual ~QcASetSolver()
{
}


};

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/

#endif /* !QcASetSolverDCL */
