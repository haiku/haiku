/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/* converts a string to a format suitable for use in urls
 * ex: "foo bar+" -> "foo+bar%2D"
 * caller must free() result
 */
extern char *urlify_string(const char *str);

/* converts string with html entities to regular utf-8 string
 * ex: "Fran&ccedil;ois" -> "François"
 * caller must free() result
 */
extern char *unentitify_string(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* _STRING_UTILS_H */
