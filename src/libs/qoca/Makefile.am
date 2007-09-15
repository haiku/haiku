$(srcdir)/Makefile.am: Makefile.am.m4
	rm -f $(srcdir)/$(@F)
	m4 -P $(srcdir)/$(<F) > $(srcdir)/$(@F)
# AFAIK, this has to be in srcdir (as opposed to bsrcdir).






##bsrcdir = $(srcdir)
bsrcdir = .



.ch.cc: $(CH2XX)
	@rm -f $(bsrcdir)/$(*F).cc && if echo "$(CH2XX) -c $< > $(bsrcdir)/$(*F).cc" \
	  && $(CH2XX) -c $< > $(bsrcdir)/$(*F).cc; \
	then :; \
	else echo "rm -f $(bsrcdir)/$(*F).cc; exit 1" \
	  && rm -f $(bsrcdir)/$(*F).cc; exit 1; fi

.ch.hh: $(CH2XX)
	@rm -f $(bsrcdir)/$(*F).hh && if echo "$(CH2XX) -i $< > $(bsrcdir)/$(*F).hh" \
	  && $(CH2XX) -i $< > $(bsrcdir)/$(*F).hh; \
	then :; \
	else echo "rm -f $(bsrcdir)/$(*F).hh; exit 1" \
	  && rm -f $(bsrcdir)/$(*F).hh; exit 1; fi

.ch.H: $(CH2XX)
	@rm -f $(bsrcdir)/$(*F).H && if echo "$(CH2XX) -d $< > $(bsrcdir)/$(*F).H" \
	  && $(CH2XX) -d $< > $(bsrcdir)/$(*F).H; \
	then :; \
	else echo "rm -f $(bsrcdir)/$(*F).H; exit 1" \
	  && rm -f $(bsrcdir)/$(*F).H; exit 1; fi

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)




DEP_FILES =  .deps/QcCassSolver.P .deps/QcCompPivotSystem.P .deps/QcCompPivotTableau.P .deps/QcConstraintRep.P .deps/QcCoreTableau.P .deps/QcDelCoreTableau.P .deps/QcDelLinEqTableau.P .deps/QcDelLinInEqTableau.P .deps/QcDenseMatrix.P .deps/QcFixedFloatRep.P .deps/QcFloatRep.P .deps/QcIneqSolverBase.P .deps/QcLinEqSolver.P .deps/QcLinEqSystem.P .deps/QcLinEqTableau.P .deps/QcLinInEqSolver.P .deps/QcLinInEqSystem.P .deps/QcLinInEqTableau.P .deps/QcLinPoly.P .deps/QcLinEqColStateVector.P .deps/QcLinEqRowStateVector.P .deps/QcLinInEqColStateVector.P .deps/QcLinInEqRowColStateVector.P .deps/QcNlpSolver.P .deps/QcNomadicFloatRep.P .deps/QcOrigRowStateVector.P .deps/QcQuasiRowColStateVector.P .deps/QcQuasiRowStateVector.P .deps/QcSolver.P .deps/QcSparseMatrix.P .deps/QcVarStow.P .deps/QcVariableBiMap.P


-include $(DEP_FILES)

.cc.ii0:
	@rm -f $(bsrcdir)/$(@F) && if echo "$(CXXCPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) -Wp,-MD,.deps/$(*F).pp $< > $(bsrcdir)/$(@F)" \
	  && $(CXXCPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) -Wp,-MD,.deps/$(*F).pp $< > $(bsrcdir)/$(@F); \
	then :; \
	else echo "rm -f $(bsrcdir)/$(@F); exit 1" \
	  && rm -f $(bsrcdir)/$(@F); exit 1; fi
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

libqoca_la_myOBJECTS =  QcCassSolver.lo QcCompPivotSystem.lo QcCompPivotTableau.lo QcConstraintRep.lo QcCoreTableau.lo QcDelCoreTableau.lo QcDelLinEqTableau.lo QcDelLinInEqTableau.lo QcDenseMatrix.lo QcFixedFloatRep.lo QcFloatRep.lo QcIneqSolverBase.lo QcLinEqSolver.lo QcLinEqSystem.lo QcLinEqTableau.lo QcLinInEqSolver.lo QcLinInEqSystem.lo QcLinInEqTableau.lo QcLinPoly.lo QcLinEqColStateVector.lo QcLinEqRowStateVector.lo QcLinInEqColStateVector.lo QcLinInEqRowColStateVector.lo QcNlpSolver.lo QcNomadicFloatRep.lo QcOrigRowStateVector.lo QcQuasiRowColStateVector.lo QcQuasiRowStateVector.lo QcSolver.lo QcSparseMatrix.lo QcVarStow.lo QcVariableBiMap.lo

ii0_files =  QcCassSolver.ii0 QcCompPivotSystem.ii0 QcCompPivotTableau.ii0 QcConstraintRep.ii0 QcCoreTableau.ii0 QcDelCoreTableau.ii0 QcDelLinEqTableau.ii0 QcDelLinInEqTableau.ii0 QcDenseMatrix.ii0 QcFixedFloatRep.ii0 QcFloatRep.ii0 QcIneqSolverBase.ii0 QcLinEqSolver.ii0 QcLinEqSystem.ii0 QcLinEqTableau.ii0 QcLinInEqSolver.ii0 QcLinInEqSystem.ii0 QcLinInEqTableau.ii0 QcLinPoly.ii0 QcLinEqColStateVector.ii0 QcLinEqRowStateVector.ii0 QcLinInEqColStateVector.ii0 QcLinInEqRowColStateVector.ii0 QcNlpSolver.ii0 QcNomadicFloatRep.ii0 QcOrigRowStateVector.ii0 QcQuasiRowColStateVector.ii0 QcQuasiRowStateVector.ii0 QcSolver.ii0 QcSparseMatrix.ii0 QcVarStow.ii0 QcVariableBiMap.ii0

libqoca.la: $(ii0_files) $(libqoca_la_myOBJECTS) $(libqoca_la_DEPENDENCIES)
	$(CXXLINK) -rpath $(libdir) $(libqoca_la_LDFLAGS) $(libqoca_la_myOBJECTS) $(libqoca_la_LIBADD) $(LIBS)


.ii0.o:
	@if test -f $@ && cmp -s $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii 2>/dev/null; then :; else \
	 echo "cp $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii" \
	  && cp $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii && \
	 echo "$(CXXCOMPILE) -c $(bsrcdir)/$(*F).ii" \
	  && $(CXXCOMPILE) -c $(bsrcdir)/$(*F).ii; \
	fi

.ii0.lo:
	@if test -f $@ && cmp -s $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii 2>/dev/null; then :; else \
	 echo "cp $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii" \
	  && cp $(bsrcdir)/$(<F) $(bsrcdir)/$(*F).ii && \
	 echo "$(LTCXXCOMPILE) -c $(bsrcdir)/$(*F).ii" \
	  && $(LTCXXCOMPILE) -c $(bsrcdir)/$(*F).ii; \
	fi


# Note that we must start off with no .deps files.




cc = cc
BUILT_SOURCES =  arith.H QcAliasedBiMap.H QcAliasedBiMap.hh QcASetSolver.H QcASetSolver.hh QcASetSolver.$(cc) QcBiMap.H QcBiMap.hh QcFloatRep.H QcFloatRep.hh QcFloatRep.$(cc) QcFixedFloatRep.H QcFixedFloatRep.hh QcFixedFloatRep.$(cc) QcIneqSolverBase.H QcIneqSolverBase.hh QcIneqSolverBase.$(cc) QcLinEqColStateVector.H QcLinEqColStateVector.hh QcLinEqColStateVector.$(cc) QcLinEqRowStateVector.H QcLinEqRowStateVector.hh QcLinEqRowStateVector.$(cc) QcLinInEqColStateVector.H QcLinInEqColStateVector.hh QcLinInEqColStateVector.$(cc) QcLinInEqRowColStateVector.H QcLinInEqRowColStateVector.hh QcLinInEqRowColStateVector.$(cc) QcNlpSolver.H QcNlpSolver.hh QcNlpSolver.$(cc) QcNomadicFloatRep.H QcNomadicFloatRep.hh QcNomadicFloatRep.$(cc) QcNullIterator.H QcOrigRowStateVector.H QcOrigRowStateVector.hh QcOrigRowStateVector.$(cc) QcQuasiRowColStateVector.H QcQuasiRowColStateVector.hh QcQuasiRowColStateVector.$(cc) QcQuasiRowStateVector.H QcQuasiRowStateVector.hh QcQuasiRowStateVector.$(cc) QcSolver.H QcSolver.hh QcSolver.$(cc) QcVarStow.H QcVarStow.hh QcVarStow.$(cc) QcVariableBiMap.H QcVariableBiMap.hh QcVariableBiMap.$(cc) 

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
pkginclude_HEADERS =  arith.H KeyIterator.hh QcASetSolver.H QcAliasedBiMap.H QcAliasedBiMap.hh QcFloat.hh QcFixedFloat.hh QcConstraint.hh QcBasicnessVarIndexIterator.hh QcBiMap.H QcBiMap.hh QcCassSolver.hh QcLinEqSolver.hh QcLinInEqSolver.hh QcUtility.hh QcDefines.hh QcEnables.H QcFixedFloatRep.H QcFixedFloatRep.hh QcFloatRep.H QcFloatRep.hh QcCassConstraint.hh QcDelLinEqSystem.hh QcDesireValueStore.hh QcCompPivotSystem.hh QcCompPivotTableau.hh QcConstraintRep.hh QcDelLinEqTableau.hh QcDelLinInEqSystem.hh QcException.hh QcIneqSolverBase.H QcLinEqSystem.hh QcLinInEqSystem.hh QcLinPoly.hh QcBiMapNotifier.hh QcDelLinInEqTableau.hh QcLinEqTableau.hh QcLinInEqTableau.hh QcLinPolyTerm.hh QcSolver.H QcSolver.hh QcSparseCoeff.hh QcConstraintBiMap.hh QcDelCoreTableau.hh QcLinEqRowColStateVector.hh QcLinInEqRowColStateVector.H QcLinInEqRowColStateVector.hh QcRowAdaptor.hh QcTableau.hh QcVariableBiMap.H QcVariableBiMap.hh QcCoreTableau.hh QcDenseMatrix.hh QcLinEqColStateVector.H QcLinEqColStateVector.hh QcLinEqRowStateVector.H QcLinEqRowStateVector.hh QcLinInEqColStateVector.H QcLinInEqColStateVector.hh QcNullableConstraint.hh QcNullableElement.hh QcNullableFloat.hh QcQuasiRowColStateVector.H QcQuasiRowColStateVector.hh QcSparseMatrixColIterator.hh QcTableauRowIterator.hh QcLinEqColState.hh QcLinEqRowState.hh QcLinInEqColState.hh QcMatrix.hh QcNlpSolver.H QcNlpSolver.hh QcNomadicFloatRep.H QcNomadicFloatRep.hh QcOrigRowStateVector.H QcOrigRowStateVector.hh QcQuasiColStateVector.hh QcQuasiRowStateVector.H QcQuasiRowStateVector.hh QcSolvedFormMatrix.hh QcSparseMatrixIterator.hh QcSparseMatrixRowIterator.hh QcMatrixIterator.hh QcOrigRowState.hh QcQuasiColState.hh QcQuasiRowState.hh QcSparseMatrix.hh QcStateVector.hh QcIterator.hh QcSparseMatrixElement.hh QcState.hh QcUnsortedListSet.hh QcVarStow.H 

dist_libqoca_sources =  QcASetSolver.H QcASetSolver.hh QcASetSolver.$(cc) QcCassSolver.cc QcCompPivotSystem.cc QcCompPivotTableau.cc QcConstraintRep.cc QcCoreTableau.cc QcDelCoreTableau.cc QcDelLinEqTableau.cc QcDelLinInEqTableau.cc QcDenseMatrix.cc QcFloatRep.cc QcIneqSolverBase.H QcIneqSolverBase.hh QcIneqSolverBase.$(cc) QcLinEqSolver.cc QcLinEqSystem.cc QcLinEqTableau.cc QcLinInEqSolver.cc QcLinInEqSystem.cc QcLinInEqTableau.cc QcLinPoly.cc QcSolver.cc QcSparseMatrix.cc KeyIterator.hh QcAliasedBiMap.H QcAliasedBiMap.hh QcBasicnessVarIndexIterator.hh QcBasicVarIndexIterator.hh QcBiMap.H QcBiMap.hh QcBiMapNotifier.hh QcCassConstraint.hh QcCassSolver.hh QcCompPivotSystem.hh QcCompPivotTableau.hh QcConstraint.hh QcConstraintBiMap.hh QcConstraintIndexIterator.hh QcConstraintRep.hh QcCoreTableau.hh QcDefines.hh QcDelCoreTableau.hh QcDelLinEqSystem.hh QcDelLinEqTableau.hh QcDelLinInEqSystem.hh QcDelLinInEqTableau.hh QcDenseMatrix.hh QcDenseMatrixColIterator.hh QcDenseMatrixIterator.hh QcDenseMatrixRowIterator.hh QcDenseTableauColIterator.hh QcDenseTableauIterator.hh QcDenseTableauRowIterator.hh QcDesireValueStore.hh QcException.hh QcFixedFloat.hh QcFixedFloatRep.cc QcFixedFloatRep.hh QcFloat.hh QcFloatRep.hh QcIterator.hh QcLinEqColState.hh QcLinEqColStateVector.H QcLinEqColStateVector.hh QcLinEqColStateVector.$(cc) QcLinEqRowColStateVector.hh QcLinEqRowState.hh QcLinEqRowStateVector.H QcLinEqRowStateVector.hh QcLinEqRowStateVector.$(cc) QcLinEqSolver.hh QcLinEqSystem.hh QcLinEqTableau.hh QcLinInEqColState.hh QcLinInEqColStateVector.H QcLinInEqColStateVector.hh QcLinInEqColStateVector.$(cc) QcLinInEqRowColStateVector.H QcLinInEqRowColStateVector.hh QcLinInEqRowColStateVector.$(cc) QcLinInEqSolver.hh QcLinInEqSystem.hh QcLinInEqTableau.hh QcLinPoly.hh QcLinPolyTerm.hh QcMatrix.hh QcMatrixIterator.hh QcOrigRowState.hh QcNlpSolver.H QcNlpSolver.hh QcNlpSolver.$(cc) QcNomadicFloatRep.cc QcNomadicFloatRep.hh QcNullIterator.H QcNullableConstraint.hh QcNullableElement.hh QcNullableFloat.hh QcOrigRowStateVector.H QcOrigRowStateVector.hh QcOrigRowStateVector.cc QcParamVarIndexIterator.hh QcQuasiColState.hh QcQuasiColStateVector.hh QcQuasiRowColStateVector.H QcQuasiRowColStateVector.hh QcQuasiRowColStateVector.$(cc) QcRowAdaptor.hh QcQuasiRowStateVector.H QcQuasiRowStateVector.hh QcQuasiRowStateVector.cc QcQuasiRowState.hh QcSolvedFormMatrix.hh QcSolver.hh QcSparseCoeff.hh QcSparseMatrix.hh QcSparseMatrixColIterator.hh QcSparseMatrixElement.hh QcSparseMatrixIterator.hh QcSparseMatrixRowIterator.hh QcState.hh QcStateVector.hh QcStructVarIndexIterator.hh QcTableau.hh QcTableauColIterator.hh QcTableauRowIterator.hh QcUnsortedListSet.hh QcUtility.hh QcVarStow.H QcVarStow.hh QcVarStow.$(cc) QcVariableBiMap.H QcVariableBiMap.hh QcVariableBiMap.$(cc) QcVariableIndexIterator.hh arith.H 

ch_files =  QcAliasedBiMap.ch QcASetSolver.ch QcBiMap.ch QcFixedFloatRep.ch QcFloatRep.ch QcIneqSolverBase.ch QcLinEqColStateVector.ch QcLinEqRowStateVector.ch QcLinInEqColStateVector.ch QcLinInEqRowColStateVector.ch QcNlpSolver.ch QcNomadicFloatRep.ch QcNullIterator.ch QcOrigRowStateVector.ch QcQuasiRowColStateVector.ch QcQuasiRowStateVector.ch QcSolver.ch QcVariableBiMap.ch QcVarStow.ch arith.ch 

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
