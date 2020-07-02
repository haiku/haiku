/* libhttpd.h - defines for libhttpd
**
** Copyright © 1995,1998,1999,2000,2001 by Jef Poskanzer <jef@mail.acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
** FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
** DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
** OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
** HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
** OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
*/

#ifndef _LIBHTTPD_H_
#define _LIBHTTPD_H_

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

#if defined(AF_INET6) && defined(IN6_IS_ADDR_V4MAPPED)
#define USE_IPV6
#endif


/* A few convenient defines. */

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#define NEW(t,n) ((t*) malloc( sizeof(t) * (n) ))
#define RENEW(o,t,n) ((t*) realloc( (void*) o, sizeof(t) * (n) ))


/* The httpd structs. */

/* A multi-family sockaddr. */
typedef union {
    struct sockaddr sa;
    struct sockaddr_in sa_in;
#ifdef USE_IPV6
    struct sockaddr_in6 sa_in6;
    struct sockaddr_storage sa_stor;
#endif /* USE_IPV6 */
    } httpd_sockaddr;

/* A server. */
typedef struct {
    char* binding_hostname;
    char* server_hostname;
    unsigned short port;
    char* cgi_pattern;
    int cgi_limit, cgi_count;
    char* charset;
    char* p3p;
    int max_age;
    char* cwd;
    int listen4_fd, listen6_fd;
    int no_log;
    FILE* logfp;
    int no_symlink_check;
    int vhost;
    int global_passwd;
    char* url_pattern;
    char* local_pattern;
    int no_empty_referers;
    //added for poorman
    int do_list_dir;
    char* index_name;
    } httpd_server;

/* A connection. */
typedef struct {
    int initialized;
    httpd_server* hs;
    httpd_sockaddr client_addr;
    char* read_buf;
    size_t read_size, read_idx, checked_idx;
    int checked_state;
    int method;
    int status;
    off_t bytes_to_send;
    off_t bytes_sent;
    char* encodedurl;
    char* decodedurl;
    char* protocol;
    char* origfilename;
    char* expnfilename;
    char* encodings;
    char* pathinfo;
    char* query;
    char* referer;
    char* useragent;
    char* accept;
    char* accepte;
    char* acceptl;
    char* cookie;
    char* contenttype;
    char* reqhost;
    char* hdrhost;
    char* hostdir;
    char* authorization;
    char* remoteuser;
    char* response;
    size_t maxdecodedurl, maxorigfilename, maxexpnfilename, maxencodings,
	maxpathinfo, maxquery, maxaccept, maxaccepte, maxreqhost, maxhostdir,
	maxremoteuser, maxresponse;
#ifdef TILDE_MAP_2
    char* altdir;
    size_t maxaltdir;
#endif /* TILDE_MAP_2 */
    size_t responselen;
    time_t if_modified_since, range_if;
    size_t contentlength;
    char* type;		/* not malloc()ed */
    char* hostname;	/* not malloc()ed */
    int mime_flag;
    int one_one;	/* HTTP/1.1 or better */
    int got_range;
    int tildemapped;	/* this connection got tilde-mapped */
    off_t first_byte_index, last_byte_index;
    int keep_alive;
    int should_linger;
    struct stat sb;
    int conn_fd;
    int processed_directory_index;
    } httpd_conn;

/* Methods. */
#define METHOD_UNKNOWN 0
#define METHOD_GET 1
#define METHOD_HEAD 2
#define METHOD_POST 3

/* States for checked_state. */
#define CHST_FIRSTWORD 0
#define CHST_FIRSTWS 1
#define CHST_SECONDWORD 2
#define CHST_SECONDWS 3
#define CHST_THIRDWORD 4
#define CHST_THIRDWS 5
#define CHST_LINE 6
#define CHST_LF 7
#define CHST_CR 8
#define CHST_CRLF 9
#define CHST_CRLFCR 10
#define CHST_BOGUS 11

#ifdef __cplusplus
extern "C" {
#endif

/* Initializes.  Does the socket(), bind(), and listen().   Returns an
** httpd_server* which includes a socket fd that you can select() on.
** Return (httpd_server*) 0 on error.
*/
extern httpd_server* httpd_initialize(
    char* hostname, httpd_sockaddr* sa4P, httpd_sockaddr* sa6P,
    unsigned short port, char* cgi_pattern, int cgi_limit, char* charset,
    char* p3p, int max_age, char* cwd, int no_log, FILE* logfp,
    int no_symlink_check, int vhost, int global_passwd, char* url_pattern,
    char* local_pattern, int no_empty_referers );

/* PoorMan: Initialize_listen_socket() is changed from static to extern.
** httpd_unlisten() needs an opposite operation that can be accessed from
** outside libhttpd. So this is it.
*/
extern int httpd_initialize_listen_socket( httpd_sockaddr* saP );

/* Change the log file. */
extern void httpd_set_logfp( httpd_server* hs, FILE* logfp );

/* Call to unlisten/close socket(s) listening for new connections. */
extern void httpd_unlisten( httpd_server* hs );

/* Call to shut down. */
extern void httpd_terminate( httpd_server* hs );


/* When a listen fd is ready to read, call this.  It does the accept() and
** returns an httpd_conn* which includes the fd to read the request from and
** write the response to.  Returns an indication of whether the accept()
** failed, succeeded, or if there were no more connections to accept.
**
** In order to minimize malloc()s, the caller passes in the httpd_conn.
** The caller is also responsible for setting initialized to zero before the
** first call using each different httpd_conn.
*/
extern int httpd_get_conn( httpd_server* hs, int listen_fd, httpd_conn* hc );
#define GC_FAIL 0
#define GC_OK 1
#define GC_NO_MORE 2

/* Checks whether the data in hc->read_buf constitutes a complete request
** yet.  The caller reads data into hc->read_buf[hc->read_idx] and advances
** hc->read_idx.  This routine checks what has been read so far, using
** hc->checked_idx and hc->checked_state to keep track, and returns an
** indication of whether there is no complete request yet, there is a
** complete request, or there won't be a valid request due to a syntax error.
*/
extern int httpd_got_request( httpd_conn* hc );
#define GR_NO_REQUEST 0
#define GR_GOT_REQUEST 1
#define GR_BAD_REQUEST 2

/* Parses the request in hc->read_buf.  Fills in lots of fields in hc,
** like the URL and the various headers.
**
** Returns -1 on error.
*/
extern int httpd_parse_request( httpd_conn* hc );

/* Starts sending data back to the client.  In some cases (directories,
** CGI programs), finishes sending by itself - in those cases, hc->file_fd
** is <0.  If there is more data to be sent, then hc->file_fd is a file
** descriptor for the file to send.  If you don't have a current timeval
** handy just pass in 0.
**
** Returns -1 on error.
*/
extern int httpd_start_request( httpd_conn* hc, struct timeval* nowP );

/* Actually sends any buffered response text. */
extern void httpd_write_response( httpd_conn* hc );

/* Call this to close down a connection and free the data.  A fine point,
** if you fork() with a connection open you should still call this in the
** parent process - the connection will stay open in the child.
** If you don't have a current timeval handy just pass in 0.
*/
extern void httpd_close_conn( httpd_conn* hc, struct timeval* nowP );

/* Call this to de-initialize a connection struct and *really* free the
** mallocced strings.
*/
extern void httpd_destroy_conn( httpd_conn* hc );


/* Send an error message back to the client. */
extern void httpd_send_err(
    httpd_conn* hc, int status, char* title, char* extraheads, char* form, char* arg );

/* Some error messages. */
extern char* httpd_err400title;
extern char* httpd_err400form;
extern char* httpd_err408title;
extern char* httpd_err408form;
extern char* httpd_err503title;
extern char* httpd_err503form;

/* Generate a string representation of a method number. */
extern char* httpd_method_str( int method );

/* Reallocate a string. */
extern void httpd_realloc_str( char** strP, size_t* maxsizeP, size_t size );

/* Format a network socket to a string representation. */
extern char* httpd_ntoa( httpd_sockaddr* saP );

/* Set NDELAY mode on a socket. */
extern void httpd_set_ndelay( int fd );

/* Clear NDELAY mode on a socket. */
extern void httpd_clear_ndelay( int fd );

/* Read the requested buffer completely, accounting for interruptions. */
extern int httpd_read_fully( int fd, void* buf, size_t nbytes );

/* Write the requested buffer completely, accounting for interruptions. */
extern int httpd_write_fully( int fd, const void* buf, size_t nbytes );

/* Generate debugging statistics syslog message. */
extern void httpd_logstats( long secs );

#ifdef __cplusplus
}
#endif

#endif /* _LIBHTTPD_H_ */
