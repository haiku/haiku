/*
        Copyright (C) 1997, Hewlett-Packard Company
*/

 /* This file contains the test-specific definitions for netperf's DNS */
 /* performance tests */

 /* For the first iteration of this suite, netperf will rely on the */
 /* gethostbyname() and gethostbyaddr() calls instead of making DNS */
 /* calls directly. Later, when there is more time, this may be */
 /* altered so that the netperf code is making the DNS requests */
 /* directly, which would mean that more of the test could be */
 /* controlled directly from netperf instead of indirectly through */
 /* config files. */

 /* In this suite, the netserver process really is not involved, it */
 /* only measures CPU utilization over the test interval. Of course, */
 /* this presumes that the netserver process will be running on the */
 /* same machine as the DNS server :) raj 7/97 */

struct	dns_rr_request_struct {
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
};

struct	dns_rr_response_struct {
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  float	cpu_rate;		/* could we measure	*/
};

struct dns_rr_results_struct {
  float	        elapsed_time;	/* how long the test ran */
  float	        cpu_util;	/* -1 if not measured */
  float	        serv_dem;	/* -1 if not measured */
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */
};

extern void scan_dns_args();

extern void send_dns_rr();

extern void recv_dns_rr();

extern void loc_cpu_rate();
extern void rem_cpu_rate();
