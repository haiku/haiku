/*
** Copyright 2002, Manuel J. Petit. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include "rld_priv.h"

int
export_load_addon(char const *name, unsigned flags)
{
	(void)(name);
	(void)(flags);

	return -1;
}

int
export_unload_addon(int lib, unsigned flags)
{
	(void)(lib);
	(void)(flags);

	return -1;
}

void *
export_addon_symbol(int lib, char const *sym, unsigned flags)
{
	(void)(lib);
	(void)(sym);
	(void)(flags);

	return 0;
}
