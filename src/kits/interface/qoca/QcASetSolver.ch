//o[d] #include <qoca/QcSolver.H>
//o[ci] #include <qoca/QcSolver.hh>

//ho class QcASetSolver
//ho   : public QcSolver
//ho {
//ho public:

inline
QcASetSolver()
  : QcSolver()
{
}

inline
QcASetSolver(unsigned hintNumConstraints, unsigned hintNumVariables)
  : QcSolver()
{
}

//begin ho
virtual ~QcASetSolver()
{
}
//end ho

//ho };

/*
  Local Variables:
  mode:c++
  c-file-style:"gnu"
  fill-column:80
  End:
  vim: set filetype=c++ :
*/
