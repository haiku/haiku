/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_REPO_H
#define PACKAGE_REPO_H


void	print_usage_and_exit(bool error);

int		command_create(int argc, const char* const* argv);
int		command_list(int argc, const char* const* argv);


#endif	// PACKAGE_REPO_H
