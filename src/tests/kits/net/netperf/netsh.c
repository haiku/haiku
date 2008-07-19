#include "netperf_version.h"

char	netsh_id[]="\
@(#)netsh.c (c) Copyright 1993-2007 Hewlett-Packard Company. Version 2.4.3pre";


/****************************************************************/
/*								*/
/*	Global include files					*/
/*								*/
/****************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#ifndef __VMS
//#include <sys/ipc.h>
#endif /* __VMS */
#endif /* WIN32 */
#include <fcntl.h>
#ifndef WIN32
#include <errno.h>
#include <signal.h>
#endif  /* !WIN32 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
 /* the following four includes should not be needed ?*/
#ifndef WIN32
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <time.h>
#include <winsock2.h>
#define netperf_socklen_t socklen_t
#endif

#ifndef STRINGS
#include <string.h>
#else /* STRINGS */
#include <strings.h>
#endif /* STRINGS */

#ifdef WIN32
extern	int	getopt(int , char **, char *) ;
#else
double atof(const char *);
#endif /* WIN32 */

/**********************************************************************/
/*                                                                    */
/*          Local Include Files                                       */
/*                                                                    */
/**********************************************************************/

#define  NETSH
#include "netsh.h"
#include "netlib.h"
#include "nettest_bsd.h"

#ifdef WANT_UNIX
#include "nettest_unix.h"
#ifndef WIN32
#include "sys/socket.h"
#endif  /* !WIN32 */
#endif /* WANT_UNIX */

#ifdef WANT_XTI
#include "nettest_xti.h"
#endif /* WANT_XTI */

#ifdef WANT_DLPI
#include "nettest_dlpi.h"
#endif /* WANT_DLPI */

#ifdef WANT_SCTP
#include "nettest_sctp.h"
#endif


/************************************************************************/
/*									*/
/*	Global constants  and macros					*/
/*									*/
/************************************************************************/

 /* Some of the args take optional parameters. Since we are using */
 /* getopt to parse the command line, we will tell getopt that they do */
 /* not take parms, and then look for them ourselves */
#define GLOBAL_CMD_LINE_ARGS "A:a:b:B:CcdDf:F:H:hi:I:k:K:l:L:n:NO:o:P:p:rt:T:v:VW:w:46"

/************************************************************************/
/*									*/
/*	Extern variables 						*/
/*									*/
/************************************************************************/

/*
extern int errno;
extern char *sys_errlist[ ];
extern int sys_nerr;
*/

/************************************************************************/
/*									*/
/*	Global variables 						*/
/*									*/
/************************************************************************/

/* some names and such                                                  */
char	*program;		/* program invocation name		*/
char	username[BUFSIZ];	/* login name of user			*/
char	cmd_file[BUFSIZ];	/* name of the commands file		*/

/* stuff to say where this test is going                                */
char	host_name[HOSTNAMESIZE];	/* remote host name or ip addr  */
char    local_host_name[HOSTNAMESIZE];  /* local hostname or ip */
char    test_name[BUFSIZ];		/* which test to run 		*/
char	test_port[PORTBUFSIZE];		/* where is the test waiting    */
char    local_test_port[PORTBUFSIZE];   /* from whence we should start */
int     address_family;                 /* which address family remote */
int     local_address_family;           /* which address family local */

/* the source of data for filling the buffers */
char    fill_file[BUFSIZ];

/* output controlling variables                                         */
int
  debug,			/* debugging level */
  print_headers,		/* do/don't display headers */
  verbosity;		/* verbosity level */

/* When specified with -B, this will be displayed at the end of the line
   for output that does not include the test header.  mostly this is
   to help identify a specific netperf result when concurrent netperfs
   are run. raj 2006-02-01 */
char *result_brand = NULL;

/* cpu variables */
int
  local_cpu_usage,	/* you guessed it			*/
  remote_cpu_usage;	/* still right !			*/

float			       
  local_cpu_rate,
  remote_cpu_rate;

int
  shell_num_cpus=1;

/* the end-test conditions for the tests - either transactions, bytes,  */
/* or time. different vars used for clarity - space is cheap ;-)        */
int	
  test_time,		/* test ends by time			*/
  test_len_ticks,       /* how many times will the timer go off before */
			/* the test is over? */
  test_bytes,		/* test ends on byte count		*/
  test_trans;		/* test ends on tran count		*/

/* the alignment conditions for the tests				*/
int
  local_recv_align,	/* alignment for local receives		*/
  local_send_align,	/* alignment for local sends		*/
  local_send_offset = 0,
  local_recv_offset = 0,
  remote_recv_align,	/* alignment for remote receives	*/
  remote_send_align,	/* alignment for remote sends		*/
  remote_send_offset = 0,
  remote_recv_offset = 0;

#if defined(WANT_INTERVALS) || defined(WANT_DEMO)
int
  interval_usecs,
  interval_wate,
  interval_burst;

int demo_mode;                    /* are we actually in demo mode? */
double demo_interval = 1000000.0; /* what is the desired interval to
				     display interval results. default
				     is one second in units of
				     microseconds */
double demo_units = 0.0;          /* what is our current best guess as
				     to how many work units must be
				     done to be near the desired
				     reporting interval? */ 

double units_this_tick;
#endif

#ifdef DIRTY
int	loc_dirty_count;
int	loc_clean_count;
int	rem_dirty_count;
int	rem_clean_count;
#endif /* DIRTY */

 /* some of the vairables for confidence intervals... */

int  confidence_level;
int  iteration_min;
int  iteration_max;
int  result_confidence_only = 0;

double interval;

 /* stuff to control the "width" of the buffer rings for sending and */
 /* receiving data */
int	send_width;
int     recv_width;

/* address family */
int	af = AF_INET;

/* did someone request processor affinity? */
int cpu_binding_requested = 0;

/* are we not establishing a control connection? */
int no_control = 0;

char netserver_usage[] = "\n\
Usage: netserver [options] \n\
\n\
Options:\n\
    -h                Display this text\n\
    -d                Increase debugging output\n\
    -L name,family    Use name to pick listen address and family for family\n\
    -p portnum        Listen for connect requests on portnum.\n\
    -4                Do IPv4\n\
    -6                Do IPv6\n\
    -v verbosity      Specify the verbosity level\n\
    -V                Display version information and exit\n\
\n";

/* netperf_usage done as two concatenated strings to make the MS
   compiler happy when compiling for x86_32.  fix from Spencer
   Frink.  */

char netperf_usage1[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
Global options:\n\
    -a send,recv      Set the local send,recv buffer alignment\n\
    -A send,recv      Set the remote send,recv buffer alignment\n\
    -B brandstr       Specify a string to be emitted with brief output\n\
    -c [cpu_rate]     Report local CPU usage\n\
    -C [cpu_rate]     Report remote CPU usage\n\
    -d                Increase debugging output\n\
    -D [secs,units] * Display interim results at least every secs seconds\n\
                      using units as the initial guess for units per second\n\
    -f G|M|K|g|m|k    Set the output units\n\
    -F fill_file      Pre-fill buffers with data from fill_file\n\
    -h                Display this text\n\
    -H name|ip,fam *  Specify the target machine and/or local ip and family\n\
    -i max,min        Specify the max and min number of iterations (15,1)\n\
    -I lvl[,intvl]    Specify confidence level (95 or 99) (99) \n\
                      and confidence interval in percentage (10)\n\
    -l testlen        Specify test duration (>0 secs) (<0 bytes|trans)\n\
    -L name|ip,fam *  Specify the local ip|name and address family\n\
    -o send,recv      Set the local send,recv buffer offsets\n\
    -O send,recv      Set the remote send,recv buffer offset\n\
    -n numcpu         Set the number of processors for CPU util\n\
    -N                Establish no control connection, do 'send' side only\n\
    -p port,lport*    Specify netserver port number and/or local port\n\
    -P 0|1            Don't/Do display test headers\n\
    -r                Allow confidence to be hit on result only\n\
    -t testname       Specify test to perform\n\
    -T lcpu,rcpu      Request netperf/netserver be bound to local/remote cpu\n\
    -v verbosity      Specify the verbosity level\n\
    -W send,recv      Set the number of send,recv buffers\n\
    -v level          Set the verbosity level (default 1, min 0)\n\
    -V                Display the netperf version and exit\n";

char netperf_usage2[] = "\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n\
\n"
"* For these options taking two parms, specifying one value with no comma\n\
will only set the first parms and will leave the second at the default\n\
value. To set the second value it must be preceded with a comma or be a\n\
comma-separated pair. This is to retain previous netperf behaviour.\n"; 


/* This routine will return the two arguments to the calling routine. */
/* If the second argument is not specified, and there is no comma, */
/* then the value of the second argument will be the same as the */
/* value of the first. If there is a comma, then the value of the */
/* second argument will be the value of the second argument ;-) */
void
break_args(char *s, char *arg1, char *arg2)

{
  char *ns;
  ns = strchr(s,',');
  if (ns) {
    /* there was a comma arg2 should be the second arg*/
    *ns++ = '\0';
    while ((*arg2++ = *ns++) != '\0');
  }
  else {
    /* there was not a comma, we can use ns as a temp s */
    /* and arg2 should be the same value as arg1 */
    ns = s;
    while ((*arg2++ = *ns++) != '\0');
  };
  while ((*arg1++ = *s++) != '\0');
}

/* break_args_explicit

   this routine is somewhat like break_args in that it will separate a
   pair of comma-separated values.  however, if there is no comma,
   this version will not ass-u-me that arg2 should be the same as
   arg1. raj 2005-02-04 */
void
break_args_explicit(char *s, char *arg1, char *arg2)

{
  char *ns;
  ns = strchr(s,',');
  if (ns) {
    /* there was a comma arg2 should be the second arg*/
    *ns++ = '\0';
    while ((*arg2++ = *ns++) != '\0');
  }
  else {
    /* there was not a comma, so we should make sure that arg2 is \0
       lest something become confused. raj 2005-02-04 */
    *arg2 = '\0';
  };
  while ((*arg1++ = *s++) != '\0');

}

/* given a string with possible values for setting an address family,
   convert that into one of the AF_mumble values - AF_INET, AF_INET6,
   AF_UNSPEC as apropriate. the family_string is compared in a
   case-insensitive manner */

int
parse_address_family(char family_string[])
{

  char temp[10];  /* gotta love magic constants :) */

  strncpy(temp,family_string,10);

  if (debug) {
    fprintf(where,
	    "Attempting to parse address family from %s derived from %s\n",
	    temp,
	    family_string);
  }
#if defined(AF_INET6)
  if (strstr(temp,"6")) {
    return(AF_INET6);
  }
#endif
  if (strstr(temp,"inet") ||
      strstr(temp,"4")) {
    return(AF_INET);
  }
  if (strstr(temp,"unspec") ||
      strstr(temp,"0")) {
    return(AF_UNSPEC);
  }
  fprintf(where,
	  "WARNING! %s not recognized as an address family, using AF_UNPSEC\n",
	  family_string);
  fprintf(where,
	  "Are you sure netperf was configured for that address family?\n");
  fflush(where);
  return(AF_UNSPEC);
}


void
set_defaults()
{
  
  /* stuff to say where this test is going                              */
  strcpy(host_name,"");	      /* remote host name or ip addr  */
  strcpy(local_host_name,""); /* we want it to be INADDR_ANY */
  strcpy(test_name,"TCP_STREAM");	/* which test to run 		*/
  strncpy(test_port,"12865",PORTBUFSIZE); /* where is the test waiting    */
  strncpy(local_test_port,"0",PORTBUFSIZE);/* INPORT_ANY as it were */
  address_family = AF_UNSPEC;
  local_address_family = AF_UNSPEC;

  /* output controlling variables                               */
  debug			= 0;/* debugging level			*/
  print_headers		= 1;/* do print test headers		*/
  verbosity		= 1;/* verbosity level			*/
  /* cpu variables */
  local_cpu_usage	= 0;/* measure local cpu		*/
  remote_cpu_usage	= 0;/* what do you think ;-)		*/
  
  local_cpu_rate	= (float)0.0;
  remote_cpu_rate	= (float)0.0;
  
  /* the end-test conditions for the tests - either transactions, bytes,  */
  /* or time. different vars used for clarity - space is cheap ;-)        */
  test_time	= 10;	/* test ends by time			*/
  test_bytes	= 0;	/* test ends on byte count		*/
  test_trans	= 0;	/* test ends on tran count		*/
  
  /* the alignment conditions for the tests				*/
  local_recv_align	= 8;	/* alignment for local receives	*/
  local_send_align	= 8;	/* alignment for local sends	*/
  remote_recv_align	= 8;	/* alignment for remote receives*/
  remote_send_align	= 8;	/* alignment for remote sends	*/
  
#ifdef WANT_INTERVALS
  /* rate controlling stuff */
  interval_usecs  = 0;
  interval_wate   = 1;
  interval_burst  = 0;
#endif /* WANT_INTERVALS */
  
#ifdef DIRTY
  /* dirty and clean cache stuff */
  loc_dirty_count = 0;
  loc_clean_count = 0;
  rem_dirty_count = 0;
  rem_clean_count = 0;
#endif /* DIRTY */

 /* some of the vairables for confidence intervals... */

  confidence_level = 99;
  iteration_min = 1;
  iteration_max = 1;
  interval = 0.05; /* five percent? */

  no_control = 0;
  strcpy(fill_file,"");
}
     

void
print_netserver_usage()
{
  fwrite(netserver_usage, sizeof(char), strlen(netserver_usage), stderr);
}


void
print_netperf_usage()
{
  fwrite(netperf_usage1, sizeof(char), strlen(netperf_usage1),  stderr);
  fwrite(netperf_usage2, sizeof(char), strlen(netperf_usage2),  stderr);
}

void
scan_cmd_line(int argc, char *argv[])
{
  extern int	optind;           /* index of first unused arg 	*/
  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	arg1[BUFSIZ],  /* argument holders		*/
    arg2[BUFSIZ];
  
  program = (char *)malloc(strlen(argv[0]) + 1);
  if (program == NULL) {
    printf("malloc(%d) failed!\n", strlen(argv[0]) + 1);
    exit(1);
  }
  strcpy(program, argv[0]);
  
  /* Go through all the command line arguments and break them */
  /* out. For those options that take two parms, specifying only */
  /* the first will set both to that value. Specifying only the */
  /* second will leave the first untouched. To change only the */
  /* first, use the form first, (see the routine break_args.. */
  
  while ((c= getopt(argc, argv, GLOBAL_CMD_LINE_ARGS)) != EOF) {
    switch (c) {
    case '?':	
    case 'h':
      print_netperf_usage();
      exit(1);
    case 'a':
      /* set local alignments */
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	local_send_align = convert(arg1);
      }
      if (arg2[0])
	local_recv_align = convert(arg2);
      break;
    case 'A':
      /* set remote alignments */
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	remote_send_align = convert(arg1);
      }
      if (arg2[0])
	remote_recv_align = convert(arg2);
      break;
    case 'c':
      /* measure local cpu usage please. the user */
      /* may have specified the cpu rate as an */
      /* optional parm */
      if (argv[optind] && isdigit((unsigned char)argv[optind][0])){
	/* there was an optional parm */
	local_cpu_rate = (float)atof(argv[optind]);
	optind++;
      }
      local_cpu_usage++;
      break;
    case 'C':
      /* measure remote cpu usage please */
      if (argv[optind] && isdigit((unsigned char)argv[optind][0])){
	/* there was an optional parm */
	remote_cpu_rate = (float)atof(argv[optind]);
	optind++;
      }
      remote_cpu_usage++;
      break;
    case 'd':
      debug++;
      break;
    case 'D':
#if (defined WANT_DEMO)
      demo_mode++;
      if (argv[optind] && isdigit((unsigned char)argv[optind][0])){
	/* there was an optional parm */
	break_args_explicit(argv[optind],arg1,arg2);
	optind++;
	if (arg1[0]) {
	  demo_interval = atof(arg1) * 1000000.0;
	}
	if (arg2[0]) {
	  demo_units = convert(arg2);
	}
      }
#else 
      printf("Sorry, Demo Mode not configured into this netperf.\n");
      printf("please consider reconfiguring netperf with\n");
      printf("--enable-demo=yes and recompiling\n");
#endif 
      break;
    case 'f':
      /* set the thruput formatting */
      libfmt = *optarg;
      break;
    case 'F':
      /* set the fill_file variable for pre-filling buffers */
      strcpy(fill_file,optarg);
      break;
    case 'i':
      /* set the iterations min and max for confidence intervals */
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	iteration_max = convert(arg1);
      }
      if (arg2[0] ) {
	iteration_min = convert(arg2);
      }
      /* if the iteration_max is < iteration_min make iteration_max
	 equal iteration_min */
      if (iteration_max < iteration_min) iteration_max = iteration_min;
      /* limit minimum to 3 iterations */
      if (iteration_max < 3) iteration_max = 3;
      if (iteration_min < 3) iteration_min = 3;
      /* limit maximum to 30 iterations */
      if (iteration_max > 30) iteration_max = 30;
      if (iteration_min > 30) iteration_min = 30;
      break;
    case 'I':
      /* set the confidence level (95 or 99) and width */
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	confidence_level = convert(arg1);
      }
      if((confidence_level != 95) && (confidence_level != 99)){
	printf("Only 95%% and 99%% confidence level is supported\n");
	exit(1);
      }
      if (arg2[0] ) {
	interval = (double) convert(arg2)/100;
      }
      /* make sure that iteration_min and iteration_max are at least
	 at a reasonable default value.  if a -i option has previously
	 been parsed, these will no longer be 1, so we can check
	 against 1 */ 
      if (iteration_min == 1) iteration_min = 3;
      if (iteration_max == 1) iteration_max = 10;

      break;
    case 'k':
      /* local dirty and clean counts */
#ifdef DIRTY
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	loc_dirty_count = convert(arg1);
      }
      if (arg2[0] ) {
	loc_clean_count = convert(arg2);
      }
#else
      printf("I don't know how to get dirty.\n");
#endif /* DIRTY */
      break;
    case 'K':
      /* remote dirty and clean counts */
#ifdef DIRTY
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	rem_dirty_count = convert(arg1);
      }
      if (arg2[0] ) {
	rem_clean_count = convert(arg2);
      }
#else
      printf("I don't know how to get dirty.\n");
#endif /* DIRTY */
      break;
    case 'n':
      shell_num_cpus = atoi(optarg);
      break;
    case 'N':
      no_control = 1;
      break;
    case 'o':
      /* set the local offsets */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	local_send_offset = convert(arg1);
      if (arg2[0])
	local_recv_offset = convert(arg2);
      break;
    case 'O':
      /* set the remote offsets */
      break_args(optarg,arg1,arg2);
      if (arg1[0]) 
	remote_send_offset = convert(arg1);
      if (arg2[0])
	remote_recv_offset = convert(arg2);
      break;
    case 'P':
      /* to print or not to print, that is */
      /* the header question */
      print_headers = convert(optarg);
      break;
    case 'r':
      /* the user wishes that we declare confidence when hit on the
	 result even if not yet reached on CPU utilization.  only
	 meaningful if cpu util is enabled */
      result_confidence_only = 1;
      break;
    case 't':
      /* set the test name */
      strcpy(test_name,optarg);
      break;
    case 'T':
      /* We want to set the processor on which netserver or netperf */
      /* will run */
      break_args(optarg,arg1,arg2);
      if (arg1[0]) {
	local_proc_affinity = convert(arg1);
	bind_to_specific_processor(local_proc_affinity,0);
      }
      if (arg2[0]) {
	remote_proc_affinity = convert(arg2);
      }
      cpu_binding_requested = 1;
      break;
    case 'W':
      /* set the "width" of the user space data buffer ring. This will */
      /* be the number of send_size buffers malloc'd in the tests */  
      break_args(optarg,arg1,arg2);
      if (arg1[0]) 
	send_width = convert(arg1);
      if (arg2[0])
	recv_width = convert(arg2);
      break;
    case 'l':
      /* determine test end conditions */
      /* assume a timed test */
      test_time = convert(optarg);
      test_bytes = test_trans = 0;
      if (test_time < 0) {
	test_bytes = -1 * test_time;
	test_trans = test_bytes;
	test_time = 0;
      }
      break;
    case 'v':
      /* say how much to say */
      verbosity = convert(optarg);
      break;
    case 'p':
      /* specify an alternate port number we use break_args_explicit
	 here to maintain backwards compatibility with previous
	 generations of netperf where having a single value did not
	 set both remote _and_ local port number. raj 2005-02-04 */
      break_args_explicit(optarg,arg1,arg2);
      if (arg1[0])
	strncpy(test_port,arg1,PORTBUFSIZE);
      if (arg2[0])
	strncpy(local_test_port,arg2,PORTBUFSIZE);
      break;
    case 'H':
      /* save-off the host identifying information, use
	 break_args_explicit since passing just one value should not
	 set both */ 
      break_args_explicit(optarg,arg1,arg2);
      if (arg1[0])
	strncpy(host_name,arg1,sizeof(host_name));
      if (arg2[0])
	address_family = parse_address_family(arg2);
      break;
    case 'L':
      /* save-off the local control socket addressing information. use
	 break_args_explicit since passing just one value should not
	 set both */
      break_args_explicit(optarg,arg1,arg2);
      if (arg1[0])
	strncpy(local_host_name,arg1,sizeof(local_host_name));
      if (arg2[0])
	local_address_family = parse_address_family(arg2);
      break;
    case 'w':
      /* We want to send requests at a certain wate. */
      /* Remember that there are 1000000 usecs in a */
      /* second, and that the packet rate is */
      /* expressed in packets per millisecond. */
#ifdef WANT_INTERVALS
      interval_usecs = convert_timespec(optarg);
      interval_wate  = interval_usecs / 1000;
#else
      fprintf(where,
	      "Packet rate control is not compiled in.\n");
#endif
      break;
    case 'b':
      /* we want to have a burst so many packets per */
      /* interval. */
#ifdef WANT_INTERVALS
      interval_burst = convert(optarg);
#else
      fprintf(where,
	      "Packet burst size is not compiled in. \n");
#endif /* WANT_INTERVALS */
      break;
    case 'B':
      result_brand = malloc(strlen(optarg)+1);
      if (NULL != result_brand) {
	strcpy(result_brand,optarg);
      }
      else {
	fprintf(where,
		"Unable to malloc space for result brand\n");
      }
      break;
    case '4':
      address_family = AF_INET;
      local_address_family = AF_INET;
      break;
    case '6':
#if defined(AF_INET6)
      address_family = AF_INET6;
      local_address_family = AF_INET6;
#else
      printf("This netperf was not compiled on an IPv6 capable system!\n");
      exit(-1);
#endif
      break;
    case 'V':
      printf("Netperf version %s\n",NETPERF_VERSION);
      exit(0);
      break;
    };
  }
  /* ok, what should our default hostname and local binding info be?
   */
  if ('\0' == host_name[0]) {
    /* host_name was not set */
    switch (address_family) {
    case AF_INET:
      strcpy(host_name,"localhost");
      break;
    case AF_UNSPEC:
      /* what to do here? case it off the local_address_family I
	 suppose */
      switch (local_address_family) {
      case AF_INET:
      case AF_UNSPEC:
	strcpy(host_name,"localhost");
	break;
#if defined(AF_INET6)
      case AF_INET6:
	strcpy(host_name,"::1");
	break;
#endif
      default:
	printf("Netperf does not understand %d as an address family\n",
	       address_family);
	exit(-1);
      }
      break; 
#if defined(AF_INET6)
    case AF_INET6:
      strcpy(host_name,"::1");
      break;
#endif
    default:
      printf("Netperf does not understand %d as an address family\n",
	     address_family);
      exit(-1);
    }
  }
  
  /* now, having established the name to which the control will
     connect, from what should it come? */
  if ('\0' == local_host_name[0]) {
    switch (local_address_family) {
    case AF_INET:
      strcpy(local_host_name,"0.0.0.0");
      break;
    case AF_UNSPEC:
      switch (address_family) {
      case AF_INET:
      case AF_UNSPEC:
	strcpy(local_host_name,"0.0.0.0");
	break;
#if defined(AF_INET6)
      case AF_INET6:
	strcpy(local_host_name,"::0");
	break;
#endif
      default:
	printf("Netperf does not understand %d as an address family\n",
	       address_family);
	exit(-1);
      }
      break;
#if defined(AF_INET6)
    case AF_INET6:
      strcpy(local_host_name,"::0");
      break;
#endif
    default:
      printf("Netperf does not understand %d as an address family\n",
	     address_family);
      exit(-1);
    }
  }

  /* so, if we aren't even going to establish a control connection we
     should set certain "remote" settings to reflect this, regardless
     of what else may have been set on the command line */
  if (no_control) {
    remote_recv_align = -1;
    remote_send_align = -1;
    remote_send_offset = -1;
    remote_recv_offset = -1;
    remote_cpu_rate = (float)-1.0;
    remote_cpu_usage = 0;
  }

  /* parsing test-specific options used to be conditional on there
    being a "--" in the option stream.  however, some of the tests
    have other initialization happening in their "scan" routines so we
    want to call them regardless. raj 2005-02-08 */
    if ((strcasecmp(test_name,"TCP_STREAM") == 0) ||
#ifdef HAVE_ICSC_EXS
    (strcasecmp(test_name,"EXS_TCP_STREAM") == 0) ||
#endif /* HAVE_ICSC_EXS */ 
#ifdef HAVE_SENDFILE
	(strcasecmp(test_name,"TCP_SENDFILE") == 0) ||
#endif /* HAVE_SENDFILE */
	(strcasecmp(test_name,"TCP_MAERTS") == 0) ||
	(strcasecmp(test_name,"TCP_RR") == 0) ||
	(strcasecmp(test_name,"TCP_CRR") == 0) ||
	(strcasecmp(test_name,"TCP_CC") == 0) ||
#ifdef DO_1644
	(strcasecmp(test_name,"TCP_TRR") == 0) ||
#endif /* DO_1644 */
#ifdef DO_NBRR
	(strcasecmp(test_name,"TCP_TRR") == 0) ||
#endif /* DO_NBRR */
	(strcasecmp(test_name,"UDP_STREAM") == 0) ||
	(strcasecmp(test_name,"UDP_RR") == 0))
      {
	scan_sockets_args(argc, argv);
      }

#ifdef WANT_DLPI
    else if ((strcasecmp(test_name,"DLCO_RR") == 0) ||
	     (strcasecmp(test_name,"DLCL_RR") == 0) ||
	     (strcasecmp(test_name,"DLCO_STREAM") == 0) ||
	     (strcasecmp(test_name,"DLCL_STREAM") == 0))
      {
	scan_dlpi_args(argc, argv);
      }
#endif /* WANT_DLPI */

#ifdef WANT_UNIX
    else if ((strcasecmp(test_name,"STREAM_RR") == 0) ||
	     (strcasecmp(test_name,"DG_RR") == 0) ||
	     (strcasecmp(test_name,"STREAM_STREAM") == 0) ||
	     (strcasecmp(test_name,"DG_STREAM") == 0))
      {
	scan_unix_args(argc, argv);
      }
#endif /* WANT_UNIX */

#ifdef WANT_XTI
    else if ((strcasecmp(test_name,"XTI_TCP_RR") == 0) ||
	     (strcasecmp(test_name,"XTI_TCP_STREAM") == 0) ||
	     (strcasecmp(test_name,"XTI_UDP_RR") == 0) ||
	     (strcasecmp(test_name,"XTI_UDP_STREAM") == 0))
      {
	scan_xti_args(argc, argv);
      }
#endif /* WANT_XTI */

#ifdef WANT_SCTP
    else if ((strcasecmp(test_name,"SCTP_STREAM") == 0) ||
	     (strcasecmp(test_name,"SCTP_RR") == 0) ||
	     (strcasecmp(test_name,"SCTP_STREAM_MANY") == 0) ||
	     (strcasecmp(test_name,"SCTP_RR_MANY") == 0))
    {
      scan_sctp_args(argc, argv);
    }
#endif

#ifdef WANT_SDP
    else if((strcasecmp(test_name,"SDP_STREAM") == 0) ||
	    (strcasecmp(test_name,"SDP_MAERTS") == 0) ||
	    (strcasecmp(test_name,"SDP_RR") == 0))
      {
	scan_sdp_args(argc, argv);
      }
#endif
    
    /* what is our default value for the output units?  if the test
       name contains "RR" or "rr" or "Rr" or "rR" then the default is
       'x' for transactions. otherwise it is 'm' for megabits
       (10^6) */

    if ('?' == libfmt) {
      /* we use a series of strstr's here because not everyone has
	 strcasestr and I don't feel like up or downshifting text */
      if ((strstr(test_name,"RR")) ||
	  (strstr(test_name,"rr")) ||
	  (strstr(test_name,"Rr")) ||
	  (strstr(test_name,"rR"))) {
	libfmt = 'x';
      }
      else {
	libfmt = 'm';
      }
    }
    else if ('x' == libfmt) {
      /* now, a format of 'x' makes no sense for anything other than
	 an RR test. if someone has been silly enough to try to set
	 that, we will reset it silently to default - namely 'm' */
      if ((strstr(test_name,"RR") == NULL) &&
	  (strstr(test_name,"rr") == NULL) &&
	  (strstr(test_name,"Rr") == NULL) &&
	  (strstr(test_name,"rR") == NULL)) {
	libfmt = 'm';
      }
    }
}


void
dump_globals()
{
  printf("Program name: %s\n", program);
  printf("Local send alignment: %d\n",local_send_align);
  printf("Local recv alignment: %d\n",local_recv_align);
  printf("Remote send alignment: %d\n",remote_send_align);
  printf("Remote recv alignment: %d\n",remote_recv_align);
  printf("Report local CPU %d\n",local_cpu_usage);
  printf("Report remote CPU %d\n",remote_cpu_usage);
  printf("Verbosity: %d\n",verbosity);
  printf("Debug: %d\n",debug);
  printf("Port: %s\n",test_port);
  printf("Test name: %s\n",test_name);
  printf("Test bytes: %d Test time: %d Test trans: %d\n",
	 test_bytes,
	 test_time,
	 test_trans);
  printf("Host name: %s\n",host_name);
  printf("\n");
}
