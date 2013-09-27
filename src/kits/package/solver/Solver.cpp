/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/Solver.h>


typedef BPackageKit::BSolver* CreateSolverFunction();


#if defined(HAIKU_TARGET_PLATFORM_HAIKU) || __GNUC__ == 2


#include <dlfcn.h>
#include <pthread.h>


static CreateSolverFunction* sCreateSolver = NULL;

static pthread_once_t sLoadLibsolvSolverAddOnInitOnce = PTHREAD_ONCE_INIT;


static void
load_libsolv_solver_add_on()
{
#if defined(HAIKU_TARGET_PLATFORM_HAIKU) || __GNUC__ == 2
	void* imageHandle = dlopen("libpackage-add-on-libsolv.so", 0);
#else
	void* imageHandle = dlopen("libpackage-add-on-libsolv_build.so", 0);
#endif
	if (imageHandle == NULL)
		return;

	sCreateSolver = (CreateSolverFunction*)dlsym(imageHandle, "create_solver");
	if (sCreateSolver == NULL)
		dlclose(imageHandle);
}

#else


static BPackageKit::BSolver* __create_libsolv_solver()
	__attribute__((weakref("__create_libsolv_solver")));
static CreateSolverFunction* sCreateSolver = &__create_libsolv_solver;


#endif


namespace BPackageKit {


BSolver::BSolver()
{
}


BSolver::~BSolver()
{
}


/*static*/ status_t
BSolver::Create(BSolver*& _solver)
{
#if defined(HAIKU_TARGET_PLATFORM_HAIKU) || __GNUC__ == 2
	pthread_once(&sLoadLibsolvSolverAddOnInitOnce, &load_libsolv_solver_add_on);
#endif
	if (sCreateSolver == NULL)
		return B_NOT_SUPPORTED;

	BSolver* solver = sCreateSolver();
	if (solver == NULL)
		return B_NO_MEMORY;

	status_t error = solver->Init();
	if (error != B_OK) {
		delete solver;
		return error;
	}

	_solver = solver;
	return B_OK;
}


}	// namespace BPackageKit
