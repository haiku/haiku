#include <stdio.h>
#include "pathname.c"

void main()
{
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/usr/lib", dirname("/usr/lib"), basename("/usr/lib"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/usr/lib/", dirname("/usr/lib/"), basename("/usr/lib/"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/usr/", dirname("/usr/"), basename("/usr/"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/usr///", dirname("/usr///"), basename("/usr///"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "///usr////", dirname("///usr////"), basename("///usr////"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "///usr////aaa//", dirname("///usr////aaa//"), basename("///usr////aaa//"));

    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr/lib/foo", dirname("usr/lib/foo"), basename("usr/lib/foo"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr/lib/foo/", dirname("usr/lib/foo/"), basename("usr/lib/foo/"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr/", dirname("usr/"), basename("usr/"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr///", dirname("usr///"), basename("usr///"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr////", dirname("usr////"), basename("usr////"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr////aaa//", dirname("usr////aaa//"), basename("usr////aaa//"));

    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/", dirname("/"), basename("/"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "////", dirname("////"), basename("////"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "usr", dirname("usr"), basename("usr"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   ".", dirname("."), basename("."));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "..", dirname(".."), basename(".."));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/foo/bar/baaz/quux/", dirname("/foo/bar/baaz/quux/"), basename("/foo/bar/baaz/quux/"));
    printf("input: '%s'\t\tdir: '%s' base: '%s'\n",
	   "/a//b", dirname("/a//b"), basename("/a//b"));
}
