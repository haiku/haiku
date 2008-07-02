/*
 * Copyright 2008, Mika Lindqvist. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <string.h>

char *
strlwr(char *str)
{
    char *c = str;
    while (*c) {
        *c = tolower(*c);
        c++;
    }

    return str;
}

