/* 
 * Taken from Wikipedia, which declared it as "public domain".
 */

#include <sys/types.h>
#include <string.h>


char *
strstr(const char *s1, const char *s2)
{
    size_t s2len;
    /* Check for the null s2 case. */
    if (*s2 == '\0')
        return (char *) s1;
    s2len = strlen(s2);
    for (; (s1 = strchr(s1, *s2)) != NULL; s1++) {
        if (strncmp(s1, s2, s2len) == 0)
            return (char *)s1;
    }
    return NULL;
}

