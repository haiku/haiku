#ifndef lint
char	nettest_id[]="\
@(#)nettest_bsd.c (c) Copyright 1993-2004 Hewlett-Packard Co. Version 2.4.3";
#endif /* lint */


/****************************************************************/
/*								*/
/*	nettest_bsd.c						*/
/*								*/
/*      the BSD sockets parsing routine...                      */
/*       ...with the addition of Windows NT, this is now also   */
/*          a Winsock test... sigh :)                           */
/*                                                              */
/*      scan_sockets_args()                                     */
/*                                                              */
/*	the actual test routines...				*/
/*								*/
/*	send_tcp_stream()	perform a tcp stream test	*/
/*	recv_tcp_stream()					*/
/*      send_tcp_maerts()       perform a tcp stream test       */
/*      recv_tcp_maerts()       in the other direction          */
/*	send_tcp_rr()		perform a tcp request/response	*/
/*	recv_tcp_rr()						*/
/*      send_tcp_conn_rr()      an RR test including connect    */
/*      recv_tcp_conn_rr()                                      */
/*      send_tcp_cc()           a connect/disconnect test with  */
/*      recv_tcp_cc()           no RR                           */
/*	send_udp_stream()	perform a udp stream test	*/
/*	recv_udp_stream()					*/
/*	send_udp_rr()		perform a udp request/response	*/
/*	recv_udp_rr()						*/
/*	loc_cpu_rate()		determine the local cpu maxrate */
/*	rem_cpu_rate()		find the remote cpu maxrate	*/
/*								*/
/****************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif
#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif
#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <fcntl.h>
#ifndef WIN32
#include <errno.h>
#include <signal.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef NOSTDLIBH
#include <malloc.h>
#endif /* NOSTDLIBH */

#ifndef WIN32
#if !defined(__VMS)
//#include <sys/ipc.h>
#endif /* !defined(__VMS) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#else /* WIN32 */
#include <process.h>
#define netperf_socklen_t socklen_t
#include <winsock2.h>

/* while it is unlikely that anyone running Windows 2000 or NT 4 is
   going to be trying to compile this, if they are they will want to
   define DONT_IPV6 in the sources file */
#ifndef DONT_IPV6
#include <ws2tcpip.h>
#endif
#include <windows.h>

#define sleep(x) Sleep((x)*1000)

#define __func__ __FUNCTION__
#endif /* WIN32 */

/* We don't want to use bare constants in the shutdown() call.  In the
   extremely unlikely event that SHUT_WR isn't defined, we will define
   it to the value we used to be passing to shutdown() anyway.  raj
   2007-02-08 */
#if !defined(SHUT_WR)
#define SHUT_WR 1
#endif

#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
# include "missing/getaddrinfo.h"
#endif

#include "netlib.h"
#include "netsh.h"
#include "nettest_bsd.h"

#if defined(WANT_HISTOGRAM) || defined(WANT_DEMO) 
#include "hist.h"
#endif /* WANT_HISTOGRAM */

/* make first_burst_size unconditional so we can use it easily enough
   when calculating transaction latency for the TCP_RR test. raj
   2007-06-08 */
int first_burst_size=0;

#if defined(HAVE_SENDFILE) && (defined(__linux) || defined(__sun__))
#include <sys/sendfile.h>
#endif /* HAVE_SENDFILE && (__linux || __sun__) */

#ifdef __HAIKU__
#undef AF_INET6
#endif



/* these variables are specific to the BSD sockets tests, but can
 * be used elsewhere if needed.  They are externed through nettest_bsd.h
 */

int	
  rss_size_req = -1,	/* requested remote socket send buffer size */
  rsr_size_req = -1,	/* requested remote socket recv buffer size */
  rss_size,		/* remote socket send buffer size	*/
  rsr_size,		/* remote socket recv buffer size	*/
  lss_size_req = -1,	/* requested local socket send buffer size */
  lsr_size_req = -1,	/* requested local socket recv buffer size */
  lss_size,		/* local  socket send buffer size 	*/
  lsr_size,		/* local  socket recv buffer size 	*/
  req_size = 1,		/* request size                   	*/
  rsp_size = 1,		/* response size			*/
  send_size,		/* how big are individual sends		*/
  recv_size;		/* how big are individual receives	*/

static  int confidence_iteration;
static  char  local_cpu_method;
static  char  remote_cpu_method;

/* these will control the width of port numbers we try to use in the */
/* TCP_CRR and/or TCP_TRR tests. raj 3/95 */
static int client_port_min = 5000;
static int client_port_max = 65535;

 /* different options for the sockets				*/

int
  loc_nodelay,		/* don't/do use NODELAY	locally		*/
  rem_nodelay,		/* don't/do use NODELAY remotely	*/
#ifdef TCP_CORK
  loc_tcpcork=0,        /* don't/do use TCP_CORK locally        */
  rem_tcpcork=0,        /* don't/do use TCP_CORK remotely       */
#endif /* TCP_CORK */
  loc_sndavoid,		/* avoid send copies locally		*/
  loc_rcvavoid,		/* avoid recv copies locally		*/
  rem_sndavoid,		/* avoid send copies remotely		*/
  rem_rcvavoid, 	/* avoid recv_copies remotely		*/
  local_connected = 0,  /* local socket type, connected/non-connected */
  remote_connected = 0; /* remote socket type, connected/non-connected */

#ifdef WANT_HISTOGRAM
#ifdef HAVE_GETHRTIME
static hrtime_t time_one;
static hrtime_t time_two;
#elif HAVE_GET_HRT
#include "hrt.h"
static hrt_t time_one;
static hrt_t time_two;
#elif defined(WIN32)
static LARGE_INTEGER time_one;
static LARGE_INTEGER time_two;
#else
static struct timeval time_one;
static struct timeval time_two;
#endif /* HAVE_GETHRTIME */
static HIST time_hist;
#endif /* WANT_HISTOGRAM */

#ifdef WANT_INTERVALS
int interval_count;
#ifndef WANT_SPIN
sigset_t signal_set;
#define INTERVALS_INIT() \
    if (interval_burst) { \
      /* zero means that we never pause, so we never should need the \
         interval timer. we used to use it for demo mode, but we deal \
	 with that with a variant on watching the clock rather than \
	 waiting for a timer. raj 2006-02-06 */ \
      start_itimer(interval_wate); \
    } \
    interval_count = interval_burst; \
    /* get the signal set for the call to sigsuspend */ \
    if (sigprocmask(SIG_BLOCK, (sigset_t *)NULL, &signal_set) != 0) { \
      fprintf(where, \
	      "%s: unable to get sigmask errno %d\n", \
	      __func__, \
	      errno); \
      fflush(where); \
      exit(1); \
    }

#define INTERVALS_WAIT() \
      /* in this case, the interval count is the count-down couter \
	 to decide to sleep for a little bit */ \
      if ((interval_burst) && (--interval_count == 0)) { \
	/* call sigsuspend and wait for the interval timer to get us \
	   out */ \
	if (debug > 1) { \
	  fprintf(where,"about to suspend\n"); \
	  fflush(where); \
	} \
	if (sigsuspend(&signal_set) == EFAULT) { \
	  fprintf(where, \
		  "%s: fault with sigsuspend.\n", \
                  __func__); \
	  fflush(where); \
	  exit(1); \
	} \
	interval_count = interval_burst; \
      }
#else
/* first out timestamp */
#ifdef HAVE_GETHRTIME
static hrtime_t intvl_one;
static hrtime_t intvl_two;
static hrtime_t *intvl_one_ptr = &intvl_one;
static hrtime_t *intvl_two_ptr = &intvl_two;
static hrtime_t *temp_intvl_ptr = &intvl_one;
#elif defined(WIN32)
static LARGE_INTEGER intvl_one;
static LARGE_INTEGER intvl_two;
static LARGE_INTEGER *intvl_one_ptr = &intvl_one;
static LARGE_INTEGER *intvl_two_ptr = &intvl_two;
static LARGE_INTEGER *temp_intvl_ptr = &intvl_one;
#else
static struct timeval intvl_one;
static struct timeval intvl_two;
static struct timeval *intvl_one_ptr = &intvl_one;
static struct timeval *intvl_two_ptr = &intvl_two;
static struct timeval *temp_intvl_ptr = &intvl_one;
#endif

#define INTERVALS_INIT() \
      if (interval_burst) { \
	HIST_timestamp(intvl_one_ptr); \
      } \
      interval_count = interval_burst; \

#define INTERVALS_WAIT() \
      /* in this case, the interval count is the count-down couter \
	 to decide to sleep for a little bit */ \
      if ((interval_burst) && (--interval_count == 0)) { \
	/* call sigsuspend and wait for the interval timer to get us \
	   out */ \
	if (debug > 1) { \
	  fprintf(where,"about to spin suspend\n"); \
	  fflush(where); \
	} \
        HIST_timestamp(intvl_two_ptr); \
        while(delta_micro(intvl_one_ptr,intvl_two_ptr) < interval_usecs) { \
	  HIST_timestamp(intvl_two_ptr); \
	} \
	temp_intvl_ptr = intvl_one_ptr; \
	intvl_one_ptr = intvl_two_ptr; \
	intvl_two_ptr = temp_intvl_ptr; \
	interval_count = interval_burst; \
      }
#endif
#endif

#ifdef WANT_DEMO
#ifdef HAVE_GETHRTIME
static hrtime_t demo_one;
static hrtime_t demo_two;
static hrtime_t *demo_one_ptr = &demo_one;
static hrtime_t *demo_two_ptr = &demo_two;
static hrtime_t *temp_demo_ptr = &demo_one;
#elif defined(WIN32)
static LARGE_INTEGER demo_one;
static LARGE_INTEGER demo_two;
static LARGE_INTEGER *demo_one_ptr = &demo_one;
static LARGE_INTEGER *demo_two_ptr = &demo_two;
static LARGE_INTEGER *temp_demo_ptr = &demo_one;
#else
static struct timeval demo_one;
static struct timeval demo_two;
static struct timeval *demo_one_ptr = &demo_one;
static struct timeval *demo_two_ptr = &demo_two;
static struct timeval *temp_demo_ptr = &demo_one;
#endif 

/* for a _STREAM test, "a" should be lss_size and "b" should be
   rsr_size. for a _MAERTS test, "a" should be lsr_size and "b" should
   be rss_size. raj 2005-04-06 */
#define DEMO_STREAM_SETUP(a,b) \
    if ((demo_mode) && (demo_units == 0)) { \
      /* take our default value of demo_units to be the larger of \
	 twice the remote's SO_RCVBUF or twice our SO_SNDBUF */ \
      if (a > b) { \
	demo_units = 2*a; \
      } \
      else { \
	demo_units = 2*b; \
      } \
    }

#define DEMO_STREAM_INTERVAL(units) \
      if (demo_mode) { \
	double actual_interval; \
	units_this_tick += units; \
	if (units_this_tick >= demo_units) { \
	  /* time to possibly update demo_units and maybe output an \
	     interim result */ \
	  HIST_timestamp(demo_two_ptr); \
	  actual_interval = delta_micro(demo_one_ptr,demo_two_ptr); \
	  /* we always want to fine-tune demo_units here whether we \
	     emit an interim result or not.  if we are short, this \
	     will lengthen demo_units.  if we are long, this will \
	     shorten it */ \
	  demo_units = demo_units * (demo_interval / actual_interval); \
	  if (actual_interval >= demo_interval) { \
	    /* time to emit an interim result */ \
	    fprintf(where, \
		    "Interim result: %7.2f %s/s over %.2f seconds\n", \
		    calc_thruput_interval(units_this_tick, \
					  actual_interval/1000000.0), \
		    format_units(), \
		    actual_interval/1000000.0); \
            fflush(where); \
	    units_this_tick = 0.0; \
	    /* now get a new starting timestamp.  we could be clever \
	       and swap pointers - the math we do probably does not \
	       take all that long, but for now this will suffice */ \
	    temp_demo_ptr = demo_one_ptr; \
	    demo_one_ptr = demo_two_ptr; \
	    demo_two_ptr = temp_demo_ptr; \
	  } \
	} \
      }

#define DEMO_RR_SETUP(a) \
    if ((demo_mode) && (demo_units == 0)) { \
      /* take whatever we are given */ \
	demo_units = a; \
    }

#define DEMO_RR_INTERVAL(units) \
      if (demo_mode) { \
	double actual_interval; \
	units_this_tick += units; \
	if (units_this_tick >= demo_units) { \
	  /* time to possibly update demo_units and maybe output an \
	     interim result */ \
	  HIST_timestamp(demo_two_ptr); \
	  actual_interval = delta_micro(demo_one_ptr,demo_two_ptr); \
	  /* we always want to fine-tune demo_units here whether we \
	     emit an interim result or not.  if we are short, this \
	     will lengthen demo_units.  if we are long, this will \
	     shorten it */ \
	  demo_units = demo_units * (demo_interval / actual_interval); \
	  if (actual_interval >= demo_interval) { \
	    /* time to emit an interim result */ \
	    fprintf(where, \
		    "Interim result: %.2f %s/s over %.2f seconds\n", \
                    units_this_tick / (actual_interval/1000000.0), \
		    "Trans", \
		    actual_interval/1000000.0); \
	    units_this_tick = 0.0; \
	    /* now get a new starting timestamp.  we could be clever \
	       and swap pointers - the math we do probably does not \
	       take all that long, but for now this will suffice */ \
	    temp_demo_ptr = demo_one_ptr; \
	    demo_one_ptr = demo_two_ptr; \
	    demo_two_ptr = temp_demo_ptr; \
	  } \
	} \
      }
#endif 

char sockets_usage[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
TCP/UDP BSD Sockets Test Options:\n\
    -b number         Send number requests at start of _RR tests\n\
    -C                Set TCP_CORK when available\n\
    -D [L][,R]        Set TCP_NODELAY locally and/or remotely (TCP_*)\n\
    -h                Display this text\n\
    -H name,fam       Use name (or IP) and family as target of data connection\n\
    -L name,fam       Use name (or IP) and family as source of data connection\n\
    -m bytes          Set the send size (TCP_STREAM, UDP_STREAM)\n\
    -M bytes          Set the recv size (TCP_STREAM, UDP_STREAM)\n\
    -n                Use the connected socket for UDP locally\n\
    -N                Use the connected socket for UDP remotely\n\
    -p min[,max]      Set the min/max port numbers for TCP_CRR, TCP_TRR\n\
    -P local[,remote] Set the local/remote port for the data socket\n\
    -r req,[rsp]      Set request/response sizes (TCP_RR, UDP_RR)\n\
    -s send[,recv]    Set local socket send/recv buffer sizes\n\
    -S send[,recv]    Set remote socket send/recv buffer sizes\n\
    -4                Use AF_INET (eg IPv4) on both ends of the data conn\n\
    -6                Use AF_INET6 (eg IPv6) on both ends of the data conn\n\
\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n"; 



/* these routines convert between the AF address space and the NF
   address space since the numeric values of AF_mumble are not the
   same across the platforms. raj 2005-02-08 */

int
nf_to_af(int nf) {
  switch(nf) {
  case NF_INET:
    return AF_INET;
    break;
  case NF_UNSPEC:
    return AF_UNSPEC;
    break;
  case NF_INET6:
#if defined(AF_INET6)
    return AF_INET6;
#else
    return AF_UNSPEC;
#endif
    break;
  default:
    return AF_UNSPEC;
    break;
  }
}

int
af_to_nf(int af) {

  switch(af) {
  case AF_INET:
    return NF_INET;
    break;
  case AF_UNSPEC:
    return NF_UNSPEC;
    break;
#if defined(AF_INET6)
  case AF_INET6:
    return NF_INET6;
    break;
#endif
  default:
    return NF_UNSPEC;
    break;
  }
}     


 /* This routine is intended to retrieve interesting aspects of tcp */
 /* for the data connection. at first, it attempts to retrieve the */
 /* maximum segment size. later, it might be modified to retrieve */
 /* other information, but it must be information that can be */
 /* retrieved quickly as it is called during the timing of the test. */
 /* for that reason, a second routine may be created that can be */
 /* called outside of the timing loop */
static
void
get_tcp_info(SOCKET socket, int *mss)
{

#ifdef TCP_MAXSEG
  netperf_socklen_t sock_opt_len;

  sock_opt_len = sizeof(netperf_socklen_t);
  if (getsockopt(socket,
		 getprotobyname("tcp")->p_proto,	
		 TCP_MAXSEG,
		 (char *)mss,
		 &sock_opt_len) == SOCKET_ERROR) {
    fprintf(where,
	    "netperf: get_tcp_info: getsockopt TCP_MAXSEG: errno %d\n",
	    errno);
    fflush(where);
    *mss = -1;
  }
#else
  *mss = -1;
#endif /* TCP_MAXSEG */
}


/* return a pointer to a completed addrinfo chain - prefer
   data_address to controlhost and utilize the specified address
   family */

struct addrinfo *
complete_addrinfo(char *controlhost, char *data_address, char *port, int family, int type, int protocol, int flags) 
{
  struct addrinfo hints;
  struct addrinfo *res;
  struct addrinfo *temp_res;

#define CHANGED_SOCK_TYPE  0x1
#define CHANGED_PROTOCOL   0x2
#define CHANGED_SCTP       0x4
  int    change_info = 0;
  static int change_warning_displayed = 0;

  int count = 0;
  int error = 0;

  char *hostname;

  /* take data-address over controlhost */
  if (data_address)
    hostname = data_address;
  else
    hostname = controlhost;

  if (debug) {
    fprintf(where,
	    "complete_addrinfo using hostname %s port %s family %s type %s prot %s flags 0x%x\n",
	    hostname,
	    port,
	    inet_ftos(family),
	    inet_ttos(type),
	    inet_ptos(protocol),
	    flags);
    fflush(where);
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = protocol;
  hints.ai_flags = flags|AI_CANONNAME;

  count = 0;
  do {
    error = getaddrinfo((char *)hostname,
                        (char *)port,
                        &hints,
                        &res);
    count += 1;
    if (error == EAI_AGAIN) {
      if (debug) {
        fprintf(where,"Sleeping on getaddrinfo EAI_AGAIN\n");
        fflush(where);
      }
      sleep(1);
    }
    /* while you see this kludge first, it is actually the second, the
       first being the one for Solaris below. The need for this kludge
       came after implementing the Solaris broken getaddrinfo kludge -
       now we see a kludge in Linux getaddrinfo where if it is given
       SOCK_STREAM and IPPROTO_SCTP it barfs with a -7
       EAI_SOCKTYPE. so, we check if the error was EAI_SOCKTYPE and if
       we were asking for IPPROTO_SCTP and if so, kludge, again... raj
       2008-10-13 */
#ifdef WANT_SCTP
    if (EAI_SOCKTYPE == error
#ifdef EAI_BADHINTS
        || EAI_BADHINTS == error
#endif
        ) {
      /* we ass-u-me this is the Linux getaddrinfo bug, clear the
	 hints.ai_protocol field, and set some state "remembering"
	 that we did this so the code for the Solaris kludge can do
	 the fix-up for us.  also flip error over to EAI_AGAIN and
	 make sure we don't "count" this time around the loop. */
      hints.ai_protocol = 0;
      error = EAI_AGAIN;
      count -= 1;
      change_info |= CHANGED_SCTP;
    }
#endif
  } while ((error == EAI_AGAIN) && (count <= 5));

  if (error) {
    fprintf(where,
	    "complete_addrinfo: could not resolve '%s' port '%s' af %d",
	    hostname,
	    port,
	    family);
    fprintf(where,
	    "\n\tgetaddrinfo returned %d %s\n",
	    error,
	    gai_strerror(error));
    fflush(where);
    exit(-1);
  }

  /* there exists at least one platform - Solaris 10 - that does not
     seem to completely honor the ai_protocol and/or ai_socktype one
     sets in the hints parm to the getaddrinfo call.  so, we need to
     walk the list of entries returned and if either of those do not
     match what we asked for, we need to go ahead and set them
     "correctly" this is based in part on some earlier SCTP-only code
     from previous revisions.  raj 2006-10-09 */

  temp_res = res;

  while (temp_res) {

    if ((type)  &&
	(temp_res->ai_socktype != type)) {
      change_info |= CHANGED_SOCK_TYPE;
      if (debug) {
	fprintf(where,
		"WARNING! Changed bogus getaddrinfo socket type %d to %d\n",
		temp_res->ai_socktype,
		type);
	fflush(where);
      }
      temp_res->ai_socktype = type;
    }

    if ((protocol) &&
	(temp_res->ai_protocol != protocol)) {
      change_info |= CHANGED_PROTOCOL;
      if (debug) {
	fprintf(where,
		"WARNING! Changed bogus getaddrinfo protocol %d to %d\n",
		temp_res->ai_protocol,
		protocol);
	fflush(where);
      }
      temp_res->ai_protocol = protocol;
    }
    temp_res = temp_res->ai_next;
  }
	
  if ((change_info & CHANGED_SOCK_TYPE) &&
      !(change_warning_displayed & CHANGED_SOCK_TYPE)) {
    change_warning_displayed |= CHANGED_SOCK_TYPE;
    fprintf(where,
	    "WARNING! getaddrinfo returned a socket type which did not\n");
    fprintf(where,
	    "match the requested type.  Please contact your vendor for\n");
    fprintf(where,
	    "a fix to this bug in getaddrinfo()\n");
    fflush(where);
  }

  /* if we dropped the protocol hint, it would be for a protocol that
     getaddrinfo() wasn't supporting yet, not for the bug that it took
     our hint and still returned zero. raj 2006-10-16 */
  if ((change_info & CHANGED_PROTOCOL) &&
      !(change_warning_displayed & CHANGED_PROTOCOL) &&
      (hints.ai_protocol != 0)) {
    change_warning_displayed |= CHANGED_PROTOCOL;
    fprintf(where,
	    "WARNING! getaddrinfo returned a protocol other than the\n");
    fprintf(where,
	    "requested protocol.  Please contact your vendor for\n");
    fprintf(where,
	    "a fix to this bug in getaddrinfo()\n");
    fflush(where);
  }

  if ((change_info & CHANGED_SCTP) &&
      !(change_warning_displayed & CHANGED_SCTP)) {
    change_warning_displayed |= CHANGED_SCTP;
    fprintf(where,
	    "WARNING! getaddrinfo on this platform does not accept IPPROTO_SCTP!\n");
    fprintf(where,
	    "Please contact your vendor for a fix to this bug in getaddrinfo().\n");
    fflush(where);
  }


  if (debug) {
    dump_addrinfo(where, res, hostname, port, family);
  }

  return(res);
}

void
complete_addrinfos(struct addrinfo **remote,struct addrinfo **local, char remote_host[], int type, int protocol, int flags) {

  *remote = complete_addrinfo(remote_host,
			      remote_data_address,
			      remote_data_port,
			      remote_data_family,
			      type,
			      protocol,
			      flags);

  /* OK, if the user has not specified a local data endpoint address
     (test-specific -L), pick the local data endpoint address based on
     the remote data family info (test-specific -H or -4 or -6
     option).  if the user has not specified remote data addressing
     info (test-specific -H, -4 -6) pick something based on the local
     control connection address (ie the global -L option). */

  if (NULL == local_data_address) {
    local_data_address = malloc(HOSTNAMESIZE);
    if (NULL == remote_data_address) {
      if (debug) {
	fprintf(where,
		"local_data_address not set, using local_host_name of '%s'\n",
		local_host_name);
	fflush(where);
      }
      strcpy(local_data_address,local_host_name);
    }
    else {
      if (debug) {
	fprintf(where,
		"local_data_address not set, using address family info\n");
	fflush(where);
      }
      /* by default, use 0.0.0.0 - assume IPv4 */
      strcpy(local_data_address,"0.0.0.0");
#if defined(AF_INET6)
      if ((AF_INET6 == local_data_family) ||
	  ((AF_UNSPEC == local_data_family) &&
	   (AF_INET6 == remote_data_family)) ||
	  ((AF_UNSPEC == local_data_family) && 
	   (AF_INET6 == (*remote)->ai_family))) {
	strcpy(local_data_address,"::0");
      }
#endif
    }
  }

  *local = complete_addrinfo("what to put here?",
			     local_data_address,
			     local_data_port,
			     local_data_family,
			     type,
			     protocol,
			     flags|AI_PASSIVE);

}

void
set_hostname_and_port(char *hostname, char *portstr, int family, int port)
{
  strcpy(hostname,"0.0.0.0");
#if defined AF_INET6
  if (AF_INET6 == family) {
    strcpy(hostname,"::0");
  }
#endif
    
  sprintf(portstr, "%u", port);

}

static unsigned short
get_port_number(struct addrinfo *res) 
{
 switch(res->ai_family) {
  case AF_INET: {
    struct sockaddr_in *foo = (struct sockaddr_in *)res->ai_addr;
    return(ntohs(foo->sin_port));
    break;
  }
#if defined(AF_INET6)
  case AF_INET6: {
    struct sockaddr_in6 *foo = (struct sockaddr_in6 *)res->ai_addr;
    return(ntohs(foo->sin6_port));
    break;
  }
#endif
  default:
    fprintf(where,
	    "Unexpected Address Family of %u\n",res->ai_family);
    fflush(where);
    exit(-1);
  }
}

/* this routine will set the port number of the sockaddr in the
   addrinfo to the specified value, based on the address family */
void
set_port_number(struct addrinfo *res, unsigned short port)
{
  switch(res->ai_family) {
  case AF_INET: {
    struct sockaddr_in *foo = (struct sockaddr_in *)res->ai_addr;
    foo->sin_port = htons(port);
    break;
  }
#if defined(AF_INET6)
  case AF_INET6: {
    struct sockaddr_in6 *foo = (struct sockaddr_in6 *)res->ai_addr;
    foo->sin6_port = htons(port);
    break;
  }
#endif
  default:
    fprintf(where,
	    "Unexpected Address Family of %u\n",res->ai_family);
    fflush(where);
    exit(-1);
  }
}



 /* This routine will create a data (listen) socket with the
  apropriate options set and return it to the caller. this replaces
  all the duplicate code in each of the test routines and should help
  make things a little easier to understand. since this routine can be
  called by either the netperf or netserver programs, all output
  should be directed towards "where." family is generally AF_INET and
  type will be either SOCK_STREAM or SOCK_DGRAM.  This routine will
  also be used by the "SCTP" tests, hence the slightly strange-looking
  SCTP stuff in the classic bsd sockets test file... vlad/raj
  2005-03-15 */

SOCKET
create_data_socket(struct addrinfo *res)
{

  SOCKET temp_socket;
  int one;
  int    on  = 1;
  

  /*set up the data socket                        */
  temp_socket = socket(res->ai_family,
		       res->ai_socktype,
		       res->ai_protocol);
  
  if (temp_socket == INVALID_SOCKET){
    fprintf(where,
	    "netperf: create_data_socket: socket: errno %d fam %s type %s prot %s errmsg %s\n",
	    errno,
	    inet_ftos(res->ai_family),
	    inet_ttos(res->ai_socktype),
	    inet_ptos(res->ai_protocol),
	    strerror(errno));
    fflush(where);
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"create_data_socket: socket %d obtained...\n",temp_socket);
    fflush(where);
  }
  
  /* Modify the local socket size. The reason we alter the send buffer
   size here rather than when the connection is made is to take care
   of decreases in buffer size. Decreasing the window size after
   connection establishment is a TCP no-no. Also, by setting the
   buffer (window) size before the connection is established, we can
   control the TCP MSS (segment size). The MSS is never (well, should
   never be) more that 1/2 the minimum receive buffer size at each
   half of the connection.  This is why we are altering the receive
   buffer size on the sending size of a unidirectional transfer. If
   the user has not requested that the socket buffers be altered, we
   will try to find-out what their values are. If we cannot touch the
   socket buffer in any way, we will set the values to -1 to indicate
   that.  */
  
  /* all the oogy nitty gritty stuff moved from here into the routine
     being called below, per patches from davidm to workaround the bug
     in Linux getsockopt().  raj 2004-06-15 */
  set_sock_buffer (temp_socket, SEND_BUFFER, lss_size_req, &lss_size);
  set_sock_buffer (temp_socket, RECV_BUFFER, lsr_size_req, &lsr_size);

  /* now, we may wish to enable the copy avoidance features on the */
  /* local system. of course, this may not be possible... */
  
#ifdef SO_RCV_COPYAVOID
  if (loc_rcvavoid) {
    if (setsockopt(temp_socket,
		   SOL_SOCKET,
		   SO_RCV_COPYAVOID,
		   (const char *)&loc_rcvavoid,
		   sizeof(int)) == SOCKET_ERROR) {
      fprintf(where,
	      "netperf: create_data_socket: Could not enable receive copy avoidance");
      fflush(where);
      loc_rcvavoid = 0;
    }
  }
#else
  /* it wasn't compiled in... */
  loc_rcvavoid = 0;
#endif /* SO_RCV_COPYAVOID */

#ifdef SO_SND_COPYAVOID
  if (loc_sndavoid) {
    if (setsockopt(temp_socket,
		   SOL_SOCKET,
		   SO_SND_COPYAVOID,
		   (const char *)&loc_sndavoid,
		   sizeof(int)) == SOCKET_ERROR) {
      fprintf(where,
	      "netperf: create_data_socket: Could not enable send copy avoidance");
      fflush(where);
      loc_sndavoid = 0;
    }
  }
#else
  /* it was not compiled in... */
  loc_sndavoid = 0;
#endif
  
  /* Now, we will see about setting the TCP_NODELAY flag on the local */
  /* socket. We will only do this for those systems that actually */
  /* support the option. If it fails, note the fact, but keep going. */
  /* If the user tries to enable TCP_NODELAY on a UDP socket, this */
  /* will cause an error to be displayed */

  /* well..... long ago and far away that would have happened, in
     particular because we would always use IPPROTO_TCP here.
     however, now we are using res->ai_protocol, which will be
     IPPROT_UDP, and while HP-UX, and I suspect no-one else on the
     planet has a UDP_mumble option that overlaps with TCP_NODELAY,
     sure as knuth made little green programs, linux has a UDP_CORK
     option that is defined as a value of 1, which is the same a
     TCP_NODELAY under Linux.  So, when asking for -D and
     "TCP_NODELAY" under Linux, we are actually setting UDP_CORK
     instead of getting an error like every other OS on the
     planet. joy and rupture. this stops a UDP_RR test cold sooo we
     have to make sure that res->ai_protocol actually makes sense for
     a _NODELAY setsockopt() or a UDP_RR test on Linux where someone
     mistakenly sets -D will hang.  raj 2005-04-21 */
  
#if defined(TCP_NODELAY) || defined(SCTP_NODELAY)
  if ((loc_nodelay) && (res->ai_protocol != IPPROTO_UDP)) {

    /* strictly speaking, since the if defined above is an OR, we
       should probably check against TCP_NODELAY being defined here.
       however, the likelihood of SCTP_NODELAY being defined and
       TCP_NODELAY _NOT_ being defined is, probably :), epsilon.  raj
       2005-03-15 */

    int option = TCP_NODELAY;

    /* I suspect that WANT_SCTP would suffice here since that is the
       only time we would have called getaddrinfo with a hints asking
       for SCTP, but just in case there is an SCTP implementation out
       there _without_ SCTP_NODELAY... raj 2005-03-15 */ 

#if defined(WANT_SCTP) && defined(SCTP_NODELAY)
    if (IPPROTO_SCTP == res->ai_protocol) {
      option = SCTP_NODELAY;
    }
#endif

    one = 1;
    if(setsockopt(temp_socket,
		  res->ai_protocol,
		  option,
		  (char *)&one,
		  sizeof(one)) == SOCKET_ERROR) {
      fprintf(where,
	      "netperf: create_data_socket: nodelay: errno %d\n",
	      errno);
      fflush(where);
    }
    
    if (debug > 1) {
      fprintf(where,
	      "netperf: create_data_socket: [TCP|SCTP]_NODELAY requested...\n");
      fflush(where);
    }
  }
#else /* TCP_NODELAY */
  
  loc_nodelay = 0;
  
#endif /* TCP_NODELAY */

#if defined(TCP_CORK)
    
    if (loc_tcpcork != 0) {
      /* the user wishes for us to set TCP_CORK on the socket */
      int one = 1;
      if (setsockopt(temp_socket,
		     getprotobyname("tcp")->p_proto,
		     TCP_CORK,
		     (char *)&one,
		     sizeof(one)) == SOCKET_ERROR) {
	perror("netperf: sendfile_tcp_stream: tcp_cork");
	exit(1);
      }
      if (debug) {
	fprintf(where,"sendfile_tcp_stream: tcp_cork...\n");
      }
    }
    
#endif /* TCP_CORK */    

  /* since some of the UDP tests do not do anything to cause an
     implicit bind() call, we need to be rather explicit about our
     bind() call here. even if the address and/or the port are zero
     (INADDR_ANY etc). raj 2004-07-20 */

  if (setsockopt(temp_socket,
		 SOL_SOCKET,
		 SO_REUSEADDR,
		 (const char *)&on,
		 sizeof(on)) < 0) {
    fprintf(where,
	    "netperf: create_data_socket: SO_REUSEADDR failed %d\n",
	    errno);
    fflush(where);
  }
  
  if (bind(temp_socket,
	   res->ai_addr,
	   res->ai_addrlen) < 0) {
    if (debug) {
      fprintf(where,
	      "netperf: create_data_socket: data socket bind failed errno %d\n",
	      errno);
      fprintf(where," port: %d\n",get_port_number(res));
      fflush(where);
    }
  }
  

  return(temp_socket);

}

#ifdef KLUDGE_SOCKET_OPTIONS


 /* This routine is for those BROKEN systems which do not correctly */
 /* pass socket attributes through calls such as accept(). It should */
 /* only be called for those broken systems. I *really* don't want to */
 /* have this, but even broken systems must be measured. raj 11/95 */
void
kludge_socket_options(int temp_socket)
{

  set_sock_buffer(temp_socket, SEND_BUFFER, lss_size_req, &lss_size);
  set_sock_buffer(temp_socket, RECV_BUFFER, lsr_size_req, &lsr_size);

  /* now, we may wish to enable the copy avoidance features on the */
  /* local system. of course, this may not be possible... */
  /* those calls were only valid for HP-UX, and I know that HP-UX is */
  /* written correctly, and so we do not need to include those calls */
  /* in this kludgy routine. raj 11/95 */

  
  /* Now, we will see about setting the TCP_NODELAY flag on the local */
  /* socket. We will only do this for those systems that actually */
  /* support the option. If it fails, note the fact, but keep going. */
  /* If the user tries to enable TCP_NODELAY on a UDP socket, this */
  /* will cause an error to be displayed */
  
#ifdef TCP_NODELAY
  if (loc_nodelay) {
    one = 1;
    if(setsockopt(temp_socket,
		  getprotobyname("tcp")->p_proto,
		  TCP_NODELAY,
		  (char *)&one,
		  sizeof(one)) == SOCKET_ERROR) {
      fprintf(where,"netperf: kludge_socket_options: nodelay: errno %d\n",
	      errno);
      fflush(where);
    }
    
    if (debug > 1) {
      fprintf(where,
	      "netperf: kludge_socket_options: TCP_NODELAY requested...\n");
      fflush(where);
    }
  }
#else /* TCP_NODELAY */
  
  loc_nodelay = 0;
  
#endif /* TCP_NODELAY */

  }

#endif /* KLUDGE_SOCKET_OPTIONS */


static void *
get_address_address(struct addrinfo *info) 
{
  struct sockaddr_in *sin;
#if defined(AF_INET6)
  struct sockaddr_in6 *sin6;
#endif

  switch(info->ai_family) {
  case AF_INET:
    sin = (struct sockaddr_in *)info->ai_addr;
    return(&(sin->sin_addr));
    break;
#if defined(AF_INET6)
  case AF_INET6:
    sin6 = (struct sockaddr_in6 *)info->ai_addr;
    return(&(sin6->sin6_addr));
    break;
#endif
  default:
    fprintf(stderr,"we never expected to get here in get_address_address\n");
    fflush(stderr);
    exit(-1);
  }
}

#if defined(WIN32) 
/* +*+ Why isn't this in the winsock headers yet? */
const char *
inet_ntop(int af, const void *src, char *dst, size_t size);
#endif

/* This routine is a generic test header printer for the topmost header */
void
print_top_test_header(char test_name[], struct addrinfo *source, struct addrinfo *destination)
{

#if defined(AF_INET6)
  char address_buf[INET6_ADDRSTRLEN];
#else
  char address_buf[16]; /* magic constant */
#endif

  /* we want to have some additional, interesting information in */
  /* the headers. we know some of it here, but not all, so we will */
  /* only print the test title here and will print the results */
  /* titles after the test is finished */
  fprintf(where,test_name);
  address_buf[0] = '\0';
  inet_ntop(source->ai_family,get_address_address(source),address_buf,sizeof(address_buf));
  fprintf(where,
	  " from %s (%s) port %u %s",
	  source->ai_canonname,
	  address_buf,
	  get_port_number(source),
	  inet_ftos(source->ai_family));
  address_buf[0] = '\0';
  inet_ntop(destination->ai_family,get_address_address(destination),address_buf,sizeof(address_buf));
  fprintf(where,
	  " to %s (%s) port %u %s",
	  destination->ai_canonname,
	  address_buf,
	  get_port_number(destination),
	  inet_ftos(destination->ai_family));

  if (iteration_max > 1) {
    fprintf(where,
	    " : +/-%3.1f%% @ %2d%% conf. %s",
	    interval/0.02,
	    confidence_level,
	    result_confidence_only ? " on result only" : "");
  }
  if ((loc_nodelay > 0) || (rem_nodelay > 0)) {
    fprintf(where," : nodelay");
  }
  if ((loc_sndavoid > 0) || 
      (loc_rcvavoid > 0) ||
      (rem_sndavoid > 0) ||
      (rem_rcvavoid > 0)) {
    fprintf(where," : copy avoidance");
  }

  if (no_control) {
    fprintf(where," : no control");
  }

#ifdef WANT_HISTOGRAM
  fprintf(where," : histogram");
#endif /* WANT_HISTOGRAM */

#ifdef WANT_INTERVALS
#ifndef WANT_SPIN
  fprintf(where," : interval");
#else
  fprintf(where," : spin interval");
#endif
#endif /* WANT_INTERVALS */

#ifdef DIRTY 
  fprintf(where," : dirty data");
#endif /* DIRTY */
#ifdef WANT_DEMO
  fprintf(where," : demo");
#endif
#ifdef WANT_FIRST_BURST
  /* a little hokey perhaps, but we really only want this to be
     emitted for tests where it actually is used, which means a
     "REQUEST/RESPONSE" test. raj 2005-11-10 */
  if (strstr(test_name,"REQUEST/RESPONSE")) {
    fprintf(where," : first burst %d",first_burst_size);
  }
#endif
  if (cpu_binding_requested) {
    fprintf(where," : cpu bind");
  }
  fprintf(where,"\n");
  
}


/* This routine implements the TCP unidirectional data transfer test */
/* (a.k.a. stream) for the sockets interface. It receives its */
/* parameters via global variables from the shell and writes its */
/* output to the standard output. */


void 
send_tcp_stream(char remote_host[])
{
  
  char *tput_title = "\
Recv   Send    Send                          \n\
Socket Socket  Message  Elapsed              \n\
Size   Size    Size     Time     Throughput  \n\
bytes  bytes   bytes    secs.    %s/sec  \n\n";
  
  char *tput_fmt_0 =
    "%7.2f %s\n";
  
  char *tput_fmt_1 =
    "%6d %6d %6d    %-6.2f   %7.2f   %s\n";
  
  char *cpu_title = "\
Recv   Send    Send                          Utilization       Service Demand\n\
Socket Socket  Message  Elapsed              Send     Recv     Send    Recv\n\
Size   Size    Size     Time     Throughput  local    remote   local   remote\n\
bytes  bytes   bytes    secs.    %-8.8s/s  %% %c      %% %c      us/KB   us/KB\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f %c %s\n";

  char *cpu_fmt_1 =
    "%6d %6d %6d    %-6.2f     %7.2f   %-6.2f   %-6.2f   %-6.3f  %-6.3f %s\n";
  
  char *ksink_fmt = "\n\
Alignment      Offset         %-8.8s %-8.8s    Sends   %-8.8s Recvs\n\
Local  Remote  Local  Remote  Xfered   Per                 Per\n\
Send   Recv    Send   Recv             Send (avg)          Recv (avg)\n\
%5d   %5d  %5d   %5d %6.4g  %6.2f    %6d   %6.2f %6d\n";

  char *ksink_fmt2 = "\n\
Maximum\n\
Segment\n\
Size (bytes)\n\
%6d\n";
  
  
  float			elapsed_time;
  
  /* what we want is to have a buffer space that is at least one */
  /* send-size greater than our send window. this will insure that we */
  /* are never trying to re-use a buffer that may still be in the hands */
  /* of the transport. This buffer will be malloc'd after we have found */
  /* the size of the local senc socket buffer. We will want to deal */
  /* with alignment and offset concerns as well. */
  
  struct ring_elt *send_ring;
  
  int len;
  unsigned int nummessages = 0;
  SOCKET send_socket;
  int bytes_remaining;
  int tcp_mss = -1;  /* possibly uninitialized on printf far below */

  /* with links like fddi, one can send > 32 bits worth of bytes */
  /* during a test... ;-) at some point, this should probably become a */
  /* 64bit integral type, but those are not entirely common yet */

  unsigned long long local_bytes_sent = 0;
  double	bytes_sent = 0.0;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;

  double	thruput;
  
  struct addrinfo *remote_res;
  struct addrinfo *local_res;
  
  struct	tcp_stream_request_struct	*tcp_stream_request;
  struct	tcp_stream_response_struct	*tcp_stream_response;
  struct	tcp_stream_results_struct	*tcp_stream_result;
  
  tcp_stream_request  = 
    (struct tcp_stream_request_struct *)netperf_request.content.test_specific_data;
  tcp_stream_response =
    (struct tcp_stream_response_struct *)netperf_response.content.test_specific_data;
  tcp_stream_result   = 
    (struct tcp_stream_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  /* complete_addrinfos will either succede or exit the process */
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);
  
  if ( print_headers ) {
    print_top_test_header("TCP STREAM TEST",local_res,remote_res);
  }

  send_ring = NULL;
  confidence_iteration = 1;
  init_stat();

  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {

    /* initialize a few counters. we have to remember that we might be */
    /* going through the loop more than once. */
    
    nummessages    =	0;
    bytes_sent     =	0.0;
    times_up       = 	0;
    
    /*set up the data socket                        */
    send_socket = create_data_socket(local_res);
    
    if (send_socket == INVALID_SOCKET){
      perror("netperf: send_tcp_stream: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_tcp_stream: send_socket obtained...\n");
    }
    
    /* at this point, we have either retrieved the socket buffer sizes, */
    /* or have tried to set them, so now, we may want to set the send */
    /* size based on that (because the user either did not use a -m */
    /* option, or used one with an argument of 0). If the socket buffer */
    /* size is not available, we will set the send size to 4KB - no */
    /* particular reason, just arbitrary... */
    if (send_size == 0) {
      if (lss_size > 0) {
	send_size = lss_size;
      }
      else {
	send_size = 4096;
      }
    }
    
    /* set-up the data buffer ring with the requested alignment and offset. */
    /* note also that we have allocated a quantity */
    /* of memory that is at least one send-size greater than our socket */
    /* buffer size. We want to be sure that there are at least two */
    /* buffers allocated - this can be a bit of a problem when the */
    /* send_size is bigger than the socket size, so we must check... the */
    /* user may have wanted to explicitly set the "width" of our send */
    /* buffers, we should respect that wish... */
    if (send_width == 0) {
      send_width = (lss_size/send_size) + 1;
      if (send_width == 1) send_width++;
    }
    
    if (send_ring == NULL) {
      /* only allocate the send ring once. this is a networking test, */
      /* not a memory allocation test. this way, we do not need a */
      /* deallocate_buffer_ring() routine, and I don't feel like */
      /* writing one anyway :) raj 11/94 */
      send_ring = allocate_buffer_ring(send_width,
				       send_size,
				       local_send_align,
				       local_send_offset);
    }

    /* If the user has requested cpu utilization measurements, we must */
    /* calibrate the cpu(s). We will perform this task within the tests */
    /* themselves. If the user has specified the cpu rate, then */
    /* calibrate_local_cpu will return rather quickly as it will have */
    /* nothing to do. If local_cpu_rate is zero, then we will go through */
    /* all the "normal" calibration stuff and return the rate back. */
    
    if (local_cpu_usage) {
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    }
    
    if (!no_control) {
      /* Tell the remote end to do a listen. The server alters the
	 socket paramters on the other side at this point, hence the
	 reason for all the values being passed in the setup
	 message. If the user did not specify any of the parameters,
	 they will be passed as 0, which will indicate to the remote
	 that no changes beyond the system's default should be
	 used. Alignment is the exception, it will default to 1, which
	 will be no alignment alterations. */
    
      netperf_request.content.request_type =	DO_TCP_STREAM;
      tcp_stream_request->send_buf_size	=	rss_size_req;
      tcp_stream_request->recv_buf_size	=	rsr_size_req;
      tcp_stream_request->receive_size	=	recv_size;
      tcp_stream_request->no_delay	=	rem_nodelay;
      tcp_stream_request->recv_alignment	=	remote_recv_align;
      tcp_stream_request->recv_offset	=	remote_recv_offset;
      tcp_stream_request->measure_cpu	=	remote_cpu_usage;
      tcp_stream_request->cpu_rate	=	remote_cpu_rate;
      if (test_time) {
	tcp_stream_request->test_length	=	test_time;
      }
      else {
	tcp_stream_request->test_length	=	test_bytes;
      }
      tcp_stream_request->so_rcvavoid	=	rem_rcvavoid;
      tcp_stream_request->so_sndavoid	=	rem_sndavoid;
#ifdef DIRTY
      tcp_stream_request->dirty_count     =       rem_dirty_count;
      tcp_stream_request->clean_count     =       rem_clean_count;
#endif /* DIRTY */
      tcp_stream_request->port            =    atoi(remote_data_port);
      tcp_stream_request->ipfamily = af_to_nf(remote_res->ai_family);
      if (debug > 1) {
	fprintf(where,
		"netperf: send_tcp_stream: requesting TCP stream test\n");
      }
      
      send_request();
      
      /* The response from the remote will contain all of the relevant
         socket parameters for this test type. We will put them back
         into the variables here so they can be displayed if desired.
         The remote will have calibrated CPU if necessary, and will
         have done all the needed set-up we will have calibrated the
         cpu locally before sending the request, and will grab the
         counter value right after the connect returns. The remote
         will grab the counter right after the accept call. This saves
         the hassle of extra messages being sent for the TCP
         tests.  */
    
      recv_response();
    
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote listen done.\n");
	rsr_size	      =	tcp_stream_response->recv_buf_size;
	rss_size	      =	tcp_stream_response->send_buf_size;
	rem_nodelay     =	tcp_stream_response->no_delay;
	remote_cpu_usage=	tcp_stream_response->measure_cpu;
	remote_cpu_rate = tcp_stream_response->cpu_rate;
	
	/* we have to make sure that the server port number is in
	   network order */
	set_port_number(remote_res,
			(short)tcp_stream_response->data_port_number);
	
	rem_rcvavoid	= tcp_stream_response->so_rcvavoid;
	rem_sndavoid	= tcp_stream_response->so_sndavoid;
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	
	exit(1);
      }
    }

#ifdef WANT_DEMO
    DEMO_STREAM_SETUP(lss_size,rsr_size)
#endif

    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		remote_res->ai_addr,
		remote_res->ai_addrlen) == INVALID_SOCKET){
      perror("netperf: send_tcp_stream: data socket connect failed");
      exit(1);
    }

    /* Data Socket set-up is finished. If there were problems, either */
    /* the connect would have failed, or the previous response would */
    /* have indicated a problem. I failed to see the value of the */
    /* extra  message after the accept on the remote. If it failed, */
    /* we'll see it here. If it didn't, we might as well start pumping */
    /* data. */ 
    
    /* Set-up the test end conditions. For a stream test, they can be */
    /* either time or byte-count based. */
    
    if (test_time) {
      /* The user wanted to end the test after a period of time. */
      times_up = 0;
      bytes_remaining = 0;
      /* in previous revisions, we had the same code repeated throught */
      /* all the test suites. this was unnecessary, and meant more */
      /* work for me when I wanted to switch to POSIX signals, so I */
      /* have abstracted this out into a routine in netlib.c. if you */
      /* are experiencing signal problems, you might want to look */
      /* there. raj 11/94 */
      start_timer(test_time);
    }
    else {
      /* The tester wanted to send a number of bytes. */
      bytes_remaining = test_bytes;
      times_up = 1;
    }
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);

    /* we only start the interval timer if we are using the
       timer-timed intervals rather than the sit and spin ones. raj
       2006-02-06 */    
#if defined(WANT_INTERVALS)
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */

    /* before we start, initialize a few variables */

#ifdef WANT_DEMO
      if (demo_mode) {
	HIST_timestamp(demo_one_ptr);
      }
#endif
      

    /* We use an "OR" to control test execution. When the test is */
    /* controlled by time, the byte count check will always return false. */
    /* When the test is controlled by byte count, the time test will */
    /* always return false. When the test is finished, the whole */
    /* expression will go false and we will stop sending data. */
    
    while ((!times_up) || (bytes_remaining > 0)) {
      
#ifdef DIRTY
      access_buffer(send_ring->buffer_ptr,
		    send_size,
		    loc_dirty_count,
		    loc_clean_count);
#endif /* DIRTY */
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp just before we go into send and then again just
	 after we come out raj 8/94 */
	/* but lets only do this if there is going to be a histogram
	   displayed */
	HIST_timestamp(&time_one);
      }
#endif /* WANT_HISTOGRAM */

      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   send_size,
		   0)) != send_size) {
      if ((len >=0) || SOCKET_EINTR(len)) {
	    /* the test was interrupted, must be the end of test */
	    break;
	  }
	perror("netperf: data send error");
	printf("len was %d\n",len);
	exit(1);
      }

      local_bytes_sent += send_size;

#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp the exit from the send call and update the histogram */
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */      

#ifdef WANT_DEMO
      DEMO_STREAM_INTERVAL(send_size)
#endif 

#if defined(WANT_INTERVALS)
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
      /* now we want to move our pointer to the next position in the */
      /* data buffer...we may also want to wrap back to the "beginning" */
      /* of the bufferspace, so we will mod the number of messages sent */
      /* by the send width, and use that to calculate the offset to add */
      /* to the base pointer. */
      nummessages++;          
      send_ring = send_ring->next;
      if (bytes_remaining) {
	bytes_remaining -= send_size;
      }
    }

    /* The test is over. Flush the buffers to the remote end. We do a */
    /* graceful release to insure that all data has been taken by the */
    /* remote. */ 

    /* but first, if the verbosity is greater than 1, find-out what */
    /* the TCP maximum segment_size was (if possible) */
    if (verbosity > 1) {
      tcp_mss = -1;
      get_tcp_info(send_socket,&tcp_mss);
    }
    
    if (shutdown(send_socket,SHUT_WR) == SOCKET_ERROR) {
      perror("netperf: cannot shutdown tcp stream socket");
      exit(1);
    }
    
    /* hang a recv() off the socket to block until the remote has */
    /* brought all the data up into the application. it will do a */
    /* shutdown to cause a FIN to be sent our way. We will assume that */
    /* any exit from the recv() call is good... raj 4/93 */
    
    recv(send_socket, send_ring->buffer_ptr, send_size, 0);
    
    /* this call will always give us the elapsed time for the test, and */
    /* will also store-away the necessaries for cpu utilization */
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured and how */
						/* long did we really */
						/* run? */
    
    /* we are finished with the socket, so close it to prevent hitting */
    /* the limit on maximum open files. */

    close(send_socket);

    if (!no_control) {
      /* Get the statistics from the remote end. The remote will have
	 calculated service demand and all those interesting
	 things. If it wasn't supposed to care, it will return obvious
	 values. */
    
      recv_response();
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote results obtained\n");
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	
	exit(1);
      }
    
      /* We now calculate what our thruput was for the test. In the
	 future, we may want to include a calculation of the thruput
	 measured by the remote, but it should be the case that for a
	 TCP stream test, that the two numbers should be *very*
	 close... We calculate bytes_sent regardless of the way the
	 test length was controlled.  If it was time, we needed to,
	 and if it was by bytes, the user may have specified a number
	 of bytes that wasn't a multiple of the send_size, so we
	 really didn't send what he asked for ;-) */
    
      bytes_sent	= ntohd(tcp_stream_result->bytes_received);
    }
    else {
      bytes_sent = (double)local_bytes_sent;
    }

    thruput	= calc_thruput(bytes_sent);
    
    if (local_cpu_usage || remote_cpu_usage) {
      /* We must now do a little math for service demand and cpu */
      /* utilization for the system(s) */
      /* Of course, some of the information might be bogus because */
      /* there was no idle counter in the kernel(s). We need to make */
      /* a note of this for the user's benefit...*/
      if (local_cpu_usage) {
	
	local_cpu_utilization	= calc_cpu_util(0.0);
	local_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      0.0,
						      0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	
	remote_cpu_utilization	= tcp_stream_result->cpu_util;
	remote_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      remote_cpu_utilization,
						      tcp_stream_result->num_cpus);
      }
      else {
	remote_cpu_utilization = (float) -1.0;
	remote_service_demand  = (float) -1.0;
      }
    }    
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }

    /* at this point, we want to calculate the confidence information. */
    /* if debugging is on, calculate_confidence will print-out the */
    /* parameters we pass it */
    
    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    
    confidence_iteration++;
  }

  /* at this point, we have finished making all the runs that we */
  /* will be making. so, we should extract what the calcuated values */
  /* are for all the confidence stuff. we could make the values */
  /* global, but that seemed a little messy, and it did not seem worth */
  /* all the mucking with header files. so, we create a routine much */
  /* like calcualte_confidence, which just returns the mean values. */
  /* raj 11/94 */

  retrieve_confident_values(&elapsed_time,
			    &thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);

  /* We are now ready to print all the information. If the user */
  /* has specified zero-level verbosity, we will just print the */
  /* local service demand, or the remote service demand. If the */
  /* user has requested verbosity level 1, he will get the basic */
  /* "streamperf" numbers. If the user has specified a verbosity */
  /* of greater than 1, we will display a veritable plethora of */
  /* background information from outside of this block as it it */
  /* not cpu_measurement specific...  */

  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }

  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(tcp_stream_result->cpu_method);
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method,
		((print_headers) || 
		 (result_brand == NULL)) ? "" : result_brand);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		remote_cpu_method,
		((print_headers) || 
		 (result_brand == NULL)) ? "" : result_brand);
      }
      break;
    case 1:
    case 2:
      if (print_headers) {
		fprintf(where,
		cpu_title,
		format_units(),
		local_cpu_method,
		remote_cpu_method);
      }
    
      fprintf(where,
	      cpu_fmt_1,		/* the format string */
	      rsr_size,		        /* remote recvbuf size */
	      lss_size,		        /* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time,		/* how long was the test */
	      thruput, 		        /* what was the xfer rate */
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand,	/* remote service demand */
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      thruput,
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    case 1:
    case 2:
      if (print_headers) {
		fprintf(where,tput_title,format_units());
      }
      fprintf(where,
	      tput_fmt_1,		/* the format string */
	      rsr_size, 		/* remote recvbuf size */
	      lss_size, 		/* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time, 		/* how long did it take */
	      thruput,                  /* how fast did it go */
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
   
    /* this stuff needs to be worked-out in the presence of confidence */
    /* intervals and multiple iterations of the test... raj 11/94 */
 
    fprintf(where,
	    ksink_fmt,
	    "Bytes",
	    "Bytes",
	    "Bytes",
	    local_send_align,
	    remote_recv_align,
	    local_send_offset,
	    remote_recv_offset,
	    bytes_sent,
	    bytes_sent / (double)nummessages,
	    nummessages,
	    bytes_sent / (double)tcp_stream_result->recv_calls,
	    tcp_stream_result->recv_calls);
    fprintf(where,
	    ksink_fmt2,
	    tcp_mss);
    fflush(where);
#ifdef WANT_HISTOGRAM
    fprintf(where,"\n\nHistogram of time spent in send() call.\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */
  }
  
}



/* This routine implements the netperf-side TCP unidirectional data
   transfer test (a.k.a. stream) for the sockets interface where the
   data flow is from the netserver to the netperf.  It receives its
   parameters via global variables from the shell and writes its
   output to the standard output. */


void 
send_tcp_maerts(char remote_host[])
{
  
  char *tput_title = "\
Recv   Send    Send                          \n\
Socket Socket  Message  Elapsed              \n\
Size   Size    Size     Time     Throughput  \n\
bytes  bytes   bytes    secs.    %s/sec  \n\n";
  
  char *tput_fmt_0 =
    "%7.2f %s\n";
  
  char *tput_fmt_1 =
    "%6d %6d %6d    %-6.2f   %7.2f    %s \n";
  
  char *cpu_title = "\
Recv   Send    Send                          Utilization       Service Demand\n\
Socket Socket  Message  Elapsed              Send     Recv     Send    Recv\n\
Size   Size    Size     Time     Throughput  local    remote   local   remote\n\
bytes  bytes   bytes    secs.    %-8.8s/s  %% %c      %% %c      us/KB   us/KB\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f %c %s\n";

  char *cpu_fmt_1 =
    "%6d %6d %6d    %-6.2f     %7.2f   %-6.2f   %-6.2f   %-6.3f  %-6.3f %s\n";
  
  char *ksink_fmt = "\n\
Alignment      Offset         %-8.8s %-8.8s    Recvs   %-8.8s Sends\n\
Local  Remote  Local  Remote  Xfered   Per                 Per\n\
Recv   Send    Recv   Send             Recv (avg)          Send (avg)\n\
%5d   %5d  %5d   %5d %6.4g  %6.2f    %6d   %6.2f %6d\n";

  char *ksink_fmt2 = "\n\
Maximum\n\
Segment\n\
Size (bytes)\n\
%6d\n";
  
  
  float			elapsed_time;
  
  /* what we want is to have a buffer space that is at least one */
  /* recv-size greater than our recv window. this will insure that we */
  /* are never trying to re-use a buffer that may still be in the hands */
  /* of the transport. This buffer will be malloc'd after we have found */
  /* the size of the local senc socket buffer. We will want to deal */
  /* with alignment and offset concerns as well. */
  
  struct ring_elt *recv_ring;
  
  int len;
  unsigned int nummessages = 0;
  SOCKET recv_socket;
  int bytes_remaining;
  int tcp_mss = -1;  /* possibly uninitialized on printf far below */

  /* with links like fddi, one can recv > 32 bits worth of bytes */
  /* during a test... ;-) at some point, this should probably become a */
  /* 64bit integral type, but those are not entirely common yet */
  double	bytes_sent = 0.0;
  unsigned long long local_bytes_recvd = 0;

  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;

  double	thruput;
  
  struct addrinfo *remote_res;
  struct addrinfo *local_res;
  
  struct	tcp_maerts_request_struct	*tcp_maerts_request;
  struct	tcp_maerts_response_struct	*tcp_maerts_response;
  struct	tcp_maerts_results_struct	*tcp_maerts_result;
  
  tcp_maerts_request  = 
    (struct tcp_maerts_request_struct *)netperf_request.content.test_specific_data;
  tcp_maerts_response =
    (struct tcp_maerts_response_struct *)netperf_response.content.test_specific_data;
  tcp_maerts_result   = 
    (struct tcp_maerts_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);
  
  if ( print_headers ) {
    print_top_test_header("TCP MAERTS TEST",local_res,remote_res);
  }

  recv_ring = NULL;
  confidence_iteration = 1;
  init_stat();

  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {

    /* initialize a few counters. we have to remember that we might be */
    /* going through the loop more than once. */
    
    nummessages    =	0;
    bytes_sent     =	0.0;
    times_up       = 	0;
    
    /*set up the data socket                        */
    recv_socket = create_data_socket(local_res);
    
    if (recv_socket == INVALID_SOCKET){
      perror("netperf: send_tcp_maerts: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_tcp_maerts: recv_socket obtained...\n");
    }

    /* at this point, we have either retrieved the socket buffer sizes, */
    /* or have tried to set them, so now, we may want to set the recv */
    /* size based on that (because the user either did not use a -m */
    /* option, or used one with an argument of 0). If the socket buffer */
    /* size is not available, we will set the recv size to 4KB - no */
    /* particular reason, just arbitrary... */
    if (recv_size == 0) {
      if (lsr_size > 0) {
	recv_size = lsr_size;
      }
      else {
	recv_size = 4096;
      }
    }

    /* set-up the data buffer ring with the requested alignment and offset. */
    /* note also that we have allocated a quantity */
    /* of memory that is at least one recv-size greater than our socket */
    /* buffer size. We want to be sure that there are at least two */
    /* buffers allocated - this can be a bit of a problem when the */
    /* recv_size is bigger than the socket size, so we must check... the */
    /* user may have wanted to explicitly set the "width" of our recv */
    /* buffers, we should respect that wish... */
    if (recv_width == 0) {
      recv_width = (lsr_size/recv_size) + 1;
      if (recv_width == 1) recv_width++;
    }
    
    if (recv_ring == NULL) {
      /* only allocate the recv ring once. this is a networking test, */
      /* not a memory allocation test. this way, we do not need a */
      /* deallocate_buffer_ring() routine, and I don't feel like */
      /* writing one anyway :) raj 11/94 */
      recv_ring = allocate_buffer_ring(recv_width,
				       recv_size,
				       local_recv_align,
				       local_recv_offset);
    }

    /* If the user has requested cpu utilization measurements, we must */
    /* calibrate the cpu(s). We will perform this task within the tests */
    /* themselves. If the user has specified the cpu rate, then */
    /* calibrate_local_cpu will return rather quickly as it will have */
    /* nothing to do. If local_cpu_rate is zero, then we will go through */
    /* all the "normal" calibration stuff and return the rate back. */
    
    if (local_cpu_usage) {
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    }
    
    if (!no_control) {
      /* Tell the remote end to do a listen. The server alters the
	 socket paramters on the other side at this point, hence the
	 reason for all the values being passed in the setup
	 message. If the user did not specify any of the parameters,
	 they will be passed as 0, which will indicate to the remote
	 that no changes beyond the system's default should be
	 used. Alignment is the exception, it will default to 1, which
	 will be no alignment alterations. */

      netperf_request.content.request_type	=	DO_TCP_MAERTS;
      tcp_maerts_request->send_buf_size	=	rss_size_req;
      tcp_maerts_request->recv_buf_size	=	rsr_size_req;
      tcp_maerts_request->send_size	=	send_size;
      tcp_maerts_request->no_delay	=	rem_nodelay;
      tcp_maerts_request->send_alignment	=	remote_send_align;
      tcp_maerts_request->send_offset	=	remote_send_offset;
      tcp_maerts_request->measure_cpu	=	remote_cpu_usage;
      tcp_maerts_request->cpu_rate	=	remote_cpu_rate;
      if (test_time) {
	tcp_maerts_request->test_length	=	test_time;
      }
      else {
	tcp_maerts_request->test_length	=	test_bytes;
      }
      tcp_maerts_request->so_rcvavoid	=	rem_rcvavoid;
      tcp_maerts_request->so_sndavoid	=	rem_sndavoid;
#ifdef DIRTY
      tcp_maerts_request->dirty_count       =       rem_dirty_count;
      tcp_maerts_request->clean_count       =       rem_clean_count;
#endif /* DIRTY */
      tcp_maerts_request->port            = atoi(remote_data_port);
      tcp_maerts_request->ipfamily        = af_to_nf(remote_res->ai_family);
      if (debug > 1) {
	fprintf(where,
		"netperf: send_tcp_maerts: requesting TCP maerts test\n");
      }
      
      send_request();
      
      /* The response from the remote will contain all of the relevant
	 socket parameters for this test type. We will put them back
	 into the variables here so they can be displayed if desired.
	 The remote will have calibrated CPU if necessary, and will
	 have done all the needed set-up we will have calibrated the
	 cpu locally before sending the request, and will grab the
	 counter value right after the connect returns. The remote
	 will grab the counter right after the accept call. This saves
	 the hassle of extra messages being sent for the TCP
	 tests.  */
      
      recv_response();
    
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote listen done.\n");
	rsr_size	=	tcp_maerts_response->recv_buf_size;
	rss_size	=	tcp_maerts_response->send_buf_size;
	rem_nodelay     =	tcp_maerts_response->no_delay;
	remote_cpu_usage=	tcp_maerts_response->measure_cpu;
	remote_cpu_rate = tcp_maerts_response->cpu_rate;
	send_size       = tcp_maerts_response->send_size;
	
	/* we have to make sure that the server port number is in
	 network order */
      set_port_number(remote_res,
		      (short)tcp_maerts_response->data_port_number);
      rem_rcvavoid	= tcp_maerts_response->so_rcvavoid;
      rem_sndavoid	= tcp_maerts_response->so_sndavoid;
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	
	exit(1);
      }
    }

#ifdef WANT_DEMO
    DEMO_STREAM_SETUP(lsr_size,rss_size)
#endif

    /*Connect up to the remote port on the data socket  */
    if (connect(recv_socket, 
		remote_res->ai_addr,
		remote_res->ai_addrlen) == INVALID_SOCKET){
      perror("netperf: send_tcp_maerts: data socket connect failed");
      exit(1);
    }

    /* Data Socket set-up is finished. If there were problems, either */
    /* the connect would have failed, or the previous response would */
    /* have indicated a problem. I failed to see the value of the */
    /* extra  message after the accept on the remote. If it failed, */
    /* we'll see it here. If it didn't, we might as well start pumping */
    /* data. */ 
    
    /* Set-up the test end conditions. For a maerts test, they can be */
    /* either time or byte-count based. */
    
    if (test_time) {
      /* The user wanted to end the test after a period of time. */
      times_up = 0;
      bytes_remaining = 0;
      /* in previous revisions, we had the same code repeated throught */
      /* all the test suites. this was unnecessary, and meant more */
      /* work for me when I wanted to switch to POSIX signals, so I */
      /* have abstracted this out into a routine in netlib.c. if you */
      /* are experiencing signal problems, you might want to look */
      /* there. raj 11/94 */
      if (!no_control) {
	/* this is a netperf to netserver test, netserver will close
	   to tell us the test is over, so use PAD_TIME to avoid
	   causing the netserver fits. */
	start_timer(test_time + PAD_TIME);
      }
      else {
	/* this is a netperf to data source test, no PAD_TIME */
	start_timer(test_time);
      }
    }
    else {
      /* The tester wanted to recv a number of bytes. we don't do that 
	 in a TCP_MAERTS test. sorry. raj 2002-06-21 */
      printf("netperf: send_tcp_maerts: test must be timed\n");
      exit(1);
    }
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);
    
#ifdef WANT_INTERVALS
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */

    /* before we start, initialize a few variables */

#ifdef WANT_DEMO
    if (demo_mode) {
      HIST_timestamp(demo_one_ptr);
    }
#endif

    /* the test will continue until we either get a zero-byte recv()
       on the socket or our failsafe timer expires. most of the time
       we trust that we get a zero-byte recieve from the socket. raj
       2002-06-21 */

#ifdef WANT_HISTOGRAM
    if (verbosity > 1) {
      /* timestamp just before we go into recv and then again just
	 after we come out raj 8/94 */
      /* but only if we are actually going to display a histogram. raj
	 2006-02-07 */
      HIST_timestamp(&time_one);
    }
#endif /* WANT_HISTOGRAM */
    
    while ((!times_up) && (len=recv(recv_socket,
				    recv_ring->buffer_ptr,
				    recv_size,
				    0)) > 0 ) {

#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp the exit from the recv call and update the histogram */
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */      

#ifdef DIRTY
      access_buffer(recv_ring->buffer_ptr,
		    recv_size,
		    loc_dirty_count,
		    loc_clean_count);
#endif /* DIRTY */

#ifdef WANT_DEMO
      DEMO_STREAM_INTERVAL(len);
#endif

#ifdef WANT_INTERVALS      
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
      /* now we want to move our pointer to the next position in the */
      /* data buffer...we may also want to wrap back to the "beginning" */
      /* of the bufferspace, so we will mod the number of messages sent */
      /* by the recv width, and use that to calculate the offset to add */
      /* to the base pointer. */
      nummessages++;          
      recv_ring = recv_ring->next;
      if (bytes_remaining) {
	bytes_remaining -= len;
      }

      local_bytes_recvd += len;

#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* make sure we timestamp just before we go into recv  */
	/* raj 2004-06-15 */
	HIST_timestamp(&time_one);
      }
#endif /* WANT_HISTOGRAM */
    
    }

    /* an EINTR is to be expected when this is a no_control test */
    if (((len < 0) || SOCKET_EINTR(len)) && (!no_control)) {
      perror("send_tcp_maerts: data recv error");
      printf("len was %d\n",len);
      exit(1);
    }
    
    /* if we get here, it must mean we had a recv return of 0 before
       the watchdog timer expired, or the watchdog timer expired and
       this was a no_control test */

    /* The test is over. Flush the buffers to the remote end. We do a
       graceful release to tell the  remote we have all the data. */  

    /* but first, if the verbosity is greater than 1, find-out what */
    /* the TCP maximum segment_size was (if possible) */
    if (verbosity > 1) {
      tcp_mss = -1;
      get_tcp_info(recv_socket,&tcp_mss);
    }
    
    if (shutdown(recv_socket,SHUT_WR) == SOCKET_ERROR) {
      perror("netperf: cannot shutdown tcp maerts socket");
      exit(1);
    }

    stop_timer();
    
    /* this call will always give us the local elapsed time for the
       test, and will also store-away the necessaries for cpu
       utilization */ 
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured and how */
						/* long did we really */
						/* run? */
    
    /* we are finished with the socket, so close it to prevent hitting */
    /* the limit on maximum open files. */

    close(recv_socket);

    if (!no_control) {
      /* Get the statistics from the remote end. The remote will have
         calculated service demand and all those interesting
         things. If it wasn't supposed to care, it will return obvious
         values. */
    
      recv_response();
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote results obtained\n");
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	
	exit(1);
      }
      
      /* We now calculate what our thruput was for the test. In the
	 future, we may want to include a calculation of the thruput
	 measured by the remote, but it should be the case that for a
	 TCP maerts test, that the two numbers should be *very*
	 close... We calculate bytes_sent regardless of the way the
	 test length was controlled.  If it was time, we needed to,
	 and if it was by bytes, the user may have specified a number
	 of bytes that wasn't a multiple of the recv_size, so we
	 really didn't recv what he asked for ;-) */
    
      bytes_sent	= ntohd(tcp_maerts_result->bytes_sent);
    }
    else {
      bytes_sent = (double)local_bytes_recvd;
    }


    thruput	= calc_thruput(bytes_sent);

    if (local_cpu_usage || remote_cpu_usage) {
      /* We must now do a little math for service demand and cpu */
      /* utilization for the system(s) */
      /* Of course, some of the information might be bogus because */
      /* there was no idle counter in the kernel(s). We need to make */
      /* a note of this for the user's benefit...*/
      if (local_cpu_usage) {
	
	local_cpu_utilization	= calc_cpu_util(0.0);
	local_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      0.0,
						      0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	
	remote_cpu_utilization	= tcp_maerts_result->cpu_util;
	remote_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      remote_cpu_utilization,
						      tcp_maerts_result->num_cpus);
      }
      else {
	remote_cpu_utilization = (float) -1.0;
	remote_service_demand  = (float) -1.0;
      }
    }    
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }

    /* at this point, we want to calculate the confidence information. */
    /* if debugging is on, calculate_confidence will print-out the */
    /* parameters we pass it */

    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    
    confidence_iteration++;
  }

  /* at this point, we have finished making all the runs that we */
  /* will be making. so, we should extract what the calcuated values */
  /* are for all the confidence stuff. we could make the values */
  /* global, but that seemed a little messy, and it did not seem worth */
  /* all the mucking with header files. so, we create a routine much */
  /* like calcualte_confidence, which just returns the mean values. */
  /* raj 11/94 */

  retrieve_confident_values(&elapsed_time,
			    &thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);

  /* We are now ready to print all the information. If the user */
  /* has specified zero-level verbosity, we will just print the */
  /* local service demand, or the remote service demand. If the */
  /* user has requested verbosity level 1, he will get the basic */
  /* "streamperf" numbers. If the user has specified a verbosity */
  /* of greater than 1, we will display a veritable plethora of */
  /* background information from outside of this block as it it */
  /* not cpu_measurement specific...  */

  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }

  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(tcp_maerts_result->cpu_method);
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method,
		((print_headers) || 
		 (result_brand == NULL)) ? "" : result_brand);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		remote_cpu_method,
		((print_headers) || 
		 (result_brand == NULL)) ? "" : result_brand);
      }
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,
		cpu_title,
		format_units(),
		local_cpu_method,
		remote_cpu_method);
      }
    
      fprintf(where,
	      cpu_fmt_1,		/* the format string */
	      rsr_size,		        /* remote recvbuf size */
	      lss_size,		        /* local sendbuf size */
	      send_size,		/* how large were the recvs */
	      elapsed_time,		/* how long was the test */
	      thruput, 		        /* what was the xfer rate */
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand,	/* remote service demand */
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      thruput,
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }
      fprintf(where,
	      tput_fmt_1,		/* the format string */
	      lsr_size, 		/* local recvbuf size */
	      rss_size, 		/* remot sendbuf size */
	      send_size,		/* how large were the recvs */
	      elapsed_time, 		/* how long did it take */
	      thruput,                  /* how fast did it go */
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
   
    /* this stuff needs to be worked-out in the presence of confidence */
    /* intervals and multiple iterations of the test... raj 11/94 */
 
    fprintf(where,
	    ksink_fmt,
	    "Bytes",
	    "Bytes",
	    "Bytes",
	    local_recv_align,
	    remote_recv_align,
	    local_recv_offset,
	    remote_recv_offset,
	    bytes_sent,
	    bytes_sent / (double)nummessages,
	    nummessages,
	    bytes_sent / (double)tcp_maerts_result->send_calls,
	    tcp_maerts_result->send_calls);
    fprintf(where,
	    ksink_fmt2,
	    tcp_mss);
    fflush(where);
#ifdef WANT_HISTOGRAM
    fprintf(where,"\n\nHistogram of time spent in recv() call.\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */
  }
  
}



#ifdef HAVE_ICSC_EXS

#include <sys/exs.h>


/* This routine implements the TCP unidirectional data transfer test */
/* (a.k.a. stream) for the sockets interface. It receives its */
/* parameters via global variables from the shell and writes its */
/* output to the standard output. */

void
send_exs_tcp_stream(char remote_host[])
{

    char *tput_title = "\
Recv   Send    Send                          \n\
Socket Socket  Message  Elapsed              \n\
Size   Size    Size     Time     Throughput  \n\
bytes  bytes   bytes    secs.    %s/sec  \n\n";

    char *tput_fmt_0 =
        "%7.2f\n";

    char *tput_fmt_1 =
        "%6d %6d %6d    %-6.2f   %7.2f   \n";

    char *cpu_title = "\
Recv   Send    Send                          Utilization       Service Demand\n\
Socket Socket  Message  Elapsed              Send     Recv     Send    Recv\n\
Size   Size    Size     Time     Throughput  local    remote   local   remote\n\
bytes  bytes   bytes    secs.    %-8.8s/s  %% %c      %% %c      us/KB   us/KB\n\n";

    char *cpu_fmt_0 =
        "%6.3f %c\n";

    char *cpu_fmt_1 =
        "%6d %6d %6d    %-6.2f     %7.2f   %-6.2f   %-6.2f   %-6.3f  %-6.3f\n";

    char *ksink_fmt = "\n\
Alignment      Offset         %-8.8s %-8.8s    Sends   %-8.8s Recvs\n\
Local  Remote  Local  Remote  Xfered   Per                 Per\n\
Send   Recv    Send   Recv             Send (avg)          Recv (avg)\n\
%5d   %5d  %5d   %5d %6.4g  %6.2f    %6d   %6.2f %6d\n";

    char *ksink_fmt2 = "\n\
Maximum\n\
Segment\n\
Size (bytes)\n\
%6d\n";


    float         elapsed_time;

    /* what we want is to have a buffer space that is at least one */
    /* send-size greater than our send window. this will insure that we */
    /* are never trying to re-use a buffer that may still be in the hands */
    /* of the transport. This buffer will be malloc'd after we have found */
    /* the size of the local senc socket buffer. We will want to deal */
    /* with alignment and offset concerns as well. */

    struct ring_elt *send_ring;

    int len;
    unsigned int nummessages = 0;
    SOCKET send_socket;
    int bytes_remaining;
    int tcp_mss = -1;  /* possibly uninitialized on printf far below */

    exs_mhandle_t exs_mhandle;
    exs_qhandle_t exs_qhandle;
#define NETPERF_EXS_PENDING  16
    int exs_aio_pending;
    int exs_aio_eagain;
    int exs_aio_dequeued;
    int exs_aio_dequeuecnt;
    int exs_evtcnt;
#define NETPERF_EXS_QSIZE    128
    exs_event_t exs_evtvec[NETPERF_EXS_QSIZE];

    /* with links like fddi, one can send > 32 bits worth of bytes */
    /* during a test... ;-) at some point, this should probably become a */
    /* 64bit integral type, but those are not entirely common yet */

    double   bytes_sent = 0.0;

    float   local_cpu_utilization;
    float   local_service_demand;
    float   remote_cpu_utilization;
    float   remote_service_demand;

    double   thruput;

    struct addrinfo *remote_res;
    struct addrinfo *local_res;

    struct   tcp_stream_request_struct   *tcp_stream_request;
    struct   tcp_stream_response_struct   *tcp_stream_response;
    struct   tcp_stream_results_struct   *tcp_stream_result;

    tcp_stream_request  =
        (struct tcp_stream_request_struct *)netperf_request.content.test_specific_data;
    tcp_stream_response =
        (struct tcp_stream_response_struct *)netperf_response.content.test_specific_data;
    tcp_stream_result   =
        (struct tcp_stream_results_struct *)netperf_response.content.test_specific_data;

#if 0 /* def WANT_HISTOGRAM */
    time_hist = HIST_new();
#endif /* WANT_HISTOGRAM */
    /* since we are now disconnected from the code that established the */
    /* control socket, and since we want to be able to use different */
    /* protocols and such, we are passed the name of the remote host and */
    /* must turn that into the test specific addressing information. */

    /* complete_addrinfos will either succede or exit the process */
    complete_addrinfos(&remote_res,
                       &local_res,
                       remote_host,
                       SOCK_STREAM,
                       IPPROTO_TCP,
                       0);

    if ( print_headers ) {
        print_top_test_header("EXS TCP STREAM TEST",local_res,remote_res);
    }

    send_ring = NULL;
    confidence_iteration = 1;
    init_stat();

    /* initialize EXS API and create event queue */
    if (exs_init (EXS_VERSION) == -1) {
        perror ("netperf: send_exs_tcp_stream: exs_init failed");
        exit (1);
    }

    if ((exs_qhandle = exs_qcreate (NETPERF_EXS_QSIZE)) == EXS_QHANDLE_INVALID) {
        perror ("netperf: send_exs_tcp_stream: exs_qcreate failed");
        exit (1);
    }
    if (debug) {
        fprintf (where, "send_exs_tcp_stream: qhandle=%d\n", exs_qhandle);
    }

    /* we have a great-big while loop which controls the number of times */
    /* we run a particular test. this is for the calculation of a */
    /* confidence interval (I really should have stayed awake during */
    /* probstats :). If the user did not request confidence measurement */
    /* (no confidence is the default) then we will only go though the */
    /* loop once. the confidence stuff originates from the folks at IBM */

    while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
           (confidence_iteration <= iteration_min)) {

        /* initialize a few counters. we have to remember that we might be */
        /* going through the loop more than once. */

        nummessages    =   0;
        bytes_sent     =   0.0;
        times_up       =    0;

        /*set up the data socket                        */
        send_socket = create_data_socket(local_res);

        if (send_socket == INVALID_SOCKET){
            perror("netperf: send_tcp_stream: tcp stream data socket");
            exit(1);
        }

        if (debug) {
            fprintf(where,"send_tcp_stream: send_socket obtained...\n");
        }

        /* at this point, we have either retrieved the socket buffer sizes, */
        /* or have tried to set them, so now, we may want to set the send */
        /* size based on that (because the user either did not use a -m */
        /* option, or used one with an argument of 0). If the socket buffer */
        /* size is not available, we will set the send size to 4KB - no */
        /* particular reason, just arbitrary... */
        if (send_size == 0) {
            if (lss_size > 0) {
                send_size = lss_size;
            }
            else {
                send_size = 4096;
            }
        }

        /* set-up the data buffer ring with the requested alignment and offset. */
        /* note also that we have allocated a quantity */
        /* of memory that is at least one send-size greater than our socket */
        /* buffer size. We want to be sure that there are at least two */
        /* buffers allocated - this can be a bit of a problem when the */
        /* send_size is bigger than the socket size, so we must check... the */
        /* user may have wanted to explicitly set the "width" of our send */
        /* buffers, we should respect that wish... */
        if (send_width == 0) {
            send_width = (lss_size/send_size) + 1;
            if (send_width == 1) send_width++;
        }

        if (send_ring == NULL) {
            /* only allocate the send ring once. this is a networking test, */
            /* not a memory allocation test. this way, we do not need a */
            /* deallocate_buffer_ring() routine, and I don't feel like */
            /* writing one anyway :) raj 11/94 */
            send_ring = allocate_exs_buffer_ring(send_width,
                                                 send_size,
                                                 local_send_align,
                                                 local_send_offset,
                                                 &exs_mhandle);
        }

        /* If the user has requested cpu utilization measurements, we must */
        /* calibrate the cpu(s). We will perform this task within the tests */
        /* themselves. If the user has specified the cpu rate, then */
        /* calibrate_local_cpu will return rather quickly as it will have */
        /* nothing to do. If local_cpu_rate is zero, then we will go through */
        /* all the "normal" calibration stuff and return the rate back. */

        if (local_cpu_usage) {
            local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
        }

        /* Tell the remote end to do a listen. The server alters the socket */
        /* paramters on the other side at this point, hence the reason for */
        /* all the values being passed in the setup message. If the user did */
        /* not specify any of the parameters, they will be passed as 0, which */
        /* will indicate to the remote that no changes beyond the system's */
        /* default should be used. Alignment is the exception, it will */
        /* default to 1, which will be no alignment alterations. */

        netperf_request.content.request_type =   DO_TCP_STREAM;
        tcp_stream_request->send_buf_size   =   rss_size_req;
        tcp_stream_request->recv_buf_size   =   rsr_size_req;
        tcp_stream_request->receive_size   =   recv_size;
        tcp_stream_request->no_delay   =   rem_nodelay;
        tcp_stream_request->recv_alignment   =   remote_recv_align;
        tcp_stream_request->recv_offset   =   remote_recv_offset;
        tcp_stream_request->measure_cpu   =   remote_cpu_usage;
        tcp_stream_request->cpu_rate   =   remote_cpu_rate;
        if (test_time) {
            tcp_stream_request->test_length   =   test_time;
        }
        else {
            tcp_stream_request->test_length   =   test_bytes;
        }
        tcp_stream_request->so_rcvavoid   =   rem_rcvavoid;
        tcp_stream_request->so_sndavoid   =   rem_sndavoid;
#ifdef DIRTY
        tcp_stream_request->dirty_count     =       rem_dirty_count;
        tcp_stream_request->clean_count     =       rem_clean_count;
#endif /* DIRTY */
        tcp_stream_request->port            =    atoi(remote_data_port);
        tcp_stream_request->ipfamily = af_to_nf(remote_res->ai_family);
        if (debug > 1) {
            fprintf(where,
                    "netperf: send_tcp_stream: requesting TCP stream test\n");
        }

        send_request();

        /* The response from the remote will contain all of the relevant    */
        /* socket parameters for this test type. We will put them back into */
        /* the variables here so they can be displayed if desired.  The   */
        /* remote will have calibrated CPU if necessary, and will have done   */
        /* all the needed set-up we will have calibrated the cpu locally   */
        /* before sending the request, and will grab the counter value right*/
        /* after the connect returns. The remote will grab the counter right*/
        /* after the accept call. This saves the hassle of extra messages   */
        /* being sent for the TCP tests.               */

        recv_response();

        if (!netperf_response.content.serv_errno) {
            if (debug)
                fprintf(where,"remote listen done.\n");
            rsr_size         =   tcp_stream_response->recv_buf_size;
            rss_size         =   tcp_stream_response->send_buf_size;
            rem_nodelay     =   tcp_stream_response->no_delay;
            remote_cpu_usage=   tcp_stream_response->measure_cpu;
            remote_cpu_rate = tcp_stream_response->cpu_rate;

            /* we have to make sure that the server port number is in */
            /* network order */
            set_port_number(remote_res,(short)tcp_stream_response->data_port_number);

            rem_rcvavoid   = tcp_stream_response->so_rcvavoid;
            rem_sndavoid   = tcp_stream_response->so_sndavoid;
        }
        else {
            Set_errno(netperf_response.content.serv_errno);
            fprintf(where,
                    "netperf: remote error %d",
                    netperf_response.content.serv_errno);
            perror("");
            fflush(where);

            exit(1);
        }

#if 0 /* def WANT_DEMO */
        DEMO_STREAM_SETUP(lss_size,rsr_size)
#endif

            /*Connect up to the remote port on the data socket  */
            if (connect(send_socket,
                        remote_res->ai_addr,
                        remote_res->ai_addrlen) == INVALID_SOCKET){
                perror("netperf: send_tcp_stream: data socket connect failed");
                exit(1);
            }

        /* Data Socket set-up is finished. If there were problems, either */
        /* the connect would have failed, or the previous response would */
        /* have indicated a problem. I failed to see the value of the */
        /* extra  message after the accept on the remote. If it failed, */
        /* we'll see it here. If it didn't, we might as well start pumping */
        /* data. */

        /* Set-up the test end conditions. For a stream test, they can be */
        /* either time or byte-count based. */

        if (test_time) {
            /* The user wanted to end the test after a period of time. */
            times_up = 0;
            bytes_remaining = 0;
            /* in previous revisions, we had the same code repeated throught */
            /* all the test suites. this was unnecessary, and meant more */
            /* work for me when I wanted to switch to POSIX signals, so I */
            /* have abstracted this out into a routine in netlib.c. if you */
            /* are experiencing signal problems, you might want to look */
            /* there. raj 11/94 */
            start_timer(test_time);
        }
        else {
            /* The tester wanted to send a number of bytes. */
            bytes_remaining = test_bytes;
            times_up = 1;
        }

        /* The cpu_start routine will grab the current time and possibly */
        /* value of the idle counter for later use in measuring cpu */
        /* utilization and/or service demand and thruput. */

        cpu_start(local_cpu_usage);

#if 0 /* def WANT_INTERVALS */
	INTERVALS_INIT();
#endif /* WANT_INTERVALS */

        /* before we start, initialize a few variables */

#if 0 /* def WANT_DEMO */
        if (demo_mode) {
            HIST_timestamp(demo_one_ptr);
        }
#endif


        /* We use an "OR" to control test execution. When the test is */
        /* controlled by time, the byte count check will always return false. */
        /* When the test is controlled by byte count, the time test will */
        /* always return false. When the test is finished, the whole */
        /* expression will go false and we will stop sending data. */

        exs_aio_pending = 0;
        exs_aio_eagain = 0;
        exs_aio_dequeuecnt = 0;

        while ((!times_up) || (bytes_remaining > 0)) {

#ifdef DIRTY
	  access_buffer(send_ring->buffer_ptr,
			send_size,
			loc_dirty_count,
			loc_clean_count);
#endif /* DIRTY */

#if 0 /* def WANT_HISTOGRAM */
            /* timestamp just before we go into send and then again just after */
            /* we come out raj 8/94 */
            HIST_timestamp(&time_one);
#endif /* WANT_HISTOGRAM */


            /* post up to NETPERF_EXS_PENDING I/Os  */
            while ((exs_aio_pending < NETPERF_EXS_PENDING) &&
                   (exs_send (send_socket, send_ring->buffer_ptr, send_size,
                              0, exs_qhandle, (exs_ahandle_t)-1, exs_mhandle) == 0)) {
                exs_aio_pending++;

                /* now we want to move our pointer to the next
		   position in the data buffer...we may also want to
		   wrap back to the "beginning" of the bufferspace, so
		   we will mod the number of messages sent by the send
		   width, and use that to calculate the offset to add
		   to the base pointer. */

                nummessages++;
                send_ring = send_ring->next;
                if (bytes_remaining) {
                    bytes_remaining -= send_size;
                }
            }

            /* check exs_send result */
            if (exs_aio_pending < NETPERF_EXS_PENDING) {
               /* standard flow control case */
                if (errno == EAGAIN)
                    exs_aio_eagain++;
                /* case of times_up */
                else if (errno == EINTR)
                    break;
                /* strange, let's stop */
                else {
                    perror ("netperf: exs_send error");
                    exit (1);
                }
            }

            /* dequeue events with "threshold" on 1/2 posted */
            exs_aio_dequeued =
                exs_qdequeue (exs_qhandle, exs_evtvec,
                              -(exs_aio_pending>>1), NULL);
            exs_aio_dequeuecnt++;

            /* check exs_dequeue result */
            if (exs_aio_dequeued < 0) {
                /* case of times_up */
                if (errno == EINTR)
                    break;
                /* strange, let's stop */
                else {
                    perror ("netperf: exs_send error");
                    exit (1);
                }
            }
            /* update number of pending I/Os */
            else {
                exs_aio_pending -= exs_aio_dequeued;
            }


#if 0 /* def WANT_HISTOGRAM */
            /* timestamp the exit from the send call and update the histogram */
            HIST_timestamp(&time_two);
            HIST_add(time_hist,delta_micro(&time_one,&time_two));
#endif /* WANT_HISTOGRAM */

#if 0 /* def WANT_DEMO */
            DEMO_STREAM_INTERVAL(send_size);
#endif

#if 0 /* def WANT_INTERVALS */
	    INTERVALS_WAIT();
#endif /* WANT_INTERVALS */

        }

        /* Collect the last completion events */
        exs_aio_dequeued =
            exs_qdequeue (exs_qhandle, exs_evtvec, -exs_aio_pending, NULL);
        exs_aio_dequeuecnt++;
        /* check exs_dequeue result and update number of pending I/Os */
        if (exs_aio_dequeued < 0) {
            perror ("netperf: exs_send error");
            exit (1);
        }
        exs_aio_pending -= exs_aio_dequeued;

        /* Display some async I/O debug info */
        if (debug) {
            fprintf (where, "send_exs_tcp_stream: "
                     "aio sent=%d eagain=%d dequeue=%d pending=%d\n",
                     nummessages, exs_aio_eagain, exs_aio_dequeuecnt, exs_aio_pending);
        }

        /* The test is over. Flush the buffers to the remote end. We do a */
        /* graceful release to insure that all data has been taken by the */
        /* remote. */

        /* but first, if the verbosity is greater than 1, find-out what */
        /* the TCP maximum segment_size was (if possible) */
        if (verbosity > 1) {
            tcp_mss = -1;
            get_tcp_info(send_socket,&tcp_mss);
        }

        if (shutdown(send_socket,SHUT_WR) == SOCKET_ERROR) {
            perror("netperf: cannot shutdown tcp stream socket");
            exit(1);
        }

        /* hang a recv() off the socket to block until the remote has */
        /* brought all the data up into the application. it will do a */
        /* shutdown to cause a FIN to be sent our way. We will assume that */
        /* any exit from the recv() call is good... raj 4/93 */

        recv(send_socket, send_ring->buffer_ptr, send_size, 0);

        /* this call will always give us the elapsed time for the test, and */
        /* will also store-away the necessaries for cpu utilization */

        cpu_stop(local_cpu_usage,&elapsed_time);   /* was cpu being */
        /* measured and how */
        /* long did we really */
        /* run? */

        /* we are finished with the socket, so close it to prevent hitting */
        /* the limit on maximum open files. */

        close(send_socket);

        /* Get the statistics from the remote end. The remote will have */
        /* calculated service demand and all those interesting things. If it */
        /* wasn't supposed to care, it will return obvious values. */

        recv_response();
        if (!netperf_response.content.serv_errno) {
            if (debug)
                fprintf(where,"remote results obtained\n");
        }
        else {
            Set_errno(netperf_response.content.serv_errno);
            fprintf(where,
                    "netperf: remote error %d",
                    netperf_response.content.serv_errno);
            perror("");
            fflush(where);

            exit(1);
        }

        /* We now calculate what our thruput was for the test. In the future, */
        /* we may want to include a calculation of the thruput measured by */
        /* the remote, but it should be the case that for a TCP stream test, */
        /* that the two numbers should be *very* close... We calculate */
        /* bytes_sent regardless of the way the test length was controlled. */
        /* If it was time, we needed to, and if it was by bytes, the user may */
        /* have specified a number of bytes that wasn't a multiple of the */
        /* send_size, so we really didn't send what he asked for ;-) */

        bytes_sent   = ntohd(tcp_stream_result->bytes_received);

        thruput   = calc_thruput(bytes_sent);

        if (local_cpu_usage || remote_cpu_usage) {
            /* We must now do a little math for service demand and cpu */
            /* utilization for the system(s) */
            /* Of course, some of the information might be bogus because */
            /* there was no idle counter in the kernel(s). We need to make */
            /* a note of this for the user's benefit...*/
            if (local_cpu_usage) {

                local_cpu_utilization   = calc_cpu_util(0.0);
                local_service_demand   = calc_service_demand(bytes_sent,
                                                             0.0,
                                                             0.0,
                                                             0);
            }
            else {
                local_cpu_utilization   = (float) -1.0;
                local_service_demand   = (float) -1.0;
            }

            if (remote_cpu_usage) {

                remote_cpu_utilization   = tcp_stream_result->cpu_util;
                remote_service_demand   = calc_service_demand(bytes_sent,
                                                              0.0,
                                                              remote_cpu_utilization,
                                                              tcp_stream_result->num_cpus);
            }
            else {
                remote_cpu_utilization = (float) -1.0;
                remote_service_demand  = (float) -1.0;
            }
        }
        else {
            /* we were not measuring cpu, for the confidence stuff, we */
            /* should make it -1.0 */
            local_cpu_utilization   = (float) -1.0;
            local_service_demand   = (float) -1.0;
            remote_cpu_utilization = (float) -1.0;
            remote_service_demand  = (float) -1.0;
        }

        /* at this point, we want to calculate the confidence information. */
        /* if debugging is on, calculate_confidence will print-out the */
        /* parameters we pass it */

        calculate_confidence(confidence_iteration,
                             elapsed_time,
                             thruput,
                             local_cpu_utilization,
                             remote_cpu_utilization,
                             local_service_demand,
                             remote_service_demand);


        confidence_iteration++;
    }

    /* at this point, we have finished making all the runs that we */
    /* will be making. so, we should extract what the calcuated values */
    /* are for all the confidence stuff. we could make the values */
    /* global, but that seemed a little messy, and it did not seem worth */
    /* all the mucking with header files. so, we create a routine much */
    /* like calcualte_confidence, which just returns the mean values. */
    /* raj 11/94 */

    retrieve_confident_values(&elapsed_time,
                              &thruput,
                              &local_cpu_utilization,
                              &remote_cpu_utilization,
                              &local_service_demand,
                              &remote_service_demand);

    /* We are now ready to print all the information. If the user */
    /* has specified zero-level verbosity, we will just print the */
    /* local service demand, or the remote service demand. If the */
    /* user has requested verbosity level 1, he will get the basic */
    /* "streamperf" numbers. If the user has specified a verbosity */
    /* of greater than 1, we will display a veritable plethora of */
    /* background information from outside of this block as it it */
    /* not cpu_measurement specific...  */

    if (confidence < 0) {
        /* we did not hit confidence, but were we asked to look for it? */
        if (iteration_max > 1) {
            display_confidence();
        }
    }

    if (local_cpu_usage || remote_cpu_usage) {
        local_cpu_method = format_cpu_method(cpu_method);
        remote_cpu_method = format_cpu_method(tcp_stream_result->cpu_method);

        switch (verbosity) {
            case 0:
                if (local_cpu_usage) {
                    fprintf(where,
                            cpu_fmt_0,
                            local_service_demand,
                            local_cpu_method);
                }
                else {
                    fprintf(where,
                            cpu_fmt_0,
                            remote_service_demand,
                            remote_cpu_method);
                }
                break;
            case 1:
            case 2:
                if (print_headers) {
                    fprintf(where,
                            cpu_title,
                            format_units(),
                            local_cpu_method,
                            remote_cpu_method);
                }

                fprintf(where,
                        cpu_fmt_1,      /* the format string */
                        rsr_size,              /* remote recvbuf size */
                        lss_size,              /* local sendbuf size */
                        send_size,      /* how large were the sends */
                        elapsed_time,      /* how long was the test */
                        thruput,               /* what was the xfer rate */
                        local_cpu_utilization,   /* local cpu */
                        remote_cpu_utilization,   /* remote cpu */
                        local_service_demand,   /* local service demand */
                        remote_service_demand);   /* remote service demand */
                break;
        }
    }
    else {
        /* The tester did not wish to measure service demand. */

        switch (verbosity) {
            case 0:
                fprintf(where,
                        tput_fmt_0,
                        thruput);
                break;
            case 1:
            case 2:
                if (print_headers) {
                    fprintf(where,tput_title,format_units());
                }
                fprintf(where,
                        tput_fmt_1,      /* the format string */
                        rsr_size,       /* remote recvbuf size */
                        lss_size,       /* local sendbuf size */
                        send_size,      /* how large were the sends */
                        elapsed_time,       /* how long did it take */
                        thruput);/* how fast did it go */
                break;
        }
    }

    /* it would be a good thing to include information about some of the */
    /* other parameters that may have been set for this test, but at the */
    /* moment, I do not wish to figure-out all the  formatting, so I will */
    /* just put this comment here to help remind me that it is something */
    /* that should be done at a later time. */

    if (verbosity > 1) {
        /* The user wanted to know it all, so we will give it to him. */
        /* This information will include as much as we can find about */
        /* TCP statistics, the alignments of the sends and receives */
        /* and all that sort of rot... */

        /* this stuff needs to be worked-out in the presence of confidence */
        /* intervals and multiple iterations of the test... raj 11/94 */

        fprintf(where,
                ksink_fmt,
                "Bytes",
                "Bytes",
                "Bytes",
                local_send_align,
                remote_recv_align,
                local_send_offset,
                remote_recv_offset,
                bytes_sent,
                bytes_sent / (double)nummessages,
                nummessages,
                bytes_sent / (double)tcp_stream_result->recv_calls,
                tcp_stream_result->recv_calls);
        fprintf(where,
                ksink_fmt2,
                tcp_mss);
        fflush(where);
#if 0 /* def WANT_HISTOGRAM */
        fprintf(where,"\n\nHistogram of time spent in send() call.\n");
        fflush(where);
        HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */
    }

}

#endif /* HAVE_ICSC_EXS */



#if defined(HAVE_SENDFILE) 

#if defined(QUICK_SENDPATH)

/*
 * a temporary stub for the sendpath() system call
 * which is defined & implemented in the kernel
 * but which has no libc stub.
 */
#include <sys/types.h>
#include <sys/scall_define.h>
#include <sys/uio.h>

ssize_t
sendpath(int s, char *path, off_t offset, size_t nbytes,
	 const struct iovec *hdtrl, int flags)
  {
    return syscall(SYS_sendpath, s, path, offset, nbytes, hdtrl, flags);
  }
#endif /* QUICK_SENDPATH */

/* This routine implements the TCP unidirectional data transfer test
   (a.k.a. stream) for the sockets interface using the sendfile()
   system call - TCP_SENDFILE.  It receives its  parameters via global
   variables from the shell and writes its  output to the standard
   output. Basically,  this is the same test as the send_tcp_stream()
   logic and we even tell the remote to do a TCP_STREAM test since for 
   all it knows, nothig is different. */

void 
sendfile_tcp_stream(remote_host)
     char	remote_host[];
{
  
  char *tput_title = "\
Recv   Send    Send                          \n\
Socket Socket  Message  Elapsed              \n\
Size   Size    Size     Time     Throughput  \n\
bytes  bytes   bytes    secs.    %s/sec  \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1 =
    "%6d %6d %6d    %-6.2f   %7.2f   \n";

  char *cpu_title = "\
Recv   Send    Send                          Utilization       Service Demand\n\
Socket Socket  Message  Elapsed              Send     Recv     Send    Recv\n\
Size   Size    Size     Time     Throughput  local    remote   local   remote\n\
bytes  bytes   bytes    secs.    %-8.8s/s  %% %c      %% %c      us/KB   us/KB\n\n";

  char *cpu_fmt_0 =
    "%6.3f %c\n";
  char *cpu_fmt_1 =
    "%6d %6d %6d    %-6.2f     %7.2f   %-6.2f   %-6.2f   %-6.3f  %-6.3f\n";

  char *ksink_fmt = "\n\
Alignment      Offset         %-8.8s %-8.8s    Sends   %-8.8s Recvs\n\
Local  Remote  Local  Remote  Xfered   Per                 Per\n\
Send   Recv    Send   Recv             Send (avg)          Recv (avg)\n\
%5d   %5d  %5d   %5d %6.4g  %6.2f    %6d   %6.2f %6d\n";

char *ksink_fmt2 = "\n\
Maximum\n\
Segment\n\
Size (bytes)\n\
%6d\n";

  float			elapsed_time;
  
  /* what we want is to have a buffer space that is at least one */
  /* send-size greater than our send window. this will insure that we */
  /* are never trying to re-use a buffer that may still be in the hands */
  /* of the transport. This buffer will be malloc'd after we have found */
  /* the size of the local senc socket buffer. We will want to deal */
  /* with alignment and offset concerns as well. */
  
  struct sendfile_ring_elt *send_ring;
  
  int len;
  unsigned int nummessages = 0;
  SOCKET send_socket;
  int bytes_remaining;
  int tcp_mss = -1;  /* possibly uninitialized on printf far below */

  /* with links like fddi, one can send > 32 bits worth of bytes */
  /* during a test... ;-) at some point, this should probably become a */
  /* 64bit integral type, but those are not entirely common yet */
  double	bytes_sent = 0.0;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;

  double	thruput;
  
  struct  addrinfo *remote_res;
  struct  addrinfo *local_res;
  struct	sockaddr_in	server;

#if defined(__linux) || defined(__sun__)
  off_t     scratch_offset;   /* the linux sendfile() call will update
				 the offset variable, which is
				 something we do _not_ want to happen
				 to the value in the send_ring! so, we
				 have to use a scratch variable. */
#endif /* __linux  || defined(__sun__) */
#if defined (USE_OSX)
   off_t    scratch_len;  /* Darwin 9.x need a value-result parameter  */
#endif
#if defined (__sun__)
   size_t  scratch_len;	/* the sun sendfilev() needs a place to 
			   tell us how many bytes were written,
			   even though it also returns the value */
   sendfilevec_t sv;
#endif /* __sun__ */
  
  struct	tcp_stream_request_struct	*tcp_stream_request;
  struct	tcp_stream_response_struct	*tcp_stream_response;
  struct	tcp_stream_results_struct	*tcp_stream_result;

  tcp_stream_request  = 
    (struct tcp_stream_request_struct *)netperf_request.content.test_specific_data;
  tcp_stream_response =
    (struct tcp_stream_response_struct *)netperf_response.content.test_specific_data;
  tcp_stream_result   = 
    (struct tcp_stream_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);

  if ( print_headers ) {
    /* we want to have some additional, interesting information in */
    /* the headers. we know some of it here, but not all, so we will */
    /* only print the test title here and will print the results */
    /* titles after the test is finished */
#ifdef QUICK_SENDPATH
    print_top_test_header("TCP SENDPATH TEST",local_res,remote_res);
#else
    print_top_test_header("TCP SENDFILE TEST",local_res,remote_res);
#endif /* QUICK_SENDPATH */
  }
  send_ring = NULL;
  confidence_iteration = 1;
  init_stat();
  
  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {
    
    /* initialize a few counters. we have to remember that we might be */
    /* going through the loop more than once. */
    
    nummessages    =	0;
    bytes_sent     =	0.0;
    times_up       = 	0;
    
    /* set up the data socket */
    send_socket = create_data_socket(local_res);

    if (send_socket == INVALID_SOCKET){
      perror("netperf: sendfile_tcp_stream: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"sendfile_tcp_stream: send_socket obtained...\n");
    }
    
#if defined(TCP_CORK)
    /* should this even be here?!? */
    if (loc_tcpcork != 0) {
      /* the user wishes for us to set TCP_CORK on the socket */
      int one = 1;
      if (setsockopt(send_socket,
		     getprotobyname("tcp")->p_proto,
		     TCP_CORK,
		     (char *)&one,
		     sizeof(one)) == SOCKET_ERROR) {
	perror("netperf: sendfile_tcp_stream: tcp_cork");
	exit(1);
      }
      if (debug) {
	fprintf(where,"sendfile_tcp_stream: tcp_cork...\n");
      }
    }
    
#endif /* TCP_CORK */    

    /* at this point, we have either retrieved the socket buffer sizes, */
    /* or have tried to set them, so now, we may want to set the send */
    /* size based on that (because the user either did not use a -m */
    /* option, or used one with an argument of 0). If the socket buffer */
    /* size is not available, we will set the send size to 4KB - no */
    /* particular reason, just arbitrary... */

    /*check for file size/ min file size here?  create file here/ back out???*/
    
    if (send_size == 0) {
      if (lss_size > 0) {
	send_size = lss_size;
      }
      else {
	send_size = 4096;
      }
    }
   
    /* set-up the data buffer ring with the requested alignment and
       offset. note also that we have allocated a quantity  of memory
       that is at least one send-size greater than our socket  buffer
       size. We want to be sure that there are at least two  buffers
       allocated - this can be a bit of a problem when the  send_size
       is bigger than the socket size, so we must check... the  user
       may have wanted to explicitly set the "width" of our send
       buffers, we should respect that wish... */ 

    /*sendring -> an offset index that will shift the starting point of the*/
    /*section of the file sent throughout the file*/
    
    if (send_width == 0) {
      send_width = (lss_size/send_size) + 1;
      if (send_width == 1) send_width++;
    }
    
    if (send_ring == NULL) { 

      /* only allocate the send ring once. this is a networking test,
	 not a memory allocation test. this way, we do not need a
	 deallocate_buffer_ring() routine, and I don't feel like
	 writing one anyway :) raj 11/94 */ 
      
      send_ring = alloc_sendfile_buf_ring(send_width,
					  send_size,
					  local_send_align,
					  local_send_offset);
    }
	
    /* If the user has requested cpu utilization measurements, we must
       calibrate the cpu(s). We will perform this task within the
       tests  themselves. If the user has specified the cpu rate, then
       calibrate_local_cpu will return rather quickly as it will have
       nothing to do. If local_cpu_rate is zero, then we will go
       through  all the "normal" calibration stuff and return the rate
       back. */ 
    
    if (local_cpu_usage) {
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    }

    /* Tell the remote end to do a listen. The server alters the
       socket  paramters on the other side at this point, hence the
       reason for  all the values being passed in the setup
       message. If the user did  not specify any of the parameters,
       they will be passed as 0, which  will indicate to the remote
       that no changes beyond the system's  default should be
       used. Alignment is the exception, it will  default to 1, which
       will be no alignment alterations. */ 

    netperf_request.content.request_type =	DO_TCP_STREAM;
    tcp_stream_request->send_buf_size	=	rss_size_req;
    tcp_stream_request->recv_buf_size	=	rsr_size_req;
    tcp_stream_request->receive_size	=	recv_size;
    tcp_stream_request->no_delay	=	rem_nodelay;
    tcp_stream_request->recv_alignment	=	remote_recv_align;
    tcp_stream_request->recv_offset	=	remote_recv_offset;
    tcp_stream_request->measure_cpu	=	remote_cpu_usage;
    tcp_stream_request->cpu_rate	=	remote_cpu_rate;

    if (test_time) {
      tcp_stream_request->test_length	=	test_time;
    }
    else {
      tcp_stream_request->test_length	=	test_bytes;
    }

    tcp_stream_request->so_rcvavoid	=	rem_rcvavoid;
    tcp_stream_request->so_sndavoid	=	rem_sndavoid;

#ifdef DIRTY
    tcp_stream_request->dirty_count       =       rem_dirty_count;
    tcp_stream_request->clean_count       =       rem_clean_count;
#endif /* DIRTY */
    tcp_stream_request->port     = atoi(remote_data_port);
    tcp_stream_request->ipfamily = af_to_nf(remote_res->ai_family);

    if (debug > 1) {
      fprintf(where,
	      "netperf: send_tcp_stream: requesting TCP stream test\n");
    }
    
    send_request();

    /* The response from the remote will contain all of the relevant
       socket parameters for this test type. We will put them back
       into the variables here so they can be displayed if desired.
       The remote will have calibrated CPU if necessary, and will have
       done all the needed set-up we will have calibrated the cpu
       locally before sending the request, and will grab the counter
       value right after the connect returns. The remote will grab the
       counter right after the accept call. This saves the hassle of
       extra messages being sent for the TCP tests.  */
    
    recv_response();

    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote listen done.\n");
      rsr_size	      =	tcp_stream_response->recv_buf_size;
      rss_size	      =	tcp_stream_response->send_buf_size;
      rem_nodelay     =	tcp_stream_response->no_delay;
      remote_cpu_usage=	tcp_stream_response->measure_cpu;
      remote_cpu_rate = tcp_stream_response->cpu_rate;

      /* we have to make sure that the server port number is in */
      /* network order */
      set_port_number(remote_res,(short)tcp_stream_response->data_port_number);
      rem_rcvavoid	= tcp_stream_response->so_rcvavoid;
      rem_sndavoid	= tcp_stream_response->so_sndavoid;
    }
    else {
      Set_errno(netperf_response.content.serv_errno);
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      
      exit(1);
    }

#ifdef WANT_DEMO
    DEMO_STREAM_SETUP(lss_size,rsr_size)
#endif

    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		remote_res->ai_addr,
		remote_res->ai_addrlen) == INVALID_SOCKET){
      perror("netperf: send_tcp_stream: data socket connect failed");
      printf(" port: %d\n",ntohs(server.sin_port));
      exit(1);
    }

    /* Data Socket set-up is finished. If there were problems, either
       the connect would have failed, or the previous response would
       have indicated a problem. I failed to see the value of the
       extra message after the accept on the remote. If it failed,
       we'll see it here. If it didn't, we might as well start pumping
       data. */
    
    /* Set-up the test end conditions. For a stream test, they can be */
    /* either time or byte-count based. */
    
    if (test_time) {
      /* The user wanted to end the test after a period of time. */
      times_up = 0;
      bytes_remaining = 0;

      /* in previous revisions, we had the same code repeated throught
         all the test suites. this was unnecessary, and meant more
         work for me when I wanted to switch to POSIX signals, so I
         have abstracted this out into a routine in netlib.c. if you
         are experiencing signal problems, you might want to look
         there. raj 11/94 */

      start_timer(test_time);
    }
    else {
      /* The tester wanted to send a number of bytes. */
      bytes_remaining = test_bytes;
      times_up = 1;
    }
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);
    
#ifdef WANT_INTERVALS
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */

   
    /* before we start, initialize a few variables */

#ifdef WANT_DEMO
    if (demo_mode) {
      HIST_timestamp(demo_one_ptr);
    }
#endif

    /* We use an "OR" to control test execution. When the test is
       controlled by time, the byte count check will always return
       false. When the test is controlled by byte count, the time test
       will always return false. When the test is finished, the whole
       expression will go false and we will stop sending data. */
    
    while ((!times_up) || (bytes_remaining > 0)) {
      
      /* the sendfile_tcp_stream test does not support making the buffers
	 dirty. 08/2000 */
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp just before we go into sendfile() and then again
         just after we come out raj 08/2000 */
	/* but only if we are actually going to display a histogram */
	HIST_timestamp(&time_one);
      }
#endif /* WANT_HISTOGRAM */
      
      /* you can look at netlib.h for a description of the fields we
	 are passing to sendfile(). 08/2000 */
#ifdef QUICK_SENDPATH
      if ((len=sendpath(send_socket, 
			fill_file,
			send_ring->offset,
			send_ring->length,
			send_ring->hdtrl,
			send_ring->flags)) != send_size)
#elif defined(__linux)
	scratch_offset = send_ring->offset;
      if ((len=sendfile(send_socket, 
			send_ring->fildes, 
			&scratch_offset,   /* modified after the call! */
			send_ring->length)) != send_size)
#elif defined (__sun__)
      /* We must call with SFV_NOWAIT and a large file size (>= 16MB) to
	 get zero-copy, as well as compiling with  -D_LARGEFILE_SOURCE
	  -D_FILE_OFFSET_BITS=64 */
      sv.sfv_fd = send_ring->fildes;
      sv.sfv_flag = SFV_NOWAIT;
      sv.sfv_off = send_ring->offset;
      sv.sfv_len =  send_ring->length;
      if ((len = sendfilev(send_socket, &sv, 1, &scratch_len)) != send_size)
#elif defined(__FreeBSD__)
	/* so close to HP-UX and yet so far away... :) */
	if ((sendfile(send_ring->fildes, 
		      send_socket, 
		      send_ring->offset,
		      send_ring->length,
		      NULL,
		      (off_t *)&len,
		      send_ring->flags) != 0) ||
	    (len != send_size))
#elif defined(USE_OSX)
    scratch_len = send_ring->length;
    if ((sendfile(send_ring->fildes,
              send_socket,
              send_ring->offset,
              (off_t *)&scratch_len,
              NULL,
              send_ring->flags) != 0) ||
        (scratch_len != send_size))
#else /* original sendile HP-UX */
	  if ((len=sendfile(send_socket, 
			    send_ring->fildes, 
			    send_ring->offset,
			    send_ring->length,
			    send_ring->hdtrl,
			    send_ring->flags)) != send_size)
#endif /* QUICK_SENDPATH */
	    {
	      /* the test was interrupted, must be the end of test. the
		 send_tcp_stream code has some WIN32 ifdefs that we do not
		 need here. */
	      if ((len >=0) || SOCKET_EINTR(len)) {
		break;
	      }
	      perror("netperf: data send error: sendfile");
	      fprintf(stderr,
		      "len was %d send_size was %d\n",
		      len,
		      send_size);
	      fflush(stderr);
	      exit(1);
	    }
      
      /*	offset += len;*/
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp the exit from the send call and update the
	   histogram */
      
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */      
    
#ifdef WANT_DEMO
      DEMO_STREAM_INTERVAL(send_size);
#endif 
  
#ifdef WANT_INTERVALS      
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
      /* now we want to move our pointer to the next position in the */
      /* data buffer...we may also want to wrap back to the "beginning" */
      /* of the bufferspace, so we will mod the number of messages sent */
      /* by the send width, and use that to calculate the offset to add */
      /* to the base pointer. */

      nummessages++;          
      send_ring = send_ring->next; 
      if (bytes_remaining) {
	bytes_remaining -= send_size;
      }
    }

    /* The test is over. Flush the buffers to the remote end. We do a
       graceful release to insure that all data has been taken by the
       remote. */

    /* but first, if the verbosity is greater than 1, find-out what */
    /* the TCP maximum segment_size was (if possible) */
    if (verbosity > 1) {
      tcp_mss = -1;
      get_tcp_info(send_socket,&tcp_mss);
    }
    
    if (shutdown(send_socket,SHUT_WR) == SOCKET_ERROR) {
      perror("netperf: cannot shutdown tcp stream socket");
      exit(1);
    }
    
    /* hang a recv() off the socket to block until the remote has */
    /* brought all the data up into the application. it will do a */
    /* shutdown to cause a FIN to be sent our way. We will assume that */
    /* any exit from the recv() call is good... raj 4/93 */

    /* since we are using sendfile() instead of send, we have no
       scratch buffer from the send_ring to use for the
       receive. however, since we "know" that the recv should be
       returning zero bytes (not that we are making the checks we
       should) we can pass the address of the flags field. raj 08/2000 
    */

    recv(send_socket,
	 &(send_ring->flags), 
	 sizeof(send_ring->flags),
	 0);
    
    /* this call will always give us the elapsed time for the test, and */
    /* will also store-away the necessaries for cpu utilization */
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured and how */
						/* long did we really */
						/* run? */
    
    /* we are finished with the socket, so close it to prevent hitting */
    /* the limit on maximum open files. */

    close(send_socket);

    /* Get the statistics from the remote end. The remote will have */
    /* calculated service demand and all those interesting things. If it */
    /* wasn't supposed to care, it will return obvious values. */
    
    recv_response();

    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote results obtained\n");
    }

    else {
      Set_errno(netperf_response.content.serv_errno);
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      
      exit(1);
    }
    
    /* We now calculate what our thruput was for the test. In the future, */
    /* we may want to include a calculation of the thruput measured by */
    /* the remote, but it should be the case that for a TCP stream test, */
    /* that the two numbers should be *very* close... We calculate */
    /* bytes_sent regardless of the way the test length was controlled. */
    /* If it was time, we needed to, and if it was by bytes, the user may */
    /* have specified a number of bytes that wasn't a multiple of the */
    /* send_size, so we really didn't send what he asked for ;-) */
    
    bytes_sent	= ntohd(tcp_stream_result->bytes_received);
    
    thruput	= calc_thruput(bytes_sent);
    
    if (local_cpu_usage || remote_cpu_usage) {

      /* We must now do a little math for service demand and cpu */
      /* utilization for the system(s) */
      /* Of course, some of the information might be bogus because */
      /* there was no idle counter in the kernel(s). We need to make */
      /* a note of this for the user's benefit...*/
      if (local_cpu_usage) {
	
	local_cpu_utilization	= calc_cpu_util(0.0);
	local_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      0.0,
						      0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	
	remote_cpu_utilization	= tcp_stream_result->cpu_util;
	remote_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      remote_cpu_utilization,
						      tcp_stream_result->num_cpus);
      }
      else {
	remote_cpu_utilization = (float) -1.0;
	remote_service_demand  = (float) -1.0;
      }
    }    
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }

    /* at this point, we want to calculate the confidence information. */
    /* if debugging is on, calculate_confidence will print-out the */
    /* parameters we pass it */
    
    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    confidence_iteration++;
  }

  /* at this point, we have finished making all the runs that we */
  /* will be making. so, we should extract what the calcuated values */
  /* are for all the confidence stuff. we could make the values */
  /* global, but that seemed a little messy, and it did not seem worth */
  /* all the mucking with header files. so, we create a routine much */
  /* like calcualte_confidence, which just returns the mean values. */
  /* raj 11/94 */

  retrieve_confident_values(&elapsed_time,
			    &thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);

  /* We are now ready to print all the information. If the user */
  /* has specified zero-level verbosity, we will just print the */
  /* local service demand, or the remote service demand. If the */
  /* user has requested verbosity level 1, he will get the basic */
  /* "streamperf" numbers. If the user has specified a verbosity */
  /* of greater than 1, we will display a veritable plethora of */
  /* background information from outside of this block as it it */
  /* not cpu_measurement specific...  */

  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }

  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(tcp_stream_result->cpu_method);
    
    switch (verbosity) {
    case 0:

    if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method);
      }

      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		remote_cpu_method);
      }

      break;

    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,
		cpu_title,
		format_units(),
		local_cpu_method,
		remote_cpu_method);
      }
    
      fprintf(where,
	      cpu_fmt_1,		/* the format string */
	      rsr_size,		        /* remote recvbuf size */
	      lss_size,		        /* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time,		/* how long was the test */
	      thruput, 		        /* what was the xfer rate */
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand);	/* remote service demand */
      break;
    }

  }

  else {
    /* The tester did not wish to measure service demand. */
    
    switch (verbosity) {
 
    case 0:

      fprintf(where,
	      tput_fmt_0,
	      thruput);
      break;

    case 1:
    case 2:

      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }

      fprintf(where,
	      tput_fmt_1,		/* the format string */
	      rsr_size, 		/* remote recvbuf size */
	      lss_size, 		/* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time, 		/* how long did it take */
	      thruput);/* how fast did it go */
      break;
    }
  }

  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
    if (verbosity > 1) {

    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
   
    /* this stuff needs to be worked-out in the presence of confidence */
    /* intervals and multiple iterations of the test... raj 11/94 */
 
    fprintf(where,
	    ksink_fmt,
	    "Bytes",
	    "Bytes",
	    "Bytes",
	    local_send_align,
	    remote_recv_align,
	    local_send_offset,
	    remote_recv_offset,
	    bytes_sent,
	    bytes_sent / (double)nummessages,
	    nummessages,
	    bytes_sent / (double)tcp_stream_result->recv_calls,
	    tcp_stream_result->recv_calls);

    fprintf(where,
	    ksink_fmt2,
	    tcp_mss);

    fflush(where);

#ifdef WANT_HISTOGRAM

    fprintf(where,"\n\nHistogram of time spent in send() call.\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */
  }
}

#endif /* HAVE_SENDFILE */

/* This is the server-side routine for the tcp stream test. It is */
/* implemented as one routine. I could break things-out somewhat, but */
/* didn't feel it was necessary. */

void
recv_tcp_stream()
{
  
  struct sockaddr_storage myaddr_in, peeraddr_in;
  SOCKET s_listen,s_data;
  netperf_socklen_t addrlen;
  int	len;
  unsigned int	receive_calls;
  float	elapsed_time;
  double   bytes_received;
  
  struct ring_elt *recv_ring;

  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];

#ifdef DO_SELECT
  fd_set readfds;
  struct timeval timeout;
#endif /* DO_SELECT */

  struct	tcp_stream_request_struct	*tcp_stream_request;
  struct	tcp_stream_response_struct	*tcp_stream_response;
  struct	tcp_stream_results_struct	*tcp_stream_results;
  
#ifdef DO_SELECT
  FD_ZERO(&readfds);
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
#endif /* DO_SELECT */

  tcp_stream_request	= 
    (struct tcp_stream_request_struct *)netperf_request.content.test_specific_data;
  tcp_stream_response	= 
    (struct tcp_stream_response_struct *)netperf_response.content.test_specific_data;
  tcp_stream_results	= 
    (struct tcp_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_stream: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_STREAM_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug) {
    fprintf(where,"recv_tcp_stream: requested alignment of %d\n",
	    tcp_stream_request->recv_alignment);
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_stream_request->send_buf_size;
  lsr_size_req = tcp_stream_request->recv_buf_size;
  loc_nodelay  = tcp_stream_request->no_delay;
  loc_rcvavoid = tcp_stream_request->so_rcvavoid;
  loc_sndavoid = tcp_stream_request->so_sndavoid;

  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_stream_request->ipfamily),
			tcp_stream_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_stream_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
#ifdef WIN32
  /* The test timer can fire during operations on the listening socket,
     so to make the start_timer below work we have to move
     it to close s_listen while we are blocked on accept. */
  win_kludge_socket2 = s_listen;
#endif
  
  /* what sort of sizes did we end-up with? */
  if (tcp_stream_request->receive_size == 0) {
    if (lsr_size > 0) {
      recv_size = lsr_size;
    }
    else {
      recv_size = 4096;
    }
  }
  else {
    recv_size = tcp_stream_request->receive_size;
  }
  
  /* we want to set-up our recv_ring in a manner analagous to what we */
  /* do on the sending side. this is more for the sake of symmetry */
  /* than for the needs of say copy avoidance, but it might also be */
  /* more realistic - this way one could conceivably go with a */
  /* double-buffering scheme when taking the data an putting it into */
  /* the filesystem or something like that. raj 7/94 */

  if (recv_width == 0) {
    recv_width = (lsr_size/recv_size) + 1;
    if (recv_width == 1) recv_width++;
  }

  recv_ring = allocate_buffer_ring(recv_width,
				   recv_size,
				   tcp_stream_request->recv_alignment,
				   tcp_stream_request->recv_offset);

  if (debug) {
    fprintf(where,"recv_tcp_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_stream_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  tcp_stream_response->cpu_rate = (float)0.0; 	/* assume no cpu */
  if (tcp_stream_request->measure_cpu) {
    tcp_stream_response->measure_cpu = 1;
    tcp_stream_response->cpu_rate = 
      calibrate_local_cpu(tcp_stream_request->cpu_rate);
  }
  else {
    tcp_stream_response->measure_cpu = 0;
  }
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_stream_response->send_buf_size = lss_size;
  tcp_stream_response->recv_buf_size = lsr_size;
  tcp_stream_response->no_delay = loc_nodelay;
  tcp_stream_response->so_rcvavoid = loc_rcvavoid;
  tcp_stream_response->so_sndavoid = loc_sndavoid;
  tcp_stream_response->receive_size = recv_size;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  if ((s_data=accept(s_listen,
		     (struct sockaddr *)&peeraddr_in,
		     &addrlen)) == INVALID_SOCKET) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    exit(1);
  }

#ifdef KLUDGE_SOCKET_OPTIONS
  /* this is for those systems which *INCORRECTLY* fail to pass */
  /* attributes across an accept() call. Including this goes against */
  /* my better judgement :( raj 11/95 */

  kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_stream_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  /* there used to be an #ifdef DIRTY call to access_buffer() here,
     but we have switched from accessing the buffer before the recv()
     call to accessing the buffer after the recv() call.  The
     accessing before was, IIRC, related to having dirty data when
     doing page-flipping copy avoidance. */

  bytes_received = 0;
  receive_calls  = 0;

  while ((len = recv(s_data, recv_ring->buffer_ptr, recv_size, 0)) != 0) {
    if (len == SOCKET_ERROR )
	{
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
    bytes_received += len;
    receive_calls++;

#ifdef DIRTY
    /* we access the buffer after the recv() call now, rather than before */
    access_buffer(recv_ring->buffer_ptr,
		  recv_size,
		  tcp_stream_request->dirty_count,
		  tcp_stream_request->clean_count);
#endif /* DIRTY */


    /* move to the next buffer in the recv_ring */
    recv_ring = recv_ring->next;

#ifdef PAUSE
    sleep(1);
#endif /* PAUSE */

#ifdef DO_SELECT
	FD_SET(s_data,&readfds);
	select(s_data+1,&readfds,NULL,NULL,&timeout);
#endif /* DO_SELECT */

  }
  
  /* perform a shutdown to signal the sender that */
  /* we have received all the data sent. raj 4/93 */

  if (shutdown(s_data,SHUT_WR) == SOCKET_ERROR) {
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
  
  cpu_stop(tcp_stream_request->measure_cpu,&elapsed_time);
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_stream: got %g bytes\n",
	    bytes_received);
    fprintf(where,
	    "recv_tcp_stream: got %d recvs\n",
	    receive_calls);
    fflush(where);
  }
  
  tcp_stream_results->bytes_received	= htond(bytes_received);
  tcp_stream_results->elapsed_time	= elapsed_time;
  tcp_stream_results->recv_calls	= receive_calls;
  
  tcp_stream_results->cpu_method = cpu_method;
  tcp_stream_results->num_cpus   = lib_num_loc_cpus;
  
  if (tcp_stream_request->measure_cpu) {
    tcp_stream_results->cpu_util	= calc_cpu_util(0.0);
  };
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_stream: test complete, sending results.\n");
    fprintf(where,
	    "                 bytes_received %g receive_calls %d\n",
	    bytes_received,
	    receive_calls);
    fprintf(where,
	    "                 len %d\n",
	    len);
    fflush(where);
  }

  send_response();

  /* we are now done with the sockets */
  close(s_data);
  close(s_listen);

  }

/* This is the server-side routine for the tcp maerts test. It is
   implemented as one routine. I could break things-out somewhat, but
   didn't feel it was necessary. */ 

void
recv_tcp_maerts()
{
  
  struct sockaddr_storage myaddr_in, peeraddr_in;
  struct addrinfo *local_res;
  char  local_name[BUFSIZ];
  char  port_buffer[PORTBUFSIZE];

  SOCKET	s_listen,s_data;
  netperf_socklen_t 	addrlen;
  int	len;
  unsigned int	send_calls;
  float	elapsed_time;
  double   bytes_sent = 0.0 ;
  
  struct ring_elt *send_ring;

  struct	tcp_maerts_request_struct	*tcp_maerts_request;
  struct	tcp_maerts_response_struct	*tcp_maerts_response;
  struct	tcp_maerts_results_struct	*tcp_maerts_results;
  
  tcp_maerts_request	= 
    (struct tcp_maerts_request_struct *)netperf_request.content.test_specific_data;
  tcp_maerts_response	= 
    (struct tcp_maerts_response_struct *)netperf_response.content.test_specific_data;
  tcp_maerts_results	= 
    (struct tcp_maerts_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_maerts: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired
     parameters and then let the initiator know that all is ready. If
     socket size defaults are to be used, then the initiator will have
     sent us 0's. If the socket sizes cannot be changed, then we will
     send-back what they are. If that information cannot be
     determined, then we send-back -1's for the sizes. If things go
     wrong for any reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It
     would be best if the error that the remote reports to the user is
     the actual error we encountered, rather than some bogus
     unexpected response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_maerts: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_MAERTS_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_maerts: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug) {
    fprintf(where,"recv_tcp_maerts: requested alignment of %d\n",
	    tcp_maerts_request->send_alignment);
    fflush(where);
  }

  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_maerts: grabbing a socket...\n");
    fflush(where);
  }
  
  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_maerts_request->send_buf_size;
  lsr_size_req = tcp_maerts_request->recv_buf_size;
  loc_nodelay = tcp_maerts_request->no_delay;
  loc_rcvavoid = tcp_maerts_request->so_rcvavoid;
  loc_sndavoid = tcp_maerts_request->so_sndavoid;

  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_maerts_request->ipfamily),
			tcp_maerts_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_maerts_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
#ifdef WIN32
  /* The test timer can fire during operations on the listening socket,
     so to make the start_timer below work we have to move
     it to close s_listen while we are blocked on accept. */
  win_kludge_socket2 = s_listen;
#endif

  
  /* what sort of sizes did we end-up with? */
  if (tcp_maerts_request->send_size == 0) {
    if (lss_size > 0) {
      send_size = lss_size;
    }
    else {
      send_size = 4096;
    }
  }
  else {
    send_size = tcp_maerts_request->send_size;
  }
  
  /* we want to set-up our recv_ring in a manner analagous to what we */
  /* do on the recving side. this is more for the sake of symmetry */
  /* than for the needs of say copy avoidance, but it might also be */
  /* more realistic - this way one could conceivably go with a */
  /* double-buffering scheme when taking the data an putting it into */
  /* the filesystem or something like that. raj 7/94 */

  if (send_width == 0) {
    send_width = (lsr_size/send_size) + 1;
    if (send_width == 1) send_width++;
  }

  send_ring = allocate_buffer_ring(send_width,
				   send_size,
				   tcp_maerts_request->send_alignment,
				   tcp_maerts_request->send_offset);

  if (debug) {
    fprintf(where,"recv_tcp_maerts: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_maerts_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  tcp_maerts_response->cpu_rate = (float)0.0; 	/* assume no cpu */
  if (tcp_maerts_request->measure_cpu) {
    tcp_maerts_response->measure_cpu = 1;
    tcp_maerts_response->cpu_rate = 
      calibrate_local_cpu(tcp_maerts_request->cpu_rate);
  }
  else {
    tcp_maerts_response->measure_cpu = 0;
  }
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_maerts_response->send_buf_size = lss_size;
  tcp_maerts_response->recv_buf_size = lsr_size;
  tcp_maerts_response->no_delay = loc_nodelay;
  tcp_maerts_response->so_rcvavoid = loc_rcvavoid;
  tcp_maerts_response->so_sndavoid = loc_sndavoid;
  tcp_maerts_response->send_size = send_size;

  send_response();
  
  addrlen = sizeof(peeraddr_in);

  /* we will start the timer before the accept() to be somewhat
     analagous to the starting of the timer before the connect() call
     in the TCP_STREAM test. raj 2002-06-21 */

  start_timer(tcp_maerts_request->test_length);

  /* Now it's time to start receiving data on the connection. We will
     first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_maerts_request->measure_cpu);
  

  if ((s_data=accept(s_listen,
		     (struct sockaddr *)&peeraddr_in,
		     &addrlen)) == INVALID_SOCKET) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    exit(1);
  }

#ifdef KLUDGE_SOCKET_OPTIONS
  
  /* this is for those systems which *INCORRECTLY* fail to pass
     attributes across an accept() call. Including this goes against
     my better judgement :( raj 11/95 */

  kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  bytes_sent = 0.0;
  send_calls  = 0;

  len = 0;   /* nt-lint; len is not initialized (printf far below) if
		times_up initially true.*/
  times_up = 0; /* must remember to initialize this little beauty */
  while (!times_up) {

#ifdef DIRTY
    /* we want to dirty some number of consecutive integers in the buffer */
    /* we are about to send. we may also want to bring some number of */
    /* them cleanly into the cache. The clean ones will follow any dirty */
    /* ones into the cache. */

  access_buffer(send_ring->buffer_ptr,
		send_size,
		tcp_maerts_request->dirty_count,
		tcp_maerts_request->clean_count);

#endif /* DIRTY */

    if((len=send(s_data,
		 send_ring->buffer_ptr,
		 send_size,
		 0)) != send_size) {
		if ((len >=0) || SOCKET_EINTR(len)) {
	      /* the test was interrupted, must be the end of test */
	      break;
		}
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }

    bytes_sent += len;
    send_calls++;

    /* more to the next buffer in the send_ring */
    send_ring = send_ring->next;

  }
  
  /* perform a shutdown to signal the sender that */
  /* we have received all the data sent. raj 4/93 */

  if (shutdown(s_data,SHUT_WR) == SOCKET_ERROR) {
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }

  /* hang a recv() off the socket to block until the remote has
     brought all the data up into the application. it will do a
     shutdown to cause a FIN to be sent our way. We will assume that
     any exit from the recv() call is good... raj 4/93 */
    
  recv(s_data, send_ring->buffer_ptr, send_size, 0);
    
  
  cpu_stop(tcp_maerts_request->measure_cpu,&elapsed_time);
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_maerts: got %g bytes\n",
	    bytes_sent);
    fprintf(where,
	    "recv_tcp_maerts: got %d sends\n",
	    send_calls);
    fflush(where);
  }
  
  tcp_maerts_results->bytes_sent	= htond(bytes_sent);
  tcp_maerts_results->elapsed_time	= elapsed_time;
  tcp_maerts_results->send_calls	= send_calls;
  
  if (tcp_maerts_request->measure_cpu) {
    tcp_maerts_results->cpu_util	= calc_cpu_util(0.0);
  };
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_maerts: test complete, sending results.\n");
    fprintf(where,
	    "                 bytes_sent %g send_calls %d\n",
	    bytes_sent,
	    send_calls);
    fprintf(where,
	    "                 len %d\n",
	    len);
    fflush(where);
  }
  
  tcp_maerts_results->cpu_method = cpu_method;
  tcp_maerts_results->num_cpus   = lib_num_loc_cpus;
  send_response();

  /* we are now done with the sockets */
  close(s_data);
  close(s_listen);

  }


 /* this routine implements the sending (netperf) side of the TCP_RR */
 /* test. */

void
send_tcp_rr(char remote_host[])
{
  
  char *tput_title = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
bytes  Bytes  bytes    bytes   secs.    per sec   \n\n";

  char *tput_title_band = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  \n\
Send   Recv   Size     Size    Time     Throughput \n\
bytes  Bytes  bytes    bytes   secs.    %s/sec   \n\n";

  char *tput_fmt_0 =
    "%7.2f %s\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   %s\n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %% %c    %% %c    us/Tr   us/Tr\n\n";

  char *cpu_title_tput = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Tput     CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    %-8.8s local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %% %c    %% %c    us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f %c %s\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6d %-6d %-6d  %-6d %-6.2f  %-6.2f  %-6.2f %-6.2f %-6.3f  %-6.3f %s\n";
  
  char *cpu_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *ksink_fmt = "\
Alignment      Offset         RoundTrip  Trans    Throughput\n\
Local  Remote  Local  Remote  Latency    Rate     %-8.8s/s\n\
Send   Recv    Send   Recv    usec/Tran  per sec  Outbound   Inbound\n\
%5d  %5d   %5d  %5d   %-6.3f   %-6.3f %-6.3f    %-6.3f\n";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  
  int	len;
  char	*temp_message_ptr;
  int	nummessages;
  SOCKET	send_socket;
  int	trans_remaining;
  double	bytes_xferd;

  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;
  
  int	rsp_bytes_left;
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct addrinfo *local_res;
  struct addrinfo *remote_res;

  struct	tcp_rr_request_struct	*tcp_rr_request;
  struct	tcp_rr_response_struct	*tcp_rr_response;
  struct	tcp_rr_results_struct	*tcp_rr_result;

#ifdef WANT_FIRST_BURST
#define REQUEST_CWND_INITIAL 2
  /* "in the beginning..." the WANT_FIRST_BURST stuff was like both
     Unix and the state of New Jersey - both were simple an unspoiled.
     then it was realized that some stacks are quite picky about
     initial congestion windows and a non-trivial initial burst of
     requests would not be individual segments even with TCP_NODELAY
     set. so, we have to start tracking a poor-man's congestion window
     up here in window space because we want to try to make something
     happen that frankly, we cannot guarantee with the specification
     of TCP.  ain't that grand?-)  raj 2006-01-30 */
  int requests_outstanding = 0;
  int request_cwnd = REQUEST_CWND_INITIAL;  /* we ass-u-me that having
					       three requests
					       outstanding at the
					       beginning of the test
					       is ok with TCP stacks
					       of interest. the first
					       two will come from our
					       first_burst loop, and
					       the third from our
					       regularly scheduled
					       send */
#endif

  tcp_rr_request = 
    (struct tcp_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_rr_response=
    (struct tcp_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_rr_result	=
    (struct tcp_rr_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */

  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);

  if ( print_headers ) {
    print_top_test_header("TCP REQUEST/RESPONSE TEST",local_res,remote_res);
  }
  
  /* initialize a few counters */
  
  send_ring = NULL;
  recv_ring = NULL;
  confidence_iteration = 1;
  init_stat();

  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {

    /* initialize a few counters. we have to remember that we might be */
    /* going through the loop more than once. */

    nummessages     = 0;
    bytes_xferd     = 0.0;
    times_up        = 0;
    timed_out       = 0;
    trans_remaining = 0;

#ifdef WANT_FIRST_BURST
    /* we have to remember to reset the number of transactions
       outstanding and the "congestion window for each new
       iteration. raj 2006-01-31 */
    requests_outstanding = 0;
    request_cwnd = REQUEST_CWND_INITIAL;
#endif


    /* set-up the data buffers with the requested alignment and offset. */
    /* since this is a request/response test, default the send_width and */
    /* recv_width to 1 and not two raj 7/94 */

    if (send_width == 0) send_width = 1;
    if (recv_width == 0) recv_width = 1;
  
    if (send_ring == NULL) {
      send_ring = allocate_buffer_ring(send_width,
				       req_size,
				       local_send_align,
				       local_send_offset);
    }

    if (recv_ring == NULL) {
      recv_ring = allocate_buffer_ring(recv_width,
				       rsp_size,
				       local_recv_align,
				       local_recv_offset);
    }
    
    /*set up the data socket                        */
    send_socket = create_data_socket(local_res);
  
    if (send_socket == INVALID_SOCKET){
      perror("netperf: send_tcp_rr: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_tcp_rr: send_socket obtained...\n");
    }
  
    /* If the user has requested cpu utilization measurements, we must */
    /* calibrate the cpu(s). We will perform this task within the tests */
    /* themselves. If the user has specified the cpu rate, then */
    /* calibrate_local_cpu will return rather quickly as it will have */
    /* nothing to do. If local_cpu_rate is zero, then we will go through */
    /* all the "normal" calibration stuff and return the rate back.*/
    
    if (local_cpu_usage) {
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    }
    
    if (!no_control) {
      /* Tell the remote end to do a listen. The server alters the
	 socket paramters on the other side at this point, hence the
	 reason for all the values being passed in the setup
	 message. If the user did not specify any of the parameters,
	 they will be passed as 0, which will indicate to the remote
	 that no changes beyond the system's default should be
	 used. Alignment is the exception, it will default to 8, which
	 will be no alignment alterations. */
    
      netperf_request.content.request_type	=	DO_TCP_RR;
      tcp_rr_request->recv_buf_size	=	rsr_size_req;
      tcp_rr_request->send_buf_size	=	rss_size_req;
      tcp_rr_request->recv_alignment    =	remote_recv_align;
      tcp_rr_request->recv_offset	=	remote_recv_offset;
      tcp_rr_request->send_alignment    =	remote_send_align;
      tcp_rr_request->send_offset	=	remote_send_offset;
      tcp_rr_request->request_size	=	req_size;
      tcp_rr_request->response_size	=	rsp_size;
      tcp_rr_request->no_delay	        =	rem_nodelay;
      tcp_rr_request->measure_cpu	=	remote_cpu_usage;
      tcp_rr_request->cpu_rate	        =	remote_cpu_rate;
      tcp_rr_request->so_rcvavoid	=	rem_rcvavoid;
      tcp_rr_request->so_sndavoid	=	rem_sndavoid;
      if (test_time) {
	tcp_rr_request->test_length	=	test_time;
      }
      else {
	tcp_rr_request->test_length	=	test_trans * -1;
      }
      tcp_rr_request->port              =      atoi(remote_data_port);
      tcp_rr_request->ipfamily = af_to_nf(remote_res->ai_family);
      
      if (debug > 1) {
	fprintf(where,"netperf: send_tcp_rr: requesting TCP rr test\n");
      }
      
      send_request();
      
      /* The response from the remote will contain all of the relevant
	 socket parameters for this test type. We will put them back
	 into the variables here so they can be displayed if desired.
	 The remote will have calibrated CPU if necessary, and will
	 have done all the needed set-up we will have calibrated the
	 cpu locally before sending the request, and will grab the
	 counter value right after the connect returns. The remote
	 will grab the counter right after the accept call. This saves
	 the hassle of extra messages being sent for the TCP
	 tests.  */
  
      recv_response();
  
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote listen done.\n");
	rsr_size          = tcp_rr_response->recv_buf_size;
	rss_size          = tcp_rr_response->send_buf_size;
	rem_nodelay       = tcp_rr_response->no_delay;
	remote_cpu_usage  = tcp_rr_response->measure_cpu;
	remote_cpu_rate   = tcp_rr_response->cpu_rate;
	/* make sure that port numbers are in network order */
	set_port_number(remote_res,(short)tcp_rr_response->data_port_number);
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	
	exit(1);
      }
    }

#ifdef WANT_DEMO
    DEMO_RR_SETUP(1000)
#endif

    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		remote_res->ai_addr,
		remote_res->ai_addrlen) == INVALID_SOCKET){
      perror("netperf: data socket connect failed");
      
      exit(1);
    }
    
    /* Data Socket set-up is finished. If there were problems, either the */
    /* connect would have failed, or the previous response would have */
    /* indicated a problem. I failed to see the value of the extra */
    /* message after the accept on the remote. If it failed, we'll see it */
    /* here. If it didn't, we might as well start pumping data. */
    
    /* Set-up the test end conditions. For a request/response test, they */
    /* can be either time or transaction based. */
    
    if (test_time) {
      /* The user wanted to end the test after a period of time. */
      times_up = 0;
      trans_remaining = 0;
      start_timer(test_time);
    }
    else {
      /* The tester wanted to send a number of bytes. */
      trans_remaining = test_bytes;
      times_up = 1;
    }
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);

#ifdef WANT_INTERVALS
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */
    
    /* We use an "OR" to control test execution. When the test is */
    /* controlled by time, the byte count check will always return false. */
    /* When the test is controlled by byte count, the time test will */
    /* always return false. When the test is finished, the whole */
    /* expression will go false and we will stop sending data. I think I */
    /* just arbitrarily decrement trans_remaining for the timed test, but */
    /* will not do that just yet... One other question is whether or not */
    /* the send buffer and the receive buffer should be the same buffer. */

#ifdef WANT_DEMO
      if (demo_mode) {
	HIST_timestamp(demo_one_ptr);
      }
#endif

    while ((!times_up) || (trans_remaining > 0)) {
      /* send the request. we assume that if we use a blocking socket, */
      /* the request will be sent at one shot. */

#ifdef WANT_FIRST_BURST
      /* we can inject no more than request_cwnd, which will grow with
	 time, and no more than first_burst_size.  we don't use <= to
	 account for the "regularly scheduled" send call.  of course
	 that makes it more a "max_outstanding_ than a
	 "first_burst_size" but for now we won't fix the names. also,
	 I suspect the extra check against < first_burst_size is
	 redundant since later I expect to make sure that request_cwnd
	 can never get larger than first_burst_size, but just at the
	 moment I'm feeling like a belt and suspenders kind of
	 programmer. raj 2006-01-30 */
      while ((first_burst_size > 0) &&
	     (requests_outstanding < request_cwnd) &&
	     (requests_outstanding < first_burst_size)) {
	if (debug) {
	  fprintf(where,
		  "injecting, req_outstndng %d req_cwnd %d burst %d\n",
		  requests_outstanding,
		  request_cwnd,
		  first_burst_size);
	}
	if ((len = send(send_socket,
			send_ring->buffer_ptr,
			req_size,
			0)) != req_size) {
	  /* we should never hit the end of the test in the first burst */
	  perror("send_tcp_rr: initial burst data send error");
	  exit(-1);
	}
	requests_outstanding += 1;
      }

#endif /* WANT_FIRST_BURST */
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp just before our call to send, and then again just
	   after the receive raj 8/94 */
	/* but only if we are actually going to display one. raj
	   2007-02-07 */

	HIST_timestamp(&time_one);
      }
#endif /* WANT_HISTOGRAM */
      
      if ((len = send(send_socket,
		      send_ring->buffer_ptr,
		      req_size,
		      0)) != req_size) {
	if (SOCKET_EINTR(len) || (errno == 0)) {
	  /* we hit the end of a */
	  /* timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_tcp_rr: data send error");
	exit(1);
      }
      send_ring = send_ring->next;

#ifdef WANT_FIRST_BURST
      requests_outstanding += 1;
#endif

      /* receive the response */
      rsp_bytes_left = rsp_size;
      temp_message_ptr  = recv_ring->buffer_ptr;
      while(rsp_bytes_left > 0) {
	if((rsp_bytes_recvd=recv(send_socket,
				 temp_message_ptr,
				 rsp_bytes_left,
				 0)) == SOCKET_ERROR) {
		if ( SOCKET_EINTR(rsp_bytes_recvd) ) {
		    /* We hit the end of a timed test. */
			timed_out = 1;
			break;
		}
	  perror("send_tcp_rr: data recv error");
	  exit(1);
	}
	rsp_bytes_left -= rsp_bytes_recvd;
	temp_message_ptr  += rsp_bytes_recvd;
      }	
      recv_ring = recv_ring->next;
      
#ifdef WANT_FIRST_BURST
      /* so, since we've gotten a response back, update the
	 bookkeeping accordingly.  there is one less request
	 outstanding and we can put one more out there than before. */
      requests_outstanding -= 1;
      if (request_cwnd < first_burst_size) {
	request_cwnd += 1;
	if (debug) {
	  fprintf(where,
		  "incr req_cwnd to %d first_burst %d reqs_outstndng %d\n",
		  request_cwnd,
		  first_burst_size,
		  requests_outstanding);
	}
      }
#endif
      if (timed_out) {
	/* we may have been in a nested while loop - we need */
	/* another call to break. */
	break;
      }
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */

#ifdef WANT_DEMO
      DEMO_RR_INTERVAL(1);
#endif

#ifdef WANT_INTERVALS      
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
      nummessages++;          
      if (trans_remaining) {
	trans_remaining--;
      }
      
      if (debug > 3) {
	if ((nummessages % 100) == 0) {
	  fprintf(where,
		  "Transaction %d completed\n",
		  nummessages);
	  fflush(where);
	}
      }
    }

    /* At this point we used to call shutdown on the data socket to be
       sure all the data was delivered, but this was not germane in a
       request/response test, and it was causing the tests to "hang"
       when they were being controlled by time. So, I have replaced
       this shutdown call with a call to close that can be found later
       in the procedure. */
    
    /* this call will always give us the elapsed time for the test,
       and will also store-away the necessaries for cpu utilization */
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured? how long */
						/* did we really run? */
    
    if (!no_control) {
      /* Get the statistics from the remote end. The remote will have
	 calculated CPU utilization. If it wasn't supposed to care, it
	 will return obvious values. */ 
    
      recv_response();
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote results obtained\n");
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	exit(1);
      }
    }
    
    /* We now calculate what our "throughput" was for the test. */
  
    bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
    thruput	= nummessages/elapsed_time;
  
    if (local_cpu_usage || remote_cpu_usage) {
      /* We must now do a little math for service demand and cpu
       utilization for the system(s) Of course, some of the
       information might be bogus because there was no idle counter in
       the kernel(s). We need to make a note of this for the user's
       benefit... */
      if (local_cpu_usage) {
	local_cpu_utilization = calc_cpu_util(0.0);
 	/* since calc_service demand is doing ms/Kunit we will
	   multiply the number of transaction by 1024 to get "good"
	   numbers */
	local_service_demand  = calc_service_demand((double) nummessages*1024,
						    0.0,
						    0.0,
						    0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = tcp_rr_result->cpu_util;
	/* since calc_service demand is doing ms/Kunit we will
	   multiply the number of transaction by 1024 to get "good"
	   numbers */
	remote_service_demand = calc_service_demand((double) nummessages*1024,
						    0.0,
						    remote_cpu_utilization,
						    tcp_rr_result->num_cpus);
      }
      else {
	remote_cpu_utilization = (float) -1.0;
	remote_service_demand  = (float) -1.0;
      }
      
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }

    /* at this point, we want to calculate the confidence information.
       if debugging is on, calculate_confidence will print-out the
       parameters we pass it */
    
    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    
    confidence_iteration++;

    /* we are now done with the socket, so close it */
    close(send_socket);

  }

  retrieve_confident_values(&elapsed_time,
			    &thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);

  /* We are now ready to print all the information. If the user has
     specified zero-level verbosity, we will just print the local
     service demand, or the remote service demand. If the user has
     requested verbosity level 1, he will get the basic "streamperf"
     numbers. If the user has specified a verbosity of greater than 1,
     we will display a veritable plethora of background information
     from outside of this block as it it not cpu_measurement
     specific...  */

  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }

  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(tcp_rr_result->cpu_method);
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method,
		((print_headers) || 
		 (result_brand == NULL)) ? "" : result_brand);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		remote_cpu_method,
		((print_headers) || 
		 (result_brand == NULL)) ? "" : result_brand);
      }
      break;
    case 1:
    case 2:
      if (print_headers) {
	if ('x' == libfmt) {
	  fprintf(where,
		  cpu_title,
		  local_cpu_method,
		  remote_cpu_method);
	}
	else {
	  fprintf(where,
		  cpu_title_tput,
		  format_units(),
		  local_cpu_method,
		  remote_cpu_method);
	}	  
      }

      fprintf(where,
	      cpu_fmt_1_line_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* guess */
	      elapsed_time,		/* how long was the test */
	      ('x' == libfmt) ? thruput : 
	      calc_thruput_interval_omni(thruput * (req_size+rsp_size),
					 1.0),
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand,	/* remote service demand */
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      fprintf(where,
	      cpu_fmt_1_line_2,
	      rss_size,
	      rsr_size);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      ('x' == libfmt) ? thruput :
	      calc_thruput_interval_omni(thruput * (req_size+rsp_size),
					 1.0),
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,
		('x' == libfmt) ? tput_title : tput_title_band,
		format_units());
      }

      fprintf(where,
	      tput_fmt_1_line_1,	/* the format string */
	      lss_size,
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      /* are we trans or do we need to convert to bytes then
		 bits? at this point, thruput is in our "confident"
		 transactions per second. we can convert to a
		 bidirectional bitrate by multiplying that by the sum
		 of the req_size and rsp_size.  we pass that to
		 calc_thruput_interval_omni with an elapsed time of
		 1.0 s to get it converted to [kmg]bits/s or
		 [KMG]Bytes/s */
	      ('x' == libfmt) ?  thruput : 
	      calc_thruput_interval_omni(thruput * (req_size+rsp_size),
					 1.0),
	      ((print_headers) || 
	       (result_brand == NULL)) ? "" : result_brand);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rss_size, 		/* remote recvbuf size */
	      rsr_size);
      
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  /* how to handle the verbose information in the presence of */
  /* confidence intervals is yet to be determined... raj 11/94 */
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */

    /* normally, you might think that if we were messing about with
       the value of libfmt we would need to put it back again, but
       since this is basically the last thing we are going to do with
       it, it does not matter.  so there :) raj 2007-06-08 */
    /* if the user was asking for transactions, then we report
       megabits per sedcond for the unidirectional throughput,
       otherwise we use the desired units. */
    if ('x' == libfmt) {
      libfmt = 'm';
    }

    fprintf(where,
	    ksink_fmt,
	    format_units(),
	    local_send_align,
	    remote_recv_offset,
	    local_send_offset,
	    remote_recv_offset,
	    /* if the user has enable burst mode, we have to remember
	       to account for that in the number of transactions
	       outstanding at any one time. otherwise we will
	       underreport the latency of individual
	       transactions. learned from saf by raj 2007-06-08  */
	    (((double)1.0/thruput)*(double)1000000.0) * 
	    (double) (1+first_burst_size),
	    thruput,
	    calc_thruput_interval_omni(thruput * (double)req_size,1.0),
	    calc_thruput_interval_omni(thruput * (double)rsp_size,1.0));

#ifdef WANT_HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */

  }
  
}

void
send_udp_stream(char remote_host[])
{
  /**********************************************************************/
  /*									*/
  /*               	UDP Unidirectional Send Test                    */
  /*									*/
  /**********************************************************************/

#define UDP_LENGTH_MAX 0XFFFF - 28

  char *tput_title = "\
Socket  Message  Elapsed      Messages                \n\
Size    Size     Time         Okay Errors   Throughput\n\
bytes   bytes    secs            #      #   %s/sec\n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1 = "\
%6d  %6d   %-7.2f   %7d %6d    %7.2f\n\
%6d           %-7.2f   %7d           %7.2f\n\n";
  
  
  char *cpu_title = "\
Socket  Message  Elapsed      Messages                   CPU      Service\n\
Size    Size     Time         Okay Errors   Throughput   Util     Demand\n\
bytes   bytes    secs            #      #   %s/sec %% %c%c     us/KB\n\n";
  
  char *cpu_fmt_0 =
    "%6.2f %c\n";
  
  char *cpu_fmt_1 = "\
%6d  %6d   %-7.2f   %7d %6d    %7.1f     %-6.2f   %-6.3f\n\
%6d           %-7.2f   %7d           %7.1f     %-6.2f   %-6.3f\n\n";
  
  unsigned int	messages_recvd;
  unsigned int 	messages_sent;
  unsigned int	failed_sends;

  float	elapsed_time,  
        local_cpu_utilization,
        remote_cpu_utilization;
  
  float	 local_service_demand, remote_service_demand;
  double local_thruput, remote_thruput;
  double bytes_sent;
  double bytes_recvd;
 
 
  int	len;
  struct ring_elt *send_ring;
  SOCKET 	data_socket;
  
  unsigned int sum_messages_sent;
  unsigned int sum_messages_recvd;
  unsigned int sum_failed_sends;
  double sum_local_thruput;

  struct addrinfo *local_res;
  struct addrinfo *remote_res;
  
  struct	udp_stream_request_struct	*udp_stream_request;
  struct	udp_stream_response_struct	*udp_stream_response;
  struct	udp_stream_results_struct	*udp_stream_results;
  
  udp_stream_request	= 
    (struct udp_stream_request_struct *)netperf_request.content.test_specific_data;
  udp_stream_response	= 
    (struct udp_stream_response_struct *)netperf_response.content.test_specific_data;
  udp_stream_results	= 
    (struct udp_stream_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_DGRAM,
		     IPPROTO_UDP,
		     0);

  if ( print_headers ) {
    print_top_test_header("UDP UNIDIRECTIONAL SEND TEST",local_res,remote_res);
  }

  send_ring            = NULL;
  confidence_iteration = 1;
  init_stat();
  sum_messages_sent    = 0;
  sum_messages_recvd   = 0;
  sum_failed_sends     = 0;
  sum_local_thruput    = 0.0;

  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {
    
    /* initialize a few counters. we have to remember that we might be */
    /* going through the loop more than once. */
    messages_sent  = 0;
    messages_recvd = 0;
    failed_sends   = 0;
    times_up       = 0;
    
    /*set up the data socket			*/
    data_socket = create_data_socket(local_res);
    
    if (data_socket == INVALID_SOCKET){
      perror("udp_send: data socket");
      exit(1);
    }
    
    /* now, we want to see if we need to set the send_size */
    if (send_size == 0) {
      if (lss_size > 0) {
	send_size = (lss_size < UDP_LENGTH_MAX ? lss_size : UDP_LENGTH_MAX);
      }
      else {
	send_size = 4096;
      }
    }
    
    
    /* set-up the data buffer with the requested alignment and offset, */
    /* most of the numbers here are just a hack to pick something nice */
    /* and big in an attempt to never try to send a buffer a second time */
    /* before it leaves the node...unless the user set the width */
    /* explicitly. */
    if (send_width == 0) send_width = 32;
    
    if (send_ring == NULL ) {
      send_ring = allocate_buffer_ring(send_width,
				       send_size,
				       local_send_align,
				       local_send_offset);
    }
    
    
    /* if the user supplied a cpu rate, this call will complete rather */
    /* quickly, otherwise, the cpu rate will be retured to us for */
    /* possible display. The Library will keep it's own copy of this data */
    /* for use elsewhere. We will only display it. (Does that make it */
    /* "opaque" to us?) */
    
    if (local_cpu_usage)
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    
    if (!no_control) {
      /* Tell the remote end to set up the data connection. The server
         sends back the port number and alters the socket parameters
         there.  Of course this is a datagram service so no connection
         is actually set up, the server just sets up the socket and
         binds it. */
    
      netperf_request.content.request_type      = DO_UDP_STREAM;
      udp_stream_request->recv_buf_size  = rsr_size_req;
      udp_stream_request->message_size   = send_size;
      udp_stream_request->recv_connected = remote_connected;
      udp_stream_request->recv_alignment = remote_recv_align;
      udp_stream_request->recv_offset    = remote_recv_offset;
      udp_stream_request->measure_cpu    = remote_cpu_usage;
      udp_stream_request->cpu_rate       = remote_cpu_rate;
      udp_stream_request->test_length    = test_time;
      udp_stream_request->so_rcvavoid    = rem_rcvavoid;
      udp_stream_request->so_sndavoid    = rem_sndavoid;
      udp_stream_request->port           = atoi(remote_data_port);
      udp_stream_request->ipfamily = af_to_nf(remote_res->ai_family);
      
      send_request();
      
      recv_response();
      
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"send_udp_stream: remote data connection done.\n");
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	perror("send_udp_stream: error on remote");
	exit(1);
      }
      
      /* Place the port number returned by the remote into the sockaddr */
      /* structure so our sends can be sent to the correct place. Also get */
      /* some of the returned socket buffer information for user display. */
      
      /* make sure that port numbers are in the proper order */
      set_port_number(remote_res,(short)udp_stream_response->data_port_number);
      
      rsr_size        = udp_stream_response->recv_buf_size;
      rss_size        = udp_stream_response->send_buf_size;
      remote_cpu_rate = udp_stream_response->cpu_rate;
    }

#ifdef WANT_DEMO
    DEMO_STREAM_SETUP(lss_size,rsr_size)
#endif

    /* We "connect" up to the remote post to allow is to use the send */
    /* call instead of the sendto call. Presumeably, this is a little */
    /* simpler, and a little more efficient. I think that it also means */
    /* that we can be informed of certain things, but am not sure */
    /* yet...also, this is the way I would expect a client to behave */
    /* when talking to a server */
    if (local_connected) { 
       if (connect(data_socket,
      		   remote_res->ai_addr,
		   remote_res->ai_addrlen) == INVALID_SOCKET){
          perror("send_udp_stream: data socket connect failed");
          exit(1);
       } else if (debug) {
          fprintf(where,"send_udp_stream: connected data socket.\n");
          fflush(where);
       }
    }
    
    /* set up the timer to call us after test_time. one of these days, */
    /* it might be nice to figure-out a nice reliable way to have the */
    /* test controlled by a byte count as well, but since UDP is not */
    /* reliable, that could prove difficult. so, in the meantime, we */
    /* only allow a UDP_STREAM test to be a timed test. */
    
    if (test_time) {
      times_up = 0;
      start_timer(test_time);
    }
    else {
      fprintf(where,"Sorry, UDP_STREAM tests must be timed.\n");
      fflush(where);
    }
    
    /* Get the start count for the idle counter and the start time */
    
    cpu_start(local_cpu_usage);
    
#ifdef WANT_INTERVALS
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */

#ifdef WANT_DEMO
    if (demo_mode) {
      HIST_timestamp(demo_one_ptr);
    }
#endif

    /* Send datagrams like there was no tomorrow. at somepoint it might */
    /* be nice to set this up so that a quantity of bytes could be sent, */
    /* but we still need some sort of end of test trigger on the receive */
    /* side. that could be a select with a one second timeout, but then */
    /* if there is a test where none of the data arrives for awile and */
    /* then starts again, we would end the test too soon. something to */
    /* think about... */
    while (!times_up) {
      
#ifdef DIRTY
      /* we want to dirty some number of consecutive integers in the buffer */
      /* we are about to send. we may also want to bring some number of */
      /* them cleanly into the cache. The clean ones will follow any dirty */
      /* ones into the cache. */

      access_buffer(send_ring->buffer_ptr,
		    send_size,
		    loc_dirty_count,
		    loc_clean_count);
#endif /* DIRTY */
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_one);
      }
#endif /* WANT_HISTOGRAM */
     
      if (local_connected) { 
         len = send(data_socket,
	  	    send_ring->buffer_ptr,
		    send_size,
		    0);
      } else {
         len = sendto(data_socket,
		      send_ring->buffer_ptr,
		      send_size,
		      0,
		      remote_res->ai_addr,
		      remote_res->ai_addrlen);
      }

      if (len != send_size) {
	if ((len >= 0) || 
	    SOCKET_EINTR(len))
	  break;
	if (errno == ENOBUFS) {
	  failed_sends++;
	  continue;
	}
	perror("udp_send: data send error");
	exit(1);
      }
      messages_sent++;          
      
      /* now we want to move our pointer to the next position in the */
      /* data buffer... */
      
      send_ring = send_ring->next;
      
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* get the second timestamp */
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */

#ifdef WANT_DEMO
      DEMO_STREAM_INTERVAL(send_size)
#endif

#ifdef WANT_INTERVALS      
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
    }
    
    /* This is a timed test, so the remote will be returning to us after */
    /* a time. We should not need to send any "strange" messages to tell */
    /* the remote that the test is completed, unless we decide to add a */
    /* number of messages to the test. */
    
    /* the test is over, so get stats and stuff */
    cpu_stop(local_cpu_usage,	
	     &elapsed_time);
    
    if (!no_control) {
      /* Get the statistics from the remote end	*/
      recv_response();
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"send_udp_stream: remote results obtained\n");
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	perror("send_udp_stream: error on remote");
	exit(1);
      }
      messages_recvd = udp_stream_results->messages_recvd;
      bytes_recvd    = (double) send_size * (double) messages_recvd;
    }
    else {
      /* since there was no control connection, we've no idea what was
	 actually received. raj 2007-02-08 */
      messages_recvd = -1;
      bytes_recvd = -1.0;
    }

    bytes_sent    = (double) send_size * (double) messages_sent;
    local_thruput = calc_thruput(bytes_sent);
    
    
    /* we asume that the remote ran for as long as we did */
    
    remote_thruput = calc_thruput(bytes_recvd);
    
    /* print the results for this socket and message size */
    
    if (local_cpu_usage || remote_cpu_usage) {
      /* We must now do a little math for service demand and cpu */
      /* utilization for the system(s) We pass zeros for the local */
      /* cpu utilization and elapsed time to tell the routine to use */
      /* the libraries own values for those. */
      if (local_cpu_usage) {
	local_cpu_utilization	= calc_cpu_util(0.0);
	/* shouldn't this really be based on bytes_recvd, since that is */
	/* the effective throughput of the test? I think that it should, */
	/* so will make the change raj 11/94 */
	local_service_demand	= calc_service_demand(bytes_recvd,
						      0.0,
						      0.0,
						      0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      /* The local calculations could use variables being kept by */
      /* the local netlib routines. The remote calcuations need to */
      /* have a few things passed to them. */
      if (remote_cpu_usage) {
	remote_cpu_utilization	= udp_stream_results->cpu_util;
	remote_service_demand	= calc_service_demand(bytes_recvd,
						      0.0,
						      remote_cpu_utilization,
						      udp_stream_results->num_cpus);
      }
      else {
	remote_cpu_utilization	= (float) -1.0;
	remote_service_demand	= (float) -1.0;
      }
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization  = (float) -1.0;
      local_service_demand   = (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }
    
    /* at this point, we want to calculate the confidence information. */
    /* if debugging is on, calculate_confidence will print-out the */
    /* parameters we pass it */
    
    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 remote_thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    /* since the routine calculate_confidence is rather generic, and */
    /* we have a few other parms of interest, we will do a little work */
    /* here to caclulate their average. */
    sum_messages_sent  += messages_sent;
    sum_messages_recvd += messages_recvd;
    sum_failed_sends   += failed_sends;
    sum_local_thruput  += local_thruput;
    
    confidence_iteration++;

    /* this datapoint is done, so we don't need the socket any longer */
    close(data_socket);

  }

  /* we should reach this point once the test is finished */

  retrieve_confident_values(&elapsed_time,
			    &remote_thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);

  /* some of the interesting values aren't covered by the generic */
  /* confidence routine */
  messages_sent    = sum_messages_sent / (confidence_iteration -1);
  messages_recvd   = sum_messages_recvd / (confidence_iteration -1);
  failed_sends     = sum_failed_sends / (confidence_iteration -1);
  local_thruput    = sum_local_thruput / (confidence_iteration -1);

  /* We are now ready to print all the information. If the user */
  /* has specified zero-level verbosity, we will just print the */
  /* local service demand, or the remote service demand. If the */
  /* user has requested verbosity level 1, he will get the basic */
  /* "streamperf" numbers. If the user has specified a verbosity */
  /* of greater than 1, we will display a veritable plethora of */
  /* background information from outside of this block as it it */
  /* not cpu_measurement specific...  */
    
  
  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }

  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(udp_stream_results->cpu_method);
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		local_cpu_method);
      }
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,
		cpu_title,
		format_units(),
		local_cpu_method,
		remote_cpu_method);
      }

      fprintf(where,
	      cpu_fmt_1,		/* the format string */
	      lss_size,		        /* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time,		/* how long was the test */
	      messages_sent,
	      failed_sends,
	      local_thruput, 		/* what was the xfer rate */
	      local_cpu_utilization,	/* local cpu */
	      local_service_demand,	/* local service demand */
	      rsr_size,
	      elapsed_time,
	      messages_recvd,
	      remote_thruput,
	      remote_cpu_utilization,	/* remote cpu */
	      remote_service_demand);	/* remote service demand */
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      local_thruput);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }
      fprintf(where,
	      tput_fmt_1,		/* the format string */
	      lss_size, 		/* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time, 		/* how long did it take */
	      messages_sent,
	      failed_sends,
	      local_thruput,
	      rsr_size, 		/* remote recvbuf size */
	      elapsed_time,
	      messages_recvd,
	      remote_thruput);
      break;
    }
  }

  fflush(where);
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    fprintf(where,"\nHistogram of time spent in send() call\n");
    fflush(where);
    HIST_report(time_hist);
  }
#endif /* WANT_HISTOGRAM */

}


 /* this routine implements the receive side (netserver) of the */
 /* UDP_STREAM performance test. */

void
recv_udp_stream()
{
  struct ring_elt *recv_ring;
  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];

  struct sockaddr_storage myaddr_in;
  SOCKET	s_data;
  netperf_socklen_t 	addrlen;
  struct sockaddr_storage remote_addr;
  netperf_socklen_t remote_addrlen;

  int	len = 0;
  unsigned int	bytes_received = 0;
  float	elapsed_time;
  
  int	message_size;
  unsigned int	messages_recvd = 0;
  
  struct	udp_stream_request_struct	*udp_stream_request;
  struct	udp_stream_response_struct	*udp_stream_response;
  struct	udp_stream_results_struct	*udp_stream_results;
  
  udp_stream_request  = 
    (struct udp_stream_request_struct *)netperf_request.content.test_specific_data;
  udp_stream_response = 
    (struct udp_stream_response_struct *)netperf_response.content.test_specific_data;
  udp_stream_results  = 
    (struct udp_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_udp_stream: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug > 1) {
    fprintf(where,"recv_udp_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = UDP_STREAM_RESPONSE;
  
  if (debug > 2) {
    fprintf(where,"recv_udp_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug > 1) {
    fprintf(where,"recv_udp_stream: requested alignment of %d\n",
	    udp_stream_request->recv_alignment);
    fflush(where);
  }

  if (recv_width == 0) recv_width = 1;

  recv_ring = allocate_buffer_ring(recv_width,
				   udp_stream_request->message_size,
				   udp_stream_request->recv_alignment,
				   udp_stream_request->recv_offset);

  if (debug > 1) {
    fprintf(where,"recv_udp_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug > 1) {
    fprintf(where,"recv_udp_stream: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lsr_size_req = udp_stream_request->recv_buf_size;
  loc_rcvavoid = udp_stream_request->so_rcvavoid;
  loc_sndavoid = udp_stream_request->so_sndavoid;
  local_connected = udp_stream_request->recv_connected;

  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(udp_stream_request->ipfamily),
			udp_stream_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(udp_stream_request->ipfamily),
				SOCK_DGRAM,
				IPPROTO_UDP,
				0);

  s_data = create_data_socket(local_res);
  
  if (s_data == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }

  udp_stream_response->test_length = udp_stream_request->test_length;
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_data, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_data);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  udp_stream_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  udp_stream_response->cpu_rate    = (float)0.0; /* assume no cpu */
  udp_stream_response->measure_cpu = 0;
  if (udp_stream_request->measure_cpu) {
    /* We will pass the rate into the calibration routine. If the */
    /* user did not specify one, it will be 0.0, and we will do a */
    /* "real" calibration. Otherwise, all it will really do is */
    /* store it away... */
    udp_stream_response->measure_cpu = 1;
    udp_stream_response->cpu_rate = 
      calibrate_local_cpu(udp_stream_request->cpu_rate);
  }
  
  message_size	= udp_stream_request->message_size;
  test_time	= udp_stream_request->test_length;
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  udp_stream_response->send_buf_size = lss_size;
  udp_stream_response->recv_buf_size = lsr_size;
  udp_stream_response->so_rcvavoid = loc_rcvavoid;
  udp_stream_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(udp_stream_request->measure_cpu);
  
#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */
  
  /* The loop will exit when the timer pops, or if we happen to recv a */
  /* message of less than send_size bytes... */
  
  times_up = 0;

  start_timer(test_time + PAD_TIME);

  if (debug) {
    fprintf(where,"recv_udp_stream: about to enter inner sanctum.\n");
    fflush(where);
  }

  /* We "connect" up to the remote post to allow us to use the recv */
  /* call instead of the recvfrom call. Presumeably, this is a little */
  /* simpler, and a little more efficient. */
 
  if (local_connected) {

    /* Receive the first message using recvfrom to find the remote address */
    remote_addrlen = sizeof(remote_addr);
    len = recvfrom(s_data, recv_ring->buffer_ptr,
                   message_size, 0,
                   (struct sockaddr*)&remote_addr, &remote_addrlen);
    if (len != message_size) {
      if ((len == SOCKET_ERROR) && !SOCKET_EINTR(len)) {
            netperf_response.content.serv_errno = errno;
            send_response();
            exit(1);
      }
    }
    messages_recvd++;
    recv_ring = recv_ring->next;


    /* Now connect with the remote socket address */
    if (connect(s_data,
                (struct sockaddr*)&remote_addr,
                remote_addrlen )== INVALID_SOCKET) {
        netperf_response.content.serv_errno = errno;
        close(s_data);
        send_response();
        exit(1);
    }

    if (debug) {
        fprintf(where,"recv_udp_stream: connected data socket\n");
        fflush(where);
     }
  }
  
  while (!times_up) {
    if(local_connected) {
       len = recv(s_data,
                  recv_ring->buffer_ptr,
                  message_size,
                  0);
    } else {
       len = recvfrom(s_data,
                      recv_ring->buffer_ptr,
    	              message_size, 
		      0,0,0);
    }
       
    if (len != message_size) {
      if ((len == SOCKET_ERROR) && !SOCKET_EINTR(len)) {
            netperf_response.content.serv_errno = errno;
	    send_response();
	    exit(1);
      }
      break;
    }
    messages_recvd++;
    recv_ring = recv_ring->next;
  }
  
  if (debug) {
    fprintf(where,"recv_udp_stream: got %d messages.\n",messages_recvd);
    fflush(where);
  }
  
  
  /* The loop now exits due timer or < send_size bytes received. in */
  /* reality, we only really support a timed UDP_STREAM test. raj */
  /* 12/95 */
  
  cpu_stop(udp_stream_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended on a timer, subtract the PAD_TIME */
    elapsed_time -= (float)PAD_TIME;
  }
  else {
    stop_timer();
  }
  
  if (debug) {
    fprintf(where,"recv_udp_stream: test ended in %f seconds.\n",elapsed_time);
    fflush(where);
  }
  
  
  /* We will count the "off" message that got us out of the loop */
  bytes_received = (messages_recvd * message_size) + len;
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_udp_stream: got %d bytes\n",
	    bytes_received);
    fflush(where);
  }
  
  netperf_response.content.response_type	= UDP_STREAM_RESULTS;
  udp_stream_results->bytes_received	= htonl(bytes_received);
  udp_stream_results->messages_recvd	= messages_recvd;
  udp_stream_results->elapsed_time	= elapsed_time;
  udp_stream_results->cpu_method        = cpu_method;
  udp_stream_results->num_cpus          = lib_num_loc_cpus;
  if (udp_stream_request->measure_cpu) {
    udp_stream_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  else {
    udp_stream_results->cpu_util	= (float) -1.0;
  }
  
  if (debug > 1) {
    fprintf(where,
	    "recv_udp_stream: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();

  close(s_data);

}

void
send_udp_rr(char remote_host[])
{
  
  char *tput_title = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
bytes  Bytes  bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %% %c    %% %c    us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f %c\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6d %-6d %-6d  %-6d %-6.2f  %-6.2f   %-6.2f %-6.2f %-6.3f  %-6.3f\n";
  
  char *cpu_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  float			elapsed_time;
  
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;

  int	len;
  int	nummessages;
  SOCKET	send_socket;
  int	trans_remaining;
  int	bytes_xferd;
  
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct addrinfo *local_res;
  struct addrinfo *remote_res;
  
  struct	udp_rr_request_struct	*udp_rr_request;
  struct	udp_rr_response_struct	*udp_rr_response;
  struct	udp_rr_results_struct	*udp_rr_result;

  udp_rr_request  =
    (struct udp_rr_request_struct *)netperf_request.content.test_specific_data;
  udp_rr_response =
    (struct udp_rr_response_struct *)netperf_response.content.test_specific_data;
  udp_rr_result	 =
    (struct udp_rr_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_DGRAM,
		     IPPROTO_UDP,
		     0);

  if ( print_headers ) {
    print_top_test_header("UDP REQUEST/RESPONSE TEST",local_res,remote_res);
  }

  /* initialize a few counters */
  
  send_ring     = NULL;
  recv_ring     = NULL;
  nummessages	= 0;
  bytes_xferd	= 0;
  times_up 	= 0;
  confidence_iteration = 1;
  init_stat();

  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {
    
    nummessages     = 0;
    bytes_xferd     = 0;
    times_up        = 0;
    trans_remaining = 0;
    
    /* set-up the data buffers with the requested alignment and offset */
    
    if (send_width == 0) send_width = 1;
    if (recv_width == 0) recv_width = 1;
    
    if (send_ring == NULL) {
      send_ring = allocate_buffer_ring(send_width,
				       req_size,
				       local_send_align,
				       local_send_offset);
    }
    
    if (recv_ring == NULL) {
      recv_ring = allocate_buffer_ring(recv_width,
				       rsp_size,
				       local_recv_align,
				       local_recv_offset);
    }
    
    /*set up the data socket                        */
    send_socket = create_data_socket(local_res);
    
    if (send_socket == INVALID_SOCKET){
      perror("netperf: send_udp_rr: udp rr data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_udp_rr: send_socket obtained...\n");
    }
    
    /* If the user has requested cpu utilization measurements, we must */
    /* calibrate the cpu(s). We will perform this task within the tests */
    /* themselves. If the user has specified the cpu rate, then */
    /* calibrate_local_cpu will return rather quickly as it will have */
    /* nothing to do. If local_cpu_rate is zero, then we will go through */
    /* all the "normal" calibration stuff and return the rate back. If */
    /* there is no idle counter in the kernel idle loop, the */
    /* local_cpu_rate will be set to -1. */
    
    if (local_cpu_usage) {
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    }
    
    if (!no_control) {
      /* Tell the remote end to do a listen. The server alters the
	 socket paramters on the other side at this point, hence the
	 reason for all the values being passed in the setup
	 message. If the user did not specify any of the parameters,
	 they will be passed as 0, which will indicate to the remote
	 that no changes beyond the system's default should be
	 used. Alignment is the exception, it will default to 8, which
	 will be no alignment alterations. */
    
      netperf_request.content.request_type	= DO_UDP_RR;
      udp_rr_request->recv_buf_size	= rsr_size_req;
      udp_rr_request->send_buf_size	= rss_size_req;
      udp_rr_request->recv_alignment      = remote_recv_align;
      udp_rr_request->recv_offset	        = remote_recv_offset;
      udp_rr_request->send_alignment      = remote_send_align;
      udp_rr_request->send_offset	        = remote_send_offset;
      udp_rr_request->request_size	= req_size;
      udp_rr_request->response_size	= rsp_size;
      udp_rr_request->measure_cpu	        = remote_cpu_usage;
      udp_rr_request->cpu_rate	        = remote_cpu_rate;
      udp_rr_request->so_rcvavoid	        = rem_rcvavoid;
      udp_rr_request->so_sndavoid	        = rem_sndavoid;
      if (test_time) {
	udp_rr_request->test_length	= test_time;
      }
      else {
	udp_rr_request->test_length	= test_trans * -1;
      }
      udp_rr_request->port                = atoi(remote_data_port);
      udp_rr_request->ipfamily = af_to_nf(remote_res->ai_family);
      
      if (debug > 1) {
	fprintf(where,"netperf: send_udp_rr: requesting UDP r/r test\n");
      }
      
      send_request();
    
      /* The response from the remote will contain all of the relevant
	 socket parameters for this test type. We will put them back
	 into the variables here so they can be displayed if desired.
	 The remote will have calibrated CPU if necessary, and will
	 have done all the needed set-up we will have calibrated the
	 cpu locally before sending the request, and will grab the
	 counter value right after the connect returns. The remote
	 will grab the counter right after the accept call. This saves
	 the hassle of extra messages being sent for the UDP
	 tests.  */
    
      recv_response();
    
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote listen done.\n");
	rsr_size	       =	udp_rr_response->recv_buf_size;
	rss_size	       =	udp_rr_response->send_buf_size;
	remote_cpu_usage =	udp_rr_response->measure_cpu;
	remote_cpu_rate  = 	udp_rr_response->cpu_rate;
	/* port numbers in proper order */
	set_port_number(remote_res,(short)udp_rr_response->data_port_number);
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	exit(1);
      }
    }

#ifdef WANT_DEMO
    DEMO_RR_SETUP(100)
#endif

    /* Connect up to the remote port on the data socket. This will set */
    /* the default destination address on this socket. With UDP, this */
    /* does make a performance difference as we may not have to do as */
    /* many routing lookups, however, I expect that a client would */
    /* behave this way. raj 1/94 */
    
    if ( connect(send_socket, 
		 remote_res->ai_addr,
		 remote_res->ai_addrlen) == INVALID_SOCKET ) {
      perror("netperf: data socket connect failed");
      exit(1);
    }
    
    /* Data Socket set-up is finished. If there were problems, either the */
    /* connect would have failed, or the previous response would have */
    /* indicated a problem. I failed to see the value of the extra */
    /* message after the accept on the remote. If it failed, we'll see it */
    /* here. If it didn't, we might as well start pumping data. */
    
    /* Set-up the test end conditions. For a request/response test, they */
    /* can be either time or transaction based. */
    
    if (test_time) {
      /* The user wanted to end the test after a period of time. */
      times_up = 0;
      trans_remaining = 0;
      start_timer(test_time);
    }
    else {
      /* The tester wanted to send a number of bytes. */
      trans_remaining = test_bytes;
      times_up = 1;
    }
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);

#ifdef WANT_DEMO
    if (demo_mode) {
      HIST_timestamp(demo_one_ptr);
    }
#endif 

#ifdef WANT_INTERVALS
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */
    
    /* We use an "OR" to control test execution. When the test is */
    /* controlled by time, the byte count check will always return */
    /* false. When the test is controlled by byte count, the time test */
    /* will always return false. When the test is finished, the whole */
    /* expression will go false and we will stop sending data. I think */
    /* I just arbitrarily decrement trans_remaining for the timed */
    /* test, but will not do that just yet... One other question is */
    /* whether or not the send buffer and the receive buffer should be */
    /* the same buffer. */

#ifdef WANT_FIRST_BURST
    {
      int i;
      for (i = 0; i < first_burst_size; i++) {
	if((len=send(send_socket,
		     send_ring->buffer_ptr,
		     req_size,
		     0)) != req_size) {
	  /* we should never hit the end of the test in the first burst */
	  perror("send_udp_rr: initial burst data send error");
	  exit(-1);
	}
      }
    }
#endif /* WANT_FIRST_BURST */

    while ((!times_up) || (trans_remaining > 0)) {
      /* send the request */
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_one);
      }
#endif
      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   req_size,
		   0)) != req_size) {
        if (SOCKET_EINTR(len)) {
	      /* We likely hit */
	      /* test-end time. */
	      break;
		}
	    perror("send_udp_rr: data send error");
	    exit(1);
	  }
      send_ring = send_ring->next;
      
      /* receive the response. with UDP we will get it all, or nothing */
      
      if((rsp_bytes_recvd=recv(send_socket,
			       recv_ring->buffer_ptr,
			       rsp_size,
			       0)) != rsp_size) {
	    if (SOCKET_EINTR(rsp_bytes_recvd))
		{
    	  /* Again, we have likely hit test-end time */
	      break;
		}
	    perror("send_udp_rr: data recv error");
	    exit(1);
      }
      recv_ring = recv_ring->next;
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }

#endif

      /* at this point, we may wish to sleep for some period of */
      /* time, so we see how long that last transaction just took, */
      /* and sleep for the difference of that and the interval. We */
      /* will not sleep if the time would be less than a */
      /* millisecond.  */

#ifdef WANT_DEMO
      DEMO_RR_INTERVAL(1);
#endif

#ifdef WANT_INTERVALS      
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
      nummessages++;          
      if (trans_remaining) {
	trans_remaining--;
      }
      
      if (debug > 3) {
	if ((nummessages % 100) == 0) {
	  fprintf(where,"Transaction %d completed\n",nummessages);
	  fflush(where);
	}
      }
      
    }
    
    /* for some strange reason, I used to call shutdown on the UDP */
    /* data socket here. I'm not sure why, because it would not have */
    /* any effect... raj 11/94 */
    
    /* this call will always give us the elapsed time for the test, and */
    /* will also store-away the necessaries for cpu utilization */
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured? how long */
						/* did we really run? */
    
    if (!no_control) {
      /* Get the statistics from the remote end. The remote will have
	 calculated service demand and all those interesting
	 things. If it wasn't supposed to care, it will return obvious
	 values. */
    
      recv_response();
      if (!netperf_response.content.serv_errno) {
	if (debug)
	  fprintf(where,"remote results obtained\n");
      }
      else {
	Set_errno(netperf_response.content.serv_errno);
	fprintf(where,
		"netperf: remote error %d",
		netperf_response.content.serv_errno);
	perror("");
	fflush(where);
	exit(1);
      }
    }

    /* We now calculate what our thruput was for the test. In the */
    /* future, we may want to include a calculation of the thruput */
    /* measured by the remote, but it should be the case that for a */
    /* UDP rr test, that the two numbers should be *very* close... */
    /* We calculate bytes_sent regardless of the way the test length */
    /* was controlled.  */
    
    bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
    thruput	= nummessages / elapsed_time;
    
    if (local_cpu_usage || remote_cpu_usage) {

      /* We must now do a little math for service demand and cpu */
      /* utilization for the system(s) Of course, some of the */
      /* information might be bogus because there was no idle counter */
      /* in the kernel(s). We need to make a note of this for the */
      /* user's benefit by placing a code for the metod used in the */
      /* test banner */

      if (local_cpu_usage) {
	local_cpu_utilization = calc_cpu_util(0.0);
	
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	
	local_service_demand  = calc_service_demand((double) nummessages*1024,
						    0.0,
						    0.0,
						    0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = udp_rr_result->cpu_util;
	
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	
	remote_service_demand  = calc_service_demand((double) nummessages*1024,
						     0.0,
						     remote_cpu_utilization,
						     udp_rr_result->num_cpus);
      }
      else {
	remote_cpu_utilization = (float) -1.0;
	remote_service_demand  = (float) -1.0;
      }
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }
    
    /* at this point, we want to calculate the confidence information. */
    /* if debugging is on, calculate_confidence will print-out the */
    /* parameters we pass it */
    
    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    
    confidence_iteration++;
    
    /* we are done with the socket */
    close(send_socket);
  }

  /* at this point, we have made all the iterations we are going to */
  /* make. */
  retrieve_confident_values(&elapsed_time,
			    &thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);
  
  /* We are now ready to print all the information. If the user */
  /* has specified zero-level verbosity, we will just print the */
  /* local service demand, or the remote service demand. If the */
  /* user has requested verbosity level 1, he will get the basic */
  /* "streamperf" numbers. If the user has specified a verbosity */
  /* of greater than 1, we will display a veritable plethora of */
  /* background information from outside of this block as it it */
  /* not cpu_measurement specific...  */
  
  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }
  
  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(udp_rr_result->cpu_method);
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		remote_cpu_method);
      }
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,
		cpu_title,
		local_cpu_method,
		remote_cpu_method);
      }
    
      fprintf(where,
	      cpu_fmt_1_line_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* guess */
	      elapsed_time,		/* how long was the test */
	      nummessages/elapsed_time,
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand);	/* remote service demand */
      fprintf(where,
	      cpu_fmt_1_line_2,
	      rss_size,
	      rsr_size);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      nummessages/elapsed_time);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }
    
      fprintf(where,
	      tput_fmt_1_line_1,	/* the format string */
	      lss_size,
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      nummessages/elapsed_time);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rss_size, 		/* remote recvbuf size */
	      rsr_size);
      
      break;
    }
  }
  fflush(where);

  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  /* how to handle the verbose information in the presence of */
  /* confidence intervals is yet to be determined... raj 11/94 */

  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* UDP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
#ifdef WANT_HISTOGRAM
    fprintf(where,"\nHistogram of request/reponse times.\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */
  }
}

 /* this routine implements the receive side (netserver) of a UDP_RR */
 /* test. */
void
recv_udp_rr()
{
  
  struct ring_elt *recv_ring;
  struct ring_elt *send_ring;

  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];

  struct sockaddr_storage        myaddr_in;
  struct sockaddr_storage    peeraddr; 
  SOCKET	s_data;
  netperf_socklen_t 	addrlen;
  int	trans_received;
  int	trans_remaining;
  int   request_bytes_recvd;
  int   response_bytes_sent;
  float	elapsed_time;
  
  struct	udp_rr_request_struct	*udp_rr_request;
  struct	udp_rr_response_struct	*udp_rr_response;
  struct	udp_rr_results_struct	*udp_rr_results;
  
  udp_rr_request  = 
    (struct udp_rr_request_struct *)netperf_request.content.test_specific_data;
  udp_rr_response = 
    (struct udp_rr_response_struct *)netperf_response.content.test_specific_data;
  udp_rr_results  = 
    (struct udp_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_udp_rr: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_udp_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = UDP_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_udp_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,"recv_udp_rr: requested recv alignment of %d offset %d\n",
	    udp_rr_request->recv_alignment,
	    udp_rr_request->recv_offset);
    fprintf(where,"recv_udp_rr: requested send alignment of %d offset %d\n",
	    udp_rr_request->send_alignment,
	    udp_rr_request->send_offset);
    fflush(where);
  }

  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  recv_ring = allocate_buffer_ring(recv_width,
				   udp_rr_request->request_size,
				   udp_rr_request->recv_alignment,
				   udp_rr_request->recv_offset);

  send_ring = allocate_buffer_ring(send_width,
				   udp_rr_request->response_size,
				   udp_rr_request->send_alignment,
				   udp_rr_request->send_offset);

  if (debug) {
    fprintf(where,"recv_udp_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_udp_rr: grabbing a socket...\n");
    fflush(where);
  }
  

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = udp_rr_request->send_buf_size;
  lsr_size_req = udp_rr_request->recv_buf_size;
  loc_rcvavoid = udp_rr_request->so_rcvavoid;
  loc_sndavoid = udp_rr_request->so_sndavoid;

  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(udp_rr_request->ipfamily),
			udp_rr_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(udp_rr_request->ipfamily),
				SOCK_DGRAM,
				IPPROTO_UDP,
				0);
				
  s_data = create_data_socket(local_res);
  
  if (s_data == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    
    exit(1);
  }
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_data, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_data);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  udp_rr_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  netperf_response.content.serv_errno   = 0;
  
  if (debug) {
    fprintf(where,
	    "recv port number %d\n",
	    ((struct sockaddr_in *)&myaddr_in)->sin_port);
    fflush(where);
  }
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  udp_rr_response->cpu_rate    = (float)0.0; 	/* assume no cpu */
  udp_rr_response->measure_cpu = 0;
  if (udp_rr_request->measure_cpu) {
    udp_rr_response->measure_cpu = 1;
    udp_rr_response->cpu_rate = calibrate_local_cpu(udp_rr_request->cpu_rate);
  }
   
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  udp_rr_response->send_buf_size = lss_size;
  udp_rr_response->recv_buf_size = lsr_size;
  udp_rr_response->so_rcvavoid   = loc_rcvavoid;
  udp_rr_response->so_sndavoid   = loc_sndavoid;
 
  send_response();
  
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(udp_rr_request->measure_cpu);
  
#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */
  
  if (udp_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(udp_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = udp_rr_request->test_length * -1;
  }

  addrlen = sizeof(peeraddr); 
  bzero((char *)&peeraddr, addrlen);
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {
    
    /* receive the request from the other side */
    if ((request_bytes_recvd = recvfrom(s_data,
		 recv_ring->buffer_ptr,
		 udp_rr_request->request_size,
		 0,
		 (struct sockaddr *)&peeraddr,
		 &addrlen)) != udp_rr_request->request_size) {
	  if ( SOCKET_EINTR(request_bytes_recvd) )
	  {
	    /* we must have hit the end of test time. */
	    break;
      }
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
    recv_ring = recv_ring->next;

    /* Now, send the response to the remote */
    if ((response_bytes_sent = sendto(s_data,
				      send_ring->buffer_ptr,
				      udp_rr_request->response_size,
				      0,
				      (struct sockaddr *)&peeraddr,
				      addrlen)) != 
	udp_rr_request->response_size) {
      if ( SOCKET_EINTR(response_bytes_sent) )
	  {
	    /* we have hit end of test time. */
	    break;
      }
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
    send_ring = send_ring->next;
    
    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug) {
      fprintf(where,
	      "recv_udp_rr: Transaction %d complete.\n",
	      trans_received);
      fflush(where);
    }
    
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(udp_rr_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_udp_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  udp_rr_results->bytes_received = (trans_received * 
				    (udp_rr_request->request_size + 
				     udp_rr_request->response_size));
  udp_rr_results->trans_received = trans_received;
  udp_rr_results->elapsed_time	 = elapsed_time;
  udp_rr_results->cpu_method     = cpu_method;
  udp_rr_results->num_cpus       = lib_num_loc_cpus;
  if (udp_rr_request->measure_cpu) {
    udp_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_udp_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();

  /* we are done with the socket now */
  close(s_data);

      }


 /* this routine implements the receive (netserver) side of a TCP_RR */
 /* test */
void
recv_tcp_rr()
{
  
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;

  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];

  struct	sockaddr_storage        myaddr_in,
  peeraddr_in;
  SOCKET	s_listen,s_data;
  netperf_socklen_t 	addrlen;
  char	*temp_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  int   sock_closed = 0;
  float	elapsed_time;
  
  struct	tcp_rr_request_struct	*tcp_rr_request;
  struct	tcp_rr_response_struct	*tcp_rr_response;
  struct	tcp_rr_results_struct	*tcp_rr_results;
  
  tcp_rr_request = 
    (struct tcp_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_rr_response =
    (struct tcp_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_rr_results =
    (struct tcp_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_rr: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* allocate the recv and send rings with the requested alignments */
  /* and offsets. raj 7/94 */
  if (debug) {
    fprintf(where,"recv_tcp_rr: requested recv alignment of %d offset %d\n",
	    tcp_rr_request->recv_alignment,
	    tcp_rr_request->recv_offset);
    fprintf(where,"recv_tcp_rr: requested send alignment of %d offset %d\n",
	    tcp_rr_request->send_alignment,
	    tcp_rr_request->send_offset);
    fflush(where);
  }

  /* at some point, these need to come to us from the remote system */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   tcp_rr_request->response_size,
				   tcp_rr_request->send_alignment,
				   tcp_rr_request->send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   tcp_rr_request->request_size,
				   tcp_rr_request->recv_alignment,
				   tcp_rr_request->recv_offset);

  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_rr_request->send_buf_size;
  lsr_size_req = tcp_rr_request->recv_buf_size;
  loc_nodelay = tcp_rr_request->no_delay;
  loc_rcvavoid = tcp_rr_request->so_rcvavoid;
  loc_sndavoid = tcp_rr_request->so_sndavoid;

  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_rr_request->ipfamily),
			tcp_rr_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_rr_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    
    exit(1);
  }
  
  
#ifdef WIN32
  /* The test timer can fire during operations on the listening socket,
     so to make the start_timer below work we have to move
     it to close s_listen while we are blocked on accept. */
  win_kludge_socket2 = s_listen;
#endif

  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in, 
		  &addrlen) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_rr_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcp_rr_response->cpu_rate = (float)0.0; 	/* assume no cpu */
  tcp_rr_response->measure_cpu = 0;

  if (tcp_rr_request->measure_cpu) {
    tcp_rr_response->measure_cpu = 1;
    tcp_rr_response->cpu_rate = calibrate_local_cpu(tcp_rr_request->cpu_rate);
  }
  
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_rr_response->send_buf_size = lss_size;
  tcp_rr_response->recv_buf_size = lsr_size;
  tcp_rr_response->no_delay = loc_nodelay;
  tcp_rr_response->so_rcvavoid = loc_rcvavoid;
  tcp_rr_response->so_sndavoid = loc_sndavoid;
  tcp_rr_response->test_length = tcp_rr_request->test_length;
  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  if ((s_data = accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == INVALID_SOCKET) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    
    exit(1);
  }
  
#ifdef KLUDGE_SOCKET_OPTIONS
  /* this is for those systems which *INCORRECTLY* fail to pass */
  /* attributes across an accept() call. Including this goes against */
  /* my better judgement :( raj 11/95 */

  kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */

#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */

  if (debug) {
    fprintf(where,"recv_tcp_rr: accept completes on the data connection.\n");
    fflush(where);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_rr_request->measure_cpu);
  
  /* The loop will exit when we hit the end of the test time, or when */
  /* we have exchanged the requested number of transactions. */
  
  if (tcp_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcp_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcp_rr_request->test_length * -1;
  }

  trans_received = 0;
  
  while ((!times_up) || (trans_remaining > 0)) {
    temp_message_ptr = recv_ring->buffer_ptr;
    request_bytes_remaining	= tcp_rr_request->request_size;
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) == SOCKET_ERROR) {
	if (SOCKET_EINTR(request_bytes_recvd))
	{
	  timed_out = 1;
	  break;
	}

	netperf_response.content.serv_errno = errno;
	send_response();
	exit(1);
      }
      else if( request_bytes_recvd == 0 ) {
	if (debug) {
	  fprintf(where,"zero is my hero\n");
	  fflush(where);
	}
	sock_closed = 1;
	break;
      }
      else {
	request_bytes_remaining -= request_bytes_recvd;
	temp_message_ptr  += request_bytes_recvd;
      }
    }

    recv_ring = recv_ring->next;

    if ((timed_out) || (sock_closed)) {
      /* we hit the end of the test based on time - or the socket
	 closed on us along the way.  bail out of here now... */
      if (debug) {
	fprintf(where,"yo5\n");
	fflush(where);
      }						
      break;
    }
    
    /* Now, send the response to the remote */
    if((bytes_sent=send(s_data,
			send_ring->buffer_ptr,
			tcp_rr_request->response_size,
			0)) == SOCKET_ERROR) {
      if (SOCKET_EINTR(bytes_sent)) {
	/* the test timer has popped */
	timed_out = 1;
	fprintf(where,"yo6\n");
	fflush(where);						
	break;
      }
      netperf_response.content.serv_errno = 992;
      send_response();
      exit(1);
    }
    
    send_ring = send_ring->next;

    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(tcp_rr_request->measure_cpu,&elapsed_time);
  
  stop_timer();

  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }

  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcp_rr_results->bytes_received = (trans_received * 
				    (tcp_rr_request->request_size + 
				     tcp_rr_request->response_size));
  tcp_rr_results->trans_received = trans_received;
  tcp_rr_results->elapsed_time   = elapsed_time;
  tcp_rr_results->cpu_method     = cpu_method;
  tcp_rr_results->num_cpus       = lib_num_loc_cpus;
  if (tcp_rr_request->measure_cpu) {
    tcp_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  /* we are now done with the sockets */
  close(s_data);
  close(s_listen);

  send_response();
  
}


void
loc_cpu_rate()
{
#if defined(USE_LOOPER)
  float dummy;
#endif

  /* a rather simple little test - it merely calibrates the local cpu */
  /* and prints the results. There are no headers to allow someone to */
  /* find a rate and use it in other tests automagically by setting a */
  /* variable equal to the output of this test. We ignore any rates */
  /* that may have been specified. In fact, we ignore all of the */
  /* command line args! */
  
  fprintf(where,
	  "%g",
	  calibrate_local_cpu(0.0));

  if (verbosity > 1)
    fprintf(where,
	    "\nThere %s %d local %s\n",
	    (lib_num_loc_cpus > 1) ? "are" : "is",
	    lib_num_loc_cpus,
	    (lib_num_loc_cpus > 1) ? "cpus" : "cpu");
	    
  /* we need the cpu_start, cpu_stop in the looper case to kill the */
  /* child proceses raj 4/95 */

#ifdef USE_LOOPER
  cpu_start(1);
  cpu_stop(1,&dummy);
#endif /* USE_LOOPER */

}	

void
rem_cpu_rate()
{
  /* this test is much like the local variant, except that it works for */
  /* the remote system, so in this case, we do pay attention to the */
  /* value of the '-H' command line argument. */
  
  fprintf(where,
	  "%g",
	  calibrate_remote_cpu());
  
  if (verbosity > 1)
    fprintf(where,
	    "\nThere %s %d remote %s\n",
	    (lib_num_rem_cpus > 1) ? "are" : "is",
	    lib_num_rem_cpus,
	    (lib_num_rem_cpus > 1) ? "cpus" : "cpu");

}


 /* this test is intended to test the performance of establishing a
    connection, exchanging a request/response pair, and repeating. it
    is expected that this would be a good starting-point for
    comparision of T/TCP with classic TCP for transactional workloads.
    it will also look (can look) much like the communication pattern
    of http for www access. */

void
send_tcp_conn_rr(char remote_host[])
{
  
  char *tput_title = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
bytes  Bytes  bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %%      %%      us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6d %-6d %-6d  %-6d %-6.2f  %-6.2f   %-6.2f %-6.2f %-6.3f  %-6.3f\n";
  
  char *cpu_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *ksink_fmt = "\n\
Alignment      Offset\n\
Local  Remote  Local  Remote\n\
Send   Recv    Send   Recv\n\
%5d  %5d   %5d  %5d\n";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  
  int	len;
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;
  char	*temp_message_ptr;
  int	nummessages;
  SOCKET	send_socket;
  int	trans_remaining;
  double	bytes_xferd;
  int	rsp_bytes_left;
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct addrinfo *local_res;
  struct addrinfo *remote_res;

  int                           myport;
  int                           ret;

  struct	tcp_conn_rr_request_struct	*tcp_conn_rr_request;
  struct	tcp_conn_rr_response_struct	*tcp_conn_rr_response;
  struct	tcp_conn_rr_results_struct	*tcp_conn_rr_result;
  
  tcp_conn_rr_request = 
    (struct tcp_conn_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_conn_rr_response = 
    (struct tcp_conn_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_conn_rr_result =
    (struct tcp_conn_rr_results_struct *)netperf_response.content.test_specific_data;
  
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);

  if ( print_headers ) {
    print_top_test_header("TCP Connect/Request/Response TEST",local_res,remote_res);
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_xferd	=	0.0;
  times_up 	= 	0;
  
  /* set-up the data buffers with the requested alignment and offset */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   req_size,
				   local_send_align,
				   local_send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   rsp_size,
				   local_recv_align,
				   local_recv_offset);


  if (debug) {
    fprintf(where,"send_tcp_conn_rr: send_socket obtained...\n");
  }
  
  /* If the user has requested cpu utilization measurements, we must */
  /* calibrate the cpu(s). We will perform this task within the tests */
  /* themselves. If the user has specified the cpu rate, then */
  /* calibrate_local_cpu will return rather quickly as it will have */
  /* nothing to do. If local_cpu_rate is zero, then we will go through */
  /* all the "normal" calibration stuff and return the rate back.*/
  
  if (local_cpu_usage) {
    local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
  }

  if (!no_control) {
  
    /* Tell the remote end to do a listen. The server alters the
       socket paramters on the other side at this point, hence the
       reason for all the values being passed in the setup message. If
       the user did not specify any of the parameters, they will be
       passed as 0, which will indicate to the remote that no changes
       beyond the system's default should be used. Alignment is the
       exception, it will default to 8, which will be no alignment
       alterations. */
  
    netperf_request.content.request_type =	DO_TCP_CRR;
    tcp_conn_rr_request->recv_buf_size	=	rsr_size_req;
    tcp_conn_rr_request->send_buf_size	=	rss_size_req;
    tcp_conn_rr_request->recv_alignment	=	remote_recv_align;
    tcp_conn_rr_request->recv_offset	=	remote_recv_offset;
    tcp_conn_rr_request->send_alignment	=	remote_send_align;
    tcp_conn_rr_request->send_offset	=	remote_send_offset;
    tcp_conn_rr_request->request_size	=	req_size;
    tcp_conn_rr_request->response_size	=	rsp_size;
    tcp_conn_rr_request->no_delay	=	rem_nodelay;
    tcp_conn_rr_request->measure_cpu	=	remote_cpu_usage;
    tcp_conn_rr_request->cpu_rate	=	remote_cpu_rate;
    tcp_conn_rr_request->so_rcvavoid	=	rem_rcvavoid;
    tcp_conn_rr_request->so_sndavoid	=	rem_sndavoid;
    if (test_time) {
      tcp_conn_rr_request->test_length	=	test_time;
    }
    else {
      tcp_conn_rr_request->test_length	=	test_trans * -1;
    }
    tcp_conn_rr_request->port           = atoi(remote_data_port);
    tcp_conn_rr_request->ipfamily       = af_to_nf(remote_res->ai_family);
    
    if (debug > 1) {
      fprintf(where,"netperf: send_tcp_conn_rr: requesting TCP crr test\n");
    }
    
    send_request();
    
    /* The response from the remote will contain all of the relevant
       socket parameters for this test type. We will put them back
       into the variables here so they can be displayed if desired.
       The remote will have calibrated CPU if necessary, and will have
       done all the needed set-up we will have calibrated the cpu
       locally before sending the request, and will grab the counter
       value right after the connect returns. The remote will grab the
       counter right after the accept call. This saves the hassle of
       extra messages being sent for the TCP tests.  */
  
    recv_response();
  
    if (!netperf_response.content.serv_errno) {
      rsr_size	       =	tcp_conn_rr_response->recv_buf_size;
      rss_size	       =	tcp_conn_rr_response->send_buf_size;
      rem_nodelay      =	tcp_conn_rr_response->no_delay;
      remote_cpu_usage =	tcp_conn_rr_response->measure_cpu;
      remote_cpu_rate  = 	tcp_conn_rr_response->cpu_rate;
      /* make sure that port numbers are in network order */
      set_port_number(remote_res,
		      (unsigned short)tcp_conn_rr_response->data_port_number);
      
      if (debug) {
	fprintf(where,"remote listen done.\n");
	fprintf(where,"remote port is %u\n",get_port_number(remote_res));
	fflush(where);
      }
    }
    else {
      Set_errno(netperf_response.content.serv_errno);
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      exit(1);
    }
  }
#ifdef WANT_DEMO
  DEMO_RR_SETUP(100)
#endif

  /* pick a nice random spot between client_port_min and */
  /* client_port_max for our initial port number */
  srand(getpid());
  if (client_port_max - client_port_min) {
    myport = client_port_min + 
      (rand() % (client_port_max - client_port_min));
  }
  else {
    myport = client_port_min;
  }
  /* there will be a ++ before the first call to bind, so subtract one */
  myport--;
  /* Set-up the test end conditions. For a request/response test, they */
  /* can be either time or transaction based. */
  
  if (test_time) {
    /* The user wanted to end the test after a period of time. */
    times_up = 0;
    trans_remaining = 0;
    start_timer(test_time);
  }
  else {
    /* The tester wanted to send a number of bytes. */
    trans_remaining = test_bytes;
    times_up = 1;
  }
  
  /* The cpu_start routine will grab the current time and possibly */
  /* value of the idle counter for later use in measuring cpu */
  /* utilization and/or service demand and thruput. */

  
  cpu_start(local_cpu_usage);

#ifdef WANT_DEMO
      if (demo_mode) {
	HIST_timestamp(demo_one_ptr);
      }
#endif
  
  /* We use an "OR" to control test execution. When the test is */
  /* controlled by time, the byte count check will always return false. */
  /* When the test is controlled by byte count, the time test will */
  /* always return false. When the test is finished, the whole */
  /* expression will go false and we will stop sending data. I think I */
  /* just arbitrarily decrement trans_remaining for the timed test, but */
  /* will not do that just yet... One other question is whether or not */
  /* the send buffer and the receive buffer should be the same buffer. */

  while ((!times_up) || (trans_remaining > 0)) {

#ifdef WANT_HISTOGRAM
    if (verbosity > 1) {
      /* timestamp just before our call to create the socket, and then */
      /* again just after the receive raj 3/95 */
      HIST_timestamp(&time_one);
    }
#endif /* WANT_HISTOGRAM */

newport:
    /* pick a new port number */
    myport++;

    /* wrap the port number when we get to client_port_max. NOTE, some */
    /* broken TCP's might treat the port number as a signed 16 bit */
    /* quantity.  we aren't interested in testing such broken */
    /* implementations :) so we won't make sure that it is below 32767 */
    /* raj 8/94  */
    if (myport >= client_port_max) {
      myport = client_port_min;
    }

    /* we do not want to use the port number that the server is */
    /* sitting at - this would cause us to fail in a loopback test. we */
    /* could just rely on the failure of the bind to get us past this, */
    /* but I'm guessing that in this one case at least, it is much */
    /* faster, given that we *know* that port number is already in use */
    /* (or rather would be in a loopback test) */

    if (myport == get_port_number(remote_res)) myport++;

    if (debug) {
      if ((nummessages % 100) == 0) {
	printf("port %d\n",myport);
      }
    }

    /* set up the data socket */
    set_port_number(local_res, (unsigned short)myport);
    send_socket = create_data_socket(local_res);
  
    if (send_socket == INVALID_SOCKET) {
      perror("netperf: send_tcp_conn_rr: tcp stream data socket");
      exit(1);
    }


    /* we used to call bind here, but that is now taken-care-of by the
       create_data_socket routine. */

    /* Connect up to the remote port on the data socket  */
    if ((ret = connect(send_socket, 
		       remote_res->ai_addr,
		       remote_res->ai_addrlen)) == INVALID_SOCKET){
      if (SOCKET_EINTR(ret))
	  {
	    /* we hit the end of a */
	    /* timed test. */
	    timed_out = 1;
	    break;
      }
      if ((SOCKET_EADDRINUSE(ret)) || SOCKET_EADDRNOTAVAIL(ret)) {
	/* likely something our explicit bind() would have caught in
           the past, so go get another port, via create_data_socket.
           yes, this is a bit more overhead than before, but the
           condition should be rather rare.  raj 2005-02-08 */
	close(send_socket);
	goto newport;
      }
      perror("netperf: data socket connect failed");
      printf("\tattempted to connect on socket %d to port %d",
	     send_socket,
	     get_port_number(remote_res));
      printf(" from port %d \n",get_port_number(local_res));
      exit(1);
    }


    /* send the request */
    if((len=send(send_socket,
		 send_ring->buffer_ptr,
		 req_size,
		 0)) != req_size) {
      if (SOCKET_EINTR(len))
	  {
	    /* we hit the end of a */
	    /* timed test. */
	    timed_out = 1;
	    break;
      }
      perror("send_tcp_conn_rr: data send error");
      exit(1);
    }
    send_ring = send_ring->next;

    /* receive the response */
    rsp_bytes_left = rsp_size;
    temp_message_ptr  = recv_ring->buffer_ptr;


    do {
      rsp_bytes_recvd = recv(send_socket,
			     temp_message_ptr,
			     rsp_bytes_left,
			     0);
      if (rsp_bytes_recvd > 0) {
	rsp_bytes_left -= rsp_bytes_recvd;
	temp_message_ptr += rsp_bytes_recvd;
      }
      else {
	break;
      }
    } while (rsp_bytes_left);


    /* OK, we are out of the loop - now what? */
    if (rsp_bytes_recvd < 0) {
      /* did the timer hit, or was there an error? */
      if (SOCKET_EINTR(rsp_bytes_recvd))
	  {
	    /* We hit the end of a timed test. */
	    timed_out = 1;
	    break;
	  }
	  perror("send_tcp_conn_rr: data recv error");
	  exit(1);
    }

    /* if this is a no_control test, we initiate connection close,
       otherwise the remote netserver does it to remain just like
       previous behaviour. raj 2007-27-08 */
    if (!no_control) {
      shutdown(send_socket,SHUT_WR);
    }

    /* we are expecting to get either a return of zero indicating
       connection close, or an error.  */
    rsp_bytes_recvd = recv(send_socket,
			   temp_message_ptr,
			   1,
			   0);

    /* our exit from the while loop should generally be when */
    /* tmp_bytes_recvd is equal to zero, which implies the connection */
    /* has been closed by the server side. By waiting until we get the */
    /* zero return we can avoid race conditions that stick us with the */
    /* TIME_WAIT connection and not the server. raj 8/96 */

    if (rsp_bytes_recvd == 0) {
      /* connection close, call close. we assume that the requisite */
      /* number of bytes have been received */
      recv_ring = recv_ring->next;

#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */

#ifdef WANT_DEMO
      DEMO_RR_INTERVAL(1)
#endif

      nummessages++;          
      if (trans_remaining) {
	trans_remaining--;
      }
    
      if (debug > 3) {
	fprintf(where,
		"Transaction %d completed on local port %d\n",
		nummessages,
		get_port_number(local_res));
	fflush(where);
      }

      close(send_socket);

    }
    else {
      /* it was less than zero - an error occured */
      if (SOCKET_EINTR(rsp_bytes_recvd))
	  {
	    /* We hit the end of a timed test. */
	    timed_out = 1;
	    break;
	  }
	  perror("send_tcp_conn_rr: data recv error");
	  exit(1);
    }
      
  }

  
  /* this call will always give us the elapsed time for the test, and */
  /* will also store-away the necessaries for cpu utilization */

  cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being measured? */
  /* how long did we really run? */
  
  if (!no_control) {
    /* Get the statistics from the remote end. The remote will have
       calculated service demand and all those interesting things. If
       it wasn't supposed to care, it will return obvious values. */
  
    recv_response();
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote results obtained\n");
    }
    else {
      Set_errno(netperf_response.content.serv_errno);
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      
      exit(1);
    }
  }

  /* We now calculate what our thruput was for the test. In the future, */
  /* we may want to include a calculation of the thruput measured by */
  /* the remote, but it should be the case that for a TCP stream test, */
  /* that the two numbers should be *very* close... We calculate */
  /* bytes_sent regardless of the way the test length was controlled. */
  /* If it was time, we needed to, and if it was by bytes, the user may */
  /* have specified a number of bytes that wasn't a multiple of the */
  /* send_size, so we really didn't send what he asked for ;-) We use */
  /* Kbytes/s as the units of thruput for a TCP stream test, where K = */
  /* 1024. A future enhancement *might* be to choose from a couple of */
  /* unit selections. */ 
  
  bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
  thruput	= calc_thruput(bytes_xferd);
  
  if (local_cpu_usage || remote_cpu_usage) {
    /* We must now do a little math for service demand and cpu */
    /* utilization for the system(s) */
    /* Of course, some of the information might be bogus because */
    /* there was no idle counter in the kernel(s). We need to make */
    /* a note of this for the user's benefit...*/
    if (local_cpu_usage) {
      if (local_cpu_rate == 0.0) {
	fprintf(where,
		"WARNING WARNING WARNING  WARNING WARNING WARNING  WARNING!\n");
	fprintf(where,
		"Local CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      local_cpu_utilization = calc_cpu_util(0.0);
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      local_service_demand  = calc_service_demand((double) nummessages*1024,
						  0.0,
						  0.0,
						  0);
    }
    else {
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
    }
    
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,
		"DANGER  DANGER  DANGER    DANGER  DANGER  DANGER    DANGER!\n");
	fprintf(where,
		"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization = tcp_conn_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization,
						  tcp_conn_rr_result->num_cpus);
    }
    else {
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }
    
    /* We are now ready to print all the information. If the user */
    /* has specified zero-level verbosity, we will just print the */
    /* local service demand, or the remote service demand. If the */
    /* user has requested verbosity level 1, he will get the basic */
    /* "streamperf" numbers. If the user has specified a verbosity */
    /* of greater than 1, we will display a veritable plethora of */
    /* background information from outside of this block as it it */
    /* not cpu_measurement specific...  */
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand);
      }
      break;
    case 1:
    case 2:

      if (print_headers) {
	fprintf(where,
		cpu_title,
		local_cpu_method,
		remote_cpu_method);
      }
      
      fprintf(where,
	      cpu_fmt_1_line_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* guess */
	      elapsed_time,		/* how long was the test */
	      nummessages/elapsed_time,
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand);	/* remote service demand */
      fprintf(where,
	      cpu_fmt_1_line_2,
	      rss_size,
	      rsr_size);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      nummessages/elapsed_time);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }

      fprintf(where,
	      tput_fmt_1_line_1,	/* the format string */
	      lss_size,
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      nummessages/elapsed_time);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rss_size, 		/* remote recvbuf size */
	      rsr_size);
      
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
    fprintf(where,
	    ksink_fmt,
	    local_send_align,
	    remote_recv_offset,
	    local_send_offset,
	    remote_recv_offset);

#ifdef WANT_HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */

  }
  
}


void
recv_tcp_conn_rr()
{
  
  char  *message;
  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];

  struct	sockaddr_storage        myaddr_in, peeraddr_in;
  SOCKET	s_listen,s_data;
  netperf_socklen_t 	addrlen;
  char	*recv_message_ptr;
  char	*send_message_ptr;
  char	*temp_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	tcp_conn_rr_request_struct	*tcp_conn_rr_request;
  struct	tcp_conn_rr_response_struct	*tcp_conn_rr_response;
  struct	tcp_conn_rr_results_struct	*tcp_conn_rr_results;
  
  tcp_conn_rr_request = 
    (struct tcp_conn_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_conn_rr_response = 
    (struct tcp_conn_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_conn_rr_results = 
    (struct tcp_conn_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_conn_rr: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_conn_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_CRR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_conn_rr: the response type is set...\n");
    fflush(where);
  }

  /* set-up the data buffer with the requested alignment and offset */
  message = (char *)malloc(DATABUFFERLEN);
  if (message == NULL) {
    printf("malloc(%d) failed!\n", DATABUFFERLEN);
    exit(1);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_conn_rr: requested recv alignment of %d offset %d\n",
	    tcp_conn_rr_request->recv_alignment,
	    tcp_conn_rr_request->recv_offset);
    fprintf(where,
	    "recv_tcp_conn_rr: requested send alignment of %d offset %d\n",
	    tcp_conn_rr_request->send_alignment,
	    tcp_conn_rr_request->send_offset);
    fflush(where);
  }

  recv_message_ptr = ALIGN_BUFFER(message, tcp_conn_rr_request->recv_alignment, tcp_conn_rr_request->recv_offset);
  
  send_message_ptr = ALIGN_BUFFER(message, tcp_conn_rr_request->send_alignment, tcp_conn_rr_request->send_offset);
  
  if (debug) {
    fprintf(where,"recv_tcp_conn_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_conn_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_conn_rr_request->send_buf_size;
  lsr_size_req = tcp_conn_rr_request->recv_buf_size;
  loc_nodelay = tcp_conn_rr_request->no_delay;
  loc_rcvavoid = tcp_conn_rr_request->so_rcvavoid;
  loc_sndavoid = tcp_conn_rr_request->so_sndavoid;
  
  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_conn_rr_request->ipfamily),
			tcp_conn_rr_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_conn_rr_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    if (debug) {
      fprintf(where,"could not create data socket\n");
      fflush(where);
    }
    exit(1);
  }

#ifdef WIN32
    /* The test timer can fire during operations on the listening socket,
       so to make the start_timer below work we have to move
       it to close s_listen while we are blocked on accept. */
    win_kludge_socket2 = s_listen;
#endif


  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not listen\n");
      fflush(where);
    }
    exit(1);
  }
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not getsockname\n");
      fflush(where);
    }
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_conn_rr_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  if (debug) {
    fprintf(where,"telling the remote to call me at %d\n",
	    tcp_conn_rr_response->data_port_number);
    fflush(where);
  }
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcp_conn_rr_response->cpu_rate = (float)0.0; 	/* assume no cpu */
  if (tcp_conn_rr_request->measure_cpu) {
    tcp_conn_rr_response->measure_cpu = 1;
    tcp_conn_rr_response->cpu_rate = 
      calibrate_local_cpu(tcp_conn_rr_request->cpu_rate);
  }
  

  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_conn_rr_response->send_buf_size = lss_size;
  tcp_conn_rr_response->recv_buf_size = lsr_size;
  tcp_conn_rr_response->no_delay = loc_nodelay;
  tcp_conn_rr_response->so_rcvavoid = loc_rcvavoid;
  tcp_conn_rr_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_conn_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcp_conn_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcp_conn_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcp_conn_rr_request->test_length * -1;
  }
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {

    /* accept a connection from the remote */
#ifdef WIN32
    /* The test timer will probably fire during this accept, 
       so to make the start_timer above work we have to move
       it to close s_listen while we are blocked on accept. */
    win_kludge_socket = s_listen;
#endif
    if ((s_data=accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == INVALID_SOCKET) {
      if (errno == EINTR) {
	/* the timer popped */
	timed_out = 1;
	break;
      }
      fprintf(where,"recv_tcp_conn_rr: accept: errno = %d\n",errno);
      fflush(where);
      close(s_listen);
      
      exit(1);
    }

    if (debug) {
      fprintf(where,"recv_tcp_conn_rr: accepted data connection.\n");
      fflush(where);
    }
  
#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */

#ifdef KLUDGE_SOCKET_OPTIONS
    /* this is for those systems which *INCORRECTLY* fail to pass */
    /* attributes across an accept() call. Including this goes against */
    /* my better judgement :( raj 11/95 */
    
    kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */
  
    temp_message_ptr	= recv_message_ptr;
    request_bytes_remaining	= tcp_conn_rr_request->request_size;
    
    /* receive the request from the other side */
    while (!times_up && (request_bytes_remaining > 0)) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) == SOCKET_ERROR) {
	if (SOCKET_EINTR(request_bytes_recvd))
	{
	  /* the timer popped */
	  timed_out = 1;
	  break;
	}
	netperf_response.content.serv_errno = errno;
	send_response();
	exit(1);
      }
      else {
	request_bytes_remaining -= request_bytes_recvd;
	temp_message_ptr  += request_bytes_recvd;
      }
    }
    
    if (timed_out) {
      /* we hit the end of the test based on time - lets */
      /* bail out of here now... */
      fprintf(where,"yo5\n");
      fflush(where);						
      break;
    }
    
    /* Now, send the response to the remote */
    if((bytes_sent=send(s_data,
			send_message_ptr,
			tcp_conn_rr_request->response_size,
			0)) == SOCKET_ERROR) {
      if (errno == EINTR) {
	/* the test timer has popped */
	timed_out = 1;
	fprintf(where,"yo6\n");
	fflush(where);						
	break;
      }
      netperf_response.content.serv_errno = 99;
      send_response();
      exit(1);
    }
    
    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug) {
      fprintf(where,
	      "recv_tcp_conn_rr: Transaction %d complete\n",
	      trans_received);
      fflush(where);
    }

    /* close the connection. the server will likely do a graceful */
    /* close of the connection, insuring that all data has arrived at */
    /* the client. for this it will call shutdown(), and then recv() and */
    /* then close(). I'm reasonably confident that this is the */
    /* appropriate sequence of calls - I would like to hear of */
    /* examples in web servers to the contrary. raj 10/95*/
#ifdef TCP_CRR_SHUTDOWN
    shutdown(s_data,SHUT_WR);
    recv(s_data,
	 recv_message_ptr,
	 1,
	 0);
    close(s_data);
#else
    close(s_data);
#endif /* TCP_CRR_SHUTDOWN */

  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(tcp_conn_rr_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_conn_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcp_conn_rr_results->bytes_received	= (trans_received * 
					   (tcp_conn_rr_request->request_size + 
					    tcp_conn_rr_request->response_size));
  tcp_conn_rr_results->trans_received	= trans_received;
  tcp_conn_rr_results->elapsed_time	= elapsed_time;
  if (tcp_conn_rr_request->measure_cpu) {
    tcp_conn_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_conn_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}


#ifdef DO_1644

 /* this test is intended to test the performance of establishing a */
 /* connection, exchanging a request/response pair, and repeating. it */
 /* is expected that this would be a good starting-point for */
 /* comparision of T/TCP with classic TCP for transactional workloads. */
 /* it will also look (can look) much like the communication pattern */
 /* of http for www access. */

int 
send_tcp_tran_rr(char remote_host[])
{
  
  char *tput_title = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
bytes  Bytes  bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %%      %%      us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6d %-6d %-6d  %-6d %-6.2f  %-6.2f   %-6.2f %-6.2f %-6.3f  %-6.3f\n";
  
  char *cpu_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *ksink_fmt = "\n\
Alignment      Offset\n\
Local  Remote  Local  Remote\n\
Send   Recv    Send   Recv\n\
%5d  %5d   %5d  %5d\n";
  
  
  int 			one = 1;
  int			timed_out = 0;
  float			elapsed_time;
  
  int	len;
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;
  char	*temp_message_ptr;
  int	nummessages;
  SOCKET	send_socket;
  int	trans_remaining;
  double	bytes_xferd;
  int	sock_opt_len = sizeof(int);
  int	rsp_bytes_left;
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct	hostent	        *hp;
  struct	sockaddr_in	server;
  struct        sockaddr_in     *myaddr;
  unsigned      int             addr;
  int                           myport;

  struct	tcp_tran_rr_request_struct	*tcp_tran_rr_request;
  struct	tcp_tran_rr_response_struct	*tcp_tran_rr_response;
  struct	tcp_tran_rr_results_struct	*tcp_tran_rr_result;
  
  tcp_tran_rr_request = 
    (struct tcp_tran_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_tran_rr_response = 
    (struct tcp_tran_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_tran_rr_result =
    (struct tcp_tran_rr_results_struct *)netperf_response.content.test_specific_data;
  
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  myaddr = (struct sockaddr_storage *)malloc(sizeof(struct sockaddr_storage));
  if (myaddr == NULL) {
    printf("malloc(%d) failed!\n", sizeof(struct sockaddr_storage));
    exit(1);
  }

  bzero((char *)&server,
	sizeof(server));
  bzero((char *)myaddr,
	sizeof(struct sockaddr_storage));
  myaddr->sin_family = AF_INET;

  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);

  if ( print_headers ) {
    print_top_test_header("TCP Transactional/Request/Response TEST",local_res,remote_res);
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_xferd	=	0.0;
  times_up 	= 	0;
  
  /* set-up the data buffers with the requested alignment and offset */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   req_size,
				   local_send_align,
				   local_send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   rsp_size,
				   local_recv_align,
				   local_recv_offset);


  if (debug) {
    fprintf(where,"send_tcp_tran_rr: send_socket obtained...\n");
  }
  
  /* If the user has requested cpu utilization measurements, we must */
  /* calibrate the cpu(s). We will perform this task within the tests */
  /* themselves. If the user has specified the cpu rate, then */
  /* calibrate_local_cpu will return rather quickly as it will have */
  /* nothing to do. If local_cpu_rate is zero, then we will go through */
  /* all the "normal" calibration stuff and return the rate back.*/
  
  if (local_cpu_usage) {
    local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
  }
  
  /* Tell the remote end to do a listen. The server alters the socket */
  /* paramters on the other side at this point, hence the reason for */
  /* all the values being passed in the setup message. If the user did */
  /* not specify any of the parameters, they will be passed as 0, which */
  /* will indicate to the remote that no changes beyond the system's */
  /* default should be used. Alignment is the exception, it will */
  /* default to 8, which will be no alignment alterations. */
  
  netperf_request.content.request_type	        =	DO_TCP_TRR;
  tcp_tran_rr_request->recv_buf_size	=	rsr_size_req;
  tcp_tran_rr_request->send_buf_size	=	rss_size_req;
  tcp_tran_rr_request->recv_alignment	=	remote_recv_align;
  tcp_tran_rr_request->recv_offset	=	remote_recv_offset;
  tcp_tran_rr_request->send_alignment	=	remote_send_align;
  tcp_tran_rr_request->send_offset	=	remote_send_offset;
  tcp_tran_rr_request->request_size	=	req_size;
  tcp_tran_rr_request->response_size	=	rsp_size;
  tcp_tran_rr_request->no_delay	        =	rem_nodelay;
  tcp_tran_rr_request->measure_cpu	=	remote_cpu_usage;
  tcp_tran_rr_request->cpu_rate	        =	remote_cpu_rate;
  tcp_tran_rr_request->so_rcvavoid	=	rem_rcvavoid;
  tcp_tran_rr_request->so_sndavoid	=	rem_sndavoid;
  if (test_time) {
    tcp_tran_rr_request->test_length	=	test_time;
  }
  else {
    tcp_tran_rr_request->test_length	=	test_trans * -1;
  }
  tcp_tran_rr_request->port             =       atoi(remote_data_port);
  tcp_tran_rr_request->ipfamily        =       af_to_nf(remote_res->ai_family);

  if (debug > 1) {
    fprintf(where,"netperf: send_tcp_tran_rr: requesting TCP_TRR test\n");
  }
  
  send_request();
  
  /* The response from the remote will contain all of the relevant 	*/
  /* socket parameters for this test type. We will put them back into 	*/
  /* the variables here so they can be displayed if desired.  The	*/
  /* remote will have calibrated CPU if necessary, and will have done	*/
  /* all the needed set-up we will have calibrated the cpu locally	*/
  /* before sending the request, and will grab the counter value right	*/
  /* after the connect returns. The remote will grab the counter right	*/
  /* after the accept call. This saves the hassle of extra messages	*/
  /* being sent for the TCP tests.					*/
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    rsr_size	=	tcp_tran_rr_response->recv_buf_size;
    rss_size	=	tcp_tran_rr_response->send_buf_size;
    rem_nodelay	=	tcp_tran_rr_response->no_delay;
    remote_cpu_usage=	tcp_tran_rr_response->measure_cpu;
    remote_cpu_rate = 	tcp_tran_rr_response->cpu_rate;
    /* make sure that port numbers are in network order */
    server.sin_port	=	tcp_tran_rr_response->data_port_number;
    server.sin_port =	htons(server.sin_port);
    if (debug) {
      fprintf(where,"remote listen done.\n");
      fprintf(where,"remote port is %d\n",ntohs(server.sin_port));
      fflush(where);
    }
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    fprintf(where,
	    "netperf: remote error %d",
	    netperf_response.content.serv_errno);
    perror("");
    fflush(where);
    exit(1);
  }

  /* pick a nice random spot between client_port_min and */
  /* client_port_max for our initial port number. if they are the */
  /* same, then just set to _min */
  if (client_port_max - client_port_min) {
    srand(getpid());
    myport = client_port_min + 
      (rand() % (client_port_max - client_port_min));
  }
  else {
    myport = client_port_min;
  }

  /* there will be a ++ before the first call to bind, so subtract one */
  myport--;
  myaddr->sin_port = htons((unsigned short)myport);
  
  /* Set-up the test end conditions. For a request/response test, they */
  /* can be either time or transaction based. */
  
  if (test_time) {
    /* The user wanted to end the test after a period of time. */
    times_up = 0;
    trans_remaining = 0;
    start_timer(test_time);
  }
  else {
    /* The tester wanted to send a number of bytes. */
    trans_remaining = test_bytes;
    times_up = 1;
  }
  
  /* The cpu_start routine will grab the current time and possibly */
  /* value of the idle counter for later use in measuring cpu */
  /* utilization and/or service demand and thruput. */
  
  cpu_start(local_cpu_usage);
  
  /* We use an "OR" to control test execution. When the test is */
  /* controlled by time, the byte count check will always return false. */
  /* When the test is controlled by byte count, the time test will */
  /* always return false. When the test is finished, the whole */
  /* expression will go false and we will stop sending data. I think I */
  /* just arbitrarily decrement trans_remaining for the timed test, but */
  /* will not do that just yet... One other question is whether or not */
  /* the send buffer and the receive buffer should be the same buffer. */

  while ((!times_up) || (trans_remaining > 0)) {

#ifdef WANT_HISTOGRAM
    if (verbosity > 1) {
      /* timestamp just before our call to create the socket, and then */
      /* again just after the receive raj 3/95 */
      HIST_timestamp(&time_one);
    }
#endif /* WANT_HISTOGRAM */

    /* set up the data socket - is this really necessary or can I just */
    /* re-use the same socket and move this cal out of the while loop. */
    /* it does introcudea *boatload* of system calls. I guess that it */
    /* all depends on "reality of programming." keeping it this way is */
    /* a bit more conservative I imagine - raj 3/95 */
    send_socket = create_data_socket(local_res);
  
    if (send_socket == INVALID_SOCKET) {
      perror("netperf: send_tcp_tran_rr: tcp stream data socket");
      exit(1);
    }

    /* we set SO_REUSEADDR on the premis that no unreserved port */
    /* number on the local system is going to be already connected to */
    /* the remote netserver's port number. One thing that I might */
    /* try later is to have the remote actually allocate a couple of */
    /* port numbers and cycle through those as well. depends on if we */
    /* can get through all the unreserved port numbers in less than */
    /* the length of the TIME_WAIT state raj 8/94 */
    one = 1;
    if(setsockopt(send_socket, SOL_SOCKET, SO_REUSEADDR,
		  (char *)&one, sock_opt_len) == SOCKET_ERROR) {
      perror("netperf: send_tcp_tran_rr: so_reuseaddr");
      exit(1);
    }

newport:
    /* pick a new port number */
    myport = ntohs(myaddr->sin_port);
    myport++;

    /* we do not want to use the port number that the server is */
    /* sitting at - this would cause us to fail in a loopback test. we */
    /* could just rely on the failure of the bind to get us past this, */
    /* but I'm guessing that in this one case at least, it is much */
    /* faster, given that we *know* that port number is already in use */
    /* (or rather would be in a loopback test) */

    if (myport == ntohs(server.sin_port)) myport++;

    /* wrap the port number when we get to 65535. NOTE, some broken */
    /* TCP's might treat the port number as a signed 16 bit quantity. */
    /* we aren't interested in testing such broken implementations :) */
    /* raj 8/94  */
    if (myport >= client_port_max) {
      myport = client_port_min;
    }
    myaddr->sin_port = htons((unsigned short)myport);

    if (debug) {
      if ((nummessages % 100) == 0) {
	printf("port %d\n",myport);
      }
    }

    /* we want to bind our socket to a particular port number. */
    if (bind(send_socket,
	     (struct sockaddr *)myaddr,
	     sizeof(struct sockaddr_storage)) == SOCKET_ERROR) {
      /* if the bind failed, someone else must have that port number */
      /* - perhaps in the listen state. since we can't use it, skip to */
      /* the next port number. we may have to do this again later, but */
      /* that's just too bad :) */
      if (debug > 1) {
	fprintf(where,
		"send_tcp_tran_rr: tried to bind to port %d errno %d\n",
		ntohs(myaddr->sin_port),
		errno);
	fflush(where);
      }
	/* yes, goto's are supposed to be evil, but they do have their */
	/* uses from time to time. the real world doesn't always have */
	/* to code to ge tthe A in CS 101 :) raj 3/95 */
	goto newport;
    }

    /* Connect up to the remote port on the data socket. Since this is */
    /* a test for RFC_1644-style transactional TCP, we can use the */
    /* sendto() call instead of calling connect and then send() */

    /* send the request */
    if((len=sendto(send_socket,
		   send_ring->buffer_ptr,
		   req_size,
		   MSG_EOF, 
		   (struct sockaddr *)&server,
		   sizeof(server))) != req_size) {
      if (SOCKET_EINTR(len))
	  {
	    /* we hit the end of a */
	    /* timed test. */
	    timed_out = 1;
	    break;
      }
      perror("send_tcp_tran_rr: data send error");
      exit(1);
    }
    send_ring = send_ring->next;

    /* receive the response */
    rsp_bytes_left = rsp_size;
    temp_message_ptr  = recv_ring->buffer_ptr;
    while(rsp_bytes_left > 0) {
      if((rsp_bytes_recvd=recv(send_socket,
			       temp_message_ptr,
			       rsp_bytes_left,
			       0)) == SOCKET_ERROR) {
	    if (SOCKET_EINTR(rsp_bytes_recvd))
		{
	      /* We hit the end of a timed test. */
	      timed_out = 1;
	      break;
		}
	    perror("send_tcp_tran_rr: data recv error");
	    exit(1);
      }
      rsp_bytes_left -= rsp_bytes_recvd;
      temp_message_ptr  += rsp_bytes_recvd;
    }	
    recv_ring = recv_ring->next;

    if (timed_out) {
      /* we may have been in a nested while loop - we need */
      /* another call to break. */
      break;
    }

    close(send_socket);

#ifdef WANT_HISTOGRAM
    if (verbosity > 1) {
      HIST_timestamp(&time_two);
      HIST_add(time_hist,delta_micro(&time_one,&time_two));
    }
#endif /* WANT_HISTOGRAM */

    nummessages++;          
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug > 3) {
      fprintf(where,
	      "Transaction %d completed on local port %d\n",
	      nummessages,
	      ntohs(myaddr->sin_port));
      fflush(where);
    }


  }
  
  /* this call will always give us the elapsed time for the test, and */
  /* will also store-away the necessaries for cpu utilization */

  cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being measured? */
  /* how long did we really run? */
  
  /* Get the statistics from the remote end. The remote will have */
  /* calculated service demand and all those interesting things. If it */
  /* wasn't supposed to care, it will return obvious values. */
  
  recv_response();
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote results obtained\n");
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    fprintf(where,
	    "netperf: remote error %d",
	    netperf_response.content.serv_errno);
    perror("");
    fflush(where);
    exit(1);
  }
  
  /* We now calculate what our thruput was for the test. In the future, */
  /* we may want to include a calculation of the thruput measured by */
  /* the remote, but it should be the case that for a TCP stream test, */
  /* that the two numbers should be *very* close... We calculate */
  /* bytes_sent regardless of the way the test length was controlled. */
  /* If it was time, we needed to, and if it was by bytes, the user may */
  /* have specified a number of bytes that wasn't a multiple of the */
  /* send_size, so we really didn't send what he asked for ;-) We use */
  /* Kbytes/s as the units of thruput for a TCP stream test, where K = */
  /* 1024. A future enhancement *might* be to choose from a couple of */
  /* unit selections. */ 
  
  bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
  thruput	= calc_thruput(bytes_xferd);
  
  if (local_cpu_usage || remote_cpu_usage) {
    /* We must now do a little math for service demand and cpu */
    /* utilization for the system(s) */
    /* Of course, some of the information might be bogus because */
    /* there was no idle counter in the kernel(s). We need to make */
    /* a note of this for the user's benefit...*/
    if (local_cpu_usage) {
      if (local_cpu_rate == 0.0) {
	fprintf(where,"WARNING WARNING WARNING  WARNING WARNING WARNING  WARNING!\n");
	fprintf(where,"Local CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      local_cpu_utilization = calc_cpu_util(0.0);
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      local_service_demand  = calc_service_demand((double) nummessages*1024,
						  0.0,
						  0.0,
						  0);
    }
    else {
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
    }
    
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,"DANGER  DANGER  DANGER    DANGER  DANGER  DANGER    DANGER!\n");
	fprintf(where,"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization = tcp_tran_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization,
						  tcp_tran_rr_result->num_cpus);
    }
    else {
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }
    
    /* We are now ready to print all the information. If the user */
    /* has specified zero-level verbosity, we will just print the */
    /* local service demand, or the remote service demand. If the */
    /* user has requested verbosity level 1, he will get the basic */
    /* "streamperf" numbers. If the user has specified a verbosity */
    /* of greater than 1, we will display a veritable plethora of */
    /* background information from outside of this block as it it */
    /* not cpu_measurement specific...  */
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand);
      }
      break;
    case 1:
    case 2:

      if (print_headers) {
	fprintf(where,
		cpu_title,
		local_cpu_method,
		remote_cpu_method);
      }
      
      fprintf(where,
	      cpu_fmt_1_line_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* guess */
	      elapsed_time,		/* how long was the test */
	      nummessages/elapsed_time,
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand);	/* remote service demand */
      fprintf(where,
	      cpu_fmt_1_line_2,
	      rss_size,
	      rsr_size);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      nummessages/elapsed_time);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }

      fprintf(where,
	      tput_fmt_1_line_1,	/* the format string */
	      lss_size,
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      nummessages/elapsed_time);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rss_size, 		/* remote recvbuf size */
	      rsr_size);
      
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
    fprintf(where,
	    ksink_fmt,
	    local_send_align,
	    remote_recv_offset,
	    local_send_offset,
	    remote_recv_offset);

#ifdef WANT_HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */

  }
  
}


int 
recv_tcp_tran_rr()
{
  
  char  *message;
  struct	sockaddr_in        myaddr_in,
  peeraddr_in;
  SOCKET	s_listen,s_data;
  netperf_socklen_t 	addrlen;
  int   NoPush = 1;

  char	*recv_message_ptr;
  char	*send_message_ptr;
  char	*temp_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	tcp_tran_rr_request_struct	*tcp_tran_rr_request;
  struct	tcp_tran_rr_response_struct	*tcp_tran_rr_response;
  struct	tcp_tran_rr_results_struct	*tcp_tran_rr_results;
  
  tcp_tran_rr_request = 
    (struct tcp_tran_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_tran_rr_response = 
    (struct tcp_tran_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_tran_rr_results = 
    (struct tcp_tran_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_tran_rr: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_tran_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_TRR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_tran_rr: the response type is set...\n");
    fflush(where);
  }

  /* set-up the data buffer with the requested alignment and offset */
  message = (char *)malloc(DATABUFFERLEN);
  if (message == NULL) {
    printf("malloc(%d) failed!\n", DATABUFFERLEN);
    exit(1);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_tran_rr: requested recv alignment of %d offset %d\n",
	    tcp_tran_rr_request->recv_alignment,
	    tcp_tran_rr_request->recv_offset);
    fprintf(where,
	    "recv_tcp_tran_rr: requested send alignment of %d offset %d\n",
	    tcp_tran_rr_request->send_alignment,
	    tcp_tran_rr_request->send_offset);
    fflush(where);
  }

  recv_message_ptr = ALIGN_BUFFER(message, tcp_tran_rr_request->recv_alignment, tcp_tran_rr_request->recv_offset);
  
  send_message_ptr = ALIGN_BUFFER(message, tcp_tran_rr_request->send_alignment, tcp_tran_rr_request->send_offset);
  
  if (debug) {
    fprintf(where,"recv_tcp_tran_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin_family      = AF_INET;
  myaddr_in.sin_addr.s_addr = INADDR_ANY;
  myaddr_in.sin_port        = htons((unsigned short)tcp_tran_rr_request->port);
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_tran_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_tran_rr_request->send_buf_size;
  lsr_size_req = tcp_tran_rr_request->recv_buf_size;
  loc_nodelay = tcp_tran_rr_request->no_delay;
  loc_rcvavoid = tcp_tran_rr_request->so_rcvavoid;
  loc_sndavoid = tcp_tran_rr_request->so_sndavoid;
  
  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_tran_rr_request->ipfamily),
			tcp_tran_rr_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_tran_rr_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    if (debug) {
      fprintf(where,"could not create data socket\n");
      fflush(where);
    }
    exit(1);
  }

#ifdef WIN32
  /* The test timer can fire during operations on the listening socket,
     so to make the start_timer below work we have to move
     it to close s_listen while we are blocked on accept. */
  win_kludge_socket2 = s_listen;
#endif


  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  if (bind(s_listen,
	   (struct sockaddr *)&myaddr_in,
	   sizeof(myaddr_in)) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not bind\n");
      fflush(where);
    }
    exit(1);
  }

  /* we want to disable the implicit PUSH on all sends. at some point, */
  /* this might want to be a parm to the test raj 3/95 */
  if (setsockopt(s_listen,
		 IPPROTO_TCP,
		 TCP_NOPUSH, 
		 (const char *)&NoPush,
		 sizeof(int)) == SOCKET_ERROR) {
    fprintf(where,
	    "recv_tcp_tran_rr: could not set TCP_NOPUSH errno %d\n",
	    errno);
    fflush(where);
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
  }

  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not listen\n");
      fflush(where);
    }
    exit(1);
  }
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not geetsockname\n");
      fflush(where);
    }
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_tran_rr_response->data_port_number = (int) ntohs(myaddr_in.sin_port);
  if (debug) {
    fprintf(where,"telling the remote to call me at %d\n",
	    tcp_tran_rr_response->data_port_number);
    fflush(where);
  }
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcp_tran_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (tcp_tran_rr_request->measure_cpu) {
    tcp_tran_rr_response->measure_cpu = 1;
    tcp_tran_rr_response->cpu_rate = 
      calibrate_local_cpu(tcp_tran_rr_request->cpu_rate);
  }
  

  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_tran_rr_response->send_buf_size = lss_size;
  tcp_tran_rr_response->recv_buf_size = lsr_size;
  tcp_tran_rr_response->no_delay = loc_nodelay;
  tcp_tran_rr_response->so_rcvavoid = loc_rcvavoid;
  tcp_tran_rr_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_tran_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcp_tran_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcp_tran_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcp_tran_rr_request->test_length * -1;
  }
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {

    /* accept a connection from the remote */
    if ((s_data=accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == INVALID_SOCKET) {
      if (errno == EINTR) {
	/* the timer popped */
	timed_out = 1;
	break;
      }
      fprintf(where,"recv_tcp_tran_rr: accept: errno = %d\n",errno);
      fflush(where);
      close(s_listen);
      
      exit(1);
    }
  
    if (debug) {
      fprintf(where,"recv_tcp_tran_rr: accepted data connection.\n");
      fflush(where);
    }
  
#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */

#ifdef KLUDGE_SOCKET_OPTIONS
  /* this is for those systems which *INCORRECTLY* fail to pass */
  /* attributes across an accept() call. Including this goes against */
  /* my better judgement :( raj 11/95 */

  kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */
  
    temp_message_ptr	= recv_message_ptr;
    request_bytes_remaining	= tcp_tran_rr_request->request_size;
    
    /* receive the request from the other side. we can just receive */
    /* until we get zero bytes, but that would be a slight structure */
    /* change in the code, with minimal perfomance effects. If */
    /* however, I has variable-length messages, I would want to do */
    /* this to avoid needing "double reads" - one for the message */
    /* length, and one for the rest of the message raj 3/95 */
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) == SOCKET_ERROR) {
	    if ( SOCKET_EINTR(request_bytes_recvd) )
		{
	      /* the timer popped */
	      timed_out = 1;
	      break;
		}
	    netperf_response.content.serv_errno = errno;
	    send_response();
	    exit(1);
      }
      else {
	request_bytes_remaining -= request_bytes_recvd;
	temp_message_ptr  += request_bytes_recvd;
      }
    }
    
    if (timed_out) {
      /* we hit the end of the test based on time - lets */
      /* bail out of here now... */
      fprintf(where,"yo5\n");
      fflush(where);						
      break;
    }
    
    /* Now, send the response to the remote we can use sendto here to */
    /* help remind people that this is an rfc 1644 style of test */
    if((bytes_sent=sendto(s_data,
			  send_message_ptr,
			  tcp_tran_rr_request->response_size,
			  MSG_EOF,
			  (struct sockaddr *)&peeraddr_in,
			  sizeof(struct sockaddr_storage))) == SOCKET_ERROR) {
      if (SOCKET_EINTR(bytes_sent)) {
	/* the test timer has popped */
	timed_out = 1;
	fprintf(where,"yo6\n");
	fflush(where);						
	break;
      }
      netperf_response.content.serv_errno = 99;
      send_response();
      exit(1);
    }
    
    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug) {
      fprintf(where,
	      "recv_tcp_tran_rr: Transaction %d complete\n",
	      trans_received);
      fflush(where);
    }

    /* close the connection. since we have disable PUSH on sends, the */
    /* FIN should be tacked-onto our last send instead of being */
    /* standalone */
    close(s_data);

  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(tcp_tran_rr_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_tran_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcp_tran_rr_results->bytes_received	= (trans_received * 
					   (tcp_tran_rr_request->request_size + 
					    tcp_tran_rr_request->response_size));
  tcp_tran_rr_results->trans_received	= trans_received;
  tcp_tran_rr_results->elapsed_time	= elapsed_time;
  if (tcp_tran_rr_request->measure_cpu) {
    tcp_tran_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_tran_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}
#endif /* DO_1644 */

#ifdef DO_NBRR
 /* this routine implements the sending (netperf) side of the TCP_RR */
 /* test using POSIX-style non-blocking sockets. */

void
send_tcp_nbrr(char remote_host[])
{
  
  char *tput_title = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
bytes  Bytes  bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %% %c    %% %c    us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f %c\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6d %-6d %-6d  %-6d %-6.2f  %-6.2f  %-6.2f %-6.2f %-6.3f  %-6.3f\n";
  
  char *cpu_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *ksink_fmt = "\
Alignment      Offset\n\
Local  Remote  Local  Remote\n\
Send   Recv    Send   Recv\n\
%5d  %5d   %5d  %5d\n";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  
  int	len;
  char	*temp_message_ptr;
  int	nummessages;
  SOCKET	send_socket;
  int	trans_remaining;
  double	bytes_xferd;

  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;
  
  int	rsp_bytes_left;
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct	hostent	        *hp;
  struct	sockaddr_storage	server;
  unsigned      int             addr;
  
  struct	tcp_rr_request_struct	*tcp_rr_request;
  struct	tcp_rr_response_struct	*tcp_rr_response;
  struct	tcp_rr_results_struct	*tcp_rr_result;

  struct addrinfo *remote_res;
  struct addrinfo *local_res;

  tcp_rr_request = 
    (struct tcp_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_rr_response=
    (struct tcp_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_rr_result	=
    (struct tcp_rr_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */

  bzero((char *)&server,
	sizeof(server));
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);

  if ( print_headers ) {
    print_top_test_header("TCP Non-Blocking REQUEST/RESPONSE TEST",local_res,remote_res);
  }
  
  /* initialize a few counters */
  
  send_ring = NULL;
  recv_ring = NULL;
  confidence_iteration = 1;
  init_stat();

  /* we have a great-big while loop which controls the number of times */
  /* we run a particular test. this is for the calculation of a */
  /* confidence interval (I really should have stayed awake during */
  /* probstats :). If the user did not request confidence measurement */
  /* (no confidence is the default) then we will only go though the */
  /* loop once. the confidence stuff originates from the folks at IBM */

  while (((confidence < 0) && (confidence_iteration < iteration_max)) ||
	 (confidence_iteration <= iteration_min)) {

    /* initialize a few counters. we have to remember that we might be */
    /* going through the loop more than once. */

    nummessages     = 0;
    bytes_xferd     = 0.0;
    times_up        = 0;
    timed_out       = 0;
    trans_remaining = 0;

    /* set-up the data buffers with the requested alignment and offset. */
    /* since this is a request/response test, default the send_width and */
    /* recv_width to 1 and not two raj 7/94 */

    if (send_width == 0) send_width = 1;
    if (recv_width == 0) recv_width = 1;
  
    if (send_ring == NULL) {
      send_ring = allocate_buffer_ring(send_width,
				       req_size,
				       local_send_align,
				       local_send_offset);
    }

    if (recv_ring == NULL) {
      recv_ring = allocate_buffer_ring(recv_width,
				       rsp_size,
				       local_recv_align,
				       local_recv_offset);
    }
    
    /*set up the data socket                        */
    send_socket = create_data_socket(local_res);
  
    if (send_socket == INVALID_SOCKET){
      perror("netperf: send_tcp_nbrr: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_tcp_nbrr: send_socket obtained...\n");
    }
  
    /* If the user has requested cpu utilization measurements, we must */
    /* calibrate the cpu(s). We will perform this task within the tests */
    /* themselves. If the user has specified the cpu rate, then */
    /* calibrate_local_cpu will return rather quickly as it will have */
    /* nothing to do. If local_cpu_rate is zero, then we will go through */
    /* all the "normal" calibration stuff and return the rate back.*/
    
    if (local_cpu_usage) {
      local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
    }
    
    /* Tell the remote end to do a listen. The server alters the socket */
    /* paramters on the other side at this point, hence the reason for */
    /* all the values being passed in the setup message. If the user did */
    /* not specify any of the parameters, they will be passed as 0, which */
    /* will indicate to the remote that no changes beyond the system's */
    /* default should be used. Alignment is the exception, it will */
    /* default to 8, which will be no alignment alterations. */
    
    netperf_request.content.request_type	=	DO_TCP_NBRR;
    tcp_rr_request->recv_buf_size	=	rsr_size_req;
    tcp_rr_request->send_buf_size	=	rss_size_req;
    tcp_rr_request->recv_alignment      =	remote_recv_align;
    tcp_rr_request->recv_offset	        =	remote_recv_offset;
    tcp_rr_request->send_alignment      =	remote_send_align;
    tcp_rr_request->send_offset	        =	remote_send_offset;
    tcp_rr_request->request_size	=	req_size;
    tcp_rr_request->response_size	=	rsp_size;
    tcp_rr_request->no_delay	        =	rem_nodelay;
    tcp_rr_request->measure_cpu	        =	remote_cpu_usage;
    tcp_rr_request->cpu_rate	        =	remote_cpu_rate;
    tcp_rr_request->so_rcvavoid	        =	rem_rcvavoid;
    tcp_rr_request->so_sndavoid	        =	rem_sndavoid;
    if (test_time) {
      tcp_rr_request->test_length	=	test_time;
    }
    else {
      tcp_rr_request->test_length	=	test_trans * -1;
    }
    
    if (debug > 1) {
      fprintf(where,"netperf: send_tcp_nbrr: requesting TCP rr test\n");
    }
    
    send_request();
    
    /* The response from the remote will contain all of the relevant 	*/
    /* socket parameters for this test type. We will put them back into */
    /* the variables here so they can be displayed if desired.  The	*/
    /* remote will have calibrated CPU if necessary, and will have done	*/
    /* all the needed set-up we will have calibrated the cpu locally	*/
    /* before sending the request, and will grab the counter value right*/
    /* after the connect returns. The remote will grab the counter right*/
    /* after the accept call. This saves the hassle of extra messages	*/
    /* being sent for the TCP tests.					*/
  
    recv_response();
  
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote listen done.\n");
      rsr_size          = tcp_rr_response->recv_buf_size;
      rss_size          = tcp_rr_response->send_buf_size;
      rem_nodelay       = tcp_rr_response->no_delay;
      remote_cpu_usage  = tcp_rr_response->measure_cpu;
      remote_cpu_rate   = tcp_rr_response->cpu_rate;
      /* make sure that port numbers are in network order */
      server.sin_port   = (unsigned short)tcp_rr_response->data_port_number;
      server.sin_port   = htons(server.sin_port);
    }
    else {
      Set_errno(netperf_response.content.serv_errno);
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      exit(1);
    }
    
    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		remote_res->ai_addr,
		remote_res->ai_addrlen) == INVALID_SOCKET){
      perror("netperf: data socket connect failed");
      
      exit(1);
    }
    
    /* now that we are connected, mark the socket as non-blocking */
    if (!set_nonblock(send_socket)) {
      perror("netperf: set_nonblock");
      exit(1);
    }

    /* Data Socket set-up is finished. If there were problems, either the */
    /* connect would have failed, or the previous response would have */
    /* indicated a problem. I failed to see the value of the extra */
    /* message after the accept on the remote. If it failed, we'll see it */
    /* here. If it didn't, we might as well start pumping data. */
    
    /* Set-up the test end conditions. For a request/response test, they */
    /* can be either time or transaction based. */
    
    if (test_time) {
      /* The user wanted to end the test after a period of time. */
      times_up = 0;
      trans_remaining = 0;
      start_timer(test_time);
    }
    else {
      /* The tester wanted to send a number of bytes. */
      trans_remaining = test_bytes;
      times_up = 1;
    }
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);

#ifdef WANT_INTERVALS
    INTERVALS_INIT();
#endif /* WANT_INTERVALS */
    
    /* We use an "OR" to control test execution. When the test is */
    /* controlled by time, the byte count check will always return false. */
    /* When the test is controlled by byte count, the time test will */
    /* always return false. When the test is finished, the whole */
    /* expression will go false and we will stop sending data. I think I */
    /* just arbitrarily decrement trans_remaining for the timed test, but */
    /* will not do that just yet... One other question is whether or not */
    /* the send buffer and the receive buffer should be the same buffer. */

    while ((!times_up) || (trans_remaining > 0)) {
      /* send the request. we assume that if we use a blocking socket, */
      /* the request will be sent at one shot. */
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	/* timestamp just before our call to send, and then again just */
	/* after the receive raj 8/94 */
	HIST_timestamp(&time_one);
      }
#endif /* WANT_HISTOGRAM */

      /* even though this is a non-blocking socket, we will assume for */
      /* the time being that we will be able to send an entire request */
      /* without getting an EAGAIN */
      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   req_size,
		   0)) != req_size) {
	if (SOCKET_EINTR(len)) {
	  /* we hit the end of a */
	  /* timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_tcp_nbrr: data send error");
	exit(1);
      }
      send_ring = send_ring->next;
      
      /* receive the response. since we are using non-blocking I/O, we */
      /* will "spin" on the recvs */
      rsp_bytes_left = rsp_size;
      temp_message_ptr  = recv_ring->buffer_ptr;
      while(rsp_bytes_left > 0) {
	if((rsp_bytes_recvd=recv(send_socket,
				 temp_message_ptr,
				 rsp_bytes_left,
				 0)) == SOCKET_ERROR) {
	  if (SOCKET_EINTR(rsp_bytes_recvd))
	  {
	    /* We hit the end of a timed test. */
	    timed_out = 1;
	    break;
	  }
#ifndef WIN32  // But what does WinNT indicate in this situation...
	  else if (errno == EAGAIN) {
	    Set_errno(0);
	    continue;
	  }
#endif
	  else {
	    perror("send_tcp_nbrr: data recv error");
	    exit(1);
	  }
	}
	rsp_bytes_left -= rsp_bytes_recvd;
	temp_message_ptr  += rsp_bytes_recvd;
      }	
      recv_ring = recv_ring->next;
      
      if (timed_out) {
	/* we may have been in a nested while loop - we need */
	/* another call to break. */
	break;
      }
      
#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */
#ifdef WANT_INTERVALS      
      INTERVALS_WAIT();
#endif /* WANT_INTERVALS */
      
      nummessages++;          
      if (trans_remaining) {
	trans_remaining--;
      }
      
      if (debug > 3) {
	if ((nummessages % 100) == 0) {
	  fprintf(where,
		  "Transaction %d completed\n",
		  nummessages);
	  fflush(where);
	}
      }
    }

    /* At this point we used to call shutdown on the data socket to be */
    /* sure all the data was delivered, but this was not germane in a */
    /* request/response test, and it was causing the tests to "hang" when */
    /* they were being controlled by time. So, I have replaced this */
    /* shutdown call with a call to close that can be found later in the */
    /* procedure. */
    
    /* this call will always give us the elapsed time for the test, and */
    /* will also store-away the necessaries for cpu utilization */
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured? how long */
						/* did we really run? */
    
    /* Get the statistics from the remote end. The remote will have */
    /* calculated service demand and all those interesting things. If it */
    /* wasn't supposed to care, it will return obvious values. */
    
    recv_response();
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote results obtained\n");
    }
    else {
      Set_errno(netperf_response.content.serv_errno);
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      
      exit(1);
    }
    
    /* We now calculate what our thruput was for the test. */
  
    bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
    thruput	= nummessages/elapsed_time;
  
    if (local_cpu_usage || remote_cpu_usage) {
      /* We must now do a little math for service demand and cpu */
      /* utilization for the system(s) */
      /* Of course, some of the information might be bogus because */
      /* there was no idle counter in the kernel(s). We need to make */
      /* a note of this for the user's benefit...*/
      if (local_cpu_usage) {
	local_cpu_utilization = calc_cpu_util(0.0);
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	local_service_demand  = calc_service_demand((double) nummessages*1024,
						    0.0,
						    0.0,
						    0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = tcp_rr_result->cpu_util;
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	remote_service_demand = calc_service_demand((double) nummessages*1024,
						    0.0,
						    remote_cpu_utilization,
						    tcp_rr_result->num_cpus);
      }
      else {
	remote_cpu_utilization = (float) -1.0;
	remote_service_demand  = (float) -1.0;
      }
      
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }

    /* at this point, we want to calculate the confidence information. */
    /* if debugging is on, calculate_confidence will print-out the */
    /* parameters we pass it */
    
    calculate_confidence(confidence_iteration,
			 elapsed_time,
			 thruput,
			 local_cpu_utilization,
			 remote_cpu_utilization,
			 local_service_demand,
			 remote_service_demand);
    
    
    confidence_iteration++;

    /* we are now done with the socket, so close it */
    close(send_socket);

  }

  retrieve_confident_values(&elapsed_time,
			    &thruput,
			    &local_cpu_utilization,
			    &remote_cpu_utilization,
			    &local_service_demand,
			    &remote_service_demand);

  /* We are now ready to print all the information. If the user */
  /* has specified zero-level verbosity, we will just print the */
  /* local service demand, or the remote service demand. If the */
  /* user has requested verbosity level 1, he will get the basic */
  /* "streamperf" numbers. If the user has specified a verbosity */
  /* of greater than 1, we will display a veritable plethora of */
  /* background information from outside of this block as it it */
  /* not cpu_measurement specific...  */

  if (confidence < 0) {
    /* we did not hit confidence, but were we asked to look for it? */
    if (iteration_max > 1) {
      display_confidence();
    }
  }

  if (local_cpu_usage || remote_cpu_usage) {
    local_cpu_method = format_cpu_method(cpu_method);
    remote_cpu_method = format_cpu_method(tcp_rr_result->cpu_method);
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand,
		local_cpu_method);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand,
		remote_cpu_method);
      }
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,
		cpu_title,
		local_cpu_method,
		remote_cpu_method);
      }

      fprintf(where,
	      cpu_fmt_1_line_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* guess */
	      elapsed_time,		/* how long was the test */
	      thruput,
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand);	/* remote service demand */
      fprintf(where,
	      cpu_fmt_1_line_2,
	      rss_size,
	      rsr_size);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      thruput);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }

      fprintf(where,
	      tput_fmt_1_line_1,	/* the format string */
	      lss_size,
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      thruput);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rss_size, 		/* remote recvbuf size */
	      rsr_size);
      
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  /* how to handle the verbose information in the presence of */
  /* confidence intervals is yet to be determined... raj 11/94 */
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
    fprintf(where,
	    ksink_fmt,
	    local_send_align,
	    remote_recv_offset,
	    local_send_offset,
	    remote_recv_offset);

#ifdef WANT_HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */

  }
  
}

 /* this routine implements the receive (netserver) side of a TCP_RR */
 /* test */
void 
recv_tcp_nbrr()
{
  
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;

  struct	sockaddr_in        myaddr_in,
  peeraddr_in;
  SOCKET	s_listen,s_data;
  netperf_socklen_t 	addrlen;
  char	*temp_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;

  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];
  
  struct	tcp_rr_request_struct	*tcp_rr_request;
  struct	tcp_rr_response_struct	*tcp_rr_response;
  struct	tcp_rr_results_struct	*tcp_rr_results;
  
  tcp_rr_request = 
    (struct tcp_rr_request_struct *)netperf_request.content.test_specific_data;
  tcp_rr_response =
    (struct tcp_rr_response_struct *)netperf_response.content.test_specific_data;
  tcp_rr_results =
    (struct tcp_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_nbrr: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: the response type is set...\n");
    fflush(where);
  }
  
  /* allocate the recv and send rings with the requested alignments */
  /* and offsets. raj 7/94 */
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: requested recv alignment of %d offset %d\n",
	    tcp_rr_request->recv_alignment,
	    tcp_rr_request->recv_offset);
    fprintf(where,"recv_tcp_nbrr: requested send alignment of %d offset %d\n",
	    tcp_rr_request->send_alignment,
	    tcp_rr_request->send_offset);
    fflush(where);
  }

  /* at some point, these need to come to us from the remote system */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   tcp_rr_request->response_size,
				   tcp_rr_request->send_alignment,
				   tcp_rr_request->send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   tcp_rr_request->request_size,
				   tcp_rr_request->recv_alignment,
				   tcp_rr_request->recv_offset);

  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin_family      = AF_INET;
  myaddr_in.sin_addr.s_addr = INADDR_ANY;
  myaddr_in.sin_port        = htons((unsigned short)tcp_rr_request->port);
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_rr_request->send_buf_size;
  lsr_size_req = tcp_rr_request->recv_buf_size;
  loc_nodelay = tcp_rr_request->no_delay;
  loc_rcvavoid = tcp_rr_request->so_rcvavoid;
  loc_sndavoid = tcp_rr_request->so_sndavoid;
  
  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_rr_request->ipfamily),
			tcp_rr_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_rr_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    
    exit(1);
  }
  
  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  if (bind(s_listen,
	   (struct sockaddr *)&myaddr_in,
	   sizeof(myaddr_in)) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in, &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_rr_response->data_port_number = (int) ntohs(myaddr_in.sin_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcp_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  tcp_rr_response->measure_cpu = 0;

  if (tcp_rr_request->measure_cpu) {
    tcp_rr_response->measure_cpu = 1;
    tcp_rr_response->cpu_rate = calibrate_local_cpu(tcp_rr_request->cpu_rate);
  }
  
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_rr_response->send_buf_size = lss_size;
  tcp_rr_response->recv_buf_size = lsr_size;
  tcp_rr_response->no_delay = loc_nodelay;
  tcp_rr_response->so_rcvavoid = loc_rcvavoid;
  tcp_rr_response->so_sndavoid = loc_sndavoid;
  tcp_rr_response->test_length = tcp_rr_request->test_length;
  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  if ((s_data = accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == INVALID_SOCKET) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: accept completes on the data connection.\n");
    fflush(where);
  }

#ifdef KLUDGE_SOCKET_OPTIONS
  /* this is for those systems which *INCORRECTLY* fail to pass */
  /* attributes across an accept() call. Including this goes against */
  /* my better judgement :( raj 11/95 */

  kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */
    
  /* now that we are connected, mark the socket as non-blocking */
  if (!set_nonblock(s_data)) {
    close(s_data);
    exit(1);
  }

  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_rr_request->measure_cpu);

#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcp_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcp_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcp_rr_request->test_length * -1;
  }

  trans_received = 0;
  
  while ((!times_up) || (trans_remaining > 0)) {
    temp_message_ptr = recv_ring->buffer_ptr;
    request_bytes_remaining	= tcp_rr_request->request_size;
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) == SOCKET_ERROR) {
	    if ( SOCKET_EINTR(request_bytes_recvd))
		{
	      /* the timer popped */
	      timed_out = 1;
	      break;
		}
#ifndef WIN32  // But what does WinNT indicate in this situation...
	    else if (errno == EAGAIN) {
	      Set_errno(0);
	      if (times_up) {
	        timed_out = 1;
	        break;
		  }
	      continue;
		}
#endif
	    else {
	      netperf_response.content.serv_errno = errno;
	      send_response();
	      exit(1);
		}
      }
      else {
	request_bytes_remaining -= request_bytes_recvd;
	temp_message_ptr  += request_bytes_recvd;
      }
    }

    recv_ring = recv_ring->next;

    if (timed_out) {
      /* we hit the end of the test based on time - lets */
      /* bail out of here now... */
      fprintf(where,"yo5\n");
      fflush(where);						
      break;
    }
    
    /* Now, send the response to the remote */
    if((bytes_sent=send(s_data,
			send_ring->buffer_ptr,
			tcp_rr_request->response_size,
			0)) == SOCKET_ERROR) {
      if (SOCKET_EINTR(bytes_sent)) {
	/* the test timer has popped */
	timed_out = 1;
	fprintf(where,"yo6\n");
	fflush(where);						
	break;
      }
      netperf_response.content.serv_errno = 992;
      send_response();
      exit(1);
    }
    
    send_ring = send_ring->next;

    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(tcp_rr_request->measure_cpu,&elapsed_time);
  
  stop_timer();

  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }

  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_nbrr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcp_rr_results->bytes_received = (trans_received * 
				    (tcp_rr_request->request_size + 
				     tcp_rr_request->response_size));
  tcp_rr_results->trans_received = trans_received;
  tcp_rr_results->elapsed_time   = elapsed_time;
  tcp_rr_results->cpu_method     = cpu_method;
  tcp_rr_results->num_cpus       = lib_num_loc_cpus;
  if (tcp_rr_request->measure_cpu) {
    tcp_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_nbrr: test complete, sending results.\n");
    fflush(where);
  }
  
  /* we are done with the socket, free it */
  close(s_data);

  send_response();
  
}

#endif /* DO_NBRR */


 /* this test is intended to test the performance of establishing a */
 /* connection, and then closing it again. this test is of somewhat */
 /* arcane interest since no packets are exchanged between the */
 /* user-space processes, but it will show the raw overhead of */
 /* establishing a TCP connection. that service demand could then be */
 /* compared with the sum of the service demands of a TCP_CRR and */
 /* TCP_RR test - presumeably, they would all relate */

void
send_tcp_cc(char remote_host[])
{
  
  char *tput_title = "\
Local /Remote\n\
Socket Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
bytes  Bytes  bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Socket Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
bytes  bytes  bytes   bytes  secs.   per sec  %%      %%      us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6d %-6d %-6d  %-6d %-6.2f  %-6.2f   %-6.2f %-6.2f %-6.3f  %-6.3f\n";
  
  char *cpu_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *ksink_fmt = "\n\
Alignment      Offset\n\
Local  Remote  Local  Remote\n\
Send   Recv    Send   Recv\n\
%5d  %5d   %5d  %5d\n";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  
  char	temp_message_ptr[1];
  int	nummessages;
  SOCKET	send_socket;
  int	trans_remaining;
  double	bytes_xferd;
  int	rsp_bytes_left = 1;
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct addrinfo *local_res;
  struct addrinfo *remote_res;

  int                           myport;
  int                           ret;

  struct	tcp_cc_request_struct	*tcp_cc_request;
  struct	tcp_cc_response_struct	*tcp_cc_response;
  struct	tcp_cc_results_struct	*tcp_cc_result;
  
  tcp_cc_request = 
    (struct tcp_cc_request_struct *)netperf_request.content.test_specific_data;
  tcp_cc_response = 
    (struct tcp_cc_response_struct *)netperf_response.content.test_specific_data;
  tcp_cc_result =
    (struct tcp_cc_results_struct *)netperf_response.content.test_specific_data;
  
  
#ifdef WANT_HISTOGRAM
  if (verbosity > 1) {
    time_hist = HIST_new();
  }
#endif /* WANT_HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  complete_addrinfos(&remote_res,
		     &local_res,
		     remote_host,
		     SOCK_STREAM,
		     IPPROTO_TCP,
		     0);

  if ( print_headers ) {
    print_top_test_header("TCP Connect/Close TEST",local_res,remote_res);
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_xferd	=	0.0;
  times_up 	= 	0;
  
  /* since there are no data buffers in this test, we need no send or */
  /* recv rings */

  if (debug) {
    fprintf(where,"send_tcp_cc: send_socket obtained...\n");
  }
  
  /* If the user has requested cpu utilization measurements, we must */
  /* calibrate the cpu(s). We will perform this task within the tests */
  /* themselves. If the user has specified the cpu rate, then */
  /* calibrate_local_cpu will return rather quickly as it will have */
  /* nothing to do. If local_cpu_rate is zero, then we will go through */
  /* all the "normal" calibration stuff and return the rate back.*/
  
  if (local_cpu_usage) {
    local_cpu_rate = calibrate_local_cpu(local_cpu_rate);
  }
  
  /* Tell the remote end to do a listen. The server alters the socket */
  /* paramters on the other side at this point, hence the reason for */
  /* all the values being passed in the setup message. If the user did */
  /* not specify any of the parameters, they will be passed as 0, which */
  /* will indicate to the remote that no changes beyond the system's */
  /* default should be used. Alignment is the exception, it will */
  /* default to 8, which will be no alignment alterations. */
  
  netperf_request.content.request_type	=	DO_TCP_CC;
  tcp_cc_request->recv_buf_size	        =	rsr_size_req;
  tcp_cc_request->send_buf_size	        =	rss_size_req;
  tcp_cc_request->recv_alignment	=	remote_recv_align;
  tcp_cc_request->recv_offset	        =	remote_recv_offset;
  tcp_cc_request->send_alignment	=	remote_send_align;
  tcp_cc_request->send_offset	        =	remote_send_offset;
  tcp_cc_request->request_size	        =	req_size;
  tcp_cc_request->response_size	        =	rsp_size;
  tcp_cc_request->no_delay	        =	rem_nodelay;
  tcp_cc_request->measure_cpu	        =	remote_cpu_usage;
  tcp_cc_request->cpu_rate	        =	remote_cpu_rate;
  tcp_cc_request->so_rcvavoid	=	rem_rcvavoid;
  tcp_cc_request->so_sndavoid	=	rem_sndavoid;
  if (test_time) {
    tcp_cc_request->test_length	=	test_time;
  }
  else {
    tcp_cc_request->test_length	=	test_trans * -1;
  }
  tcp_cc_request->port          = atoi(remote_data_port);
  tcp_cc_request->ipfamily  = af_to_nf(remote_res->ai_family);

  if (debug > 1) {
    fprintf(where,"netperf: send_tcp_cc: requesting TCP crr test\n");
  }
  
  send_request();
  
  /* The response from the remote will contain all of the relevant 	*/
  /* socket parameters for this test type. We will put them back into 	*/
  /* the variables here so they can be displayed if desired.  The	*/
  /* remote will have calibrated CPU if necessary, and will have done	*/
  /* all the needed set-up we will have calibrated the cpu locally	*/
  /* before sending the request, and will grab the counter value right	*/
  /* after the connect returns. The remote will grab the counter right	*/
  /* after the accept call. This saves the hassle of extra messages	*/
  /* being sent for the TCP tests.					*/
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    rsr_size	=	tcp_cc_response->recv_buf_size;
    rss_size	=	tcp_cc_response->send_buf_size;
    rem_nodelay	=	tcp_cc_response->no_delay;
    remote_cpu_usage=	tcp_cc_response->measure_cpu;
    remote_cpu_rate = 	tcp_cc_response->cpu_rate;
    /* make sure that port numbers are in network order */
    set_port_number(remote_res,(unsigned short)tcp_cc_response->data_port_number);

    if (debug) {
      fprintf(where,"remote listen done.\n");
      fprintf(where,"remote port is %d\n",get_port_number(remote_res));
      fflush(where);
    }
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    fprintf(where,
	    "netperf: remote error %d",
	    netperf_response.content.serv_errno);
    perror("");
    fflush(where);
    exit(1);
  }

#ifdef WANT_DEMO
  DEMO_RR_SETUP(100)
#endif
  
  /* pick a nice random spot between client_port_min and */
  /* client_port_max for our initial port number */
  srand(getpid());
  if (client_port_max - client_port_min) {
    myport = client_port_min + 
      (rand() % (client_port_max - client_port_min));
  }
  else {
    myport = client_port_min;
  }
  /* there will be a ++ before the first call to bind, so subtract one */
  myport--;
  
  /* Set-up the test end conditions. For a request/response test, they */
  /* can be either time or transaction based. */
  
  if (test_time) {
    /* The user wanted to end the test after a period of time. */
    times_up = 0;
    trans_remaining = 0;
    start_timer(test_time);
  }
  else {
    /* The tester wanted to send a number of bytes. */
    trans_remaining = test_bytes;
    times_up = 1;
  }
  
  /* The cpu_start routine will grab the current time and possibly */
  /* value of the idle counter for later use in measuring cpu */
  /* utilization and/or service demand and thruput. */
  
  cpu_start(local_cpu_usage);

#ifdef WANT_DEMO
  if (demo_mode) {
    HIST_timestamp(demo_one_ptr);
  }
#endif

  /* We use an "OR" to control test execution. When the test is */
  /* controlled by time, the byte count check will always return false. */
  /* When the test is controlled by byte count, the time test will */
  /* always return false. When the test is finished, the whole */
  /* expression will go false and we will stop sending data. I think I */
  /* just arbitrarily decrement trans_remaining for the timed test, but */
  /* will not do that just yet... One other question is whether or not */
  /* the send buffer and the receive buffer should be the same buffer. */

  while ((!times_up) || (trans_remaining > 0)) {

#ifdef WANT_HISTOGRAM
    if (verbosity > 1) {
      /* timestamp just before our call to create the socket, and then */
      /* again just after the receive raj 3/95 */
      HIST_timestamp(&time_one);
    }
#endif /* WANT_HISTOGRAM */

    /* set up the data socket */
    /* newport: is this label really required any longer? */
    /* pick a new port number */
    myport++;

    /* wrap the port number when we get to client_port_max. NOTE, some */
    /* broken TCP's might treat the port number as a signed 16 bit */
    /* quantity.  we aren't interested in testing such broken */
    /* implementations :) so we won't make sure that it is below 32767 */
    /* raj 8/94  */
    if (myport >= client_port_max) {
      myport = client_port_min;
    }

    /* we do not want to use the port number that the server is */
    /* sitting at - this would cause us to fail in a loopback test. we */
    /* could just rely on the failure of the bind to get us past this, */
    /* but I'm guessing that in this one case at least, it is much */
    /* faster, given that we *know* that port number is already in use */
    /* (or rather would be in a loopback test) */

    if (myport == get_port_number(remote_res)) myport++;

    if (debug) {
      if ((nummessages % 100) == 0) {
	printf("port %d\n",myport);
      }
    }
    set_port_number(local_res, (unsigned short)myport);
    send_socket = create_data_socket(local_res);

    if (send_socket == INVALID_SOCKET) {
      perror("netperf: send_tcp_cc: tcp stream data socket");
      exit(1);
    }

    /* we used to have a call to bind() here, but that is being
       taken care of by create_data_socket(). raj 2005-02-08 */

    /* Connect up to the remote port on the data socket  */
    if ((ret = connect(send_socket, 
		       remote_res->ai_addr,
		       remote_res->ai_addrlen)) == INVALID_SOCKET){
      if (SOCKET_EINTR(ret))
	  {
	    /* we hit the end of a */
	    /* timed test. */
	    timed_out = 1;
	    break;
      }
      perror("netperf: data socket connect failed");
      printf("\tattempted to connect on socket %d to port %d",
	     send_socket,
	     get_port_number(remote_res));
      printf(" from port %u \n",get_port_number(local_res));
      exit(1);
    }

    /* we hang in a recv() to get the remote's close indication */

    rsp_bytes_recvd=recv(send_socket,
			 temp_message_ptr,
			 rsp_bytes_left,
			 0);


    if (rsp_bytes_recvd == 0) {
      /* connection close, call close. we assume that the requisite */
      /* number of bytes have been received */

#ifdef WANT_HISTOGRAM
      if (verbosity > 1) {
	HIST_timestamp(&time_two);
	HIST_add(time_hist,delta_micro(&time_one,&time_two));
      }
#endif /* WANT_HISTOGRAM */

#ifdef WANT_DEMO
      DEMO_RR_INTERVAL(1)
#endif

      nummessages++;          
      if (trans_remaining) {
	trans_remaining--;
      }
    
      if (debug > 3) {
	fprintf(where,
		"Transaction %d completed on local port %u\n",
		nummessages,
		get_port_number(local_res));
	fflush(where);
      }

      close(send_socket);

    }
    else {
      /* it was less than zero - an error occured */
      if (SOCKET_EINTR(rsp_bytes_recvd))
	  {
	    /* We hit the end of a timed test. */
	    timed_out = 1;
	    break;
	  }
	  perror("send_tcp_cc: data recv error");
	  exit(1);
    }
      
  }

  
  /* this call will always give us the elapsed time for the test, and */
  /* will also store-away the necessaries for cpu utilization */

  cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being measured? */
  /* how long did we really run? */
  
  /* Get the statistics from the remote end. The remote will have */
  /* calculated service demand and all those interesting things. If it */
  /* wasn't supposed to care, it will return obvious values. */
  
  recv_response();
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote results obtained\n");
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    fprintf(where,
	    "netperf: remote error %d",
	     netperf_response.content.serv_errno);
    perror("");
    fflush(where);
      
    exit(1);
  }
  
  /* We now calculate what our thruput was for the test. In the future, */
  /* we may want to include a calculation of the thruput measured by */
  /* the remote, but it should be the case that for a TCP stream test, */
  /* that the two numbers should be *very* close... We calculate */
  /* bytes_sent regardless of the way the test length was controlled. */
  /* If it was time, we needed to, and if it was by bytes, the user may */
  /* have specified a number of bytes that wasn't a multiple of the */
  /* send_size, so we really didn't send what he asked for ;-) We use */
  /* Kbytes/s as the units of thruput for a TCP stream test, where K = */
  /* 1024. A future enhancement *might* be to choose from a couple of */
  /* unit selections. */ 
  
  bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
  thruput	= calc_thruput(bytes_xferd);
  
  if (local_cpu_usage || remote_cpu_usage) {
    /* We must now do a little math for service demand and cpu */
    /* utilization for the system(s) */
    /* Of course, some of the information might be bogus because */
    /* there was no idle counter in the kernel(s). We need to make */
    /* a note of this for the user's benefit...*/
    if (local_cpu_usage) {
      if (local_cpu_rate == 0.0) {
	fprintf(where,"WARNING WARNING WARNING  WARNING WARNING WARNING  WARNING!\n");
	fprintf(where,"Local CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      local_cpu_utilization = calc_cpu_util(0.0);
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      local_service_demand  = calc_service_demand((double) nummessages*1024,
						  0.0,
						  0.0,
						  0);
    }
    else {
      local_cpu_utilization	= (float) -1.0;
      local_service_demand	= (float) -1.0;
    }
    
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,"DANGER  DANGER  DANGER    DANGER  DANGER  DANGER    DANGER!\n");
	fprintf(where,"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization = tcp_cc_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization,
						  tcp_cc_result->num_cpus);
    }
    else {
      remote_cpu_utilization = (float) -1.0;
      remote_service_demand  = (float) -1.0;
    }
    
    /* We are now ready to print all the information. If the user */
    /* has specified zero-level verbosity, we will just print the */
    /* local service demand, or the remote service demand. If the */
    /* user has requested verbosity level 1, he will get the basic */
    /* "streamperf" numbers. If the user has specified a verbosity */
    /* of greater than 1, we will display a veritable plethora of */
    /* background information from outside of this block as it it */
    /* not cpu_measurement specific...  */
    
    switch (verbosity) {
    case 0:
      if (local_cpu_usage) {
	fprintf(where,
		cpu_fmt_0,
		local_service_demand);
      }
      else {
	fprintf(where,
		cpu_fmt_0,
		remote_service_demand);
      }
      break;
    case 1:
    case 2:

      if (print_headers) {
	fprintf(where,
		cpu_title,
		local_cpu_method,
		remote_cpu_method);
      }
      
      fprintf(where,
	      cpu_fmt_1_line_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* guess */
	      elapsed_time,		/* how long was the test */
	      nummessages/elapsed_time,
	      local_cpu_utilization,	/* local cpu */
	      remote_cpu_utilization,	/* remote cpu */
	      local_service_demand,	/* local service demand */
	      remote_service_demand);	/* remote service demand */
      fprintf(where,
	      cpu_fmt_1_line_2,
	      rss_size,
	      rsr_size);
      break;
    }
  }
  else {
    /* The tester did not wish to measure service demand. */
    switch (verbosity) {
    case 0:
      fprintf(where,
	      tput_fmt_0,
	      nummessages/elapsed_time);
      break;
    case 1:
    case 2:
      if (print_headers) {
	fprintf(where,tput_title,format_units());
      }

      fprintf(where,
	      tput_fmt_1_line_1,	/* the format string */
	      lss_size,
	      lsr_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      nummessages/elapsed_time);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rss_size, 		/* remote recvbuf size */
	      rsr_size);
      
      break;
    }
  }
  
  /* it would be a good thing to include information about some of the */
  /* other parameters that may have been set for this test, but at the */
  /* moment, I do not wish to figure-out all the  formatting, so I will */
  /* just put this comment here to help remind me that it is something */
  /* that should be done at a later time. */
  
  if (verbosity > 1) {
    /* The user wanted to know it all, so we will give it to him. */
    /* This information will include as much as we can find about */
    /* TCP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
    fprintf(where,
	    ksink_fmt,
	    local_send_align,
	    remote_recv_offset,
	    local_send_offset,
	    remote_recv_offset);

#ifdef WANT_HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* WANT_HISTOGRAM */

  }
  
}


void
recv_tcp_cc()
{
  
  char  *message;

  struct addrinfo *local_res;
  char local_name[BUFSIZ];
  char port_buffer[PORTBUFSIZE];

  struct	sockaddr_storage        myaddr_in,  peeraddr_in;
  SOCKET	s_listen,s_data;
  netperf_socklen_t 	addrlen;
  char	*recv_message_ptr;
  char	*send_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	tcp_cc_request_struct	*tcp_cc_request;
  struct	tcp_cc_response_struct	*tcp_cc_response;
  struct	tcp_cc_results_struct	*tcp_cc_results;
  
  tcp_cc_request = 
    (struct tcp_cc_request_struct *)netperf_request.content.test_specific_data;
  tcp_cc_response = 
    (struct tcp_cc_response_struct *)netperf_response.content.test_specific_data;
  tcp_cc_results = 
    (struct tcp_cc_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcp_cc: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen socket with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the socket sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_tcp_cc: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_CC_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_cc: the response type is set...\n");
    fflush(where);
  }

  /* set-up the data buffer with the requested alignment and offset */
  message = (char *)malloc(DATABUFFERLEN);
  if (message == NULL) {
    printf("malloc(%d) failed!\n", DATABUFFERLEN);
    exit(1);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_cc: requested recv alignment of %d offset %d\n",
	    tcp_cc_request->recv_alignment,
	    tcp_cc_request->recv_offset);
    fprintf(where,
	    "recv_tcp_cc: requested send alignment of %d offset %d\n",
	    tcp_cc_request->send_alignment,
	    tcp_cc_request->send_offset);
    fflush(where);
  }

  recv_message_ptr = ALIGN_BUFFER(message, tcp_cc_request->recv_alignment, tcp_cc_request->recv_offset);
  
  send_message_ptr = ALIGN_BUFFER(message, tcp_cc_request->send_alignment, tcp_cc_request->send_offset);

  if (debug) {
    fprintf(where,"recv_tcp_cc: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_cc: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = tcp_cc_request->send_buf_size;
  lsr_size_req = tcp_cc_request->recv_buf_size;
  loc_nodelay = tcp_cc_request->no_delay;
  loc_rcvavoid = tcp_cc_request->so_rcvavoid;
  loc_sndavoid = tcp_cc_request->so_sndavoid;

  set_hostname_and_port(local_name,
			port_buffer,
			nf_to_af(tcp_cc_request->ipfamily),
			tcp_cc_request->port);

  local_res = complete_addrinfo(local_name,
				local_name,
				port_buffer,
				nf_to_af(tcp_cc_request->ipfamily),
				SOCK_STREAM,
				IPPROTO_TCP,
				0);

  s_listen = create_data_socket(local_res);
  
  if (s_listen == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    if (debug) {
      fprintf(where,"could not create data socket\n");
      fflush(where);
    }
    exit(1);
  }

#ifdef WIN32
  /* The test timer can fire during operations on the listening socket,
     so to make the start_timer below work we have to move
     it to close s_listen while we are blocked on accept. */
  win_kludge_socket2 = s_listen;
#endif


  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not listen\n");
      fflush(where);
    }
    exit(1);
  }
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not geetsockname\n");
      fflush(where);
    }
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcp_cc_response->data_port_number = 
    (int) ntohs(((struct sockaddr_in *)&myaddr_in)->sin_port);
  if (debug) {
    fprintf(where,"telling the remote to call me at %d\n",
	    tcp_cc_response->data_port_number);
    fflush(where);
  }
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcp_cc_response->cpu_rate = (float)0.0; 	/* assume no cpu */
  if (tcp_cc_request->measure_cpu) {
    tcp_cc_response->measure_cpu = 1;
    tcp_cc_response->cpu_rate = 
      calibrate_local_cpu(tcp_cc_request->cpu_rate);
  }
  

  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcp_cc_response->send_buf_size = lss_size;
  tcp_cc_response->recv_buf_size = lsr_size;
  tcp_cc_response->no_delay = loc_nodelay;
  tcp_cc_response->so_rcvavoid = loc_rcvavoid;
  tcp_cc_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcp_cc_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcp_cc_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcp_cc_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcp_cc_request->test_length * -1;
  }
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {
#ifdef WIN32
    /* The test timer will probably fire during this accept, 
       so to make the start_timer above work we have to move
       it to close s_listen while we are blocked on accept. */
    win_kludge_socket = s_listen;
#endif
    /* accept a connection from the remote */
    if ((s_data=accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == INVALID_SOCKET) {
      if (errno == EINTR) {
	/* the timer popped */
	timed_out = 1;
	break;
      }
      fprintf(where,"recv_tcp_cc: accept: errno = %d\n",errno);
      fflush(where);
      close(s_listen);
      
      exit(1);
    }

#ifdef KLUDGE_SOCKET_OPTIONS
    /* this is for those systems which *INCORRECTLY* fail to pass */
    /* attributes across an accept() call. Including this goes against */
    /* my better judgement :( raj 11/95 */
    
    kludge_socket_options(s_data);

#endif /* KLUDGE_SOCKET_OPTIONS */
  
#ifdef WIN32
  /* this is used so the timer thread can close the socket out from */
  /* under us, which to date is the easiest/cleanest/least */
  /* Windows-specific way I can find to force the winsock calls to */
  /* return WSAEINTR with the test is over. anything that will run on */
  /* 95 and NT and is closer to what netperf expects from Unix signals */
  /* and such would be appreciated raj 1/96 */
  win_kludge_socket = s_data;
#endif /* WIN32 */

    if (debug) {
      fprintf(where,"recv_tcp_cc: accepted data connection.\n");
      fflush(where);
    }
  

    /* close the connection. the server will likely do a graceful */
    /* close of the connection, insuring that all data has arrived at */
    /* the client. for this it will call shutdown(), and then recv() and */
    /* then close(). I'm reasonably confident that this is the */
    /* appropriate sequence of calls - I would like to hear of */
    /* examples in web servers to the contrary. raj 10/95*/
    close(s_data);

    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug) {
      fprintf(where,
	      "recv_tcp_cc: Transaction %d complete\n",
	      trans_received);
      fflush(where);
    }

  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(tcp_cc_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_cc: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcp_cc_results->bytes_received	= (trans_received * 
					   (tcp_cc_request->request_size + 
					    tcp_cc_request->response_size));
  tcp_cc_results->trans_received	= trans_received;
  tcp_cc_results->elapsed_time	= elapsed_time;
  if (tcp_cc_request->measure_cpu) {
    tcp_cc_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcp_cc: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}

void
print_sockets_usage()
{

  fwrite(sockets_usage, sizeof(char), strlen(sockets_usage), stdout);
  exit(1);

}

void
scan_sockets_args(int argc, char *argv[])

{

#define SOCKETS_ARGS "b:CDnNhH:L:m:M:p:P:r:s:S:T:Vw:W:z46"

  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	
    arg1[BUFSIZ],  /* argument holders		*/
    arg2[BUFSIZ];

  if (debug) {
    int i;
    printf("%s called with the following argument vector\n",
	   __func__);
    for (i = 0; i< argc; i++) {
      printf("%s ",argv[i]);
    }
    printf("\n");
  }

  strncpy(local_data_port,"0",sizeof(local_data_port));
  strncpy(remote_data_port,"0",sizeof(remote_data_port));

  /* Go through all the command line arguments and break them */
  /* out. For those options that take two parms, specifying only */
  /* the first will set both to that value. Specifying only the */
  /* second will leave the first untouched. To change only the */
  /* first, use the form "first," (see the routine break_args.. */
  
  while ((c= getopt(argc, argv, SOCKETS_ARGS)) != EOF) {
    switch (c) {
    case '?':	
    case '4':
      remote_data_family = AF_INET;
      local_data_family = AF_INET;
      break;
    case '6':
#if defined(AF_INET6)
      remote_data_family = AF_INET6;
      local_data_family = AF_INET6;
#else
      fprintf(stderr,
	      "This netperf was not compiled on an IPv6 capable host!\n");
      fflush(stderr);
      exit(-1);
#endif
      break;
    case 'h':
      print_sockets_usage();
      exit(1);
    case 'b':
#ifdef WANT_FIRST_BURST
      first_burst_size = atoi(optarg);
#else /* WANT_FIRST_BURST */
      printf("Initial request burst functionality not compiled-in!\n");
#endif /* WANT_FIRST_BURST */
      break;
    case 'C':
#ifdef TCP_CORK
      /* set TCP_CORK */
      loc_tcpcork = 1;
      rem_tcpcork = 1; /* however, at first, we ony have cork affect loc */
#else 
      printf("WARNING: TCP_CORK not available on this platform!\n");
#endif /* TCP_CORK */
      break;
    case 'D':
      /* set the TCP nodelay flag */
      loc_nodelay = 1;
      rem_nodelay = 1;
      break;
    case 'H':
      break_args_explicit(optarg,arg1,arg2);
      if (arg1[0]) {
	/* make sure we leave room for the NULL termination boys and
	   girls. raj 2005-02-82 */ 
	remote_data_address = malloc(strlen(arg1)+1);
	strncpy(remote_data_address,arg1,strlen(arg1));
      }
      if (arg2[0])
	remote_data_family = parse_address_family(arg2);
      break;
    case 'L':
      break_args_explicit(optarg,arg1,arg2);
      if (arg1[0]) {
	/* make sure we leave room for the NULL termination boys and
	   girls. raj 2005-02-82 */ 
	local_data_address = malloc(strlen(arg1)+1);
	strncpy(local_data_address,arg1,strlen(arg1));
      }
      if (arg2[0])
	local_data_family = parse_address_family(arg2);
      break;
    case 's':
      /* set local socket sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	lss_size_req = convert(arg1);
      if (arg2[0])
	lsr_size_req = convert(arg2);
      break;
    case 'S':
      /* set remote socket sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	rss_size_req = convert(arg1);
      if (arg2[0])
	rsr_size_req = convert(arg2);
      break;
    case 'r':
      /* set the request/response sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	req_size = convert(arg1);
      if (arg2[0])	
	rsp_size = convert(arg2);
      break;
    case 'm':
      /* set the send size */
      send_size = convert(optarg);
      break;
    case 'M':
      /* set the recv size */
      recv_size = convert(optarg);
      break;
    case 'n':
      /* set the local socket type*/
      local_connected = 1;
      break;
    case 'N':
      /* set the remote socket type*/
      remote_connected = 1;
      break;
    case 'p':
      /* set the min and max port numbers for the TCP_CRR and TCP_TRR */
      /* tests. */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	client_port_min = atoi(arg1);
      if (arg2[0])	
	client_port_max = atoi(arg2);
      break;
    case 'P':
      /* set the local and remote data port numbers for the tests to
	 allow them to run through those blankety blank end-to-end
	 breaking firewalls. raj 2004-06-15 */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	strncpy(local_data_port,arg1,sizeof(local_data_port));
      if (arg2[0])	
	strncpy(remote_data_port,arg2,sizeof(remote_data_port));
      break;
    case 't':
      /* set the test name */
      strcpy(test_name,optarg);
      break;
    case 'W':
      /* set the "width" of the user space data */
      /* buffer. This will be the number of */
      /* send_size buffers malloc'd in the */
      /* *_STREAM test. It may be enhanced to set */
      /* both send and receive "widths" but for now */
      /* it is just the sending *_STREAM. */
      send_width = convert(optarg);
      break;
    case 'V' :
      /* we want to do copy avoidance and will set */
      /* it for everything, everywhere, if we really */
      /* can. of course, we don't know anything */
      /* about the remote... */
#ifdef SO_SND_COPYAVOID
      loc_sndavoid = 1;
#else
      loc_sndavoid = 0;
      printf("Local send copy avoidance not available.\n");
#endif
#ifdef SO_RCV_COPYAVOID
      loc_rcvavoid = 1;
#else
      loc_rcvavoid = 0;
      printf("Local recv copy avoidance not available.\n");
#endif
      rem_sndavoid = 1;
      rem_rcvavoid = 1;
      break;
    };
  }

#if defined(WANT_FIRST_BURST) 
#if defined(WANT_HISTOGRAM)
  /* if WANT_FIRST_BURST and WANT_HISTOGRAM are defined and the user
     indeed wants a non-zero first burst size, and we would emit a
     histogram, then we should emit a warning that the two are not
     compatible. raj 2006-01-31 */
  if ((first_burst_size > 0) && (verbosity >= 2)) {
    fprintf(stderr,
	    "WARNING! Histograms and first bursts are incompatible!\n");
    fflush(stderr);
  }
#endif
#endif

  /* we do not want to make remote_data_address non-NULL because if
     the user has not specified a remote adata address, we want to
     take it from the hostname in the -H global option. raj
     2005-02-08 */

  /* so, if there is to be no control connection, we want to have some
     different settings for a few things */

  if (no_control) {

    if (strcmp(remote_data_port,"0") == 0) {
      /* we need to select either the discard port, echo port or
	 chargen port dedepending on the test name. raj 2007-02-08 */
      if (strstr(test_name,"STREAM") ||
	  strstr(test_name,"SENDFILE")) {
	strncpy(remote_data_port,"discard",sizeof(remote_data_port));
      }
      else if (strstr(test_name,"RR")) {
	strncpy(remote_data_port,"echo",sizeof(remote_data_port));
      }
      else if (strstr(test_name,"MAERTS")) {
	strncpy(remote_data_port,"chargen",sizeof(remote_data_port));
      }
      else {
	printf("No default port known for the %s test, please set one yourself\n",test_name);
	exit(-1);
      }
    }
    remote_data_port[sizeof(remote_data_port) - 1] = '\0';

    /* I go back and forth on whether these should become -1 or if
       they should become 0 for a no_control test. what do you think?
       raj 2006-02-08 */

    rem_rcvavoid = -1;
    rem_sndavoid = -1;
    rss_size_req = -1;
    rsr_size_req = -1;
    rem_nodelay = -1;

    if (strstr(test_name,"STREAM") ||
	strstr(test_name,"SENDFILE")) {
      recv_size = -1;
    }
    else if (strstr(test_name,"RR")) {
      /* I am however _certain_ that for a no control RR test the
	 response size must equal the request size since 99 times out
	 of ten we will be speaking to the echo service somewhere */
      rsp_size = req_size;
    }
    else if (strstr(test_name,"MAERTS")) {
      send_size = -1;
    }
    else {
      printf("No default port known for the %s test, please set one yourself\n",test_name);
      exit(-1);
    }
  }
}
