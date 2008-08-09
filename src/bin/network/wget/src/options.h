/* struct options.
   Copyright (C) 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
   2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

This file is part of GNU Wget.

GNU Wget is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

GNU Wget is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wget.  If not, see <http://www.gnu.org/licenses/>.

Additional permission under GNU GPL version 3 section 7

If you modify this program, or any covered work, by linking or
combining it with the OpenSSL project's OpenSSL library (or a
modified version of that library), containing parts covered by the
terms of the OpenSSL or SSLeay licenses, the Free Software Foundation
grants you additional permission to convey the resulting work.
Corresponding Source for a non-source form of such a combination
shall include the source code for the parts of OpenSSL used as well
as that of the covered work.  */

struct options
{
  int verbose;			/* Are we verbose?  (First set to -1,
                                   hence not boolean.) */
  bool quiet;			/* Are we quiet? */
  int ntry;			/* Number of tries per URL */
  bool retry_connrefused;	/* Treat CONNREFUSED as non-fatal. */
  bool background;		/* Whether we should work in background. */
  bool ignore_length;		/* Do we heed content-length at all?  */
  bool recursive;		/* Are we recursive? */
  bool spanhost;		/* Do we span across hosts in
				   recursion? */
  int  max_redirect;            /* Maximum number of times we'll allow
                                   a page to redirect. */
  bool relative_only;		/* Follow only relative links. */
  bool no_parent;		/* Restrict access to the parent
				   directory.  */
  int reclevel;			/* Maximum level of recursion */
  bool dirstruct;		/* Do we build the directory structure
				  as we go along? */
  bool no_dirstruct;		/* Do we hate dirstruct? */
  int cut_dirs;			/* Number of directory components to cut. */
  bool add_hostdir;		/* Do we add hostname directory? */
  bool protocol_directories;	/* Whether to prepend "http"/"ftp" to dirs. */
  bool noclobber;		/* Disables clobbering of existing
				   data. */
  char *dir_prefix;		/* The top of directory tree */
  char *lfilename;		/* Log filename */
  char *input_filename;		/* Input filename */
  bool force_html;		/* Is the input file an HTML file? */

  bool spider;			/* Is Wget in spider mode? */

  char **accepts;		/* List of patterns to accept. */
  char **rejects;		/* List of patterns to reject. */
  char **excludes;		/* List of excluded FTP directories. */
  char **includes;		/* List of FTP directories to
				   follow. */
  bool ignore_case;		/* Whether to ignore case when
				   matching dirs and files */

  char **domains;		/* See host.c */
  char **exclude_domains;
  bool dns_cache;		/* whether we cache DNS lookups. */

  char **follow_tags;           /* List of HTML tags to recursively follow. */
  char **ignore_tags;           /* List of HTML tags to ignore if recursing. */

  bool follow_ftp;		/* Are FTP URL-s followed in recursive
				   retrieving? */
  bool retr_symlinks;		/* Whether we retrieve symlinks in
				   FTP. */
  char *output_document;	/* The output file to which the
				   documents will be printed.  */

  char *user;			/* Generic username */
  char *passwd;			/* Generic password */
  
  bool always_rest;		/* Always use REST. */
  char *ftp_user;		/* FTP username */
  char *ftp_passwd;		/* FTP password */
  bool netrc;			/* Whether to read .netrc. */
  bool ftp_glob;		/* FTP globbing */
  bool ftp_pasv;			/* Passive FTP. */

  char *http_user;		/* HTTP username. */
  char *http_passwd;		/* HTTP password. */
  char **user_headers;		/* User-defined header(s). */
  bool http_keep_alive;		/* whether we use keep-alive */

  bool use_proxy;		/* Do we use proxy? */
  bool allow_cache;		/* Do we allow server-side caching? */
  char *http_proxy, *ftp_proxy, *https_proxy;
  char **no_proxy;
  char *base_href;
  char *progress_type;		/* progress indicator type. */
  char *proxy_user; /*oli*/
  char *proxy_passwd;

  double read_timeout;		/* The read/write timeout. */
  double dns_timeout;		/* The DNS timeout. */
  double connect_timeout;	/* The connect timeout. */

  bool random_wait;		/* vary from 0 .. wait secs by random()? */
  double wait;			/* The wait period between retrievals. */
  double waitretry;		/* The wait period between retries. - HEH */
  bool use_robots;		/* Do we heed robots.txt? */

  wgint limit_rate;		/* Limit the download rate to this
				   many bps. */
  SUM_SIZE_INT quota;		/* Maximum file size to download and
				   store. */

  int numurls;			/* Number of successfully downloaded
				   URLs #### should be removed because
				   it's not a setting, but a global var */

  bool server_response;		/* Do we print server response? */
  bool save_headers;		/* Do we save headers together with
				   file? */

#ifdef ENABLE_DEBUG
  bool debug;			/* Debugging on/off */
#endif

#ifdef MSDOS
  bool wdebug;                  /* Watt-32 tcp/ip debugging on/off */
#endif

  bool timestamping;		/* Whether to use time-stamping. */

  bool backup_converted;	/* Do we save pre-converted files as *.orig? */
  bool backups;			/* Are numeric backups made? */

  char *useragent;		/* User-Agent string, which can be set
				   to something other than Wget. */
  char *referer;		/* Naughty Referer, which can be
				   set to something other than
				   NULL. */
  bool convert_links;		/* Will the links be converted
				   locally? */
  bool remove_listing;		/* Do we remove .listing files
				   generated by FTP? */
  bool htmlify;			/* Do we HTML-ify the OS-dependent
				   listings? */

  char *dot_style;
  wgint dot_bytes;		/* How many bytes in a printing
				   dot. */
  int dots_in_line;		/* How many dots in one line. */
  int dot_spacing;		/* How many dots between spacings. */

  bool delete_after;		/* Whether the files will be deleted
				   after download. */

  bool html_extension;		/* Use ".html" extension on all text/html? */

  bool page_requisites;		/* Whether we need to download all files
				   necessary to display a page properly. */
  char *bind_address;		/* What local IP address to bind to. */

#ifdef HAVE_SSL
  enum {
    secure_protocol_auto,
    secure_protocol_sslv2,
    secure_protocol_sslv3,
    secure_protocol_tlsv1
  } secure_protocol;		/* type of secure protocol to use. */
  bool check_cert;		/* whether to validate the server's cert */
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

  bool cookies;			/* whether cookies are used. */
  char *cookies_input;		/* file we're loading the cookies from. */
  char *cookies_output;		/* file we're saving the cookies to. */
  bool keep_session_cookies;	/* whether session cookies should be
				   saved and loaded. */

  char *post_data;		/* POST query string */
  char *post_file_name;		/* File to post */

  enum {
    restrict_unix,
    restrict_windows
  } restrict_files_os;		/* file name restriction ruleset. */
  bool restrict_files_ctrl;	/* non-zero if control chars in URLs
				   are restricted from appearing in
				   generated file names. */
  enum {
    restrict_no_case_restriction,
    restrict_lowercase,
    restrict_uppercase
  } restrict_files_case;	/* file name case restriction. */

  bool strict_comments;		/* whether strict SGML comments are
				   enforced.  */

  bool preserve_perm;           /* whether remote permissions are used
				  or that what is set by umask. */

#ifdef ENABLE_IPV6
  bool ipv4_only;		/* IPv4 connections have been requested. */
  bool ipv6_only;		/* IPv4 connections have been requested. */
#endif
  enum {
    prefer_ipv4,
    prefer_ipv6,
    prefer_none
  } prefer_family;		/* preferred address family when more
				   than one type is available */
  
  bool content_disposition;	/* Honor HTTP Content-Disposition header. */
  bool auth_without_challenge;  /* Issue Basic authentication creds without
                                   waiting for a challenge. */
};

extern struct options opt;
