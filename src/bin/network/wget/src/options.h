/* struct options.
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

In addition, as a special exception, the Free Software Foundation
gives permission to link the code of its release of Wget with the
OpenSSL project's "OpenSSL" library (or with modified versions of it
that use the same license as the "OpenSSL" library), and distribute
the linked executables.  You must obey the GNU General Public License
in all respects for all of the code used other than "OpenSSL".  If you
modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do
so, delete this exception statement from your version.  */

struct options
{
  int verbose;			/* Are we verbose? */
  int quiet;			/* Are we quiet? */
  int ntry;			/* Number of tries per URL */
  int retry_connrefused;	/* Treat CONNREFUSED as non-fatal. */
  int background;		/* Whether we should work in background. */
  int ignore_length;		/* Do we heed content-length at all?  */
  int recursive;		/* Are we recursive? */
  int spanhost;			/* Do we span across hosts in
				   recursion? */
  int relative_only;		/* Follow only relative links. */
  int no_parent;		/* Restrict access to the parent
				   directory.  */
  int reclevel;			/* Maximum level of recursion */
  int dirstruct;		/* Do we build the directory structure
				  as we go along? */
  int no_dirstruct;		/* Do we hate dirstruct? */
  int cut_dirs;			/* Number of directory components to cut. */
  int add_hostdir;		/* Do we add hostname directory? */
  int protocol_directories;	/* Whether to prepend "http"/"ftp" to dirs. */
  int noclobber;		/* Disables clobbering of existing
				   data. */
  char *dir_prefix;		/* The top of directory tree */
  char *lfilename;		/* Log filename */
  char *input_filename;		/* Input filename */
  int force_html;		/* Is the input file an HTML file? */

  int spider;			/* Is Wget in spider mode? */

  char **accepts;		/* List of patterns to accept. */
  char **rejects;		/* List of patterns to reject. */
  char **excludes;		/* List of excluded FTP directories. */
  char **includes;		/* List of FTP directories to
				   follow. */

  char **domains;		/* See host.c */
  char **exclude_domains;
  int dns_cache;		/* whether we cache DNS lookups. */

  char **follow_tags;           /* List of HTML tags to recursively follow. */
  char **ignore_tags;           /* List of HTML tags to ignore if recursing. */

  int follow_ftp;		/* Are FTP URL-s followed in recursive
				   retrieving? */
  int retr_symlinks;		/* Whether we retrieve symlinks in
				   FTP. */
  char *output_document;	/* The output file to which the
				   documents will be printed.  */

  char *user;			/* Generic username */
  char *passwd;			/* Generic password */
  
  int always_rest;		/* Always use REST. */
  char *ftp_user;		/* FTP username */
  char *ftp_passwd;		/* FTP password */
  int netrc;			/* Whether to read .netrc. */
  int ftp_glob;			/* FTP globbing */
  int ftp_pasv;			/* Passive FTP. */

  char *http_user;		/* HTTP username. */
  char *http_passwd;		/* HTTP password. */
  char **user_headers;		/* User-defined header(s). */
  int http_keep_alive;		/* whether we use keep-alive */

  int use_proxy;		/* Do we use proxy? */
  int allow_cache;		/* Do we allow server-side caching? */
  char *http_proxy, *ftp_proxy, *https_proxy;
  char **no_proxy;
  char *base_href;
  char *progress_type;		/* progress indicator type. */
  char *proxy_user; /*oli*/
  char *proxy_passwd;

  double read_timeout;		/* The read/write timeout. */
  double dns_timeout;		/* The DNS timeout. */
  double connect_timeout;	/* The connect timeout. */

  int random_wait;		/* vary from 0 .. wait secs by random()? */
  double wait;			/* The wait period between retrievals. */
  double waitretry;		/* The wait period between retries. - HEH */
  int use_robots;		/* Do we heed robots.txt? */

  wgint limit_rate;		/* Limit the download rate to this
				   many bps. */
  SUM_SIZE_INT quota;		/* Maximum file size to download and
				   store. */
  int numurls;			/* Number of successfully downloaded
				   URLs */

  int server_response;		/* Do we print server response? */
  int save_headers;		/* Do we save headers together with
				   file? */

#ifdef ENABLE_DEBUG
  int debug;			/* Debugging on/off */
#endif

  int timestamping;		/* Whether to use time-stamping. */

  int backup_converted;		/* Do we save pre-converted files as *.orig? */
  int backups;			/* Are numeric backups made? */

  char *useragent;		/* Naughty User-Agent, which can be
				   set to something other than
				   Wget. */
  char *referer;		/* Naughty Referer, which can be
				   set to something other than
				   NULL. */
  int convert_links;		/* Will the links be converted
				   locally? */
  int remove_listing;		/* Do we remove .listing files
				   generated by FTP? */
  int htmlify;			/* Do we HTML-ify the OS-dependent
				   listings? */

  char *dot_style;
  wgint dot_bytes;		/* How many bytes in a printing
				   dot. */
  int dots_in_line;		/* How many dots in one line. */
  int dot_spacing;		/* How many dots between spacings. */

  int delete_after;		/* Whether the files will be deleted
				   after download. */

  int html_extension;		/* Use ".html" extension on all text/html? */

  int page_requisites;		/* Whether we need to download all files
				   necessary to display a page properly. */
  char *bind_address;		/* What local IP address to bind to. */

#ifdef HAVE_SSL
  enum {
    secure_protocol_auto,
    secure_protocol_sslv2,
    secure_protocol_sslv3,
    secure_protocol_tlsv1
  } secure_protocol;		/* type of secure protocol to use. */
  int check_cert;		/* whether to validate the server's cert */
  char *cert_file;		/* external client certificate to use. */
  char *private_key;		/* private key file (if not internal). */
  enum keyfile_type {
    keyfile_pem,
    keyfile_asn1
  } cert_type;			/* type of client certificate file */
  enum keyfile_type
    private_key_type;		/* type of private key file */

  char *ca_directory;		/* CA directory (hash files) */
  char *ca_cert;		/* CA certificate file to use */


  char *random_file;		/* file with random data to seed the PRNG */
  char *egd_file;		/* file name of the egd daemon socket */
#endif /* HAVE_SSL */

  int   cookies;		/* whether cookies are used. */
  char *cookies_input;		/* file we're loading the cookies from. */
  char *cookies_output;		/* file we're saving the cookies to. */
  int   keep_session_cookies;	/* whether session cookies should be
				   saved and loaded. */

  char *post_data;		/* POST query string */
  char *post_file_name;		/* File to post */

  enum {
    restrict_unix,
    restrict_windows
  } restrict_files_os;		/* file name restriction ruleset. */
  int restrict_files_ctrl;	/* non-zero if control chars in URLs
				   are restricted from appearing in
				   generated file names. */

  int strict_comments;		/* whether strict SGML comments are
				   enforced.  */

  int preserve_perm;           /* whether remote permissions are used
				  or that what is set by umask. */

#ifdef ENABLE_IPV6
  int ipv4_only;		/* IPv4 connections have been requested. */
  int ipv6_only;		/* IPv4 connections have been requested. */
#endif
  enum {
    prefer_ipv4,
    prefer_ipv6,
    prefer_none
  } prefer_family;		/* preferred address family when more
				   than one type is available */
};

extern struct options opt;
