#ifdef NEED_MAKEFILE_EDIT
#error you must first edit and customize the makefile to your platform
#endif /* NEED_MAKEFILE_EDIT */
char	netsh_id[]="\
@(#)netsh.c (c) Copyright 1993-2003 Hewlett-Packard Company. Version 2.2pl3";


/****************************************************************/
/*								*/
/*	Global include files					*/
/*								*/
/****************************************************************/

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#ifndef __VMS
#include <sys/ipc.h>
#endif /* __VMS */
#endif /* WIN32 */
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
 /* the following four includes should not be needed ?*/
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef STRINGS
#include <string.h>
#else /* STRINGS */
#include <strings.h>
#endif /* STRINGS */

#ifdef WIN32
extern	int	getopt(int , char **, char *) ;
#else
double atof();
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
#ifdef DO_UNIX
#include "nettest_unix.h"
#include "sys/socket.h"
#endif /* DO_UNIX */
#ifdef DO_IPV6
#include "nettest_ipv6.h"
#endif /* DO_IPV6 */
/************************************************************************/
/*									*/
/*	Global constants  and macros					*/
/*									*/
/************************************************************************/

 /* Some of the args take optional parameters. Since we are using */
 /* getopt to parse the command line, we will tell getopt that they do */
 /* not take parms, and then look for them ourselves */
#define GLOBAL_CMD_LINE_ARGS "A:a:b:CcdDf:F:H:hi:I:l:n:O:o:P:p:t:v:W:w:46"

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
char    test_name[BUFSIZ];			/* which test to run 		*/
short	test_port;			/* where is the test waiting    */

/* the source of data for filling the buffers */
char    fill_file[BUFSIZ];

/* output controlling variables                                         */
int
  debug,			/* debugging level */
  print_headers,		/* do/don't display headers */
  verbosity;		/* verbosity level */

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

#ifdef INTERVALS
int
  interval_usecs,
  interval_wate,
  interval_burst;

int demo_mode;
double units_this_tick;
#endif /* INTERVALS */

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

double interval;

 /* stuff to control the "width" of the buffer rings for sending and */
 /* receiving data */
int	send_width;
int     recv_width;

/* address family */
int	af = AF_INET;

char netserver_usage[] = "\n\
Usage: netserver [options] \n\
\n\
Options:\n\
    -h                Display this text\n\
    -p portnum        Listen for connect requests on portnum.\n\
\n";

char netperf_usage[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
Global options:\n\
    -a send,recv      Set the local send,recv buffer alignment\n\
    -A send,recv      Set the remote send,recv buffer alignment\n\
    -c [cpu_rate]     Report local CPU usage\n\
    -C [cpu_rate]     Report remote CPU usage\n\
    -d                Increase debugging output\n\
    -f G|M|K|g|m|k    Set the output units\n\
    -F fill_file      Pre-fill buffers with data from fill_file\n\
    -h                Display this text\n\
    -H name|ip        Specify the target machine\n\
    -i max,min        Specify the max and min number of iterations (15,1)\n\
    -I lvl[,intvl]    Specify confidence level (95 or 99) (99) \n\
                      and confidence interval in percentage (10)\n\
    -l testlen        Specify test duration (>0 secs) (<0 bytes|trans)\n\
    -o send,recv      Set the local send,recv buffer offsets\n\
    -O send,recv      Set the remote send,recv buffer offset\n\
    -n numcpu         Set the number of processors for CPU util\n\
    -p port           Specify netserver port number\n\
    -P 0|1            Don't/Do display test headers\n\
    -t testname       Specify test to perform\n\
    -v verbosity      Specify the verbosity level\n\
    -W send,recv      Set the number of send,recv buffers\n\
\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n"; 


/* This routine will return the two arguments to the calling routine. */
/* If the second argument is not specified, and there is no comma, */
/* then the value of the second argument will be the same as the */
/* value of the first. If there is a comma, then the value of the */
/* second argument will be the value of the second argument ;-) */
void
break_args(s, arg1, arg2)
char	*s, *arg1, *arg2;

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

void
set_defaults()
{
  
  /* stuff to say where this test is going                              */
  strcpy(host_name,"localhost");	/* remote host name or ip addr  */
  strcpy(test_name,"TCP_STREAM");	/* which test to run 		*/
  test_port	= 12865;	        /* where is the test waiting    */
  
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
  
#ifdef INTERVALS
  /* rate controlling stuff */
  interval_usecs  = 0;
  interval_wate   = 1;
  interval_burst  = 0;
#endif /* INTERVALS */
  
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

  strcpy(fill_file,"");
}
     

void
print_netserver_usage()
{
  fprintf(stderr,netserver_usage);
}


void
print_netperf_usage()
{
  fprintf(stderr,netperf_usage);
}

void
scan_cmd_line(argc, argv)
     int	argc;
     char	*argv[];

{
  extern int	optind;           /* index of first unused arg 	*/
  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	arg1[BUFSIZ],  /* argument holders		*/
  arg2[BUFSIZ];
  
  program = (char *)malloc(strlen(argv[0]) + 1);
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
    case 'd':
      debug++;
      break;
    case 'D':
#if (defined INTERVALS) && (defined __hpux)
      demo_mode++;
#else /* INTERVALS __hpux */
      printf("Sorry, Demo Mode requires -DINTERVALS compilation \n");
      printf("as well as a mechanism to dynamically select syscall \n");
      printf("restart or interruption. I only know how to do this \n");
      printf("for HP-UX. Please examine the code in netlib.c:catcher() \n");
      printf("and let me know of more standard alternatives. \n");
      printf("                             Rick Jones <raj@cup.hp.com>\n");
      exit(1);
#endif /* INTERVALS __hpux */
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
      /* limit maximum to 30 iterations */
      if(iteration_max>30) iteration_max=30;
      if(iteration_min>30) iteration_min=30;
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
    case 't':
      /* set the test name */
      strcpy(test_name,optarg);
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
    case 'c':
      /* measure local cpu usage please. the user */
      /* may have specified the cpu rate as an */
      /* optional parm */
      if (argv[optind] && isdigit(argv[optind][0])){
	/* there was an optional parm */
	local_cpu_rate = (float)atof(argv[optind]);
	optind++;
      }
      local_cpu_usage++;
      break;
    case 'C':
      /* measure remote cpu usage please */
      if (argv[optind] && isdigit(argv[optind][0])){
	/* there was an optional parm */
	remote_cpu_rate = (float)atof(argv[optind]);
	optind++;
      }
      remote_cpu_usage++;
      break;
    case 'p':
      /* specify an alternate port number */
      test_port = (short) convert(optarg);
      break;
    case 'H':
      /* save-off the host identifying information */
      strcpy(host_name,optarg);
      break;
    case 'w':
      /* We want to send requests at a certain wate. */
      /* Remember that there are 1000000 usecs in a */
      /* second, and that the packet rate is */
      /* expressed in packets per millisecond. */
#ifdef INTERVALS
      interval_wate  = convert(optarg);
      interval_usecs = interval_wate * 1000;
#else
      fprintf(where,
	      "Packet rate control is not compiled in.\n");
#endif
      break;
    case 'b':
      /* we want to have a burst so many packets per */
      /* interval. */
#ifdef INTERVALS
      interval_burst = convert(optarg);
#else
      fprintf(where,
	      "Packet burst size is not compiled in. \n");
#endif /* INTERVALS */
      break;
    };
  }
  /* we have encountered a -- in global command-line */
  /* processing and assume that this means we have gotten to the */
  /* test specific options. this is a bit kludgy and if anyone has */
  /* a better solution, i would love to see it */
  if (optind != argc) {
    if ((strcasecmp(test_name,"TCP_STREAM") == 0) || 
#ifdef HAVE_SENDFILE
	(strcasecmp(test_name,"TCP_SENDFILE") == 0) ||
#endif /* HAVE_SENDFILE */
	(strcasecmp(test_name,"TCP_MAERTS") == 0) ||
	(strcasecmp(test_name,"TCP_RR") == 0) ||
	(strcasecmp(test_name,"TCP_CRR") == 0) ||
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
#ifdef DO_DLPI
    else if ((strcasecmp(test_name,"DLCO_RR") == 0) ||
	     (strcasecmp(test_name,"DLCL_RR") == 0) ||
	     (strcasecmp(test_name,"DLCO_STREAM") == 0) ||
	     (strcasecmp(test_name,"DLCL_STREAM") == 0))
      {
	scan_dlpi_args(argc, argv);
      }
#endif /* DO_DLPI */
#ifdef DO_UNIX
    else if ((strcasecmp(test_name,"STREAM_RR") == 0) ||
	     (strcasecmp(test_name,"DG_RR") == 0) ||
	     (strcasecmp(test_name,"STREAM_STREAM") == 0) ||
	     (strcasecmp(test_name,"DG_STREAM") == 0))
      {
	scan_unix_args(argc, argv);
      }
#endif /* DO_UNIX */
#ifdef DO_FORE
    else if ((strcasecmp(test_name,"FORE_RR") == 0) ||
	     (strcasecmp(test_name,"FORE_STREAM") == 0))
      {
	scan_fore_args(argc, argv);
      }
#endif /* DO_FORE */
#ifdef DO_HIPPI
    else if ((strcasecmp(test_name,"HIPPI_RR") == 0) ||
	     (strcasecmp(test_name,"HIPPI_STREAM") == 0))
      {
	scan_hippi_args(argc, argv);
      }
#endif /* DO_HIPPI */
#ifdef DO_XTI
    else if ((strcasecmp(test_name,"XTI_TCP_RR") == 0) ||
	     (strcasecmp(test_name,"XTI_TCP_STREAM") == 0) ||
	     (strcasecmp(test_name,"XTI_UDP_RR") == 0) ||
	     (strcasecmp(test_name,"XTI_UDP_STREAM") == 0))
      {
	scan_xti_args(argc, argv);
      }
#endif /* DO_XTI */
#ifdef DO_LWP
    else if ((strcasecmp(test_name,"LWPSTR_RR") == 0) ||
	     (strcasecmp(test_name,"LWPSTR_STREAM") == 0) ||
	     (strcasecmp(test_name,"LWPDG_RR") == 0) ||
	     (strcasecmp(test_name,"LWPDG_STREAM") == 0))
      {
	scan_lwp_args(argc, argv);
      }
#endif /* DO_LWP */
#ifdef DO_IPV6
    else if ((strcasecmp(test_name,"TCPIPV6_RR") == 0) ||
	     (strcasecmp(test_name,"TCPIPV6_CRR") == 0) ||
	     (strcasecmp(test_name,"TCPIPV6_STREAM") == 0) ||
	     (strcasecmp(test_name,"UDPIPV6_RR") == 0) ||
	     (strcasecmp(test_name,"UDPIPV6_STREAM") == 0))
      {
	scan_ipv6_args(argc, argv);
      }
#endif /* DO_IPV6 */
#ifdef DO_DNS
    else if (strcasecmp(test_name,"DNS_RR") == 0)
      {
	scan_dns_args(argc, argv);
      }
#endif /* DO_DNS */
  }

#ifdef DO_IPV6
  /* address family check */   
  if ((strcasecmp(test_name,"TCPIPV6_RR") == 0) ||
      (strcasecmp(test_name,"TCPIPV6_CRR") == 0) ||
      (strcasecmp(test_name,"TCPIPV6_STREAM") == 0) ||
      (strcasecmp(test_name,"UDPIPV6_RR") == 0) ||
      (strcasecmp(test_name,"UDPIPV6_STREAM") == 0))
    {
      af = AF_INET6;
    }
#endif /* DO_IPV6 */
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
  printf("Port: %d\n",test_port);
  printf("Test name: %s\n",test_name);
  printf("Test bytes: %d Test time: %d Test trans: %d\n",
	 test_bytes,
	 test_time,
	 test_trans);
  printf("Host name: %s\n",host_name);
  printf("\n");
}
