/*
 * memset --- initialize memory
 *
 * We supply this routine for those systems that aren't standard yet.
 */

void *
memset(dest, val, l)
void *dest;
register int val;
register size_t l;
{
	register char *ret = dest;
	register char *d = dest;

	while (l--)
		*d++ = val;

	return ((void *) ret);
}
