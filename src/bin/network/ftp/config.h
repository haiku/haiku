/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */
/* $Id: config.h.in,v 1.28 2002/06/10 08:10:33 lukem Exp $ */


/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
/* #undef _ALL_SOURCE */
#endif

/* Define if the closedir function returns void instead of int.  */
/* #undef CLOSEDIR_VOID */

/* Define if the `getpgrp' function takes no argument.  */
#define GETPGRP_VOID 1

/* Define if your C compiler doesn't accept -c and -o together.  */
/* #undef NO_MINUS_C_MINUS_O */

/* Define if your Fortran 77 compiler doesn't accept -c and -o together. */
/* #undef F77_NO_MINUS_C_MINUS_O */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to the type of arg1 for select(). */
/* #undef SELECT_TYPE_ARG1 */

/* Define to the type of args 2, 3 and 4 for select(). */
/* #undef SELECT_TYPE_ARG234 */

/* Define to the type of arg5 for select(). */
/* #undef SELECT_TYPE_ARG5 */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if the closedir function returns void instead of int.  */
/* #undef VOID_CLOSEDIR */

/* The number of bytes in a off_t.  */
#define SIZEOF_OFF_T 8

/* Define if you have the err function.  */
/* #undef HAVE_ERR */

/* Define if you have the fgetln function.  */
#define HAVE_FGETLN 1

/* Define if you have the fparseln function.  */
/* #undef HAVE_FPARSELN */

/* Define if you have the fseeko function.  */
#define HAVE_FSEEKO 1

/* Define if you have the getaddrinfo function.  */
/* #undef HAVE_GETADDRINFO */

/* Define if you have the gethostbyname2 function.  */
/* #undef HAVE_GETHOSTBYNAME2 */

/* Define if you have the getnameinfo function.  */
/* #undef HAVE_GETNAMEINFO */

/* Define if you have the getpassphrase function.  */
/* #undef HAVE_GETPASSPHRASE */

/* Define if you have the getpgrp function.  */
#define HAVE_GETPGRP 1

/* Define if you have the inet_ntop function.  */
/* #undef HAVE_INET_NTOP */

/* Define if you have the inet_pton function.  */
/* #undef HAVE_INET_PTON */

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have the mkstemp function.  */
#define HAVE_MKSTEMP 1

/* Define if you have the poll function.  */
/* #undef HAVE_POLL */

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setprogname function.  */
/* #undef HAVE_SETPROGNAME */

/* Define if you have the sl_init function.  */
/* #undef HAVE_SL_INIT */

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the strdup function.  */
#define HAVE_STRDUP 1

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strlcat function.  */
#define HAVE_STRLCAT 1

/* Define if you have the strlcpy function.  */
#define HAVE_STRLCPY 1

/* Define if you have the strptime function.  */
/* #undef HAVE_STRPTIME */

/* Define if you have the strsep function.  */
/* #undef HAVE_STRSEP */

/* Define if you have the strtoll function.  */
#define HAVE_STRTOLL 1

/* Define if you have the strunvis function.  */
/* #undef HAVE_STRUNVIS */

/* Define if you have the strvis function.  */
/* #undef HAVE_STRVIS */

/* Define if you have the timegm function.  */
/* #undef HAVE_TIMEGM */

/* Define if you have the usleep function.  */
/* #undef HAVE_USLEEP */

/* Define if you have the <arpa/nameser.h> header file.  */
#define HAVE_ARPA_NAMESER_H 1

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <err.h> header file.  */
#define HAVE_ERR_H 1

/* Define if you have the <libutil.h> header file.  */
/* #undef HAVE_LIBUTIL_H */

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <paths.h> header file.  */
/* #undef HAVE_PATHS_H */

/* Define if you have the <poll.h> header file.  */
/* #undef HAVE_POLL_H */

/* Define if you have the <regex.h> header file.  */
#define HAVE_REGEX_H 1

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/poll.h> header file.  */
/* #undef HAVE_SYS_POLL_H */

/* Define if you have the <termcap.h> header file.  */
#define HAVE_TERMCAP_H 1

/* Define if you have the <util.h> header file.  */
/* #undef HAVE_UTIL_H */

/* Define if you have the <vis.h> header file.  */
/* #undef HAVE_VIS_H */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the socket library (-lsocket).  */
#define HAVE_LIBSOCKET 1

/* Define if you have the tinfo library (-ltinfo).  */
/* #undef HAVE_LIBTINFO */

/* Define if you have the util library (-lutil).  */
/* #undef HAVE_LIBUTIL */

/* Define if your compiler supports `long long' */
#define HAVE_LONG_LONG 1

/* Define if *printf() uses %qd to print `long long' (otherwise uses %lld) */
/* #undef HAVE_PRINTF_QD */

/* Define if in_port_t exists */
#define HAVE_IN_PORT_T 1

/* Define if sa_family_t exists in <sys/socket.h> */
/* #undef HAVE_SA_FAMILY_T */

/* Define if struct sockaddr.sa_len exists (implies sockaddr_in.sin_len, etc) */
#define HAVE_SOCKADDR_SA_LEN 1

/* Define if socklen_t exists */
/* #undef HAVE_SOCKLEN_T */

/* Define if AF_INET6 exists in <sys/socket.h> */
#define HAVE_AF_INET6 1

/* Define if `struct sockaddr_in6' exists in <netinet/in.h> */
/* #undef HAVE_SOCKADDR_IN6 */

/* Define if `struct addrinfo' exists in <netdb.h> */
#define HAVE_ADDRINFO 1

/*
 * Define if <netdb.h> contains AI_NUMERICHOST et al.
 * Systems which only implement RFC2133 will need this.
 */
/* #undef HAVE_RFC2553_NETDB */

/* Define if `struct direct' has a d_namlen element */
/* #undef HAVE_D_NAMLEN */

/* Define if h_errno exists in <netdb.h> */
#define HAVE_H_ERRNO_D 1

/* Define if fclose() is declared in <stdio.h> */
#define HAVE_FCLOSE_D 1

/* Define if getpass() is declared in <stdlib.h> or <unistd.h> */
/* #undef HAVE_GETPASS_D */

/* Define if optarg is declared in <stdlib.h> or <unistd.h> */
#define HAVE_OPTARG_D 1

/* Define if optind is declared in <stdlib.h> or <unistd.h> */
#define HAVE_OPTIND_D 1

/* Define if pclose() is declared in <stdio.h> */
#define HAVE_PCLOSE_D 1

/* Define if `long long' is supported and sizeof(off_t) >= 8 */
#define HAVE_QUAD_SUPPORT 1

/* Define if strptime() is declared in <time.h> */
/* #undef HAVE_STRPTIME_D */

/*
 * Define this if compiling with SOCKS (the firewall traversal library).
 * Also, you must define connect, getsockname, bind, accept, listen, and
 * select to their R-versions.
 */
/* #undef	SOCKS */
/* #undef	SOCKS4 */
/* #undef	SOCKS5 */
/* #undef	connect */
/* #undef	getsockname */
/* #undef	bind */
/* #undef	accept */
/* #undef	listen */
/* #undef	select */
/* #undef	dup */
/* #undef	dup2 */
/* #undef	fclose */
/* #undef	gethostbyname */
/* #undef	getpeername */
/* #undef	read */
/* #undef	recv */
/* #undef	recvfrom */
/* #undef	rresvport */
/* #undef	send */
/* #undef	sendto */
/* #undef	shutdown */
/* #undef	write */
