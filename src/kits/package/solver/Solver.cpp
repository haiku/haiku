/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/solver/Solver.h>


typedef BPackageKit::BSolver* CreateSolverFunction();


#include <dlfcn.h>
#include <pthread.h>


static CreateSolverFunction* sCreateSolver = NULL;

static pthread_once_t sLoadLibsolvSolverAddOnInitOnce = PTHREAD_ONCE_INIT;


static void
load_libsolv_solver_add_on()
{
	int flags = 0;
#ifdef HAIKU_TARGET_PLATFORM_HAIKU
	void* imageHandle = dlopen("libpackage-add-on-libsolv.so", flags);
#else
#ifdef HAIKU_HOST_PLATFORM_LINUX
	flags = RTLD_LAZY | RTLD_LOCAL;
#endif
	void* imageHandle = dlopen("libpackage-add-on-libsolv_build.so", flags);
#endif
	if (imageHandle == NULL)
		return;

	sCreateSolver = (CreateSolverFunction*)dlsym(imageHandle, "create_solver");
	if (sCreateSolver == NULL)
		dlclose(imageHandle);
}


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
	pthread_once(&sLoadLibsolvSolverAddOnInitOnce, &load_libsolv_solver_add_on);
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
