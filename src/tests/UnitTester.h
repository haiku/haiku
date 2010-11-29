/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIT_TESTER_SHELL_H
#define UNIT_TESTER_SHELL_H


#include <TestShell.h>

#include <string>


class UnitTesterShell : public BTestShell {
public:
								UnitTesterShell(
									const std::string &description = "",
									SyncObject *syncObject = NULL);

protected:
	virtual	void				PrintDescription(int argc, char *argv[]);
	virtual	void				PrintValidArguments();
	virtual	void				LoadDynamicSuites();
};


#endif // UNIT_TESTER_SHELL_H
