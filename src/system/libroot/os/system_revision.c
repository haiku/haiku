/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>.
 * Distributed under the terms of the MIT License.
 */


#include <system_revision.h>


// Haiku SVN revision. Will be set when copying libroot.so to the image.
// Lives in a separate section so that it can easily be found.
static char sHaikuRevision[SYSTEM_REVISION_LENGTH]
	__attribute__((section("_haiku_revision")));


const char*
get_system_revision()
{
	return sHaikuRevision;
}
