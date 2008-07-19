/*
        Copyright (C) 1993-2005 Hewlett-Packard Company
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_SYS_SOCKET_H)
# include <sys/socket.h>
#endif
#if defined(HAVE_NETDB_H)
# include <netdb.h>
#endif
#if !defined(HAVE_GETADDRINFO) || !defined(HAVE_GETNAMEINFO)
# include "missing/getaddrinfo.h"
#endif

#define PAD_TIME 4
/* library routine specifc defines                                      */
#define         MAXSPECDATA     62      /* how many ints worth of data  */
                                        /* can tests send...            */
#define         MAXTIMES        4       /* how many times may we loop   */
                                        /* to calibrate                 */
#define         MAXCPUS         256     /* how many CPU's can we track */
#define         MAXMESSAGESIZE  65536
#define         MAXALIGNMENT    16384
#define         MAXOFFSET        4096
#define         DATABUFFERLEN   MAXMESSAGESIZE+MAXALIGNMENT+MAXOFFSET

#define         DEBUG_ON                1
#define         DEBUG_OFF               2
#define         DEBUG_OK                3
#define         NODE_IDENTIFY           4
#define         CPU_CALIBRATE           5

#define         DO_TCP_STREAM           10
#define         TCP_STREAM_RESPONSE     11
#define         TCP_STREAM_RESULTS      12

#define         DO_TCP_RR               13
#define         TCP_RR_RESPONSE         14
#define         TCP_RR_RESULTS          15

#define         DO_UDP_STREAM           16
#define         UDP_STREAM_RESPONSE     17
#define         UDP_STREAM_RESULTS      18

#define         DO_UDP_RR               19
#define         UDP_RR_RESPONSE         20
#define         UDP_RR_RESULTS          21

#define         DO_DLPI_CO_STREAM       22
#define         DLPI_CO_STREAM_RESPONSE 23
#define         DLPI_CO_STREAM_RESULTS  24

#define         DO_DLPI_CO_RR           25
#define         DLPI_CO_RR_RESPONSE     26
#define         DLPI_CO_RR_RESULTS      27

#define         DO_DLPI_CL_STREAM       28
#define         DLPI_CL_STREAM_RESPONSE 29
#define         DLPI_CL_STREAM_RESULTS  30

#define         DO_DLPI_CL_RR           31
#define         DLPI_CL_RR_RESPONSE     32
#define         DLPI_CL_RR_RESULTS      33

#define         DO_TCP_CRR              34
#define         TCP_CRR_RESPONSE        35
#define         TCP_CRR_RESULTS         36

#define         DO_STREAM_STREAM        37
#define         STREAM_STREAM_RESPONSE  38
#define         STREAM_STREAM_RESULTS   39

#define         DO_STREAM_RR            40
#define         STREAM_RR_RESPONSE      41
#define         STREAM_RR_RESULTS       42

#define         DO_DG_STREAM            43
#define         DG_STREAM_RESPONSE      44
#define         DG_STREAM_RESULTS       45

#define         DO_DG_RR                46
#define         DG_RR_RESPONSE          47
#define         DG_RR_RESULTS           48

#define         DO_FORE_STREAM          49
#define         FORE_STREAM_RESPONSE    50
#define         FORE_STREAM_RESULTS     51

#define         DO_FORE_RR              52
#define         FORE_RR_RESPONSE        53
#define         FORE_RR_RESULTS         54

#define         DO_HIPPI_STREAM         55
#define         HIPPI_STREAM_RESPONSE   56
#define         HIPPI_STREAM_RESULTS    57

#define         DO_HIPPI_RR             52
#define         HIPPI_RR_RESPONSE       53
#define         HIPPI_RR_RESULTS        54

#define         DO_XTI_TCP_STREAM       55
#define         XTI_TCP_STREAM_RESPONSE 56
#define         XTI_TCP_STREAM_RESULTS  57

#define         DO_XTI_TCP_RR           58
#define         XTI_TCP_RR_RESPONSE     59
#define         XTI_TCP_RR_RESULTS      60

#define         DO_XTI_UDP_STREAM       61
#define         XTI_UDP_STREAM_RESPONSE 62
#define         XTI_UDP_STREAM_RESULTS  63

#define         DO_XTI_UDP_RR           64
#define         XTI_UDP_RR_RESPONSE     65
#define         XTI_UDP_RR_RESULTS      66

#define         DO_XTI_TCP_CRR          67
#define         XTI_TCP_CRR_RESPONSE    68
#define         XTI_TCP_CRR_RESULTS     69

#define         DO_TCP_TRR              70
#define         TCP_TRR_RESPONSE        71
#define         TCP_TRR_RESULTS         72

#define         DO_TCP_NBRR             73
#define         TCP_NBRR_RESPONSE       74
#define         TCP_NBRR_RESULTS        75

#define         DO_TCPIPV6_STREAM           76
#define         TCPIPV6_STREAM_RESPONSE     77
#define         TCPIPV6_STREAM_RESULTS      78

#define         DO_TCPIPV6_RR               79
#define         TCPIPV6_RR_RESPONSE         80
#define         TCPIPV6_RR_RESULTS          81

#define         DO_UDPIPV6_STREAM           82
#define         UDPIPV6_STREAM_RESPONSE     83
#define         UDPIPV6_STREAM_RESULTS      84

#define         DO_UDPIPV6_RR               85
#define         UDPIPV6_RR_RESPONSE         86
#define         UDPIPV6_RR_RESULTS          87

#define         DO_TCPIPV6_CRR              88
#define         TCPIPV6_CRR_RESPONSE        89
#define         TCPIPV6_CRR_RESULTS         90

#define         DO_TCPIPV6_TRR              91
#define         TCPIPV6_TRR_RESPONSE        92
#define         TCPIPV6_TRR_RESULTS         93

#define         DO_TCP_MAERTS               94
#define         TCP_MAERTS_RESPONSE         95
#define         TCP_MAERTS_RESULTS          96

#define         DO_LWPSTR_STREAM           100
#define         LWPSTR_STREAM_RESPONSE     110
#define         LWPSTR_STREAM_RESULTS      120

#define         DO_LWPSTR_RR               130
#define         LWPSTR_RR_RESPONSE         140
#define         LWPSTR_RR_RESULTS          150

#define         DO_LWPDG_STREAM            160
#define         LWPDG_STREAM_RESPONSE      170
#define         LWPDG_STREAM_RESULTS       180

#define         DO_LWPDG_RR                190
#define         LWPDG_RR_RESPONSE          200
#define         LWPDG_RR_RESULTS           210

#define         DO_TCP_CC                  300
#define         TCP_CC_RESPONSE            301
#define         TCP_CC_RESULTS             302

/* The DNS_RR test has been removed from netperf but we leave these
   here for historical purposes.  Those wanting to do DNS_RR tests
   should use netperf4 instead. */
#define         DO_DNS_RR                  400
#define         DNS_RR_RESPONSE            401
#define         DNS_RR_RESULTS             402

#define         DO_SCTP_STREAM             500
#define         SCTP_STREAM_RESPONSE       501
#define         SCTP_STREAM_RESULT         502

#define         DO_SCTP_STREAM_MANY        510
#define         SCTP_STREAM_MANY_RESPONSE  511
#define         SCTP_STREAM_MANY_RESULT    512

#define         DO_SCTP_RR                 520
#define         SCTP_RR_RESPONSE           521
#define         SCTP_RR_RESULT             502

#define         DO_SCTP_RR_MANY            530
#define         SCTP_RR_MANY_RESPONSE      531
#define         SCTP_RR_MANY_RESULT        532

#define         DO_SDP_STREAM              540
#define         SDP_STREAM_RESPONSE        541
#define         SDP_STREAM_RESULTS         542

#define         DO_SDP_RR                  543
#define         SDP_RR_RESPONSE            544
#define         SDP_RR_RESULTS             545

#define         DO_SDP_MAERTS              546
#define         SDP_MAERTS_RESPONSE        547
#define         SDP_MAERTS_RESULTS         548

#define         DO_SDP_CRR                 549
#define         SDP_CRR_RESPONSE           550
#define         SDP_CRR_RESULTS            551

#define         DO_SDP_CC                  552
#define         SDP_CC_RESPONSE            553
#define         SDP_CC_RESULTS             554

#if HAVE_INTTYPES_H
# include <inttypes.h>
#else
# if HAVE_STDINT_H
#  include <stdint.h>
# endif
#endif

enum sock_buffer{
  SEND_BUFFER,
  RECV_BUFFER
};

 /* some of the fields in these structures are going to be doubles and */
 /* such. so, we probably want to ensure that they will start on */
 /* "double" boundaries. this will break compatability to pre-2.1 */
 /* releases, but then, backwards compatability has never been a */
 /* stated goal of netperf. raj 11/95 */

union netperf_request_struct {
  struct {
    int     request_type;
    int     dummy;
    int     test_specific_data[MAXSPECDATA];
  } content;
  double dummy;
};

union netperf_response_struct {
  struct {
    int response_type;
    int serv_errno;
    int test_specific_data[MAXSPECDATA];
  } content;
  double dummy;
};

struct ring_elt {
  struct ring_elt *next;  /* next element in the ring */
  char *buffer_base;      /* in case we have to free it at somepoint */
  char *buffer_ptr;       /* the aligned and offset pointer */
};

/* +*+ SAF  Sorry about the hacks with errno; NT made me do it :(

 WinNT does define an errno.
 It is mostly a legacy from the XENIX days.

 Depending upon the version of the C run time that is linked in, it is
 either a simple variable (like UNIX code expects), but more likely it
 is the address of a procedure to return the error number.  So any
 code that sets errno is likely to be overwriting the address of this
 procedure.  Worse, only a tiny fraction of NT's errors get set
 through errno.

 So I have changed the netperf code to use a define Set_errno when
 that is it's intent.  On non-windows platforms this is just an
 assignment to errno.  But on NT this calls SetLastError.

 I also define errno (now only used on right side of assignments)
 on NT to be GetLastError.

 Similarly, perror is defined on NT, but it only accesses the same
 XENIX errors that errno covers.  So on NT this is redefined to be
 Perror and it expands all GetLastError texts. */


#ifdef WIN32
/* INVALID_SOCKET == INVALID_HANDLE_VALUE == (unsigned int)(~0) */
/* SOCKET_ERROR == -1 */
#define ENOTSOCK WSAENOTSOCK
#define EINTR    WSAEINTR
#define ENOBUFS  WSAENOBUFS
#define EWOULDBLOCK    WSAEWOULDBLOCK
#define EAFNOSUPPORT  WSAEAFNOSUPPORT
/* I don't use a C++ style of comment because it upsets some C
   compilers, possibly even when it is inside an ifdef WIN32... */
/* from public\sdk\inc\crt\errno.h */
#define ENOSPC          28

#ifdef errno
/* delete the one from stdlib.h  */
/*#define errno       (*_errno()) */
#undef errno
#endif
#define errno GetLastError()
#define Set_errno(num) SetLastError((num))

#define perror(text) PrintWin32Error(stderr, (text))
#define Print_errno(stream, text) PrintWin32Error((stream), (text))

extern void PrintWin32Error(FILE *stream, LPSTR text);

#if !defined(NT_PERF) && !defined(USE_LOOPER)
#define NT_PERF
#endif
#else
/* Really shouldn't use manifest constants! */
/*+*+SAF There are other examples of "== -1" and "<0" that probably */
/*+*+SAF should be cleaned up as well. */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#define SOCKET int
#define Set_errno(num) errno = (num)

#define Print_errno(stream, text) fprintf((stream), "%s  errno %d\n", (text), errno)
#endif

/* Robin & Rick's kludge to try to have a timer signal EINTR by closing  */
/* the socket from another thread can also return several other errors. */
/* Let's define a macro to hide all of this. */

#ifndef WIN32
#define SOCKET_EINTR(return_value) (errno == EINTR)
#define SOCKET_EADDRINUSE(return_value) (errno == EADDRINUSE)
#define SOCKET_EADDRNOTAVAIL(return_value) (errno == EADDRNOTAVAIL)

#else

/* not quite sure I like the extra cases for WIN32 but that is what my
   WIN32 expert sugested.  I'm not sure what WSA's to put for
   EADDRINUSE */

#define SOCKET_EINTR(return_value) \
		(((return_value) == SOCKET_ERROR) && \
	     ((errno == EINTR) || \
	      (errno == WSAECONNABORTED) || \
	      (errno == WSAECONNRESET) ))
#define SOCKET_EADDRINUSE(return_value) \
		(((return_value) == SOCKET_ERROR) && \
	     ((errno == WSAEADDRINUSE) ))
#define SOCKET_EADDRNOTAVAIL(return_value) \
		(((return_value) == SOCKET_ERROR) && \
	     ((errno == WSAEADDRNOTAVAIL) ))
#endif

#ifdef HAVE_SENDFILE

struct sendfile_ring_elt {
  struct sendfile_ring_elt *next; /* next element in the ring */
  int fildes;                     /* the file descriptor of the source
				     file */ 
  off_t offset;                   /* the offset from the beginning of
				     the file for this send */
  size_t length;                  /* the number of bytes to send -
				     this is redundant with the
				     send_size variable but I decided
				     to include it anyway */
  struct iovec *hdtrl;            /* a pointer to a header/trailer
				     that we do not initially use and
				     so should be set to NULL when the 
				     ring is setup. */
  int flags;                      /* the flags to pass to sendfile() - 
				     presently unused and should be
				     set to zero when the ring is
				     setup. */
};

#endif /* HAVE_SENDFILE */

 /* the diferent codes to denote the type of CPU utilization */
 /* methods used */
#define CPU_UNKNOWN     0
#define HP_IDLE_COUNTER 1
#define PSTAT           2
#define TIMES           3
#define LOOPER          4
#define GETRUSAGE       5
#define NT_METHOD       6
#define KSTAT           7
#define PROC_STAT       8
#define SYSCTL          9
#define PERFSTAT       10
#define KSTAT_10       11
#define OSX            12

#define BADCH ('?')

#ifndef NETLIB
#ifdef WIN32
#ifndef _GETOPT_
#define _GETOPT_

int getopt(int argc, char **argv, char *optstring);

extern char *optarg;		/* returned arg to go with this option */
extern int optind;		/* index to next argv element to process */
extern int opterr;		/* should error messages be printed? */
extern int optopt;		/* */

#endif /* _GETOPT_ */

extern  SOCKET     win_kludge_socket, win_kludge_socket2;
#endif /* WIN32 */

extern  int   local_proc_affinity, remote_proc_affinity;

/* these are to allow netperf to be run easily through those evil,
   end-to-end breaking things known as firewalls */
extern char local_data_port[10];
extern char remote_data_port[10];

extern char *local_data_address;
extern char *remote_data_address;

extern int local_data_family;
extern int remote_data_family;

extern  union netperf_request_struct netperf_request;
extern  union netperf_response_struct netperf_response;

extern  float    lib_local_cpu_util;
extern  float    lib_elapsed;
extern  float    lib_local_maxrate;

extern  char    libfmt;

extern  int     cpu_method;
extern  int     lib_num_loc_cpus;
extern  int     lib_num_rem_cpus;
extern  SOCKET  server_sock;
extern  int     times_up;
extern  FILE    *where;
extern  int     loops_per_msec;
extern  float   lib_local_per_cpu_util[];
  
extern  void    netlib_init();
extern  int     netlib_get_page_size();
extern  void    install_signal_catchers();
extern  void    establish_control(char hostname[], 
				  char port[], 
				  int af,
				  char local_hostname[],
				  char local_port[],
				  int local_af);
extern  void    shutdown_control();
extern  void    init_stat();
extern  void    send_request();
extern  void    recv_response();
extern  void    send_response();
extern  void    recv_request();
extern  void    dump_request();
extern  void    dump_addrinfo(FILE *dumploc, struct addrinfo *info,
			      char *host, char *port, int family);
extern  void    start_timer(int time);
extern  void    stop_timer();
extern  void    cpu_start(int measure_cpu);
extern  void    cpu_stop(int measure_cpu, float *elapsed);
extern  void	calculate_confidence(int confidence_iterations,
		     float time,
		     double result,
		     float loc_cpu,
		     float rem_cpu,
		     float loc_sd,
		     float rem_sd);
extern  void	retrieve_confident_values(float *elapsed_time,
			  double *thruput,
			  float *local_cpu_utilization,
			  float *remote_cpu_utilization,
			  float *local_service_demand,
			  float *remote_service_demand);
extern  void    display_confidence();
extern  void    set_sock_buffer(SOCKET sd,
				enum sock_buffer which,
				int requested_size,
				int *effective_sizep);
extern  char   *format_units();

extern  char    *inet_ftos(int family);
extern  char    *inet_ttos(int type);
extern  char    *inet_ptos(int protocol);
extern  double  ntohd(double net_double);
extern  double  htond(double host_double);
extern  int     inet_nton(int af, const void *src, char *dst, int cnt);
extern  void    libmain();
extern  double  calc_thruput(double units_received);
extern  double  calc_thruput_interval(double units_received,double elapsed);
extern  double  calc_thruput_omni(double units_received);
extern  double  calc_thruput_interval_omni(double units_received,double elapsed);
extern  float   calibrate_local_cpu(float local_cpu_rate);
extern  float   calibrate_remote_cpu();
extern  void    bind_to_specific_processor(int processor_affinity,int use_cpu_map);
extern int      set_nonblock (SOCKET sock);

#ifndef WIN32

/* WIN32 requires that at least one of the file sets to select be
 non-null.  Since msec_sleep routine is only called by nettest_dlpi &
 nettest_unix, let's duck this issue. */

extern int msec_sleep( int msecs );
#endif  /* WIN32 */
extern  float   calc_cpu_util(float elapsed_time);
extern  float	calc_service_demand(double units_sent,
				    float elapsed_time,
				    float cpu_utilization,
				    int num_cpus);
extern  float	calc_service_demand_trans(double units_sent,
					  float elapsed_time,
					  float cpu_utilization,
					  int num_cpus);
#if defined(__hpux)
extern  void    catcher(int, siginfo_t *,void *);
#else
extern  void    catcher(int);
#endif /* __hpux */
extern  struct ring_elt *allocate_buffer_ring();
extern void access_buffer(char *buffer_ptr,
			  int length,
			  int dirty_count,
			  int clean_count);

#ifdef HAVE_ICSC_EXS
extern  struct ring_elt *allocate_exs_buffer_ring();
#endif /* HAVE_ICSC_EXS */
#ifdef HAVE_SENDFILE
extern  struct sendfile_ring_elt *alloc_sendfile_buf_ring();
#endif /* HAVE_SENDFILE */
#ifdef WANT_DLPI
/* it seems that AIX in its finite wisdom has some bogus define in an
   include file which defines "rem_addr" which then screws-up this extern
   unless we change the names to protect the guilty. reported by Eric
   Jones */
extern int dl_connect(int fd, unsigned char *remote_addr, int remote_addr_len);
extern int dl_bind(int fd, int sap, int mode, char *dlsap_ptr, int *dlsap_len);
extern  int     dl_open(char devfile[], int ppa);
#endif /* WANT_DLPI */
extern  char    format_cpu_method(int method);
extern unsigned int convert(char *string);
extern unsigned int convert_timespec(char *string);

#ifdef WANT_INTERVALS
extern void start_itimer(unsigned int interval_len_msec);
#endif
 /* these are all for the confidence interval stuff */
extern double confidence;

#endif

#ifdef WIN32
#define close(x)	closesocket(x)
#define strcasecmp(a,b) _stricmp(a,b)
#define getpid() ((int)GetCurrentProcessId())
#endif

#ifdef WIN32
#if 0
/* Should really use safe string functions; but not for now... */
#include <strsafe.h>
/* Microsoft has deprecated _snprintf; it isn't guarenteed to null terminate the result buffer. */
/* They want us to call StringCbPrintf instead; it always null terminates the string. */
#endif

#define snprintf _snprintf
#endif

/* Define a macro to align a buffer with an offset from a power of 2
   boundary. */

#ifndef WIN32
#define ULONG_PTR unsigned long
#endif

#define ALIGN_BUFFER(BufPtr, Align, Offset) \
  (char *)(( (ULONG_PTR)(BufPtr) + \
			(ULONG_PTR) (Align) -1) & \
			~((ULONG_PTR) (Align) - 1)) + (ULONG_PTR)(Offset)

 /* if your system has bcopy and bzero, include it here, otherwise, we */
 /* will try to use memcpy aand memset. fix from Bruce Barnett @ GE. */
#if defined(hpux) || defined (__VMS)
#define HAVE_BCOPY
#define HAVE_BZERO
#endif

#ifdef WIN32
#define HAVE_MIN
#else
#define _stdcall
#define _cdecl
#endif

#ifndef HAVE_BCOPY
#define bcopy(s,d,h) memcpy((d),(s),(h))
#endif /* HAVE_BCOPY */

#ifndef HAVE_BZERO
#define bzero(p,h) memset((p),0,(h))
#endif /* HAVE_BZERO */

#ifndef HAVE_MIN
#define min(a,b) ((a < b) ? a : b)
#endif /* HAVE_MIN */

#ifdef USE_PERFSTAT
# include <libperfstat.h>
#endif
