#ifndef BEUTILS_H
#define BEUTILS_H

#include <FindDirectory.h>
#include <Path.h>

status_t TestForAddonExistence(const char* name, directory_which which,
							   const char* section, BPath& outPath);

#endif
