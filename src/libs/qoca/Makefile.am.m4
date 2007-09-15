$(srcdir)/Makefile.am: Makefile.am.m4
	rm -f $(srcdir)/$(@F)
	m4 -P $(srcdir)/$(<F) > $(srcdir)/$(@F)
# AFAIK, this has to be in srcdir (as opposed to bsrcdir).

m4_changequote([[, ]])m4_dnl

m4_define([[m4_echodo]], [[echo "m4_patsubst([[$1]], [[["\\`]\|\$\$]], [[\\\&]])" \
	  && $1]])

m4_define([[m4_outputas]],
	[[@rm -f $2 && if m4_echodo([[$1 > $2]]); \
	then :; \
	else m4_echodo([[rm -f $2; exit 1]]); fi]])

##bsrcdir = $(srcdir)
bsrcdir = .

m4_define([[m4_ch2xx_rule]],
[[.ch.$2: $(CH2XX)
	m4_outputas([[$(CH2XX) $1 $<]], [[$(bsrcdir)/$(*F).$2]])]])

m4_ch2xx_rule(-c, cc)

m4_ch2xx_rule(-i, hh)

m4_ch2xx_rule(-d, H)

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

m4_dnl Like:  tr -s ' \t\n'  ' '
m4_define([[m4u_white]], [[m4_patsubst([[$1]], [[[ 	
]+]], [[ ]])]])

m4_define(m4_libqoca_la_OBJECTS, m4u_white([[
	QcCassSolver.lo QcCompPivotSystem.lo QcCompPivotTableau.lo
	QcConstraintRep.lo QcCoreTableau.lo QcDelCoreTableau.lo
	QcDelLinEqTableau.lo QcDelLinInEqTableau.lo QcDenseMatrix.lo
	QcFixedFloatRep.lo
	QcFloatRep.lo
	QcIneqSolverBase.lo
	QcLinEqSolver.lo QcLinEqSystem.lo QcLinEqTableau.lo
	QcLinInEqSolver.lo QcLinInEqSystem.lo QcLinInEqTableau.lo
	QcLinPoly.lo
	QcLinEqColStateVector.lo
	QcLinEqRowStateVector.lo
	QcLinInEqColStateVector.lo
	QcLinInEqRowColStateVector.lo
	QcNlpSolver.lo
	QcNomadicFloatRep.lo
	QcOrigRowStateVector.lo
	QcQuasiRowColStateVector.lo
	QcQuasiRowStateVector.lo
	QcSolver.lo QcSparseMatrix.lo
	QcVarStow.lo
	QcVariableBiMap.lo]]))m4_dnl

DEP_FILES = m4_patsubst(m4_libqoca_la_OBJECTS, [[\(\w*\)\.lo]], [[.deps/\1.P]])


-include $(DEP_FILES)

.cc.ii0:
	m4_outputas([[$(CXXCPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) -Wp,-MD,.deps/$(*F).pp $<]],
		    [[$(bsrcdir)/$(@F)]])
	@rm -f .deps/$(*F).P
	@-sed 's/\.o/.ii0/g' < .deps/$(*F).pp > .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm .deps/$(*F).pp


CXXFLAGS = @CXXFLAGS@
CXXLD = $(CXX)
CXXLINK = $(LIBTOOL) --mode=link $(CXXLD) $(AM_CXXFLAGS) $(CXXFLAGS) $(LDFLAGS) -o $@
CXXCOMPILE = $(CXX) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)
LTCXXCOMPILE = $(LIBTOOL) --mode=compile $(CXX) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CXXFLAGS) $(CXXFLAGS)

libqoca_la_myOBJECTS = m4_libqoca_la_OBJECTS

ii0_files = m4_patsubst(m4_libqoca_la_OBJECTS, \.lo, .ii0)

libqoca.la: $(ii0_files) $(libqoca_la_myOBJECTS) $(libqoca_la_DEPENDENCIES)
	$(CXXLINK) -rpath $(libdir) $(libqoca_la_LDFLAGS) $(libqoca_la_myOBJECTS) $(libqoca_la_LIBADD) $(LIBS)

m4_define([[m4_compile]],
[[.ii0.$1:
	@if test -f [[$]]@ && cmp -s $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii 2>/dev/null; then :; else \
	 m4_echodo([[cp $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii]]) && \
	 m4_echodo([[$($2[[]]CXXCOMPILE) -c $(bsrcdir)/$(*F).ii]]); \
	fi
]])m4_dnl

m4_compile(o,)
m4_compile(lo,LT)

# Note that we must start off with no .deps files.

m4_dnl Expand `foo' to `foo.H foo.hh foo.$(cc)'.
m4_define([[m4u_Hhc]], [[m4_patsubst(.H .hh .$(cc), \., $1.)]])
m4_define([[m4u_Hh]], [[m4_patsubst(.H .hh, \., $1.)]])

cc = cc
BUILT_SOURCES = m4u_white([[
	arith.H
	m4u_Hh(QcAliasedBiMap)
	m4u_Hhc(QcASetSolver)
	m4u_Hh(QcBiMap)
	m4u_Hhc(QcFloatRep)
	m4u_Hhc(QcFixedFloatRep)
	m4u_Hhc(QcIneqSolverBase)
	m4u_Hhc(QcLinEqColStateVector)
	m4u_Hhc(QcLinEqRowStateVector)
	m4u_Hhc(QcLinInEqColStateVector)
	m4u_Hhc(QcLinInEqRowColStateVector)
	m4u_Hhc(QcNlpSolver)
	m4u_Hhc(QcNomadicFloatRep)
	QcNullIterator.H
	m4u_Hhc(QcOrigRowStateVector)
	m4u_Hhc(QcQuasiRowColStateVector)
	m4u_Hhc(QcQuasiRowStateVector)
	m4u_Hhc(QcSolver)
	m4u_Hhc(QcVarStow)
	m4u_Hhc(QcVariableBiMap)
	]])
m4_dnl	m4_patsubst(m4_libqoca_la_OBJECTS, \.lo, .ii0)

SUFFIXES = .ch .cc .hh .H .ii0

lib_LTLIBRARIES = libqoca.la
# Version info is `CURRENT[:REVISION[:AGE]]': *see (libtool)Versioning,
# in the libtool info page.
libqoca_la_LDFLAGS = -version-info 0:0:0
libqoca_la_LIBADD = -lstdc++ $(lwn) $(lgmp)

# Package visibility is implemented in C++ as `#ifdef QOCA_INTERNALS'.
# N.B. This is only for methods; for vars, make the var itself private
# and provide package-visible inline access methods.
# N.B.2: Also be careful of the needs of inline functions.
INCLUDES = -DQOCA_INTERNALS -I.. -I$(top_srcdir)/cxx

# Things needed by user programs.  User programs #include only Qc.h, but Qc.h
# #includes the rest of these.
pkginclude_HEADERS = m4u_white([[
	arith.H KeyIterator.hh
	QcASetSolver.H
	m4u_Hh(QcAliasedBiMap)
	QcFloat.hh QcFixedFloat.hh QcConstraint.hh
	QcBasicnessVarIndexIterator.hh
	m4u_Hh(QcBiMap)
	QcCassSolver.hh QcLinEqSolver.hh QcLinInEqSolver.hh
	QcUtility.hh QcDefines.hh
	QcEnables.H
	QcFixedFloatRep.H QcFixedFloatRep.hh
	QcFloatRep.H QcFloatRep.hh
	QcCassConstraint.hh QcDelLinEqSystem.hh QcDesireValueStore.hh
	QcCompPivotSystem.hh QcCompPivotTableau.hh QcConstraintRep.hh
	QcDelLinEqTableau.hh QcDelLinInEqSystem.hh QcException.hh
	QcIneqSolverBase.H
	QcLinEqSystem.hh QcLinInEqSystem.hh QcLinPoly.hh
	QcBiMapNotifier.hh QcDelLinInEqTableau.hh QcLinEqTableau.hh
	QcLinInEqTableau.hh QcLinPolyTerm.hh QcSolver.H QcSolver.hh
	QcSparseCoeff.hh QcConstraintBiMap.hh QcDelCoreTableau.hh
	QcLinEqRowColStateVector.hh
	QcLinInEqRowColStateVector.H QcLinInEqRowColStateVector.hh
	QcRowAdaptor.hh QcTableau.hh
	m4u_Hh(QcVariableBiMap)
	QcCoreTableau.hh QcDenseMatrix.hh
	QcLinEqColStateVector.H QcLinEqColStateVector.hh
	QcLinEqRowStateVector.H QcLinEqRowStateVector.hh
	QcLinInEqColStateVector.H QcLinInEqColStateVector.hh
	QcNullableConstraint.hh QcNullableElement.hh QcNullableFloat.hh
	QcQuasiRowColStateVector.H QcQuasiRowColStateVector.hh
	QcSparseMatrixColIterator.hh
	QcTableauRowIterator.hh QcLinEqColState.hh QcLinEqRowState.hh
	QcLinInEqColState.hh QcMatrix.hh
	QcNlpSolver.H QcNlpSolver.hh
	QcNomadicFloatRep.H QcNomadicFloatRep.hh
	QcOrigRowStateVector.H QcOrigRowStateVector.hh
	QcQuasiColStateVector.hh
	QcQuasiRowStateVector.H QcQuasiRowStateVector.hh
	QcSolvedFormMatrix.hh QcSparseMatrixIterator.hh
	QcSparseMatrixRowIterator.hh QcMatrixIterator.hh
	QcOrigRowState.hh QcQuasiColState.hh QcQuasiRowState.hh
	QcSparseMatrix.hh QcStateVector.hh QcIterator.hh
	QcSparseMatrixElement.hh QcState.hh QcUnsortedListSet.hh
	QcVarStow.H
	]])

dist_libqoca_sources = m4u_white([[
	m4u_Hhc(QcASetSolver)
	QcCassSolver.cc QcCompPivotSystem.cc QcCompPivotTableau.cc
	QcConstraintRep.cc QcCoreTableau.cc QcDelCoreTableau.cc
	QcDelLinEqTableau.cc QcDelLinInEqTableau.cc QcDenseMatrix.cc
	QcFloatRep.cc
	m4u_Hhc(QcIneqSolverBase)
	QcLinEqSolver.cc QcLinEqSystem.cc QcLinEqTableau.cc
	QcLinInEqSolver.cc QcLinInEqSystem.cc QcLinInEqTableau.cc
	QcLinPoly.cc QcSolver.cc QcSparseMatrix.cc
	KeyIterator.hh
	m4u_Hh(QcAliasedBiMap)
	QcBasicnessVarIndexIterator.hh
	QcBasicVarIndexIterator.hh
	m4u_Hh(QcBiMap)
	QcBiMapNotifier.hh
	QcCassConstraint.hh QcCassSolver.hh QcCompPivotSystem.hh
	QcCompPivotTableau.hh QcConstraint.hh QcConstraintBiMap.hh
	QcConstraintIndexIterator.hh QcConstraintRep.hh QcCoreTableau.hh
	QcDefines.hh QcDelCoreTableau.hh QcDelLinEqSystem.hh QcDelLinEqTableau.hh
	QcDelLinInEqSystem.hh QcDelLinInEqTableau.hh QcDenseMatrix.hh
	QcDenseMatrixColIterator.hh QcDenseMatrixIterator.hh
	QcDenseMatrixRowIterator.hh QcDenseTableauColIterator.hh
	QcDenseTableauIterator.hh QcDenseTableauRowIterator.hh
	QcDesireValueStore.hh QcException.hh QcFixedFloat.hh
	QcFixedFloatRep.cc QcFixedFloatRep.hh
	QcFloat.hh QcFloatRep.hh QcIterator.hh
	QcLinEqColState.hh
	m4u_Hhc(QcLinEqColStateVector)
	QcLinEqRowColStateVector.hh
	QcLinEqRowState.hh
	m4u_Hhc(QcLinEqRowStateVector)
	QcLinEqSolver.hh
	QcLinEqSystem.hh QcLinEqTableau.hh QcLinInEqColState.hh
	m4u_Hhc(QcLinInEqColStateVector)
	m4u_Hhc(QcLinInEqRowColStateVector)
	QcLinInEqSolver.hh QcLinInEqSystem.hh QcLinInEqTableau.hh QcLinPoly.hh
	QcLinPolyTerm.hh QcMatrix.hh QcMatrixIterator.hh QcOrigRowState.hh
	m4u_Hhc(QcNlpSolver)
	QcNomadicFloatRep.cc QcNomadicFloatRep.hh
	QcNullIterator.H
	QcNullableConstraint.hh QcNullableElement.hh QcNullableFloat.hh
	QcOrigRowStateVector.H QcOrigRowStateVector.hh QcOrigRowStateVector.cc
	QcParamVarIndexIterator.hh QcQuasiColState.hh
	QcQuasiColStateVector.hh
	m4u_Hhc(QcQuasiRowColStateVector)
	QcRowAdaptor.hh
	QcQuasiRowStateVector.H QcQuasiRowStateVector.hh QcQuasiRowStateVector.cc
	QcQuasiRowState.hh
	QcSolvedFormMatrix.hh QcSolver.hh
	QcSparseCoeff.hh QcSparseMatrix.hh QcSparseMatrixColIterator.hh
	QcSparseMatrixElement.hh QcSparseMatrixIterator.hh
	QcSparseMatrixRowIterator.hh QcState.hh QcStateVector.hh
	QcStructVarIndexIterator.hh QcTableau.hh QcTableauColIterator.hh
	QcTableauRowIterator.hh QcUnsortedListSet.hh QcUtility.hh
	m4u_Hhc(QcVarStow)
	m4u_Hhc(QcVariableBiMap)
	QcVariableIndexIterator.hh
	arith.H
	]])

ch_files = m4u_white([[
	QcAliasedBiMap.ch
	QcASetSolver.ch
	QcBiMap.ch
	QcFixedFloatRep.ch QcFloatRep.ch
	QcIneqSolverBase.ch
	QcLinEqColStateVector.ch
	QcLinEqRowStateVector.ch QcLinInEqColStateVector.ch
	QcLinInEqRowColStateVector.ch
	QcNlpSolver.ch
	QcNomadicFloatRep.ch QcNullIterator.ch
	QcOrigRowStateVector.ch QcQuasiRowColStateVector.ch
	QcQuasiRowStateVector.ch QcSolver.ch
	QcVariableBiMap.ch
	QcVarStow.ch
	arith.ch
	]])

libqoca_la_SOURCES =
# Rationale for emptiness: If it has any .cc files, then automake
# produces .cc.o rules, which stuffs things up (because make sometimes
# tries to use them directly instead of using the .ii0.o rule).  If it
# had .ii0 files, then the .ii0 files would be put into distdir, which
# is wrong because .ii0 files contain system header file contents.

EXTRA_DIST = $(dist_libqoca_sources) Makefile.am.m4 $(ch_files)

CLEANFILES = *.ii *.ii0

DISTCLEANFILES = -r .deps

dist-hook:
	@rm -f $(distdir)/Makefile.tmp
	grep -v '^-include ' < $(distdir)/Makefile.in > $(distdir)/Makefile.tmp
	mv $(distdir)/Makefile.tmp $(distdir)/Makefile.in
