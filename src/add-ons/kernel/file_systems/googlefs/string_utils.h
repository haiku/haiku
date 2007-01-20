#ifndef _STRING_UTILS_H
#define _STRING_UTILS_H

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

#endif /* _STRING_UTILS_H */
