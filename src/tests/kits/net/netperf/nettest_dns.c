#ifdef DO_DNS
#ifndef lint
char	nettest_dns_id[]="\
@(#)nettest_dns.c (c) Copyright 1997 Hewlett-Packard Co. Version 2.1pl4";
#else
#define DIRTY
#define HISTOGRAM
#define INTERVALS
#endif /* lint */

/****************************************************************/
/*								*/
/*	nettest_dns.c						*/
/*								*/
/*      scan_dns_args()                                         */
/*                                                              */
/*	the actual test routines...				*/
/*								*/
/*	send_dns_rr()		perform a dns request/response	*/
/*	recv_dns_rr()						*/
/*								*/
/****************************************************************/
     

 /* For the first iteration of this suite, netperf will rely on the */
 /* gethostbyname() and gethostbyaddr() calls instead of making DNS */
 /* calls directly. Later, when there is more time, this may be */
 /* altered so that the netperf code is making the DNS requests */
 /* directly, which would mean that more of the test could be */
 /* controlled directly from netperf instead of indirectly through */
 /* config files. raj 7/97 */

 /* In this suite, the netserver process really is not involved, it */
 /* only measures CPU utilization over the test interval. Of course, */
 /* this presumes that the netserver process will be running on the */
 /* same machine as the DNS server :) raj 7/97 */


#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef NOSTDLIBH
#include <malloc.h>
#else /* NOSTDLIBH */
#include <stdlib.h>
#endif /* NOSTDLIBH */

#ifndef WIN32
#include <sys/ipc.h>
#include <unistd.h>
#
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>
#else /* WIN32 */
#include <process.h>
#include <windows.h>
#include <winsock.h>
#define close(x)	closesocket(x)
#endif /* WIN32 */

#include "netlib.h"
#include "netsh.h"
#include "nettest_dns.h"

#ifdef HISTOGRAM
#ifdef __sgi
#include <sys/time.h>
#endif /* __sgi */
#include "hist.h"
#endif /* HISTOGRAM */



 /* these variables are specific to the DNS tests. declare them static */
 /* to make them global only to this file. */ 

static  int confidence_iteration;
static  char  local_cpu_method;
static  char  remote_cpu_method;

static  char  request_file[1024];

static  unsigned int dns_server_addr = INADDR_ANY;

#ifdef HISTOGRAM
static struct timeval time_one;
static struct timeval time_two;
static HIST time_hist;
#endif /* HISTOGRAM */


char dns_usage[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
DNS Test Options:\n\
    -f filename       Specify the request source file\n\
                        Default: /tmp/netperf_dns_requests\n\
    -h                Display this text\n\
\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n"; 
     

 /* this routine implements the client (netperf) side of the DNS_RR */
 /* test. */

void
send_dns_rr(remote_host)
     char	remote_host[];
{
  
  char *tput_title = "\
Elapsed  Transaction\n\
Time     Rate         \n\
Seconds  per second   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Time    Rate     local  remote local   remote\n\
secs.   per sec  %% %c    %% %c    us/Tr   us/Tr\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f %c\n";
  
  char *cpu_fmt_1_line_1 = "\
%-6.2f  %-6.2f  %-6.2f %-6.2f %-6.3f  %-6.3f\n";
  
  int			timed_out = 0;
  float			elapsed_time;
  
/*   char	*temp_message_ptr; */
  int	numtrans;
  int	trans_remaining;

  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct	hostent	        *hp;
  unsigned      int             addr;

  struct	dns_rr_request_struct	*dns_rr_request;
  struct	dns_rr_response_struct	*dns_rr_response;
  struct	dns_rr_results_struct	*dns_rr_result;
  
 /* we will save the old resolver state here and then muck with the */
 /* copy. of course, if we are just going to exit anyway, I'm not sure */
 /* if we need to save the state or not :) this is slightly */
 /* complicated in that on some old systems (eg HP-UX 9.X) , the */
 /* variable type is "state", where on newer systems (eg HP-UX 10.30), */
 /* the variable type is _res_state. raj 7/97 */

#ifdef __RES
  struct __res_state saved_res;
#else /* __RES */
  struct state      saved_res;
#endif /* __RES */

  /* these will be parameters to res_query() at some point, they will */
  /* come from the log file used to drive the test. for now, they will */
  /* be hard-coded. raj 7/97 */

  int           class = C_IN;  /* Internet class of resource records */
  int           type = T_A;    /* basic host address lookup */
  u_char        server_response[1024]; /* where the data goes */
  int	        len = sizeof(server_response);

#ifdef INTERVALS
  int	interval_count;
  sigset_t signal_set;
#endif /* INTERVALS */

  dns_rr_request = 
    (struct dns_rr_request_struct *)netperf_request.content.test_specific_data;
  dns_rr_response=
    (struct dns_rr_response_struct *)netperf_response.content.test_specific_data;
  dns_rr_result	=
    (struct dns_rr_results_struct *)netperf_response.content.test_specific_data;


  /* we need to get the IP address of the DNS server we will be */
  /* accessing. Because it may not be possible to run netserver on the */
  /* system acting as the DNS server, there is a test-specific option */
  /* to specify the name/addr of the DNS server. if that is not set, */
  /* we will use the hostname associated with the control connection. */
  /* we want to look this information-up *before* we redirect the */
  /* resolver libraries to the DNS SUT :) raj 7/97 */

  if (dns_server_addr == INADDR_ANY) {
    /* we use the control connection hostname as the way to find the */
    /* IP address of the DNS SUT */
    if ((dns_server_addr = inet_addr(remote_host)) == -1) {
      /* it was not an IP address, try it as a name, if it was */
      /* all-ones broadcast, we don't want to send to that anyway */
      if ((hp = gethostbyname(remote_host)) == NULL) {
	/* we have no idea what it is */
	fprintf(where,
		"send_dns_rr: could not resolve the DNS SUT %s\n",
		remote_host);
	fflush(where);
	exit(1);
      }
      else {
	/* it was a valid remote_host. at some point, we really need */
	/* to worry about IPv6 addresses and such, for now simply */
	/* assume v4 and a four byte address */
	bcopy(hp->h_addr,
	      (char *)&dns_server_addr,
	      4);
      }
    }
  }


#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */

  if ( print_headers ) {
    fprintf(where,"DNS REQUEST/RESPONSE TEST");
    fprintf(where," to %s", inet_ntoa(dns_server_addr));
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
    fprintf(where,"\n");
  }
  
  /* initialize a few counters */
  
  confidence_iteration = 1;
  init_stat();

  /* we previoulsy used gethostbyname to establish the control */
  /* connection t the netwserver. now we want to have our DNS queries */
  /* go to the SUT, not the DNS server that got us the control */
  /* connection information. so, we save the old resolver state (may */
  /* not really be needed) and then set it up with the IP address of */
  /* the DNS server we really want to access. if netperf ever goes */
  /* threaded, this will have to be revisisted. raj 7/97 */

  bcopy((char *) &_res, (char *)&saved_res, sizeof(_res));
  
  /* switch the state over to the test server and set the count */
  /* correctly. there are probably other things we want to play with */
  /* too, but those will have to wait until later. by doing this */
  /* switch, we bypass anything in the /etc/resolv.conf file of the */
  /* client, elminates netperf's dependency on that file being set-up */
  /* correctly for the test. raj 7/97 */

  _res.nscount = 1;
  _res.nsaddr_list[0].sin_addr.s_addr = dns_server_addr;

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

    numtrans        = 0;
    times_up        = 0;
    timed_out       = 0;
    trans_remaining = 0;

    if (debug) {
      fprintf(where,"send_dns_rr: counter initialization complete...\n");
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
    
    /* Tell the remote end to get ready. All the netserver side really */
    /* does in this test is measure CPU utilization. raj 7/97 */
    
    netperf_request.content.request_type =	DO_DNS_RR;
    dns_rr_request->measure_cpu	         =	remote_cpu_usage;
    dns_rr_request->cpu_rate	         =	remote_cpu_rate;
    if (test_time) {
      dns_rr_request->test_length	 =	test_time;
    }
    else {
      /* the DNS tests only support running for a length of time, so */
      /* if the user requested a number of transactions, we must abort */
      /* and tell him why. raj 7/97 */
      fprintf(where,
	      "netperf send_dns_rr: test length must be a time\n");
      fflush(where);
      exit(1);
    }
    
    if (debug > 1) {
      fprintf(where,"netperf: send_dns_rr: requesting DNS rr test\n");
    }
    
    send_request();
    
    /* The response from the remote will contain all of the relevant 	*/
    /* parameters for this test type. We will put them back into the */
    /* variables here so they can be displayed if desired.  The	remote */
    /* will have calibrated CPU if necessary, and will have done all */
    /* the needed set-up we will have calibrated the cpu locally */
    /* before sending the request, and will grab the counter value */
    /* right after the response returns. The remote will grab the */
    /* counter right after the response is sent. This is about as */
    /* close as we can get them - we trust that the delta will be */
    /* sufficiently small. raj 7/97 */
  
    recv_response();
  
    if (!netperf_response.content.serv_errno) {
      remote_cpu_usage  = dns_rr_response->measure_cpu;
      remote_cpu_rate   = dns_rr_response->cpu_rate;
      if (debug) {
	fprintf(where,"remote listen done.\n");
	fprintf(where," cpu_rate %g\n",remote_cpu_rate);
	fflush(where);
      }
    }
    else {
      errno = netperf_response.content.serv_errno;
      fprintf(where,
	      "netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      
      exit(1);
    }
    
    /* Set-up the test end conditions. For a DNS_RR test, this can */
    /* only be time based, and that was checked earlier. raj 7/97 */
    
    times_up = 0;
    start_timer(test_time);
    
    /* The cpu_start routine will grab the current time and possibly */
    /* value of the idle counter for later use in measuring cpu */
    /* utilization and/or service demand and thruput. */
    
    cpu_start(local_cpu_usage);

#ifdef INTERVALS
    if ((interval_burst) || (demo_mode)) {
      /* zero means that we never pause, so we never should need the */
      /* interval timer, unless we are in demo_mode */
      start_itimer(interval_wate);
    }
    interval_count = interval_burst;
    /* get the signal set for the call to sigsuspend */
    if (sigprocmask(SIG_BLOCK, (sigset_t *)NULL, &signal_set) != 0) {
      fprintf(where,
	      "send_dns_rr: unable to get sigmask errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
#endif /* INTERVALS */
    
    while (!times_up) {
      
#ifdef HISTOGRAM
      /* timestamp just before our call to  gethostbyname or */
      /* gethostbyaddr, and then again just  after the call. raj 7/97 */
      gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */
      
      /* until we implement the file, just request a fixed name. the */
      /* file will include what we are to request, and an indication */
      /* of whether or not it was supposed to "work" or "fail" at that */
      /* point we will learn how to interpret the server's response */
      /* raj 7/97 */
      
      /* we use res_query because we may be looking for just about */
      /* anything, and we do not want to be controlled in any way by */
      /* the client's nsswitch file. */
      if (res_query("hpindio.cup.hp.com",
		    class,
		    type,
		    server_response,
		    len) == -1) {
	fprintf(where,
		"send_dns_rr: res_query failed. errno %d\n",
		errno);
	fflush(where);
      }

#ifdef HISTOGRAM
      gettimeofday(&time_two,NULL);
      HIST_add(time_hist,delta_micro(&time_one,&time_two));
#endif /* HISTOGRAM */
#ifdef INTERVALS      
      if (demo_mode) {
	units_this_tick += 1;
      }
      /* in this case, the interval count is the count-down couter */
      /* to decide to sleep for a little bit */
      if ((interval_burst) && (--interval_count == 0)) {
	/* call sigsuspend and wait for the interval timer to get us */
	/* out */
	if (debug) {
	  fprintf(where,"about to suspend\n");
	  fflush(where);
	}
	if (sigsuspend(&signal_set) == EFAULT) {
	  fprintf(where,
		  "send_udp_rr: fault with signal set!\n");
	  fflush(where);
	  exit(1);
	}
	interval_count = interval_burst;
      }
#endif /* INTERVALS */
      
      numtrans++;          
      
      if (debug > 3) {
	if ((numtrans % 100) == 0) {
	  fprintf(where,
		  "Transaction %d completed\n",
		  numtrans);
	  fflush(where);
	}
      }
    }

    /* this call will always give us the elapsed time for the test, and */
    /* will also store-away the necessaries for cpu utilization */
    
    cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured? how long */
						/* did we really run? */
    
    /* Get the statistics from the remote end. The remote will have */
    /* calculated CPU utilization. If it wasn't supposed to care, it */
    /* will return obvious values. */ 
    
    recv_response();
    if (!netperf_response.content.serv_errno) {
      if (debug) {
	fprintf(where,"remote results obtained\n");
	fprintf(where,
		"remote elapsed time %g\n",dns_rr_result->elapsed_time);
	fprintf(where,
		"remote cpu util %g\n",dns_rr_result->cpu_util);
	fprintf(where,
		"remote num cpu %g\n",dns_rr_result->num_cpus);
	fflush(where);
      }
		
    }
    else {
      errno = netperf_response.content.serv_errno;
      fprintf(where,"netperf: remote error %d",
	      netperf_response.content.serv_errno);
      perror("");
      fflush(where);
      exit(1);
    }
    
    /* We now calculate what our throughput was for the test. */
  
    thruput	= numtrans/elapsed_time;
  
    /* for this test, the remote CPU util is really the only */
    /* interesting one - although I guess one can consider the local */
    /* CPU util as giving an idea if the client saturated before the */
    /* server */

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
	local_service_demand  = calc_service_demand((double) numtrans*1024,
						    0.0,
						    0.0,
						    0);
      }
      else {
	local_cpu_utilization	= (float) -1.0;
	local_service_demand	= (float) -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = dns_rr_result->cpu_util;
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	remote_service_demand = calc_service_demand((double) numtrans*1024,
						    0.0,
						    remote_cpu_utilization,
						    dns_rr_result->num_cpus);
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
    remote_cpu_method = format_cpu_method(dns_rr_result->cpu_method);
    
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
	      elapsed_time,		/* how long was the test */
	      thruput,
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
	      tput_fmt_1_line_1,	/* the format string */
	      elapsed_time, 		/* how long did it take */
	      thruput);
      
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

#ifdef HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */

  }
  
}


 /* this routine implements the receive (netserver) side of a DNS_RR */
 /* test */
void
recv_dns_rr()
{
  
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	dns_rr_request_struct	*dns_rr_request;
  struct	dns_rr_response_struct	*dns_rr_response;
  struct	dns_rr_results_struct	*dns_rr_results;
  
  dns_rr_request = 
    (struct dns_rr_request_struct *)netperf_request.content.test_specific_data;
  dns_rr_response =
    (struct dns_rr_response_struct *)netperf_response.content.test_specific_data;
  dns_rr_results =
    (struct dns_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dns_rr: entered...\n");
    fflush(where);
  }
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_dns_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = DNS_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_dns_rr: the response type is set...\n");
    fflush(where);
  }
  
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  dns_rr_response->cpu_rate = (float)0.0; 	/* assume no cpu */
  dns_rr_response->measure_cpu = 0;

  if (dns_rr_request->measure_cpu) {
    dns_rr_response->measure_cpu = 1;
    dns_rr_response->cpu_rate = calibrate_local_cpu(dns_rr_request->cpu_rate);
  }
  
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  dns_rr_response->test_length = dns_rr_request->test_length;
  send_response();
  
 /* the WIN32 stuff needs fleshing-out */
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
    fprintf(where,"recv_dns_rr: initial response sent.\n");
    fflush(where);
  }
  
  /* Now it's time to first grab the apropriate counters */
  
  cpu_start(dns_rr_request->measure_cpu);
  
  /* The loop will exit when we hit the end of the test time */
  
  times_up = 0;
  start_timer(dns_rr_request->test_length + PAD_TIME);

  /* since we know that a signal will be coming, we use pause() here. */
  /* using sleep leads to a premature exit that messes-up the CPU */
  /* utilization figures. raj 7/97 */
  
  pause();
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(dns_rr_request->measure_cpu,&elapsed_time);
  
  stop_timer();

  /* we ended the test by time, which was at least 2 seconds */
  /* longer than we wanted to run. so, we want to subtract */
  /* PAD_TIME from the elapsed_time. */
  elapsed_time -= PAD_TIME;

  /* send the results to the sender			*/
  
  dns_rr_results->elapsed_time   = elapsed_time;
  dns_rr_results->cpu_method     = cpu_method;
  dns_rr_results->num_cpus       = lib_num_loc_cpus;
  if (dns_rr_request->measure_cpu) {
    dns_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_dns_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}


void
print_dns_usage()
{

  printf("%s",dns_usage);
  exit(1);

}
void
scan_dns_args(argc, argv)
     int	argc;
     char	*argv[];

{
#define DNS_ARGS "f:hH:"
  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	
    arg1[BUFSIZ],  /* argument holders		*/
    arg2[BUFSIZ];
  
  struct hostent *hp;

  /* Go through all the command line arguments and break them */
  /* out. For those options that take two parms, specifying only */
  /* the first will set both to that value. Specifying only the */
  /* second will leave the first untouched. To change only the */
  /* first, use the form "first," (see the routine break_args.. */
  
  while ((c= getopt(argc, argv, DNS_ARGS)) != EOF) {
    switch (c) {
    case '?':	
    case 'h':
      print_dns_usage();
      exit(1);
    case 'f':
      /* this is the name of the log file from which we pick what to */
      /* request */
      strcpy(request_file,optarg);

      if (debug) {
	fprintf(where,"Will take DNS names from %s\n",optarg);
	fflush(where);
      }
      
      break;
    case 'H':
      /* someone wishes to specify the DNS server as being different */
      /* from the endpoint of the control connection */

      if ((dns_server_addr = inet_addr(optarg)) == -1) {
	/* it was not an IP address, try it as a name, if it was */
	/* all-ones broadcast, we don't want to send to that anyway */
	if ((hp = gethostbyname(optarg)) == NULL) {
	  /* we have no idea what it is */
	  fprintf(where,
		  "scan_dns_args: could not resolve the DNS SUT %s\n",
		  optarg);
	  fflush(where);
	  exit(1);
	}
	else {
	  /* it was a valid remote_host. at some point, we really need */
	  /* to worry about IPv6 addresses and such, for now simply */
	  /* assume v4 and a four byte address */
	  bcopy(hp->h_addr,
		(char *)&dns_server_addr,
		4);
	}
      }

      break;
    };
  }
}
#endif /* DO_DNS */
