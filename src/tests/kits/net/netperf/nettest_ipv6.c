#ifdef DO_IPV6
#ifndef lint
char	nettest_ipv6_id[]="\
@(#)nettest_ipv6.c (c) Copyright 1995-2003 Hewlett-Packard Co. Version 2.2pl3";
#else
#define DIRTY
#define HISTOGRAM
#define INTERVALS
#endif /* lint */

/****************************************************************/
/*								*/
/*	nettest_ipv6.c						*/
/*								*/
/*      the IPV6 BSD sockets parsing routine...                 */
/*                                                              */
/*      scan_ipv6_args()                                        */
/*                                                              */
/*	the actual test routines...				*/
/*								*/
/*	send_tcpipv6_stream()	perform a tcpipv6 stream test	*/
/*	recv_tcpipv6_stream()					*/
/*	send_tcpipv6_rr()	perform a tcpipv6 reqt/resp	*/
/*	recv_tcpipv6_rr()					*/
/*      send_tcpipv6_conn_rr()  an RR test including connect    */
/*      recv_tcpipv6_conn_rr()                                  */
/*	send_udpipv6_stream()	perform a udp stream test	*/
/*	recv_udpipv6_stream()					*/
/*	send_udpipv6_rr()	perform a udp request/response	*/
/*	recv_udpipv6_rr()					*/
/*								*/
/****************************************************************/

 /* for the initial development systems, it is/was necessary to define */
 /* INET6 before including the files */
#define INET6
     
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#ifdef NOSTDLIBH
#include <malloc.h>
#else /* NOSTDLIBH */
#include <stdlib.h>
#endif /* NOSTDLIBH */
#include <netdb.h>

#include "netlib.h"
#include "netsh.h"
#include "nettest_bsd.h"
#include "nettest_ipv6.h"

#ifdef HISTOGRAM
#ifdef __sgi
#include <sys/time.h>
#endif /* __sgi */
#include "hist.h"
#endif /* HISTOGRAM */



 /* these variables are specific to the BSD sockets tests. declare */
 /* them static to make them global only to this file. */

static int	
  rss_size,		/* remote socket send buffer size	*/
  rsr_size,		/* remote socket recv buffer size	*/
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
 /* TCPIPV6_CRR and/or TCPIPV6_TRR tests. raj 3/95 */
static int client_port_min = 5000;
static int client_port_max = 65535;

 /* different options for the sockets				*/

int
  loc_nodelay,		/* don't/do use NODELAY	locally		*/
  rem_nodelay,		/* don't/do use NODELAY remotely	*/
  loc_sndavoid,		/* avoid send copies locally		*/
  loc_rcvavoid,		/* avoid recv copies locally		*/
  rem_sndavoid,		/* avoid send copies remotely		*/
  rem_rcvavoid;		/* avoid recv_copies remotely		*/

#ifdef HISTOGRAM
static struct timeval time_one;
static struct timeval time_two;
static HIST time_hist;
#endif /* HISTOGRAM */

 /* this is here since the V6 namespace may not be the same os the V4 */
 /* (still used for the control connection) space. raj 10/95 */

char test_host[HOSTNAMESIZE] = "\0";

char ipv6_usage[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
TCPIPV6/UDP BSD Sockets Test Options:\n\
    -D [L][,R]     Set TCP_NODELAY locally and/or remotely (TCPIPV6_*)\n\
    -h             Display this text\n\
    -H v6name      Use v6name for the remote machine name in the test\n\
    -m bytes       Set the send size (TCPIPV6_STREAM, UDPIPV6_STREAM)\n\
    -M bytes       Set the recv size (TCPIPV6_STREAM, UDPIPV6_STREAM)\n\
    -p min[,max]   Set the min/max port nums for TCPIPV6_CRR, TCPIPV6_TRR\n\
    -r bytes       Set request size (TCPIPV6_RR, UDPIPV6_RR)\n\
    -R bytes       Set response size (TCPIPV6_RR, UDPIPV6_RR)\n\
    -s send[,recv] Set local socket send/recv buffer sizes\n\
    -S send[,recv] Set remote socket send/recv buffer sizes\n\
\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n"; 
     

static
int
hostname2addr(hostname, sin6)
	char *hostname;
	struct sockaddr_in6 *sin6;
{
	struct addrinfo hints, *res;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	error = getaddrinfo(hostname, NULL, &hints, &res);
	if (error)
		return -1;

	memcpy(sin6, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	return 0;
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
get_tcp_info(socket, mss)
     int socket;
     int *mss;
{

  int sock_opt_len;

#ifdef TCP_MAXSEG
  sock_opt_len = sizeof(int);
  if (getsockopt(socket,
		 getprotobyname("tcp")->p_proto,	
		 TCP_MAXSEG,
		 (char *)mss,
		 &sock_opt_len) < 0) {
    fprintf(where,
	    "netperf: get_tcp_info: getsockopt TCP_MAXSEG: errno %d\n",
	    errno);
    fflush(where);
    lss_size = -1;
  }
#else
  *mss = -1;
#endif /* TCP_MAXSEG */
}


 /* This routine will create a data (listen) socket with the apropriate */
 /* options set and return it to the caller. this replaces all the */
 /* duplicate code in each of the test routines and should help make */
 /* things a little easier to understand. since this routine can be */
 /* called by either the netperf or netserver programs, all output */
 /* should be directed towards "where." family is generally AF_INET6, */
 /* and type will be either SOCK_STREAM or SOCK_DGRAM */
static
int
create_data_socket(family, type)
     int family;
     int type;
{

  int temp_socket;
  int one;
  int sock_opt_len;

  /*set up the data socket                        */
  temp_socket = socket(family, 
		       type,
		       0);
  
  if (temp_socket < 0){
    fprintf(where,
	    "netperf: create_data_socket: socket: %d\n",
	    errno);
    fflush(where);
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"create_data_socket: socket %d obtained...\n",temp_socket);
    fflush(where);
  }
  
  /* Modify the local socket size. The reason we alter the send buffer */
  /* size here rather than when the connection is made is to take care */
  /* of decreases in buffer size. Decreasing the window size after */
  /* connection establishment is a TCP no-no. Also, by setting the */
  /* buffer (window) size before the connection is established, we can */
  /* control the TCP MSS (segment size). The MSS is never more that 1/2 */
  /* the minimum receive buffer size at each half of the connection. */
  /* This is why we are altering the receive buffer size on the sending */
  /* size of a unidirectional transfer. If the user has not requested */
  /* that the socket buffers be altered, we will try to find-out what */
  /* their values are. If we cannot touch the socket buffer in any way, */
  /* we will set the values to -1 to indicate that.  */
  
#ifdef SO_SNDBUF
  if (lss_size > 0) {
    if(setsockopt(temp_socket, SOL_SOCKET, SO_SNDBUF,
		  (char *)&lss_size, sizeof(int)) < 0) {
      fprintf(where,
	      "netperf: create_data_socket: SO_SNDBUF option: errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
    if (debug > 1) {
      fprintf(where,
	      "netperf: create_data_socket: SO_SNDBUF of %d requested.\n",
	      lss_size);
      fflush(where);
    }
  }
  if (lsr_size > 0) {
    if(setsockopt(temp_socket, SOL_SOCKET, SO_RCVBUF,
		  (char *)&lsr_size, sizeof(int)) < 0) {
      fprintf(where,
	      "netperf: create_data_socket: SO_RCVBUF option: errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
    if (debug > 1) {
      fprintf(where,
	      "netperf: create_data_socket: SO_SNDBUF of %d requested.\n",
	      lss_size);
      fflush(where);
    }
  }
  
  
  /* Now, we will find-out what the size actually became, and report */
  /* that back to the user. If the call fails, we will just report a -1 */
  /* back to the initiator for the recv buffer size. */
  
  sock_opt_len = sizeof(int);
  if (getsockopt(temp_socket,
		 SOL_SOCKET,	
		 SO_SNDBUF,
		 (char *)&lss_size,
		 &sock_opt_len) < 0) {
    fprintf(where,
	    "netperf: create_data_socket: getsockopt SO_SNDBUF: errno %d\n",
	    errno);
    fflush(where);
    lss_size = -1;
  }
  if (getsockopt(temp_socket,
		 SOL_SOCKET,	
		 SO_RCVBUF,
		 (char *)&lsr_size,
		 &sock_opt_len) < 0) {
    fprintf(where,
	    "netperf: create_data_socket: getsockopt SO_SNDBUF: errno %d\n",
	    errno);
    fflush(where);
    lsr_size = -1;
  }
  
  if (debug) {
    fprintf(where,"netperf: create_data_socket: socket sizes determined...\n");
    fprintf(where,"                       send: %d recv: %d\n",
	    lss_size,lsr_size);
    fflush(where);
  }
  
#else /* SO_SNDBUF */
  
  lss_size = -1;
  lsr_size = -1;
  
#endif /* SO_SNDBUF */

  /* now, we may wish to enable the copy avoidance features on the */
  /* local system. of course, this may not be possible... */
  
#ifdef SO_RCV_COPYAVOID
  if (loc_rcvavoid) {
    if (setsockopt(temp_socket,
		   SOL_SOCKET,
		   SO_RCV_COPYAVOID,
		   &loc_rcvavoid,
		   sizeof(int)) < 0) {
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
		   &loc_sndavoid,
		   sizeof(int)) < 0) {
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
  
  /* Now, we will see about setting the TCP_NO_DELAY flag on the local */
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
		  sizeof(one)) < 0) {
      fprintf(where,"netperf: create_data_socket: nodelay: errno %d\n",
	      errno);
      fflush(where);
    }
    
    if (debug > 1) {
      fprintf(where,
	      "netperf: create_data_socket: TCP_NODELAY requested...\n");
      fflush(where);
    }
  }
#else /* TCP_NODELAY */
  
  loc_nodelay = 0;
  
#endif /* TCP_NODELAY */

  return(temp_socket);

}


/* This routine implements the TCP unidirectional data transfer test */
/* (a.k.a. stream) for the sockets interface. It receives its */
/* parameters via global variables from the shell and writes its */
/* output to the standard output. */


void 
send_tcpipv6_stream(remote_host)
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
  
#ifdef INTERVALS
  int interval_count;
  sigset_t signal_set;
#endif
  
  /* what we want is to have a buffer space that is at least one */
  /* send-size greater than our send window. this will insure that we */
  /* are never trying to re-use a buffer that may still be in the hands */
  /* of the transport. This buffer will be malloc'd after we have found */
  /* the size of the local senc socket buffer. We will want to deal */
  /* with alignment and offset concerns as well. */
  
#ifdef DIRTY
  int	*message_int_ptr;
#endif

  struct ring_elt *send_ring;
  
  int len;
  unsigned int nummessages = 0;
  int send_socket;
  int bytes_remaining;
  int tcp_mss;

  /* with links like fddi, one can send > 32 bits worth of bytes */
  /* during a test... ;-) at some point, this should probably become a */
  /* 64bit integral type, but those are not entirely common yet */
  double	bytes_sent = 0.0;
  
#ifdef DIRTY
  int	i;
#endif /* DIRTY */
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;

  double	thruput;
  
  struct	hostent	        *hp;
  struct	sockaddr_in6	server;
  
  struct	tcpipv6_stream_request_struct	*tcpipv6_stream_request;
  struct	tcpipv6_stream_response_struct	*tcpipv6_stream_response;
  struct	tcpipv6_stream_results_struct	*tcpipv6_stream_result;
  
  tcpipv6_stream_request  = 
    (struct tcpipv6_stream_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_stream_response =
    (struct tcpipv6_stream_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_stream_result   = 
    (struct tcpipv6_stream_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  
  if ((hostname2addr(remote_host, &server)) == -1) {
    fprintf(where,
	    "send_tcpipv6_stream: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  if ( print_headers ) {
    /* we want to have some additional, interesting information in */
    /* the headers. we know some of it here, but not all, so we will */
    /* only print the test title here and will print the results */
    /* titles after the test is finished */
    fprintf(where,"TCPIPV6 STREAM TEST");
    fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_nodelay || rem_nodelay) {
      fprintf(where," : nodelay");
    }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    send_socket = create_data_socket(AF_INET6, 
				     SOCK_STREAM);
    
    if (send_socket < 0){
      perror("netperf: send_tcpipv6_stream: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_tcpipv6_stream: send_socket obtained...\n");
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
    
    /* Tell the remote end to do a listen. The server alters the socket */
    /* paramters on the other side at this point, hence the reason for */
    /* all the values being passed in the setup message. If the user did */
    /* not specify any of the parameters, they will be passed as 0, which */
    /* will indicate to the remote that no changes beyond the system's */
    /* default should be used. Alignment is the exception, it will */
    /* default to 1, which will be no alignment alterations. */
    
    netperf_request.content.request_type	=	DO_TCPIPV6_STREAM;
    tcpipv6_stream_request->send_buf_size	=	rss_size;
    tcpipv6_stream_request->recv_buf_size	=	rsr_size;
    tcpipv6_stream_request->receive_size	=	recv_size;
    tcpipv6_stream_request->no_delay	=	rem_nodelay;
    tcpipv6_stream_request->recv_alignment	=	remote_recv_align;
    tcpipv6_stream_request->recv_offset	=	remote_recv_offset;
    tcpipv6_stream_request->measure_cpu	=	remote_cpu_usage;
    tcpipv6_stream_request->cpu_rate	=	remote_cpu_rate;
    if (test_time) {
      tcpipv6_stream_request->test_length	=	test_time;
    }
    else {
      tcpipv6_stream_request->test_length	=	test_bytes;
    }
    tcpipv6_stream_request->so_rcvavoid	=	rem_rcvavoid;
    tcpipv6_stream_request->so_sndavoid	=	rem_sndavoid;
#ifdef DIRTY
    tcpipv6_stream_request->dirty_count       =       rem_dirty_count;
    tcpipv6_stream_request->clean_count       =       rem_clean_count;
#endif /* DIRTY */
    
    
    if (debug > 1) {
      fprintf(where,
	      "netperf: send_tcpipv6_stream: requesting TCP stream test\n");
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
      rsr_size	      =	tcpipv6_stream_response->recv_buf_size;
      rss_size	      =	tcpipv6_stream_response->send_buf_size;
      rem_nodelay     =	tcpipv6_stream_response->no_delay;
      remote_cpu_usage=	tcpipv6_stream_response->measure_cpu;
      remote_cpu_rate = tcpipv6_stream_response->cpu_rate;

      /* we have to make sure that the server port number is in */
      /* network order */
      server.sin6_port   = tcpipv6_stream_response->data_port_number;
      server.sin6_port   = htons(server.sin6_port); 
      rem_rcvavoid	= tcpipv6_stream_response->so_rcvavoid;
      rem_sndavoid	= tcpipv6_stream_response->so_sndavoid;
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
      exit(1);
    }
    
    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		(struct sockaddr *)&server,
		sizeof(server)) <0){
      perror("netperf: send_tcpipv6_stream: data socket connect failed");
      printf(" port: %d\n",ntohs(server.sin6_port));
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
	      "send_tcpipv6_stream: unable to get sigmask errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
#endif /* INTERVALS */

#ifdef DIRTY
    /* initialize the random number generator for putting dirty stuff */
    /* into the send buffer. raj */
    srand((int) getpid());
#endif
    
    /* before we start, initialize a few variables */

    /* We use an "OR" to control test execution. When the test is */
    /* controlled by time, the byte count check will always return false. */
    /* When the test is controlled by byte count, the time test will */
    /* always return false. When the test is finished, the whole */
    /* expression will go false and we will stop sending data. */
    
    while ((!times_up) || (bytes_remaining > 0)) {
      
#ifdef DIRTY
      /* we want to dirty some number of consecutive integers in the buffer */
      /* we are about to send. we may also want to bring some number of */
      /* them cleanly into the cache. The clean ones will follow any dirty */
      /* ones into the cache. at some point, we might want to replace */
      /* the rand() call with something from a table to reduce our call */
      /* overhead during the test, but it is not a high priority item. */
      message_int_ptr = (int *)(send_ring->buffer_ptr);
      for (i = 0; i < loc_dirty_count; i++) {
	*message_int_ptr = rand();
	message_int_ptr++;
      }
      for (i = 0; i < loc_clean_count; i++) {
	loc_dirty_count = *message_int_ptr;
	message_int_ptr++;
      }
#endif /* DIRTY */
      
#ifdef HISTOGRAM
      /* timestamp just before we go into send and then again just after */
      /* we come out raj 8/94 */
      gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */
      
      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   send_size,
		   0)) != send_size) {
	if ((len >=0) || (errno == EINTR)) {
	  /* the test was interrupted, must be the end of test */
	  break;
	}
	perror("netperf: data send error");
	printf("len was %d\n",len);
	exit(1);
      }

#ifdef HISTOGRAM
      /* timestamp the exit from the send call and update the histogram */
      gettimeofday(&time_two,NULL);
      HIST_add(time_hist,delta_micro(&time_one,&time_two));
#endif /* HISTOGRAM */      

#ifdef INTERVALS      
      if (demo_mode) {
	units_this_tick += send_size;
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
		  "send_tcpipv6_stream: fault with sigsuspend.\n");
	  fflush(where);
	  exit(1);
	}
	interval_count = interval_burst;
      }
#endif /* INTERVALS */
      
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
    
    if (shutdown(send_socket,1) == -1) {
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

    /* Get the statistics from the remote end. The remote will have */
    /* calculated service demand and all those interesting things. If it */
    /* wasn't supposed to care, it will return obvious values. */
    
    recv_response();
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote results obtained\n");
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
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
    
    bytes_sent	= ntohd(tcpipv6_stream_result->bytes_received);

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
	local_cpu_utilization	= -1.0;
	local_service_demand	= -1.0;
      }
      
      if (remote_cpu_usage) {
	
	remote_cpu_utilization	= tcpipv6_stream_result->cpu_util;
	remote_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      remote_cpu_utilization,
						      tcpipv6_stream_result->num_cpus);
      }
      else {
	remote_cpu_utilization = -1.0;
	remote_service_demand  = -1.0;
      }
    }    
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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
    remote_cpu_method = format_cpu_method(tcpipv6_stream_result->cpu_method);
    
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
	    bytes_sent / (double)tcpipv6_stream_result->recv_calls,
	    tcpipv6_stream_result->recv_calls);
    fprintf(where,
	    ksink_fmt2,
	    tcp_mss);
    fflush(where);
#ifdef HISTOGRAM
    fprintf(where,"\n\nHistogram of time spent in send() call.\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */
  }
  
}


/* This is the server-side routine for the tcp stream test. It is */
/* implemented as one routine. I could break things-out somewhat, but */
/* didn't feel it was necessary. */

void
recv_tcpipv6_stream()
{
  
  struct sockaddr_in6 myaddr_in, peeraddr_in;
  int	s_listen,s_data;
  int 	addrlen;
  int	len;
  unsigned int	receive_calls;
  float	elapsed_time;
  double   bytes_received;
  
  struct ring_elt *recv_ring;

#ifdef DIRTY
  int   *message_int_ptr;
  int   dirty_count;
  int   clean_count;
  int   i;
#endif
  
  struct	tcpipv6_stream_request_struct	*tcpipv6_stream_request;
  struct	tcpipv6_stream_response_struct	*tcpipv6_stream_response;
  struct	tcpipv6_stream_results_struct	*tcpipv6_stream_results;
  
  tcpipv6_stream_request	= 
    (struct tcpipv6_stream_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_stream_response	= 
    (struct tcpipv6_stream_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_stream_results	= 
    (struct tcpipv6_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcpipv6_stream: entered...\n");
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
    fprintf(where,"recv_tcpipv6_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCPIPV6_STREAM_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_stream: requested alignment of %d\n",
	    tcpipv6_stream_request->recv_alignment);
    fflush(where);
  }

  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  myaddr_in.sin6_port        = 0;
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_stream: grabbing a socket...\n");
    fflush(where);
  }
  
  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size = tcpipv6_stream_request->send_buf_size;
  lsr_size = tcpipv6_stream_request->recv_buf_size;
  loc_nodelay = tcpipv6_stream_request->no_delay;
  loc_rcvavoid = tcpipv6_stream_request->so_rcvavoid;
  loc_sndavoid = tcpipv6_stream_request->so_sndavoid;

  s_listen = create_data_socket(AF_INET6,
				SOCK_STREAM);
  
  if (s_listen < 0) {
    if (debug) {
      fprintf(where,
	      "recv_tcpipv6_stream: unable to grab a socket\n");
      fflush(where);
    }
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
	   sizeof(myaddr_in)) == -1) {
    if (debug) {
      fprintf(where,
	      "recv_tcpipv6_stream: bind failled errno %d...\n",
	      errno);
      fflush(where);
    }
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* what sort of sizes did we end-up with? */
  if (tcpipv6_stream_request->receive_size == 0) {
    if (lsr_size > 0) {
      recv_size = lsr_size;
    }
    else {
      recv_size = 4096;
    }
  }
  else {
    recv_size = tcpipv6_stream_request->receive_size;
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
				   tcpipv6_stream_request->recv_alignment,
				   tcpipv6_stream_request->recv_offset);

  if (debug) {
    fprintf(where,"recv_tcpipv6_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == -1){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcpipv6_stream_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  tcpipv6_stream_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (tcpipv6_stream_request->measure_cpu) {
    tcpipv6_stream_response->measure_cpu = 1;
    tcpipv6_stream_response->cpu_rate = 
      calibrate_local_cpu(tcpipv6_stream_request->cpu_rate);
  }
  else {
    tcpipv6_stream_response->measure_cpu = 0;
  }
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcpipv6_stream_response->send_buf_size = lss_size;
  tcpipv6_stream_response->recv_buf_size = lsr_size;
  tcpipv6_stream_response->no_delay = loc_nodelay;
  tcpipv6_stream_response->so_rcvavoid = loc_rcvavoid;
  tcpipv6_stream_response->so_sndavoid = loc_sndavoid;
  tcpipv6_stream_response->receive_size = recv_size;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  if ((s_data=accept(s_listen,
		     (struct sockaddr *)&peeraddr_in,
		     &addrlen)) == -1) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    exit(1);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcpipv6_stream_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
#ifdef DIRTY
    /* we want to dirty some number of consecutive integers in the buffer */
    /* we are about to recv. we may also want to bring some number of */
    /* them cleanly into the cache. The clean ones will follow any dirty */
    /* ones into the cache. */

  dirty_count = tcpipv6_stream_request->dirty_count;
  clean_count = tcpipv6_stream_request->clean_count;
  message_int_ptr = (int *)recv_ring->buffer_ptr;
  for (i = 0; i < dirty_count; i++) {
    *message_int_ptr = rand();
    message_int_ptr++;
  }
  for (i = 0; i < clean_count; i++) {
    dirty_count = *message_int_ptr;
    message_int_ptr++;
  }
#endif /* DIRTY */

  bytes_received = 0;
  receive_calls  = 0;

  while ((len = recv(s_data, recv_ring->buffer_ptr, recv_size, 0)) != 0) {
    if (len < 0) {
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
    bytes_received += len;
    receive_calls++;

    /* more to the next buffer in the recv_ring */
    recv_ring = recv_ring->next;

#ifdef DIRTY
    message_int_ptr = (int *)(recv_ring->buffer_ptr);
    for (i = 0; i < dirty_count; i++) {
      *message_int_ptr = rand();
      message_int_ptr++;
    }
    for (i = 0; i < clean_count; i++) {
      dirty_count = *message_int_ptr;
      message_int_ptr++;
    }
#endif /* DIRTY */
  }
  
  /* perform a shutdown to signal the sender that */
  /* we have received all the data sent. raj 4/93 */

  if (shutdown(s_data,1) == -1) {
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
  
  cpu_stop(tcpipv6_stream_request->measure_cpu,&elapsed_time);
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_stream: got %g bytes\n",
	    bytes_received);
    fprintf(where,
	    "recv_tcpipv6_stream: got %d recvs\n",
	    receive_calls);
    fflush(where);
  }
  
  tcpipv6_stream_results->bytes_received	= htond(bytes_received);
  tcpipv6_stream_results->elapsed_time	= elapsed_time;
  tcpipv6_stream_results->recv_calls	= receive_calls;
  
  if (tcpipv6_stream_request->measure_cpu) {
    tcpipv6_stream_results->cpu_util	= calc_cpu_util(0.0);
  };
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_stream: test complete, sending results.\n");
    fprintf(where,
	    "                 bytes_received %g receive_calls %d\n",
	    bytes_received,
	    receive_calls);
    fprintf(where,
	    "                 len %d\n",
	    len);
    fflush(where);
  }
  
  tcpipv6_stream_results->cpu_method = cpu_method;
  send_response();

  /* we are now done with the sockets */
  close(s_data);
  close(s_listen);

}


 /* this routine implements the sending (netperf) side of the TCPIPV6_RR */
 /* test. */

void
send_tcpipv6_rr(remote_host)
     char	remote_host[];
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
%5d  %5d   %5d  %5d";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  
  int	len;
  char	*temp_message_ptr;
  int	nummessages;
  int	send_socket;
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
  struct	sockaddr_in6	server;
  
  struct	tcpipv6_rr_request_struct	*tcpipv6_rr_request;
  struct	tcpipv6_rr_response_struct	*tcpipv6_rr_response;
  struct	tcpipv6_rr_results_struct	*tcpipv6_rr_result;
  
#ifdef INTERVALS
  int	interval_count;
  sigset_t signal_set;
#endif /* INTERVALS */

  tcpipv6_rr_request = 
    (struct tcpipv6_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_rr_response=
    (struct tcpipv6_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_rr_result	=
    (struct tcpipv6_rr_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */

  bzero((char *)&server,
	sizeof(server));
  
  /* see if we need to use a different hostname */
  if (test_host[0] != '\0') {
    strcpy(remote_host,test_host);
    if (debug > 1) {
      fprintf(where,"Switching test host to %s\n",remote_host);
      fflush(where);
    }
  }

  
  
  if ((hostname2addr(remote_host, &server)) == -1) {
    fprintf(where,
	    "send_tcpipv6_rr: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  
  if ( print_headers ) {
    fprintf(where,"TCPIPV6 REQUEST/RESPONSE TEST");
    fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_nodelay || rem_nodelay) {
      fprintf(where," : nodelay");
    }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    send_socket = create_data_socket(AF_INET6, 
				     SOCK_STREAM);
  
    if (send_socket < 0){
      perror("netperf: send_tcpipv6_rr: tcp stream data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_tcpipv6_rr: send_socket obtained...\n");
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
    
    netperf_request.content.request_type	=	DO_TCPIPV6_RR;
    tcpipv6_rr_request->recv_buf_size	=	rsr_size;
    tcpipv6_rr_request->send_buf_size	=	rss_size;
    tcpipv6_rr_request->recv_alignment      =	remote_recv_align;
    tcpipv6_rr_request->recv_offset	        =	remote_recv_offset;
    tcpipv6_rr_request->send_alignment      =	remote_send_align;
    tcpipv6_rr_request->send_offset	        =	remote_send_offset;
    tcpipv6_rr_request->request_size	=	req_size;
    tcpipv6_rr_request->response_size	=	rsp_size;
    tcpipv6_rr_request->no_delay	        =	rem_nodelay;
    tcpipv6_rr_request->measure_cpu	        =	remote_cpu_usage;
    tcpipv6_rr_request->cpu_rate	        =	remote_cpu_rate;
    tcpipv6_rr_request->so_rcvavoid	        =	rem_rcvavoid;
    tcpipv6_rr_request->so_sndavoid	        =	rem_sndavoid;
    if (test_time) {
      tcpipv6_rr_request->test_length	=	test_time;
    }
    else {
      tcpipv6_rr_request->test_length	=	test_trans * -1;
    }
    
    if (debug > 1) {
      fprintf(where,"netperf: send_tcpipv6_rr: requesting TCP rr test\n");
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
      rsr_size          = tcpipv6_rr_response->recv_buf_size;
      rss_size          = tcpipv6_rr_response->send_buf_size;
      rem_nodelay       = tcpipv6_rr_response->no_delay;
      remote_cpu_usage  = tcpipv6_rr_response->measure_cpu;
      remote_cpu_rate   = tcpipv6_rr_response->cpu_rate;
      /* make sure that port numbers are in network order */
      server.sin6_port   = tcpipv6_rr_response->data_port_number;
      server.sin6_port   = htons(server.sin6_port);
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
      exit(1);
    }
    
    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		(struct sockaddr *)&server,
		sizeof(server)) <0){
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
	      "send_tcpipv6_rr: unable to get sigmask errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
#endif /* INTERVALS */
    
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
      
#ifdef HISTOGRAM
      /* timestamp just before our call to send, and then again just */
      /* after the receive raj 8/94 */
      gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */
      
      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   req_size,
		   0)) != req_size) {
	if ((errno == EINTR) || (errno == 0)) {
	  /* we hit the end of a */
	  /* timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_tcpipv6_rr: data send error");
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
				 0)) < 0) {
	  if (errno == EINTR) {
	    /* We hit the end of a timed test. */
	    timed_out = 1;
	    break;
	  }
	  perror("send_tcpipv6_rr: data recv error");
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
		  "send_udpipv6_rr: fault with signal set!\n");
	  fflush(where);
	  exit(1);
	}
	interval_count = interval_burst;
      }
#endif /* INTERVALS */
      
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
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
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
	local_cpu_utilization	= -1.0;
	local_service_demand	= -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = tcpipv6_rr_result->cpu_util;
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	remote_service_demand = calc_service_demand((double) nummessages*1024,
						    0.0,
						    remote_cpu_utilization,
						    tcpipv6_rr_result->num_cpus);
      }
      else {
	remote_cpu_utilization = -1.0;
	remote_service_demand  = -1.0;
      }
      
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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
    remote_cpu_method = format_cpu_method(tcpipv6_rr_result->cpu_method);
    
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

#ifdef HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */

  }
  
}

void
send_udpipv6_stream(remote_host)
char	remote_host[];
{
  /**********************************************************************/
  /*									*/
  /*               	UDP Unidirectional Send Test                    */
  /*									*/
  /**********************************************************************/
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
  int 	data_socket;
  
  unsigned int sum_messages_sent;
  unsigned int sum_messages_recvd;
  unsigned int sum_failed_sends;
  double sum_local_thruput;

#ifdef INTERVALS
  int	interval_count;
  sigset_t signal_set;
#endif /* INTERVALS */
#ifdef DIRTY
  int	*message_int_ptr;
  int	i;
#endif /* DIRTY */
  
  struct	hostent	        *hp;
  struct	sockaddr_in6	server;
  
  struct	udpipv6_stream_request_struct	*udpipv6_stream_request;
  struct	udpipv6_stream_response_struct	*udpipv6_stream_response;
  struct	udpipv6_stream_results_struct	*udpipv6_stream_results;
  
  udpipv6_stream_request	= 
    (struct udpipv6_stream_request_struct *)netperf_request.content.test_specific_data;
  udpipv6_stream_response	= 
    (struct udpipv6_stream_response_struct *)netperf_response.content.test_specific_data;
  udpipv6_stream_results	= 
    (struct udpipv6_stream_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  
  /* see if we need to use a different hostname */
  if (test_host[0] != '\0') {
    strcpy(remote_host,test_host);
    if (debug > 1) {
      fprintf(where,"Switching test host to %s\n",remote_host);
      fflush(where);
    }
  }
  if ((hostname2addr(remote_host,&server)) == -1) {
    fprintf(where,
	    "send_udpipv6_stream: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  if ( print_headers ) {
    fprintf(where,"UDPIPV6 UNIDIRECTIONAL SEND TEST");
    fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    data_socket = create_data_socket(AF_INET6,
				     SOCK_DGRAM);
    
    if (data_socket < 0){
      perror("udp_send: data socket");
      exit(1);
    }
    
    /* now, we want to see if we need to set the send_size */
    if (send_size == 0) {
      if (lss_size > 0) {
	send_size = lss_size;
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
    
    /* Tell the remote end to set up the data connection. The server */
    /* sends back the port number and alters the socket parameters there. */
    /* Of course this is a datagram service so no connection is actually */
    /* set up, the server just sets up the socket and binds it. */
    
    netperf_request.content.request_type      = DO_UDPIPV6_STREAM;
    udpipv6_stream_request->recv_buf_size  = rsr_size;
    udpipv6_stream_request->message_size   = send_size;
    udpipv6_stream_request->recv_alignment = remote_recv_align;
    udpipv6_stream_request->recv_offset    = remote_recv_offset;
    udpipv6_stream_request->measure_cpu    = remote_cpu_usage;
    udpipv6_stream_request->cpu_rate       = remote_cpu_rate;
    udpipv6_stream_request->test_length    = test_time;
    udpipv6_stream_request->so_rcvavoid    = rem_rcvavoid;
    udpipv6_stream_request->so_sndavoid    = rem_sndavoid;
    
    send_request();
    
    recv_response();
    
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"send_udpipv6_stream: remote data connection done.\n");
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("send_udpipv6_stream: error on remote");
      exit(1);
    }
    
    /* Place the port number returned by the remote into the sockaddr */
    /* structure so our sends can be sent to the correct place. Also get */
    /* some of the returned socket buffer information for user display. */
    
    /* make sure that port numbers are in the proper order */
    server.sin6_port = udpipv6_stream_response->data_port_number;
    server.sin6_port = htons(server.sin6_port);
    rsr_size        = udpipv6_stream_response->recv_buf_size;
    rss_size        = udpipv6_stream_response->send_buf_size;
    remote_cpu_rate = udpipv6_stream_response->cpu_rate;
    
    /* We "connect" up to the remote post to allow is to use the send */
    /* call instead of the sendto call. Presumeably, this is a little */
    /* simpler, and a little more efficient. I think that it also means */
    /* that we can be informed of certain things, but am not sure */
    /* yet...also, this is the way I would expect a client to behave */
    /* when talking to a server */
    
    if (connect(data_socket,
		(struct sockaddr *)&server,
		sizeof(server)) <0){
      perror("send_udpipv6_stream: data socket connect failed");
      exit(1);
    }
    
    /* set up the timer to call us after test_time. one of these days, */
    /* it might be nice to figure-out a nice reliable way to have the */
    /* test controlled by a byte count as well, but since UDP is not */
    /* reliable, that could prove difficult. so, in the meantime, we */
    /* only allow a UDPIPV6_STREAM test to be a timed test. */
    
    if (test_time) {
      times_up = 0;
      start_timer(test_time);
    }
    else {
      fprintf(where,"Sorry, UDPIPV6_STREAM tests must be timed.\n");
      fflush(where);
    }
    
    /* Get the start count for the idle counter and the start time */
    
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
	      "send_udpipv6_stream: unable to get sigmask errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
#endif /* INTERVALS */
    
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
      message_int_ptr = (int *)(send_ring->buffer_ptr);
      for (i = 0; i < loc_dirty_count; i++) {
	*message_int_ptr = 4;
	message_int_ptr++;
      }
      for (i = 0; i < loc_clean_count; i++) {
	loc_dirty_count = *message_int_ptr;
	message_int_ptr++;
      }
#endif /* DIRTY */
      
#ifdef HISTOGRAM
      gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */
      
      if ((len=send(data_socket,
		    send_ring->buffer_ptr,
		    send_size,
		    0))  != send_size) {
	if ((len >= 0) || (errno == EINTR))
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
      
      
#ifdef HISTOGRAM
      /* get the second timestamp */
      gettimeofday(&time_two,NULL);
      HIST_add(time_hist,delta_micro(&time_one,&time_two));
#endif /* HISTOGRAM */
#ifdef INTERVALS      
      if (demo_mode) {
	units_this_tick += send_size;
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
		  "send_udpipv6_stream: fault with signal set!\n");
	  fflush(where);
	  exit(1);
	}
	interval_count = interval_burst;
      }
#endif /* INTERVALS */
      
    }
    
    /* This is a timed test, so the remote will be returning to us after */
    /* a time. We should not need to send any "strange" messages to tell */
    /* the remote that the test is completed, unless we decide to add a */
    /* number of messages to the test. */
    
    /* the test is over, so get stats and stuff */
    cpu_stop(local_cpu_usage,	
	     &elapsed_time);
    
    /* Get the statistics from the remote end	*/
    recv_response();
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"send_udpipv6_stream: remote results obtained\n");
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("send_udpipv6_stream: error on remote");
      exit(1);
    }
    
    bytes_sent    = (double) send_size * (double) messages_sent;
    local_thruput = calc_thruput(bytes_sent);
    
    messages_recvd = udpipv6_stream_results->messages_recvd;
    bytes_recvd    = (double) send_size * (double) messages_recvd;
    
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
	local_cpu_utilization	= -1.0;
	local_service_demand	= -1.0;
      }
      
      /* The local calculations could use variables being kept by */
      /* the local netlib routines. The remote calcuations need to */
      /* have a few things passed to them. */
      if (remote_cpu_usage) {
	remote_cpu_utilization	= udpipv6_stream_results->cpu_util;
	remote_service_demand	= calc_service_demand(bytes_recvd,
						      0.0,
						      remote_cpu_utilization,
						      udpipv6_stream_results->num_cpus);
      }
      else {
	remote_cpu_utilization	= -1.0;
	remote_service_demand	= -1.0;
      }
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization  = -1.0;
      local_service_demand   = -1.0;
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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
    remote_cpu_method = format_cpu_method(udpipv6_stream_results->cpu_method);
    
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
#ifdef HISTOGRAM
  if (verbosity > 1) {
    fprintf(where,"\nHistogram of time spent in send() call\n");
    fflush(where);
    HIST_report(time_hist);
  }
#endif /* HISTOGRAM */

}


 /* this routine implements the receive side (netserver) of the */
 /* UDPIPV6_STREAM performance test. */

void
recv_udpipv6_stream()
{
  struct ring_elt *recv_ring;

  struct sockaddr_in6 myaddr_in;
  int	s_data;
  int 	addrlen;
  int	len = 0;
  unsigned int	bytes_received = 0;
  float	elapsed_time;
  
  unsigned int	message_size;
  unsigned int	messages_recvd = 0;
  
  struct	udpipv6_stream_request_struct	*udpipv6_stream_request;
  struct	udpipv6_stream_response_struct	*udpipv6_stream_response;
  struct	udpipv6_stream_results_struct	*udpipv6_stream_results;
  
  udpipv6_stream_request  = 
    (struct udpipv6_stream_request_struct *)netperf_request.content.test_specific_data;
  udpipv6_stream_response = 
    (struct udpipv6_stream_response_struct *)netperf_response.content.test_specific_data;
  udpipv6_stream_results  = 
    (struct udpipv6_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_udpipv6_stream: entered...\n");
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
    fprintf(where,"recv_udpipv6_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = UDPIPV6_STREAM_RESPONSE;
  
  if (debug > 2) {
    fprintf(where,"recv_udpipv6_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug > 1) {
    fprintf(where,"recv_udpipv6_stream: requested alignment of %d\n",
	    udpipv6_stream_request->recv_alignment);
    fflush(where);
  }

  if (recv_width == 0) recv_width = 1;

  recv_ring = allocate_buffer_ring(recv_width,
				   udpipv6_stream_request->message_size,
				   udpipv6_stream_request->recv_alignment,
				   udpipv6_stream_request->recv_offset);

  if (debug > 1) {
    fprintf(where,"recv_udpipv6_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
  myaddr_in.sin6_port        = 0;
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug > 1) {
    fprintf(where,"recv_udpipv6_stream: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lsr_size = udpipv6_stream_request->recv_buf_size;
  loc_rcvavoid = udpipv6_stream_request->so_rcvavoid;
  loc_sndavoid = udpipv6_stream_request->so_sndavoid;
    
  s_data = create_data_socket(AF_INET6,
			      SOCK_DGRAM);
  
  if (s_data < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  if (bind(s_data,
	   (struct sockaddr *)&myaddr_in,
	   sizeof(myaddr_in)) == -1) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  udpipv6_stream_response->test_length = udpipv6_stream_request->test_length;
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_data, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == -1){
    netperf_response.content.serv_errno = errno;
    close(s_data);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  udpipv6_stream_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  udpipv6_stream_response->cpu_rate    = 0.0; /* assume no cpu */
  udpipv6_stream_response->measure_cpu = 0;
  if (udpipv6_stream_request->measure_cpu) {
    /* We will pass the rate into the calibration routine. If the */
    /* user did not specify one, it will be 0.0, and we will do a */
    /* "real" calibration. Otherwise, all it will really do is */
    /* store it away... */
    udpipv6_stream_response->measure_cpu = 1;
    udpipv6_stream_response->cpu_rate = 
      calibrate_local_cpu(udpipv6_stream_request->cpu_rate);
  }
  
  message_size	= udpipv6_stream_request->message_size;
  test_time	= udpipv6_stream_request->test_length;
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  udpipv6_stream_response->send_buf_size = lss_size;
  udpipv6_stream_response->recv_buf_size = lsr_size;
  udpipv6_stream_response->so_rcvavoid = loc_rcvavoid;
  udpipv6_stream_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(udpipv6_stream_request->measure_cpu);
  
  /* The loop will exit when the timer pops, or if we happen to recv a */
  /* message of less than send_size bytes... */
  
  times_up = 0;
  start_timer(test_time + PAD_TIME);
  
  if (debug) {
    fprintf(where,"recv_udpipv6_stream: about to enter inner sanctum.\n");
    fflush(where);
  }
  
  while (!times_up) {
    if ((len = recv(s_data, 
		    recv_ring->buffer_ptr,
		    message_size, 
		    0)) != message_size) {
      if ((len == -1) && (errno != EINTR)) {
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
    fprintf(where,"recv_udpipv6_stream: got %d messages.\n",messages_recvd);
    fflush(where);
  }
  
  
  /* The loop now exits due timer or < send_size bytes received. */
  
  cpu_stop(udpipv6_stream_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended on a timer, subtract the PAD_TIME */
    elapsed_time -= (float)PAD_TIME;
  }
  else {
    alarm(0);
  }
  
  if (debug) {
    fprintf(where,"recv_udpipv6_stream: test ended in %f seconds.\n",elapsed_time);
    fflush(where);
  }
  
  
  /* We will count the "off" message that got us out of the loop */
  bytes_received = (messages_recvd * message_size) + len;
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_udpipv6_stream: got %d bytes\n",
	    bytes_received);
    fflush(where);
  }
  
  netperf_response.content.response_type	= UDPIPV6_STREAM_RESULTS;
  udpipv6_stream_results->bytes_received	= htond(bytes_received);
  udpipv6_stream_results->messages_recvd	= messages_recvd;
  udpipv6_stream_results->elapsed_time	= elapsed_time;
  udpipv6_stream_results->cpu_method        = cpu_method;
  if (udpipv6_stream_request->measure_cpu) {
    udpipv6_stream_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  else {
    udpipv6_stream_results->cpu_util	= -1.0;
  }
  
  if (debug > 1) {
    fprintf(where,
	    "recv_udpipv6_stream: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();

  close(s_data);

}

void
send_udpipv6_rr(remote_host)
char	remote_host[];
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
  int	send_socket;
  int	trans_remaining;
  int	bytes_xferd;
  
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct	hostent	        *hp;
  struct	sockaddr_in6	server, myaddr_in;
  int	        addrlen;
  
  struct	udpipv6_rr_request_struct	*udpipv6_rr_request;
  struct	udpipv6_rr_response_struct	*udpipv6_rr_response;
  struct	udpipv6_rr_results_struct	*udpipv6_rr_result;

#ifdef INTERVALS
  int	interval_count;
  sigset_t signal_set;
#endif /* INTERVALS */
  
  udpipv6_rr_request  =
    (struct udpipv6_rr_request_struct *)netperf_request.content.test_specific_data;
  udpipv6_rr_response =
    (struct udpipv6_rr_response_struct *)netperf_response.content.test_specific_data;
  udpipv6_rr_result	 =
    (struct udpipv6_rr_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  
  /* see if we need to use a different hostname */
  if (test_host[0] != '\0') {
    strcpy(remote_host,test_host);
    if (debug > 1) {
      fprintf(where,"Switching test host to %s\n",remote_host);
      fflush(where);
    }
  }
  if ((hostname2addr(remote_host,&server)) == -1) {
    fprintf(where,
	    "send_udpipv6_rr: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  if ( print_headers ) {
    fprintf(where,"UDPIPV6 REQUEST/RESPONSE TEST");
        fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    bytes_xferd     = 0.0;
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
    send_socket = create_data_socket(AF_INET6, 
				     SOCK_DGRAM);
    
    if (send_socket < 0){
      perror("netperf: send_udpipv6_rr: udp rr data socket");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"send_udpipv6_rr: send_socket obtained...\n");
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
    
    /* Tell the remote end to do a listen. The server alters the socket */
    /* paramters on the other side at this point, hence the reason for */
    /* all the values being passed in the setup message. If the user did */
    /* not specify any of the parameters, they will be passed as 0, which */
    /* will indicate to the remote that no changes beyond the system's */
    /* default should be used. Alignment is the exception, it will */
    /* default to 8, which will be no alignment alterations. */
    
    netperf_request.content.request_type	= DO_UDPIPV6_RR;
    udpipv6_rr_request->recv_buf_size	= rsr_size;
    udpipv6_rr_request->send_buf_size	= rss_size;
    udpipv6_rr_request->recv_alignment      = remote_recv_align;
    udpipv6_rr_request->recv_offset	        = remote_recv_offset;
    udpipv6_rr_request->send_alignment      = remote_send_align;
    udpipv6_rr_request->send_offset	        = remote_send_offset;
    udpipv6_rr_request->request_size	= req_size;
    udpipv6_rr_request->response_size	= rsp_size;
    udpipv6_rr_request->measure_cpu	        = remote_cpu_usage;
    udpipv6_rr_request->cpu_rate	        = remote_cpu_rate;
    udpipv6_rr_request->so_rcvavoid	        = rem_rcvavoid;
    udpipv6_rr_request->so_sndavoid	        = rem_sndavoid;
    if (test_time) {
      udpipv6_rr_request->test_length	= test_time;
    }
    else {
      udpipv6_rr_request->test_length	= test_trans * -1;
    }
    
    if (debug > 1) {
      fprintf(where,"netperf: send_udpipv6_rr: requesting UDP r/r test\n");
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
    /* being sent for the UDP tests.					*/
    
    recv_response();
    
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote listen done.\n");
      rsr_size	       =	udpipv6_rr_response->recv_buf_size;
      rss_size	       =	udpipv6_rr_response->send_buf_size;
      remote_cpu_usage =	udpipv6_rr_response->measure_cpu;
      remote_cpu_rate  = 	udpipv6_rr_response->cpu_rate;
      /* port numbers in proper order */
      server.sin6_port  =	udpipv6_rr_response->data_port_number;
      server.sin6_port  = 	htons(server.sin6_port);
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
      exit(1);
    }
    
    /* Connect up to the remote port on the data socket. This will set */
    /* the default destination address on this socket. With UDP, this */
    /* does make a performance difference as we may not have to do as */
    /* many routing lookups, however, I expect that a client would */
    /* behave this way. raj 1/94 */
    
    if ( connect(send_socket, 
		 (struct sockaddr *)&server,
		 sizeof(server)) < 0 ) {
      perror("netperf: data socket connect failed");
      exit(1);
    }
    
    /* now get the port number assigned by the system  */
    addrlen = sizeof(myaddr_in);
    if (getsockname(send_socket, 
		    (struct sockaddr *)&myaddr_in,
		    &addrlen) == -1){
      perror("send_udpipv6_rr: getsockname");
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
	      "send_udpipv6_rr: unable to get sigmask errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
#endif /* INTERVALS */
    
    /* We use an "OR" to control test execution. When the test is */
    /* controlled by time, the byte count check will always return */
    /* false. When the test is controlled by byte count, the time test */
    /* will always return false. When the test is finished, the whole */
    /* expression will go false and we will stop sending data. I think */
    /* I just arbitrarily decrement trans_remaining for the timed */
    /* test, but will not do that just yet... One other question is */
    /* whether or not the send buffer and the receive buffer should be */
    /* the same buffer. */

    while ((!times_up) || (trans_remaining > 0)) {
      /* send the request */
#ifdef HISTOGRAM
      gettimeofday(&time_one,NULL);
#endif
      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   req_size,
		   0)) != req_size) {
	if (errno == EINTR) {
	  /* We likely hit */
	  /* test-end time. */
	  break;
	}
	perror("send_udpipv6_rr: data send error");
	exit(1);
      }
      send_ring = send_ring->next;
      
      /* receive the response. with UDP we will get it all, or nothing */
      
      if((rsp_bytes_recvd=recv(send_socket,
			       recv_ring->buffer_ptr,
			       rsp_size,
			       0)) != rsp_size) {
	if (errno == EINTR) {
	  /* Again, we have likely hit test-end time */
	  break;
	}
	perror("send_udpipv6_rr: data recv error");
	exit(1);
      }
      recv_ring = recv_ring->next;
      
#ifdef HISTOGRAM
      gettimeofday(&time_two,NULL);
      HIST_add(time_hist,delta_micro(&time_one,&time_two));
      
      /* at this point, we may wish to sleep for some period of */
      /* time, so we see how long that last transaction just took, */
      /* and sleep for the difference of that and the interval. We */
      /* will not sleep if the time would be less than a */
      /* millisecond.  */
#endif
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
		  "send_udpipv6_rr: fault with signal set!\n");
	  fflush(where);
	  exit(1);
	}
	interval_count = interval_burst;
      }
#endif /* INTERVALS */
      
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
    
    /* Get the statistics from the remote end. The remote will have */
    /* calculated service demand and all those interesting things. If */
    /* it wasn't supposed to care, it will return obvious values. */
    
    recv_response();
    if (!netperf_response.content.serv_errno) {
      if (debug)
	fprintf(where,"remote results obtained\n");
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
      exit(1);
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
	local_cpu_utilization	= -1.0;
	local_service_demand	= -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = udpipv6_rr_result->cpu_util;
	
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	
	remote_service_demand  = calc_service_demand((double) nummessages*1024,
						     0.0,
						     remote_cpu_utilization,
						     udpipv6_rr_result->num_cpus);
      }
      else {
	remote_cpu_utilization = -1.0;
	remote_service_demand  = -1.0;
      }
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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
    remote_cpu_method = format_cpu_method(udpipv6_rr_result->cpu_method);
    
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
    
#ifdef HISTOGRAM
    fprintf(where,"\nHistogram of reqeuest/reponse times.\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */
  }
}

 /* this routine implements the receive side (netserver) of a UDPIPV6_RR */
 /* test. */
void
recv_udpipv6_rr()
{
  
  struct ring_elt *recv_ring;
  struct ring_elt *send_ring;

  struct	sockaddr_in6        myaddr_in,
  peeraddr_in;
  int	s_data;
  int 	addrlen;
  int	trans_received;
  int	trans_remaining;
  float	elapsed_time;
  
  struct	udpipv6_rr_request_struct	*udpipv6_rr_request;
  struct	udpipv6_rr_response_struct	*udpipv6_rr_response;
  struct	udpipv6_rr_results_struct	*udpipv6_rr_results;
  
  udpipv6_rr_request  = 
    (struct udpipv6_rr_request_struct *)netperf_request.content.test_specific_data;
  udpipv6_rr_response = 
    (struct udpipv6_rr_response_struct *)netperf_response.content.test_specific_data;
  udpipv6_rr_results  = 
    (struct udpipv6_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_udpipv6_rr: entered...\n");
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
    fprintf(where,"recv_udpipv6_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = UDPIPV6_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_udpipv6_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,"recv_udpipv6_rr: requested recv alignment of %d offset %d\n",
	    udpipv6_rr_request->recv_alignment,
	    udpipv6_rr_request->recv_offset);
    fprintf(where,"recv_udpipv6_rr: requested send alignment of %d offset %d\n",
	    udpipv6_rr_request->send_alignment,
	    udpipv6_rr_request->send_offset);
    fflush(where);
  }

  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  recv_ring = allocate_buffer_ring(recv_width,
				   udpipv6_rr_request->request_size,
				   udpipv6_rr_request->recv_alignment,
				   udpipv6_rr_request->recv_offset);

  send_ring = allocate_buffer_ring(send_width,
				   udpipv6_rr_request->response_size,
				   udpipv6_rr_request->send_alignment,
				   udpipv6_rr_request->send_offset);

  if (debug) {
    fprintf(where,"recv_udpipv6_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
  myaddr_in.sin6_port        = 0;
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_udpipv6_rr: grabbing a socket...\n");
    fflush(where);
  }
  

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size = udpipv6_rr_request->send_buf_size;
  lsr_size = udpipv6_rr_request->recv_buf_size;
  loc_rcvavoid = udpipv6_rr_request->so_rcvavoid;
  loc_sndavoid = udpipv6_rr_request->so_sndavoid;

  s_data = create_data_socket(AF_INET6,
			      SOCK_DGRAM);
  
  if (s_data < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    
    exit(1);
  }
  
  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  if (bind(s_data,
	   (struct sockaddr *)&myaddr_in,
	   sizeof(myaddr_in)) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_data);
    send_response();
    
    exit(1);
  }
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_data, 
		  (struct sockaddr *)&myaddr_in,
		  &addrlen) == -1){
    netperf_response.content.serv_errno = errno;
    close(s_data);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  udpipv6_rr_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  netperf_response.content.serv_errno   = 0;
  
  fprintf(where,"recv port number %d\n",myaddr_in.sin6_port);
  fflush(where);
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  udpipv6_rr_response->cpu_rate    = 0.0; 	/* assume no cpu */
  udpipv6_rr_response->measure_cpu = 0;
  if (udpipv6_rr_request->measure_cpu) {
    udpipv6_rr_response->measure_cpu = 1;
    udpipv6_rr_response->cpu_rate = calibrate_local_cpu(udpipv6_rr_request->cpu_rate);
  }
   
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  udpipv6_rr_response->send_buf_size = lss_size;
  udpipv6_rr_response->recv_buf_size = lsr_size;
  udpipv6_rr_response->so_rcvavoid   = loc_rcvavoid;
  udpipv6_rr_response->so_sndavoid   = loc_sndavoid;
 
  send_response();
  
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(udpipv6_rr_request->measure_cpu);
  
  if (udpipv6_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(udpipv6_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = udpipv6_rr_request->test_length * -1;
  }
  
  addrlen = sizeof(peeraddr_in);
  bzero((char *)&peeraddr_in, addrlen);
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {
    
    /* receive the request from the other side */
    if (recvfrom(s_data,
		 recv_ring->buffer_ptr,
		 udpipv6_rr_request->request_size,
		 0,
		 (struct sockaddr *)&peeraddr_in,
		 &addrlen) != udpipv6_rr_request->request_size) {
      if (errno == EINTR) {
	/* we must have hit the end of test time. */
	break;
      }
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
    recv_ring = recv_ring->next;

    /* Now, send the response to the remote */
    if (sendto(s_data,
	       send_ring->buffer_ptr,
	       udpipv6_rr_request->response_size,
	       0,
	       (struct sockaddr *)&peeraddr_in,
	       addrlen) != udpipv6_rr_request->response_size) {
      if (errno == EINTR) {
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
	      "recv_udpipv6_rr: Transaction %d complete.\n",
	      trans_received);
      fflush(where);
    }
    
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(udpipv6_rr_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_udpipv6_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  udpipv6_rr_results->bytes_received = (trans_received * 
				    (udpipv6_rr_request->request_size + 
				     udpipv6_rr_request->response_size));
  udpipv6_rr_results->trans_received = trans_received;
  udpipv6_rr_results->elapsed_time	 = elapsed_time;
  udpipv6_rr_results->cpu_method     = cpu_method;
  if (udpipv6_rr_request->measure_cpu) {
    udpipv6_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_udpipv6_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();

  /* we are done with the socket now */
  close(s_data);

}

 /* this routine implements the receive (netserver) side of a TCPIPV6_RR */
 /* test */
void
recv_tcpipv6_rr()
{
  
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;

  struct	sockaddr_in6        myaddr_in,
  peeraddr_in;
  int	s_listen,s_data;
  int 	addrlen;
  char	*temp_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	tcpipv6_rr_request_struct	*tcpipv6_rr_request;
  struct	tcpipv6_rr_response_struct	*tcpipv6_rr_response;
  struct	tcpipv6_rr_results_struct	*tcpipv6_rr_results;
  
  tcpipv6_rr_request = 
    (struct tcpipv6_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_rr_response =
    (struct tcpipv6_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_rr_results =
    (struct tcpipv6_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcpipv6_rr: entered...\n");
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
    fprintf(where,"recv_tcpipv6_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCPIPV6_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* allocate the recv and send rings with the requested alignments */
  /* and offsets. raj 7/94 */
  if (debug) {
    fprintf(where,"recv_tcpipv6_rr: requested recv alignment of %d offset %d\n",
	    tcpipv6_rr_request->recv_alignment,
	    tcpipv6_rr_request->recv_offset);
    fprintf(where,"recv_tcpipv6_rr: requested send alignment of %d offset %d\n",
	    tcpipv6_rr_request->send_alignment,
	    tcpipv6_rr_request->send_offset);
    fflush(where);
  }

  /* at some point, these need to come to us from the remote system */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   tcpipv6_rr_request->response_size,
				   tcpipv6_rr_request->send_alignment,
				   tcpipv6_rr_request->send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   tcpipv6_rr_request->request_size,
				   tcpipv6_rr_request->recv_alignment,
				   tcpipv6_rr_request->recv_offset);

  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
  myaddr_in.sin6_port        = 0;
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size = tcpipv6_rr_request->send_buf_size;
  lsr_size = tcpipv6_rr_request->recv_buf_size;
  loc_nodelay = tcpipv6_rr_request->no_delay;
  loc_rcvavoid = tcpipv6_rr_request->so_rcvavoid;
  loc_sndavoid = tcpipv6_rr_request->so_sndavoid;
  
  s_listen = create_data_socket(AF_INET6,
				SOCK_STREAM);
  
  if (s_listen < 0) {
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
	   sizeof(myaddr_in)) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in, &addrlen) == -1){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcpipv6_rr_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcpipv6_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  tcpipv6_rr_response->measure_cpu = 0;

  if (tcpipv6_rr_request->measure_cpu) {
    tcpipv6_rr_response->measure_cpu = 1;
    tcpipv6_rr_response->cpu_rate = calibrate_local_cpu(tcpipv6_rr_request->cpu_rate);
  }
  
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcpipv6_rr_response->send_buf_size = lss_size;
  tcpipv6_rr_response->recv_buf_size = lsr_size;
  tcpipv6_rr_response->no_delay = loc_nodelay;
  tcpipv6_rr_response->so_rcvavoid = loc_rcvavoid;
  tcpipv6_rr_response->so_sndavoid = loc_sndavoid;
  tcpipv6_rr_response->test_length = tcpipv6_rr_request->test_length;
  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  if ((s_data = accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) ==
      -1) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_rr: accept completes on the data connection.\n");
    fflush(where);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcpipv6_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcpipv6_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcpipv6_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcpipv6_rr_request->test_length * -1;
  }

  trans_received = 0;
  
  while ((!times_up) || (trans_remaining > 0)) {
    temp_message_ptr = recv_ring->buffer_ptr;
    request_bytes_remaining	= tcpipv6_rr_request->request_size;
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) < 0) {
	if (errno == EINTR) {
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
			tcpipv6_rr_request->response_size,
			0)) == -1) {
      if (errno == EINTR) {
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
  
  cpu_stop(tcpipv6_rr_request->measure_cpu,&elapsed_time);
  
  alarm(0);

  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }

  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcpipv6_rr_results->bytes_received = (trans_received * 
				    (tcpipv6_rr_request->request_size + 
				     tcpipv6_rr_request->response_size));
  tcpipv6_rr_results->trans_received = trans_received;
  tcpipv6_rr_results->elapsed_time   = elapsed_time;
  tcpipv6_rr_results->cpu_method     = cpu_method;
  if (tcpipv6_rr_request->measure_cpu) {
    tcpipv6_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  /* we are now done with the sockets */
  close(s_data);
  close(s_listen);

  send_response();
  
}



 /* this test is intended to test the performance of establishing a */
 /* connection, exchanging a reqeuest/response pair, and repeating. it */
 /* is expected that this would be a good starting-point for */
 /* comparision of T/TCP with classic TCP for transactional workloads. */
 /* it will also look (can look) much like the communication pattern */
 /* of http for www access. */

void
send_tcpipv6_conn_rr(remote_host)
     char	remote_host[];
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
  int	send_socket;
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
  struct	sockaddr_in6	server;
  struct        sockaddr_in6     *myaddr;
  int                            myport;

  struct	tcpipv6_conn_rr_request_struct	*tcpipv6_conn_rr_request;
  struct	tcpipv6_conn_rr_response_struct	*tcpipv6_conn_rr_response;
  struct	tcpipv6_conn_rr_results_struct	*tcpipv6_conn_rr_result;
  
  tcpipv6_conn_rr_request = 
    (struct tcpipv6_conn_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_conn_rr_response = 
    (struct tcpipv6_conn_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_conn_rr_result =
    (struct tcpipv6_conn_rr_results_struct *)netperf_response.content.test_specific_data;
  
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  myaddr = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));

  bzero((char *)&server,
	sizeof(server));
  bzero((char *)myaddr,
	sizeof(struct sockaddr_in6));
  myaddr->sin6_family = AF_INET6;
#ifdef SIN6_LEN
  myaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  

  /* see if we need to use a different hostname */
  if (test_host[0] != '\0') {
    strcpy(remote_host,test_host);
    if (debug > 1) {
      fprintf(where,"Switching test host to %s\n",remote_host);
      fflush(where);
    }
  }
  if ((hostname2addr(remote_host,&server)) == -1) {
    fprintf(where,
	    "send_tcpipv6_conn_rr: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  if ( print_headers ) {
    fprintf(where,"TCPIPV6 Connect/Request/Response TEST");
    fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_nodelay || rem_nodelay) {
      fprintf(where," : nodelay");
    }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    fprintf(where,"send_tcpipv6_conn_rr: send_socket obtained...\n");
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
  
  netperf_request.content.request_type	        =	DO_TCP_CRR;
  tcpipv6_conn_rr_request->recv_buf_size	=	rsr_size;
  tcpipv6_conn_rr_request->send_buf_size	=	rss_size;
  tcpipv6_conn_rr_request->recv_alignment	=	remote_recv_align;
  tcpipv6_conn_rr_request->recv_offset	=	remote_recv_offset;
  tcpipv6_conn_rr_request->send_alignment	=	remote_send_align;
  tcpipv6_conn_rr_request->send_offset	=	remote_send_offset;
  tcpipv6_conn_rr_request->request_size	=	req_size;
  tcpipv6_conn_rr_request->response_size	=	rsp_size;
  tcpipv6_conn_rr_request->no_delay	        =	rem_nodelay;
  tcpipv6_conn_rr_request->measure_cpu	=	remote_cpu_usage;
  tcpipv6_conn_rr_request->cpu_rate	        =	remote_cpu_rate;
  tcpipv6_conn_rr_request->so_rcvavoid	=	rem_rcvavoid;
  tcpipv6_conn_rr_request->so_sndavoid	=	rem_sndavoid;
  if (test_time) {
    tcpipv6_conn_rr_request->test_length	=	test_time;
  }
  else {
    tcpipv6_conn_rr_request->test_length	=	test_trans * -1;
  }
  
  if (debug > 1) {
    fprintf(where,"netperf: send_tcpipv6_conn_rr: requesting TCP crr test\n");
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
    rsr_size	=	tcpipv6_conn_rr_response->recv_buf_size;
    rss_size	=	tcpipv6_conn_rr_response->send_buf_size;
    rem_nodelay	=	tcpipv6_conn_rr_response->no_delay;
    remote_cpu_usage=	tcpipv6_conn_rr_response->measure_cpu;
    remote_cpu_rate = 	tcpipv6_conn_rr_response->cpu_rate;
    /* make sure that port numbers are in network order */
    server.sin6_port	=	tcpipv6_conn_rr_response->data_port_number;
    server.sin6_port =	htons(server.sin6_port);
    if (debug) {
      fprintf(where,"remote listen done.\n");
      fprintf(where,"remote port is %d\n",ntohs(server.sin6_port));
      fflush(where);
    }
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
    
    exit(1);
  }
  
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
  myaddr->sin6_port = htons(myport);
  
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

#ifdef HISTOGRAM
    /* timestamp just before our call to create the socket, and then */
    /* again just after the receive raj 3/95 */
    gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */

    /* set up the data socket */
    send_socket = create_data_socket(AF_INET6, 
				     SOCK_STREAM);
  
    if (send_socket < 0) {
      perror("netperf: send_tcpipv6_conn_rr: tcp stream data socket");
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
		  (char *)&one, sock_opt_len) < 0) {
      perror("netperf: send_tcpipv6_conn_rr: so_reuseaddr");
      exit(1);
    }

newport:
    /* pick a new port number */
    myport = ntohs(myaddr->sin6_port);
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

    if (myport == ntohs(server.sin6_port)) myport++;

    myaddr->sin6_port = htons(myport);

    if (debug) {
      if ((nummessages % 100) == 0) {
	printf("port %d\n",myport);
      }
    }

    /* we want to bind our socket to a particular port number. */
    if (bind(send_socket,
	     (struct sockaddr *)myaddr,
	     sizeof(struct sockaddr_in6)) < 0) {
      /* if the bind failed, someone else must have that port number */
      /* - perhaps in the listen state. since we can't use it, skip to */
      /* the next port number. we may have to do this again later, but */
      /* that's just too bad :) */
      if (errno == EINTR) {
	/* we hit the end of a */
	/* timed test. */
	timed_out = 1;
	break;
      }
      if (debug > 1) {
	fprintf(where,
		"send_tcpipv6_conn_rr: tried to bind to port %d errno %d\n",
		ntohs(myaddr->sin6_port),
		errno);
	fflush(where);
      }
	/* yes, goto's are supposed to be evil, but they do have their */
	/* uses from time to time. the real world doesn't always have */
	/* to code to ge tthe A in CS 101 :) raj 3/95 */
	goto newport;
    }

    /* Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		(struct sockaddr *)&server,
		sizeof(server)) <0){
      if (errno == EINTR) {
	/* we hit the end of a */
	/* timed test. */
	timed_out = 1;
	break;
      }
      perror("netperf: data socket connect failed");
      printf("\tattempted to connect on socket %d to port %d",
	     send_socket,
	     ntohs(server.sin6_port));
      printf(" from port %d \n",ntohs(myaddr->sin6_port));
      exit(1);
    }

    /* send the request */
    if((len=send(send_socket,
		 send_ring->buffer_ptr,
		 req_size,
		 0)) != req_size) {
      if (errno == EINTR) {
	/* we hit the end of a */
	/* timed test. */
	timed_out = 1;
	break;
      }
      perror("send_tcpipv6_conn_rr: data send error");
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
			       0)) < 0) {
	if (errno == EINTR) {
	  /* We hit the end of a timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_tcpipv6_conn_rr: data recv error");
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

#ifdef HISTOGRAM
    gettimeofday(&time_two,NULL);
    HIST_add(time_hist,delta_micro(&time_one,&time_two));
#endif /* HISTOGRAM */

    nummessages++;          
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug > 3) {
      fprintf(where,
	      "Transaction %d completed on local port %d\n",
	      nummessages,
	      ntohs(myaddr->sin6_port));
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
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
    
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
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
    }
    
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,"DANGER  DANGER  DANGER    DANGER  DANGER  DANGER    DANGER!\n");
	fprintf(where,"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization = tcpipv6_conn_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization,
						  tcpipv6_conn_rr_result->num_cpus);
    }
    else {
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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

#ifdef HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */

  }
  
}


void
recv_tcpipv6_conn_rr()
{
  
  char message[MAXMESSAGESIZE+MAXALIGNMENT+MAXOFFSET];
  struct	sockaddr_in6        myaddr_in,
  peeraddr_in;
  int	s_listen,s_data;
  int 	addrlen;
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
  
  struct	tcpipv6_conn_rr_request_struct	*tcpipv6_conn_rr_request;
  struct	tcpipv6_conn_rr_response_struct	*tcpipv6_conn_rr_response;
  struct	tcpipv6_conn_rr_results_struct	*tcpipv6_conn_rr_results;
  
  tcpipv6_conn_rr_request = 
    (struct tcpipv6_conn_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_conn_rr_response = 
    (struct tcpipv6_conn_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_conn_rr_results = 
    (struct tcpipv6_conn_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcpipv6_conn_rr: entered...\n");
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
    fprintf(where,"recv_tcpipv6_conn_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCP_CRR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_conn_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_conn_rr: requested recv alignment of %d offset %d\n",
	    tcpipv6_conn_rr_request->recv_alignment,
	    tcpipv6_conn_rr_request->recv_offset);
    fprintf(where,
	    "recv_tcpipv6_conn_rr: requested send alignment of %d offset %d\n",
	    tcpipv6_conn_rr_request->send_alignment,
	    tcpipv6_conn_rr_request->send_offset);
    fflush(where);
  }
  recv_message_ptr = (char *)(( (long)message + 
			(long) tcpipv6_conn_rr_request->recv_alignment -1) & 
			~((long) tcpipv6_conn_rr_request->recv_alignment - 1));
  recv_message_ptr = recv_message_ptr + tcpipv6_conn_rr_request->recv_offset;
  
  send_message_ptr = (char *)(( (long)message + 
			(long) tcpipv6_conn_rr_request->send_alignment -1) & 
			~((long) tcpipv6_conn_rr_request->send_alignment - 1));
  send_message_ptr = send_message_ptr + tcpipv6_conn_rr_request->send_offset;
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_conn_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
  myaddr_in.sin6_port        = 0;
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_conn_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size = tcpipv6_conn_rr_request->send_buf_size;
  lsr_size = tcpipv6_conn_rr_request->recv_buf_size;
  loc_nodelay = tcpipv6_conn_rr_request->no_delay;
  loc_rcvavoid = tcpipv6_conn_rr_request->so_rcvavoid;
  loc_sndavoid = tcpipv6_conn_rr_request->so_sndavoid;
  
  s_listen = create_data_socket(AF_INET6,
				SOCK_STREAM);
  
  if (s_listen < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    if (debug) {
      fprintf(where,"could not create data socket\n");
      fflush(where);
    }
    exit(1);
  }

  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  if (bind(s_listen,
	   (struct sockaddr *)&myaddr_in,
	   sizeof(myaddr_in)) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    if (debug) {
      fprintf(where,"could not bind\n");
      fflush(where);
    }
    exit(1);
  }

  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == -1) {
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
		  &addrlen) == -1){
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
  
  tcpipv6_conn_rr_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  if (debug) {
    fprintf(where,"telling the remote to call me at %d\n",
	    tcpipv6_conn_rr_response->data_port_number);
    fflush(where);
  }
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcpipv6_conn_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (tcpipv6_conn_rr_request->measure_cpu) {
    tcpipv6_conn_rr_response->measure_cpu = 1;
    tcpipv6_conn_rr_response->cpu_rate = 
      calibrate_local_cpu(tcpipv6_conn_rr_request->cpu_rate);
  }
  

  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcpipv6_conn_rr_response->send_buf_size = lss_size;
  tcpipv6_conn_rr_response->recv_buf_size = lsr_size;
  tcpipv6_conn_rr_response->no_delay = loc_nodelay;
  tcpipv6_conn_rr_response->so_rcvavoid = loc_rcvavoid;
  tcpipv6_conn_rr_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcpipv6_conn_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcpipv6_conn_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcpipv6_conn_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcpipv6_conn_rr_request->test_length * -1;
  }
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {

    /* accept a connection from the remote */
    if ((s_data=accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == -1) {
      if (errno == EINTR) {
	/* the timer popped */
	timed_out = 1;
	break;
      }
      fprintf(where,"recv_tcpipv6_conn_rr: accept: errno = %d\n",errno);
      fflush(where);
      close(s_listen);
      
      exit(1);
    }
  
    if (debug) {
      fprintf(where,"recv_tcpipv6_conn_rr: accepted data connection.\n");
      fflush(where);
    }
  
    temp_message_ptr	= recv_message_ptr;
    request_bytes_remaining	= tcpipv6_conn_rr_request->request_size;
    
    /* receive the request from the other side */
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) < 0) {
	if (errno == EINTR) {
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
			tcpipv6_conn_rr_request->response_size,
			0)) == -1) {
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
	      "recv_tcpipv6_conn_rr: Transaction %d complete\n",
	      trans_received);
      fflush(where);
    }

    /* close the connection */
    close(s_data);

  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(tcpipv6_conn_rr_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_conn_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcpipv6_conn_rr_results->bytes_received	= (trans_received * 
					   (tcpipv6_conn_rr_request->request_size + 
					    tcpipv6_conn_rr_request->response_size));
  tcpipv6_conn_rr_results->trans_received	= trans_received;
  tcpipv6_conn_rr_results->elapsed_time	= elapsed_time;
  if (tcpipv6_conn_rr_request->measure_cpu) {
    tcpipv6_conn_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_conn_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}


#ifdef DO_1644

 /* this test is intended to test the performance of establishing a */
 /* connection, exchanging a reqeuest/response pair, and repeating. it */
 /* is expected that this would be a good starting-point for */
 /* comparision of T/TCP with classic TCP for transactional workloads. */
 /* it will also look (can look) much like the communication pattern */
 /* of http for www access. */

int 
send_tcpipv6_tran_rr(remote_host)
     char	remote_host[];
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
  int	send_socket;
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
  struct	sockaddr_in6	server;
  struct        sockaddr_in6     *myaddr;
  int                            myport;

  struct	tcpipv6_tran_rr_request_struct	*tcpipv6_tran_rr_request;
  struct	tcpipv6_tran_rr_response_struct	*tcpipv6_tran_rr_response;
  struct	tcpipv6_tran_rr_results_struct	*tcpipv6_tran_rr_result;
  
  tcpipv6_tran_rr_request = 
    (struct tcpipv6_tran_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_tran_rr_response = 
    (struct tcpipv6_tran_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_tran_rr_result =
    (struct tcpipv6_tran_rr_results_struct *)netperf_response.content.test_specific_data;
  
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  myaddr = (struct sockaddr_in6 *)malloc(sizeof(struct sockaddr_in6));

  bzero((char *)&server,
	sizeof(server));
  bzero((char *)myaddr,
	sizeof(struct sockaddr_in6));
  myaddr->sin6_family = AF_INET6;
#ifdef SIN6_LEN
  myaddr->sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  

  /* see if we need to use a different hostname */
  if (test_host[0] != '\0') {
    strcpy(remote_host,test_host);
    if (debug > 1) {
      fprintf(where,"Switching test host to %s\n",remote_host);
      fflush(where);
    }
  }
  if ((hp = hostname2addr(remote_host,AF_INET6)) == NULL) {
    fprintf(where,
	    "send_tcpipv6_tran_rr: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  bcopy(hp->h_addr,
	(char *)&server.sin6_addr,
	hp->h_length);
  
  server.sin6_family = hp->h_addrtype;
#ifdef SIN6_LEN
  server.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  
  if ( print_headers ) {
    fprintf(where,"TCPIPV6 Transactional/Request/Response TEST");
    fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_nodelay || rem_nodelay) {
      fprintf(where," : nodelay");
    }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    fprintf(where,"send_tcpipv6_tran_rr: send_socket obtained...\n");
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
  
  netperf_request.content.request_type	        =	DO_TCPIPV6_TRR;
  tcpipv6_tran_rr_request->recv_buf_size	=	rsr_size;
  tcpipv6_tran_rr_request->send_buf_size	=	rss_size;
  tcpipv6_tran_rr_request->recv_alignment	=	remote_recv_align;
  tcpipv6_tran_rr_request->recv_offset	=	remote_recv_offset;
  tcpipv6_tran_rr_request->send_alignment	=	remote_send_align;
  tcpipv6_tran_rr_request->send_offset	=	remote_send_offset;
  tcpipv6_tran_rr_request->request_size	=	req_size;
  tcpipv6_tran_rr_request->response_size	=	rsp_size;
  tcpipv6_tran_rr_request->no_delay	        =	rem_nodelay;
  tcpipv6_tran_rr_request->measure_cpu	=	remote_cpu_usage;
  tcpipv6_tran_rr_request->cpu_rate	        =	remote_cpu_rate;
  tcpipv6_tran_rr_request->so_rcvavoid	=	rem_rcvavoid;
  tcpipv6_tran_rr_request->so_sndavoid	=	rem_sndavoid;
  if (test_time) {
    tcpipv6_tran_rr_request->test_length	=	test_time;
  }
  else {
    tcpipv6_tran_rr_request->test_length	=	test_trans * -1;
  }
  
  if (debug > 1) {
    fprintf(where,"netperf: send_tcpipv6_tran_rr: requesting TCPIPV6_TRR test\n");
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
    rsr_size	=	tcpipv6_tran_rr_response->recv_buf_size;
    rss_size	=	tcpipv6_tran_rr_response->send_buf_size;
    rem_nodelay	=	tcpipv6_tran_rr_response->no_delay;
    remote_cpu_usage=	tcpipv6_tran_rr_response->measure_cpu;
    remote_cpu_rate = 	tcpipv6_tran_rr_response->cpu_rate;
    /* make sure that port numbers are in network order */
    server.sin6_port	=	tcpipv6_tran_rr_response->data_port_number;
    server.sin6_port =	htons(server.sin6_port);
    if (debug) {
      fprintf(where,"remote listen done.\n");
      fprintf(where,"remote port is %d\n",ntohs(server.sin6_port));
      fflush(where);
    }
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
    
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
  myaddr->sin6_port = htons(myport);
  
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

#ifdef HISTOGRAM
    /* timestamp just before our call to create the socket, and then */
    /* again just after the receive raj 3/95 */
    gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */

    /* set up the data socket - is this really necessary or can I just */
    /* re-use the same socket and move this cal out of the while loop. */
    /* it does introcudea *boatload* of system calls. I guess that it */
    /* all depends on "reality of programming." keeping it this way is */
    /* a bit more conservative I imagine - raj 3/95 */
    send_socket = create_data_socket(AF_INET6, 
				     SOCK_STREAM);
  
    if (send_socket < 0) {
      perror("netperf: send_tcpipv6_tran_rr: tcp stream data socket");
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
		  (char *)&one, sock_opt_len) < 0) {
      perror("netperf: send_tcpipv6_tran_rr: so_reuseaddr");
      exit(1);
    }

newport:
    /* pick a new port number */
    myport = ntohs(myaddr->sin6_port);
    myport++;

    /* we do not want to use the port number that the server is */
    /* sitting at - this would cause us to fail in a loopback test. we */
    /* could just rely on the failure of the bind to get us past this, */
    /* but I'm guessing that in this one case at least, it is much */
    /* faster, given that we *know* that port number is already in use */
    /* (or rather would be in a loopback test) */

    if (myport == ntohs(server.sin6_port)) myport++;

    /* wrap the port number when we get to 65535. NOTE, some broken */
    /* TCP's might treat the port number as a signed 16 bit quantity. */
    /* we aren't interested in testing such broken implementations :) */
    /* raj 8/94  */
    if (myport >= client_port_max) {
      myport = client_port_min;
    }
    myaddr->sin6_port = htons(myport);

    if (debug) {
      if ((nummessages % 100) == 0) {
	printf("port %d\n",myport);
      }
    }

    /* we want to bind our socket to a particular port number. */
    if (bind(send_socket,
	     (struct sockaddr *)myaddr,
	     sizeof(struct sockaddr_in6)) < 0) {
      /* if the bind failed, someone else must have that port number */
      /* - perhaps in the listen state. since we can't use it, skip to */
      /* the next port number. we may have to do this again later, but */
      /* that's just too bad :) */
      if (debug > 1) {
	fprintf(where,
		"send_tcpipv6_tran_rr: tried to bind to port %d errno %d\n",
		ntohs(myaddr->sin6_port),
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
      if (errno == EINTR) {
	/* we hit the end of a */
	/* timed test. */
	timed_out = 1;
	break;
      }
      perror("send_tcpipv6_tran_rr: data send error");
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
			       0)) < 0) {
	if (errno == EINTR) {
	  /* We hit the end of a timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_tcpipv6_tran_rr: data recv error");
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

#ifdef HISTOGRAM
    gettimeofday(&time_two,NULL);
    HIST_add(time_hist,delta_micro(&time_one,&time_two));
#endif /* HISTOGRAM */

    nummessages++;          
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug > 3) {
      fprintf(where,
	      "Transaction %d completed on local port %d\n",
	      nummessages,
	      ntohs(myaddr->sin6_port));
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
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
    
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
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
    }
    
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,"DANGER  DANGER  DANGER    DANGER  DANGER  DANGER    DANGER!\n");
	fprintf(where,"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization = tcpipv6_tran_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization,
						  tcpipv6_tran_rr_result->num_cpus);
    }
    else {
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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

#ifdef HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */

  }
  
}


int 
recv_tcpipv6_tran_rr()
{
  
  char message[MAXMESSAGESIZE+MAXALIGNMENT+MAXOFFSET];
  struct	sockaddr_in6        myaddr_in,
  peeraddr_in;
  int	s_listen,s_data;
  int 	addrlen;
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
  
  struct	tcpipv6_tran_rr_request_struct	*tcpipv6_tran_rr_request;
  struct	tcpipv6_tran_rr_response_struct	*tcpipv6_tran_rr_response;
  struct	tcpipv6_tran_rr_results_struct	*tcpipv6_tran_rr_results;
  
  tcpipv6_tran_rr_request = 
    (struct tcpipv6_tran_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_tran_rr_response = 
    (struct tcpipv6_tran_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_tran_rr_results = 
    (struct tcpipv6_tran_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_tcpipv6_tran_rr: entered...\n");
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
    fprintf(where,"recv_tcpipv6_tran_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = TCPIPV6_TRR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_tran_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_tran_rr: requested recv alignment of %d offset %d\n",
	    tcpipv6_tran_rr_request->recv_alignment,
	    tcpipv6_tran_rr_request->recv_offset);
    fprintf(where,
	    "recv_tcpipv6_tran_rr: requested send alignment of %d offset %d\n",
	    tcpipv6_tran_rr_request->send_alignment,
	    tcpipv6_tran_rr_request->send_offset);
    fflush(where);
  }
  recv_message_ptr = (char *)(( (long)message + 
			(long) tcpipv6_tran_rr_request->recv_alignment -1) & 
			~((long) tcpipv6_tran_rr_request->recv_alignment - 1));
  recv_message_ptr = recv_message_ptr + tcpipv6_tran_rr_request->recv_offset;
  
  send_message_ptr = (char *)(( (long)message + 
			(long) tcpipv6_tran_rr_request->send_alignment -1) & 
			~((long) tcpipv6_tran_rr_request->send_alignment - 1));
  send_message_ptr = send_message_ptr + tcpipv6_tran_rr_request->send_offset;
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_tran_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
  myaddr_in.sin6_port        = 0;
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcpipv6_tran_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size = tcpipv6_tran_rr_request->send_buf_size;
  lsr_size = tcpipv6_tran_rr_request->recv_buf_size;
  loc_nodelay = tcpipv6_tran_rr_request->no_delay;
  loc_rcvavoid = tcpipv6_tran_rr_request->so_rcvavoid;
  loc_sndavoid = tcpipv6_tran_rr_request->so_sndavoid;
  
  s_listen = create_data_socket(AF_INET6,
				SOCK_STREAM);
  
  if (s_listen < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    if (debug) {
      fprintf(where,"could not create data socket\n");
      fflush(where);
    }
    exit(1);
  }

  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  if (bind(s_listen,
	   (struct sockaddr *)&myaddr_in,
	   sizeof(myaddr_in)) == -1) {
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
		 &NoPush,
		 sizeof(int))) {
    fprintf(where,
	    "recv_tcpipv6_tran_rr: could not set TCP_NOPUSH errno %d\n",
	    errno);
    fflush(where);
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
  }

  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == -1) {
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
		  &addrlen) == -1){
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
  
  tcpipv6_tran_rr_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  if (debug) {
    fprintf(where,"telling the remote to call me at %d\n",
	    tcpipv6_tran_rr_response->data_port_number);
    fflush(where);
  }
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcpipv6_tran_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (tcpipv6_tran_rr_request->measure_cpu) {
    tcpipv6_tran_rr_response->measure_cpu = 1;
    tcpipv6_tran_rr_response->cpu_rate = 
      calibrate_local_cpu(tcpipv6_tran_rr_request->cpu_rate);
  }
  

  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcpipv6_tran_rr_response->send_buf_size = lss_size;
  tcpipv6_tran_rr_response->recv_buf_size = lsr_size;
  tcpipv6_tran_rr_response->no_delay = loc_nodelay;
  tcpipv6_tran_rr_response->so_rcvavoid = loc_rcvavoid;
  tcpipv6_tran_rr_response->so_sndavoid = loc_sndavoid;

  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcpipv6_tran_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcpipv6_tran_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcpipv6_tran_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcpipv6_tran_rr_request->test_length * -1;
  }
  
  trans_received = 0;

  while ((!times_up) || (trans_remaining > 0)) {

    /* accept a connection from the remote */
    if ((s_data=accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) == -1) {
      if (errno == EINTR) {
	/* the timer popped */
	timed_out = 1;
	break;
      }
      fprintf(where,"recv_tcpipv6_tran_rr: accept: errno = %d\n",errno);
      fflush(where);
      close(s_listen);
      
      exit(1);
    }
  
    if (debug) {
      fprintf(where,"recv_tcpipv6_tran_rr: accepted data connection.\n");
      fflush(where);
    }
  
    temp_message_ptr	= recv_message_ptr;
    request_bytes_remaining	= tcpipv6_tran_rr_request->request_size;
    
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
				   0)) < 0) {
	if (errno == EINTR) {
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
			  tcpipv6_tran_rr_request->response_size,
			  MSG_EOF,
			  &peeraddr_in,
			  sizeof(struct sockaddr_in6))) == -1) {
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
	      "recv_tcpipv6_tran_rr: Transaction %d complete\n",
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
  
  cpu_stop(tcpipv6_tran_rr_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_tran_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  tcpipv6_tran_rr_results->bytes_received	= (trans_received * 
					   (tcpipv6_tran_rr_request->request_size + 
					    tcpipv6_tran_rr_request->response_size));
  tcpipv6_tran_rr_results->trans_received	= trans_received;
  tcpipv6_tran_rr_results->elapsed_time	= elapsed_time;
  if (tcpipv6_tran_rr_request->measure_cpu) {
    tcpipv6_tran_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_tcpipv6_tran_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}
#endif /* DO_1644 */

#ifdef DO_NBRR
 /* this routine implements the sending (netperf) side of the TCPIPV6_RR */
 /* test using POSIX-style non-blocking sockets. */

int 
send_tcp_nbrr(remote_host)
     char	remote_host[];
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
%5d  %5d   %5d  %5d";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  
  int	len;
  char	*temp_message_ptr;
  int	nummessages;
  int	send_socket;
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
  struct	sockaddr_in6	server;
  
  struct	tcpipv6_rr_request_struct	*tcpipv6_rr_request;
  struct	tcpipv6_rr_response_struct	*tcpipv6_rr_response;
  struct	tcpipv6_rr_results_struct	*tcpipv6_rr_result;
  
#ifdef INTERVALS
  int	interval_count;
  sigset_t signal_set;
#endif /* INTERVALS */

  tcpipv6_rr_request = 
    (struct tcpipv6_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_rr_response=
    (struct tcpipv6_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_rr_result	=
    (struct tcpipv6_rr_results_struct *)netperf_response.content.test_specific_data;
  
#ifdef HISTOGRAM
  time_hist = HIST_new();
#endif /* HISTOGRAM */

  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */

  bzero((char *)&server,
	sizeof(server));
  
  /* see if we need to use a different hostname */
  if (test_host[0] != '\0') {
    strcpy(remote_host,test_host);
    if (debug > 1) {
      fprintf(where,"Switching test host to %s\n",remote_host);
      fflush(where);
    }
  }
  if ((hp = hostname2addr(remote_host,AF_INET6)) == NULL) {
    fprintf(where,
	    "send_tcp_nbrr: could not resolve the name %s\n",
	    remote_host);
    fflush(where);
  }
  
  bcopy(hp->h_addr,
	(char *)&server.sin6_addr,
	hp->h_length);
  
  server.sin6_family = hp->h_addrtype;
#ifdef SIN6_LEN
  server.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  
  if ( print_headers ) {
    fprintf(where,"TCP Non-Blocking REQUEST/RESPONSE TEST");
    fprintf(where," to %s", remote_host);
    if (iteration_max > 1) {
      fprintf(where,
	      " : +/-%3.1f%% @ %2d%% conf.",
	      interval/0.02,
	      confidence_level);
      }
    if (loc_nodelay || rem_nodelay) {
      fprintf(where," : nodelay");
    }
    if (loc_sndavoid || 
	loc_rcvavoid ||
	rem_sndavoid ||
	rem_rcvavoid) {
      fprintf(where," : copy avoidance");
    }
#ifdef HISTOGRAM
    fprintf(where," : histogram");
#endif /* HISTOGRAM */
#ifdef INTERVALS
    fprintf(where," : interval");
#endif /* INTERVALS */
#ifdef DIRTY 
    fprintf(where," : dirty data");
#endif /* DIRTY */
    fprintf(where,"\n");
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
    send_socket = create_data_socket(AF_INET6, 
				     SOCK_STREAM);
  
    if (send_socket < 0){
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
    
    netperf_request.request_type	=	DO_TCP_NBRR;
    tcpipv6_rr_request->recv_buf_size	=	rsr_size;
    tcpipv6_rr_request->send_buf_size	=	rss_size;
    tcpipv6_rr_request->recv_alignment      =	remote_recv_align;
    tcpipv6_rr_request->recv_offset	        =	remote_recv_offset;
    tcpipv6_rr_request->send_alignment      =	remote_send_align;
    tcpipv6_rr_request->send_offset	        =	remote_send_offset;
    tcpipv6_rr_request->request_size	=	req_size;
    tcpipv6_rr_request->response_size	=	rsp_size;
    tcpipv6_rr_request->no_delay	        =	rem_nodelay;
    tcpipv6_rr_request->measure_cpu	        =	remote_cpu_usage;
    tcpipv6_rr_request->cpu_rate	        =	remote_cpu_rate;
    tcpipv6_rr_request->so_rcvavoid	        =	rem_rcvavoid;
    tcpipv6_rr_request->so_sndavoid	        =	rem_sndavoid;
    if (test_time) {
      tcpipv6_rr_request->test_length	=	test_time;
    }
    else {
      tcpipv6_rr_request->test_length	=	test_trans * -1;
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
      rsr_size          = tcpipv6_rr_response->recv_buf_size;
      rss_size          = tcpipv6_rr_response->send_buf_size;
      rem_nodelay       = tcpipv6_rr_response->no_delay;
      remote_cpu_usage  = tcpipv6_rr_response->measure_cpu;
      remote_cpu_rate   = tcpipv6_rr_response->cpu_rate;
      /* make sure that port numbers are in network order */
      server.sin6_port   = tcpipv6_rr_response->data_port_number;
      server.sin6_port   = htons(server.sin6_port);
    }
    else {
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
      exit(1);
    }
    
    /*Connect up to the remote port on the data socket  */
    if (connect(send_socket, 
		(struct sockaddr *)&server,
		sizeof(server)) <0){
      perror("netperf: data socket connect failed");
      
      exit(1);
    }
    
    /* now that we are connected, mark the socket as non-blocking */
    if (fcntl(send_socket, F_SETFL, O_NONBLOCK) == -1) {
      perror("netperf: fcntl");
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
	      "send_tcp_nbrr: unable to get sigmask errno %d\n",
	      errno);
      fflush(where);
      exit(1);
    }
#endif /* INTERVALS */
    
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
      
#ifdef HISTOGRAM
      /* timestamp just before our call to send, and then again just */
      /* after the receive raj 8/94 */
      gettimeofday(&time_one,NULL);
#endif /* HISTOGRAM */

      /* even though this is a non-blocking socket, we will assume for */
      /* the time being that we will be able to send an entire request */
      /* without getting an EAGAIN */
      if((len=send(send_socket,
		   send_ring->buffer_ptr,
		   req_size,
		   0)) != req_size) {
	if ((errno == EINTR) || (errno == 0)) {
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
				 0)) < 0) {
	  if (errno == EINTR) {
	    /* We hit the end of a timed test. */
	    timed_out = 1;
	    break;
	  }
	  else if (errno == EAGAIN) {
	    errno = 0;
	    continue;
	  }
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
		  "send_udpipv6_rr: fault with signal set!\n");
	  fflush(where);
	  exit(1);
	}
	interval_count = interval_burst;
      }
#endif /* INTERVALS */
      
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
      errno = netperf_response.content.serv_errno;
      perror("netperf: remote error");
      
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
	local_cpu_utilization	= -1.0;
	local_service_demand	= -1.0;
      }
      
      if (remote_cpu_usage) {
	remote_cpu_utilization = tcpipv6_rr_result->cpu_util;
	/* since calc_service demand is doing ms/Kunit we will */
	/* multiply the number of transaction by 1024 to get */
	/* "good" numbers */
	remote_service_demand = calc_service_demand((double) nummessages*1024,
						    0.0,
						    remote_cpu_utilization,
						    tcpipv6_rr_result->num_cpus);
      }
      else {
	remote_cpu_utilization = -1.0;
	remote_service_demand  = -1.0;
      }
      
    }
    else {
      /* we were not measuring cpu, for the confidence stuff, we */
      /* should make it -1.0 */
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
      remote_cpu_utilization = -1.0;
      remote_service_demand  = -1.0;
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
    remote_cpu_method = format_cpu_method(tcpipv6_rr_result->cpu_method);
    
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

#ifdef HISTOGRAM
    fprintf(where,"\nHistogram of request/response times\n");
    fflush(where);
    HIST_report(time_hist);
#endif /* HISTOGRAM */

  }
  
}

 /* this routine implements the receive (netserver) side of a TCPIPV6_RR */
 /* test */
int 
recv_tcp_nbrr()
{
  
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;

  struct	sockaddr_in6        myaddr_in,
  peeraddr_in;
  int	s_listen,s_data;
  int 	addrlen;
  char	*temp_message_ptr;
  int	trans_received;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	tcpipv6_rr_request_struct	*tcpipv6_rr_request;
  struct	tcpipv6_rr_response_struct	*tcpipv6_rr_response;
  struct	tcpipv6_rr_results_struct	*tcpipv6_rr_results;
  
  tcpipv6_rr_request = 
    (struct tcpipv6_rr_request_struct *)netperf_request.content.test_specific_data;
  tcpipv6_rr_response =
    (struct tcpipv6_rr_response_struct *)netperf_response.content.test_specific_data;
  tcpipv6_rr_results =
    (struct tcpipv6_rr_results_struct *)netperf_response.content.test_specific_data;
  
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
  
  netperf_response.content.response_type = TCPIPV6_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: the response type is set...\n");
    fflush(where);
  }
  
  /* allocate the recv and send rings with the requested alignments */
  /* and offsets. raj 7/94 */
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: requested recv alignment of %d offset %d\n",
	    tcpipv6_rr_request->recv_alignment,
	    tcpipv6_rr_request->recv_offset);
    fprintf(where,"recv_tcp_nbrr: requested send alignment of %d offset %d\n",
	    tcpipv6_rr_request->send_alignment,
	    tcpipv6_rr_request->send_offset);
    fflush(where);
  }

  /* at some point, these need to come to us from the remote system */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   tcpipv6_rr_request->response_size,
				   tcpipv6_rr_request->send_alignment,
				   tcpipv6_rr_request->send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   tcpipv6_rr_request->request_size,
				   tcpipv6_rr_request->recv_alignment,
				   tcpipv6_rr_request->recv_offset);

  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_in,
	sizeof(myaddr_in));
  myaddr_in.sin6_family      = AF_INET6;
/*  myaddr_in.sin6_addr.s_addr = INADDR_ANY; */
  myaddr_in.sin6_port        = 0;
#ifdef SIN6_LEN
  myaddr_in.sin6_len = sizeof(struct sockaddr_in6);
#endif /* SIN6_LEN */  
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_data_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size = tcpipv6_rr_request->send_buf_size;
  lsr_size = tcpipv6_rr_request->recv_buf_size;
  loc_nodelay = tcpipv6_rr_request->no_delay;
  loc_rcvavoid = tcpipv6_rr_request->so_rcvavoid;
  loc_sndavoid = tcpipv6_rr_request->so_sndavoid;
  
  s_listen = create_data_socket(AF_INET6,
				SOCK_STREAM);
  
  if (s_listen < 0) {
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
	   sizeof(myaddr_in)) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now, let's set-up the socket to listen for connections */
  if (listen(s_listen, 5) == -1) {
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  
  /* now get the port number assigned by the system  */
  addrlen = sizeof(myaddr_in);
  if (getsockname(s_listen,
		  (struct sockaddr *)&myaddr_in, &addrlen) == -1){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_in contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  tcpipv6_rr_response->data_port_number = (int) ntohs(myaddr_in.sin6_port);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  tcpipv6_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  tcpipv6_rr_response->measure_cpu = 0;

  if (tcpipv6_rr_request->measure_cpu) {
    tcpipv6_rr_response->measure_cpu = 1;
    tcpipv6_rr_response->cpu_rate = calibrate_local_cpu(tcpipv6_rr_request->cpu_rate);
  }
  
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  tcpipv6_rr_response->send_buf_size = lss_size;
  tcpipv6_rr_response->recv_buf_size = lsr_size;
  tcpipv6_rr_response->no_delay = loc_nodelay;
  tcpipv6_rr_response->so_rcvavoid = loc_rcvavoid;
  tcpipv6_rr_response->so_sndavoid = loc_sndavoid;
  tcpipv6_rr_response->test_length = tcpipv6_rr_request->test_length;
  send_response();
  
  addrlen = sizeof(peeraddr_in);
  
  if ((s_data = accept(s_listen,
		       (struct sockaddr *)&peeraddr_in,
		       &addrlen)) ==  -1) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"recv_tcp_nbrr: accept completes on the data connection.\n");
    fflush(where);
  }
    
  /* now that we are connected, mark the socket as non-blocking */
  if (fcntl(s_data, F_SETFL, O_NONBLOCK) == -1) {
    close(s_data);
    exit(1);
  }

  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(tcpipv6_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (tcpipv6_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(tcpipv6_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = tcpipv6_rr_request->test_length * -1;
  }

  trans_received = 0;
  
  while ((!times_up) || (trans_remaining > 0)) {
    temp_message_ptr = recv_ring->buffer_ptr;
    request_bytes_remaining	= tcpipv6_rr_request->request_size;
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) < 0) {
	if (errno == EINTR) {
	  /* the timer popped */
	  timed_out = 1;
	  break;
	}
	else if (errno == EAGAIN) {
	  errno = 0;
	  if (times_up) {
	    timed_out = 1;
	    break;
	  }
	  continue;
	}
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
			tcpipv6_rr_request->response_size,
			0)) == -1) {
      if (errno == EINTR) {
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
  
  cpu_stop(tcpipv6_rr_request->measure_cpu,&elapsed_time);
  
  alarm(0);

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
  
  tcpipv6_rr_results->bytes_received = (trans_received * 
				    (tcpipv6_rr_request->request_size + 
				     tcpipv6_rr_request->response_size));
  tcpipv6_rr_results->trans_received = trans_received;
  tcpipv6_rr_results->elapsed_time   = elapsed_time;
  tcpipv6_rr_results->cpu_method     = cpu_method;
  if (tcpipv6_rr_request->measure_cpu) {
    tcpipv6_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
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


void
print_ipv6_usage()
{

  printf("%s",ipv6_usage);
  exit(1);

}
void
scan_ipv6_args(argc, argv)
     int	argc;
     char	*argv[];

{
#define IPV6_ARGS "DhH:m:M:p:r:s:S:Vw:W:z"
  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	
    arg1[BUFSIZ],  /* argument holders		*/
    arg2[BUFSIZ];
  
  /* Go through all the command line arguments and break them */
  /* out. For those options that take two parms, specifying only */
  /* the first will set both to that value. Specifying only the */
  /* second will leave the first untouched. To change only the */
  /* first, use the form "first," (see the routine break_args.. */
  
  while ((c= getopt(argc, argv, IPV6_ARGS)) != EOF) {
    switch (c) {
    case '?':	
    case 'h':
      print_ipv6_usage();
      exit(1);
    case 'H':
      /* we need to use a hostname different from that for the control */
      /* connection. */
      strcpy(test_host,optarg);
      break;
    case 'D':
      /* set the TCP nodelay flag */
      loc_nodelay = 1;
      rem_nodelay = 1;
      break;
    case 's':
      /* set local socket sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	lss_size = convert(arg1);
      if (arg2[0])
	lsr_size = convert(arg2);
      break;
    case 'S':
      /* set remote socket sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	rss_size = convert(arg1);
      if (arg2[0])
	rsr_size = convert(arg2);
      break;
    case 'r':
      /* set the request/response sizes */
printf("optarg |%s|\n",optarg);
      break_args(optarg,arg1,arg2);
printf("arg1 |%s|, arg2 |%s|\n",arg1,arg2);
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
    case 'p':
      /* set the min and max port numbers for the TCP_CRR and TCPIPV6_TRR */
      /* tests. */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	client_port_min = atoi(arg1);
      if (arg2[0])	
	client_port_max = atoi(arg2);
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
}
#endif /* DO_IPV6 */
