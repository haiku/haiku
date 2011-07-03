/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_H
#define PACKAGE_H


void	print_usage_and_exit(bool error);

int		command_add(int argc, const char* const* argv);
int		command_create(int argc, const char* const* argv);
int		command_dump(int argc, const char* const* argv);
int		command_extract(int argc, const char* const* argv);
int		command_list(int argc, const char* const* argv);


#endif	// PACKAGE_H
