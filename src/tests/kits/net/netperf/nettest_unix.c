#ifdef lint
#define WANT_UNIX
#define DIRTY
#define WANT_INTERVALS
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WANT_UNIX
char	nettest_unix_id[]="\
@(#)nettest_unix.c (c) Copyright 1994-2007 Hewlett-Packard Co. Version 2.4.3";
     
/****************************************************************/
/*								*/
/*	nettest_bsd.c						*/
/*								*/
/*      the BSD sockets parsing routine...                      */
/*                                                              */
/*      scan_unix_args()                                        */
/*                                                              */
/*	the actual test routines...				*/
/*								*/
/*	send_stream_stream()  perform a stream stream test	*/
/*	recv_stream_stream()					*/
/*	send_stream_rr()      perform a stream request/response	*/
/*	recv_stream_rr()					*/
/*	send_dg_stream()      perform a dg stream test	        */
/*	recv_dg_stream()					*/
/*	send_dg_rr()	      perform a dg request/response	*/
/*	recv_dg_rr()						*/
/*	loc_cpu_rate()	      determine the local cpu maxrate   */
/*	rem_cpu_rate()	      find the remote cpu maxrate	*/
/*								*/
/****************************************************************/
     
 /* at some point, I might want to go-in and see if I really need all */
 /* these includes, but for the moment, we'll let them all just sit */
 /* there. raj 8/94 */
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
//#include <sys/ipc.h>
#include <sys/socket.h>
#include <errno.h>
#include <signal.h>
#include <sys/un.h>
#include <unistd.h>
#else /* WIN32 */
#include <process.h>
#include <winsock2.h>
#include <windows.h>
#endif /* WIN32 */
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifdef NOSTDLIBH
#include <malloc.h>
#else /* NOSTDLIBH */
#include <stdlib.h>
#endif /* NOSTDLIBH */

#include <sys/stat.h>

   
#include "netlib.h"
#include "netsh.h"
#include "nettest_unix.h"



 /* these variables are specific to the UNIX sockets tests. declare */
 /* them static to make them global only to this file. */

#define UNIX_PRFX "netperf."
#define UNIX_LENGTH_MAX 0xFFFF - 28

static char
  path_prefix[32]; 

static int	
  rss_size,		/* remote socket send buffer size	*/
  rsr_size,		/* remote socket recv buffer size	*/
  lss_size_req,		/* requested local socket send buffer size */
  lsr_size_req,		/* requested local socket recv buffer size */
  lss_size,		/* local  socket send buffer size 	*/
  lsr_size,		/* local  socket recv buffer size 	*/
  req_size = 1,		/* request size                   	*/
  rsp_size = 1,		/* response size			*/
  send_size,		/* how big are individual sends		*/
  recv_size;		/* how big are individual receives	*/

 /* different options for the sockets				*/


char unix_usage[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
STREAM/DG UNIX Sockets Test Options:\n\
    -h                Display this text\n\
    -m bytes          Set the send size (STREAM_STREAM, DG_STREAM)\n\
    -M bytes          Set the recv size (STREAM_STREAM, DG_STREAM)\n\
    -p dir            Set the directory where pipes are created\n\
    -r req,res        Set request,response size (STREAM_RR, DG_RR)\n\
    -s send[,recv]    Set local socket send/recv buffer sizes\n\
    -S send[,recv]    Set remote socket send/recv buffer sizes\n\
\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n"; 

 /* this routing initializes all the test specific variables */

static void
init_test_vars()
{
  rss_size  = 0;
  rsr_size  = 0;
  lss_size_req = 0;
  lsr_size_req = 0;
  lss_size  = 0;
  lsr_size  = 0;
  req_size  = 1;
  rsp_size  = 1;
  send_size = 0;
  recv_size = 0;

  strcpy(path_prefix,"/tmp");

}     

 /* This routine will create a data (listen) socket with the apropriate */
 /* options set and return it to the caller. this replaces all the */
 /* duplicate code in each of the test routines and should help make */
 /* things a little easier to understand. since this routine can be */
 /* called by either the netperf or netserver programs, all output */
 /* should be directed towards "where." family is generally AF_UNIX, */
 /* and type will be either SOCK_STREAM or SOCK_DGRAM */
SOCKET
create_unix_socket(int family, int type)
{

  SOCKET temp_socket;
  int sock_opt_len;

  /*set up the data socket                        */
  temp_socket = socket(family, 
		       type,
		       0);
  
  if (temp_socket == INVALID_SOCKET){
    fprintf(where,
	    "netperf: create_unix_socket: socket: %d\n",
	    errno);
    fflush(where);
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"create_unix_socket: socket %d obtained...\n",temp_socket);
    fflush(where);
  }
  
  /* Modify the local socket size. The reason we alter the send buffer */
  /* size here rather than when the connection is made is to take care */
  /* of decreases in buffer size. Decreasing the window size after */
  /* connection establishment is a STREAM no-no. Also, by setting the */
  /* buffer (window) size before the connection is established, we can */
  /* control the STREAM MSS (segment size). The MSS is never more that 1/2 */
  /* the minimum receive buffer size at each half of the connection. */
  /* This is why we are altering the receive buffer size on the sending */
  /* size of a unidirectional transfer. If the user has not requested */
  /* that the socket buffers be altered, we will try to find-out what */
  /* their values are. If we cannot touch the socket buffer in any way, */
  /* we will set the values to -1 to indicate that.  */
  
  set_sock_buffer(temp_socket, SEND_BUFFER, lss_size_req, &lss_size);
  set_sock_buffer(temp_socket, RECV_BUFFER, lsr_size_req, &lsr_size);

  return(temp_socket);

}


/* This routine implements the STREAM unidirectional data transfer test */
/* (a.k.a. stream) for the sockets interface. It receives its */
/* parameters via global variables from the shell and writes its */
/* output to the standard output. */


void 
send_stream_stream(char remote_host[])
{
  
  char *tput_title = "\
Recv   Send    Send                          \n\
Socket Socket  Message  Elapsed              \n\
Size   Size    Size     Time     Throughput  \n\
bytes  bytes   bytes    secs.    %s/sec  \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1 =
    "%5d  %5d  %6d    %-6.2f   %7.2f   \n";
  
  char *cpu_title = "\
Recv   Send    Send                          Utilization    Service Demand\n\
Socket Socket  Message  Elapsed              Send   Recv    Send    Recv\n\
Size   Size    Size     Time     Throughput  local  remote  local   remote\n\
bytes  bytes   bytes    secs.    %-8.8s/s  %%      %%       us/KB   us/KB\n\n";
  
  char *cpu_fmt_0 =
    "%6.3f\n";
  
  char *cpu_fmt_1 =
    "%5d  %5d  %6d    %-6.2f     %7.2f   %-6.2f %-6.2f  %-6.3f  %-6.3f\n";
  
  char *ksink_fmt = "\n\
Alignment      Offset         %-8.8s %-8.8s    Sends   %-8.8s Recvs\n\
Local  Remote  Local  Remote  Xfered   Per                 Per\n\
Send   Recv    Send   Recv             Send (avg)          Recv (avg)\n\
%5d   %5d  %5d   %5d %6.4g  %6.2f     %6d %6.2f   %6d\n";
  
  
  float			elapsed_time;
  
#ifdef WANT_INTERVALS
  int interval_count;
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
#include <sys/stat.h>

  struct ring_elt *send_ring;
  
  int	len = 0;
  int	nummessages;
  SOCKET send_socket;
  int	bytes_remaining;
  /* with links like fddi, one can send > 32 bits worth of bytes */
  /* during a test... ;-) */
  double	bytes_sent;
  
#ifdef DIRTY
  int	i;
#endif /* DIRTY */
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct	sockaddr_un	server;
  
  struct	stream_stream_request_struct	*stream_stream_request;
  struct	stream_stream_response_struct	*stream_stream_response;
  struct	stream_stream_results_struct	*stream_stream_result;
  
  stream_stream_request  = 
    (struct stream_stream_request_struct *)netperf_request.content.test_specific_data;
  stream_stream_response =
    (struct stream_stream_response_struct *)netperf_response.content.test_specific_data;
  stream_stream_result   = 
    (struct stream_stream_results_struct *)netperf_response.content.test_specific_data;
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  server.sun_family = AF_UNIX;
  
  
  if ( print_headers ) {
    fprintf(where,"STREAM STREAM TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      fprintf(where,cpu_title,format_units());
    else
      fprintf(where,tput_title,format_units());
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_sent	=	0.0;
  times_up 	= 	0;
  
  /*set up the data socket                        */
  send_socket = create_unix_socket(AF_UNIX, 
				   SOCK_STREAM);
  
  if (send_socket == INVALID_SOCKET){
    perror("netperf: send_stream_stream: stream stream data socket");
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"send_stream_stream: send_socket obtained...\n");
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
  
  send_ring = allocate_buffer_ring(send_width,
				   send_size,
				   local_send_align,
				   local_send_offset);

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
  /* default to 1, which will be no alignment alterations. */
  
  netperf_request.content.request_type		=	DO_STREAM_STREAM;
  stream_stream_request->send_buf_size	=	rss_size;
  stream_stream_request->recv_buf_size	=	rsr_size;
  stream_stream_request->receive_size	=	recv_size;
  stream_stream_request->recv_alignment	=	remote_recv_align;
  stream_stream_request->recv_offset	=	remote_recv_offset;
  stream_stream_request->measure_cpu	=	remote_cpu_usage;
  stream_stream_request->cpu_rate	=	remote_cpu_rate;
  if (test_time) {
    stream_stream_request->test_length	=	test_time;
  }
  else {
    stream_stream_request->test_length	=	test_bytes;
  }
#ifdef DIRTY
  stream_stream_request->dirty_count    =       rem_dirty_count;
  stream_stream_request->clean_count    =       rem_clean_count;
#endif /* DIRTY */
  
  
  if (debug > 1) {
    fprintf(where,
	    "netperf: send_stream_stream: requesting STREAM stream test\n");
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
  /* being sent for the STREAM tests.					*/
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote listen done.\n");
    rsr_size	        =	stream_stream_response->recv_buf_size;
    rss_size	        =	stream_stream_response->send_buf_size;
    remote_cpu_usage    =	stream_stream_response->measure_cpu;
    remote_cpu_rate     = 	stream_stream_response->cpu_rate;
    strcpy(server.sun_path,stream_stream_response->unix_path);
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    perror("netperf: send_stream_stream: remote error");
    exit(1);
  }
  
  /*Connect up to the remote port on the data socket  */
  if (connect(send_socket, 
	      (struct sockaddr *)&server,
	      sizeof(server)) == INVALID_SOCKET){
    perror("netperf: send_stream_stream: data socket connect failed");
    printf(" path: %s\n",server.sun_path);
    exit(1);
  }
  
  /* Data Socket set-up is finished. If there were problems, either the */
  /* connect would have failed, or the previous response would have */
  /* indicated a problem. I failed to see the value of the extra */
  /* message after the accept on the remote. If it failed, we'll see it */
  /* here. If it didn't, we might as well start pumping data. */
  
  /* Set-up the test end conditions. For a stream test, they can be */
  /* either time or byte-count based. */
  
  if (test_time) {
    /* The user wanted to end the test after a period of time. */
    times_up = 0;
    bytes_remaining = 0;
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
  
  /* We use an "OR" to control test execution. When the test is */
  /* controlled by time, the byte count check will always return false. */
  /* When the test is controlled by byte count, the time test will */
  /* always return false. When the test is finished, the whole */
  /* expression will go false and we will stop sending data. */
  
#ifdef DIRTY
  /* initialize the random number generator for putting dirty stuff */
  /* into the send buffer. raj */
  srand((int) getpid());
#endif
  
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
#ifdef WANT_INTERVALS
    for (interval_count = 0;
	 interval_count < interval_wate;
	 interval_count++);
#endif
    
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
  
  if (close(send_socket) == -1) {
    perror("netperf: send_stream_stream: cannot close socket");
    exit(1);
  }
  
  /* this call will always give us the elapsed time for the test, and */
  /* will also store-away the necessaries for cpu utilization */
  
  cpu_stop(local_cpu_usage,&elapsed_time);	/* was cpu being */
						/* measured and how */
						/* long did we really */
						/* run? */  
  
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
    perror("netperf: remote error");
    
    exit(1);
  }
  
  /* We now calculate what our thruput was for the test. In the future, */
  /* we may want to include a calculation of the thruput measured by */
  /* the remote, but it should be the case that for a STREAM stream test, */
  /* that the two numbers should be *very* close... We calculate */
  /* bytes_sent regardless of the way the test length was controlled. */
  /* If it was time, we needed to, and if it was by bytes, the user may */
  /* have specified a number of bytes that wasn't a multiple of the */
  /* send_size, so we really didn't send what he asked for ;-) */
  
  bytes_sent	= ((double) send_size * (double) nummessages) + len;
  thruput	= calc_thruput(bytes_sent);

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
      if (remote_cpu_rate == 0.0) {
	fprintf(where,"DANGER   DANGER  DANGER   DANGER   DANGER  DANGER   DANGER!\n");
	fprintf(where,"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization	= stream_stream_result->cpu_util;
      remote_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      remote_cpu_utilization,
						      stream_stream_result->num_cpus);
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
    /* STREAM statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
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
	    bytes_sent / (double)stream_stream_result->recv_calls,
	    stream_stream_result->recv_calls);
  }
  
}


/* This is the server-side routine for the stream stream test. It is */
/* implemented as one routine. I could break things-out somewhat, but */
/* didn't feel it was necessary. */

void
recv_stream_stream()
{
  
  struct sockaddr_un myaddr_un, peeraddr_un;
  SOCKET s_listen,s_data;
  int 	addrlen;
  int	len;
  int	receive_calls = 0;
  float	elapsed_time;
  int   bytes_received;
  
  struct ring_elt *recv_ring;

#ifdef DIRTY
  char	*message_ptr;
  int   *message_int_ptr;
  int   dirty_count;
  int   clean_count;
  int   i;
#endif
  
  struct	stream_stream_request_struct	*stream_stream_request;
  struct	stream_stream_response_struct	*stream_stream_response;
  struct	stream_stream_results_struct	*stream_stream_results;
  
  stream_stream_request	= 
    (struct stream_stream_request_struct *)netperf_request.content.test_specific_data;
  stream_stream_response	= 
    (struct stream_stream_response_struct *)netperf_response.content.test_specific_data;
  stream_stream_results	= 
    (struct stream_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_stream_stream: entered...\n");
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
    fprintf(where,"recv_stream_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = STREAM_STREAM_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_stream_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug) {
    fprintf(where,"recv_stream_stream: requested alignment of %d\n",
	    stream_stream_request->recv_alignment);
    fflush(where);
  }

  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_un,
	sizeof(myaddr_un));
  myaddr_un.sun_family      = AF_UNIX;
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_stream_stream: grabbing a socket...\n");
    fflush(where);
  }
  
  /* create_unix_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = stream_stream_request->send_buf_size;
  lsr_size_req = stream_stream_request->recv_buf_size;

  s_listen = create_unix_socket(AF_UNIX,
				SOCK_STREAM);
  
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
  
  strcpy(myaddr_un.sun_path,tempnam(path_prefix,"netperf."));
  if (debug) {
    fprintf(where,"selected a path of %s\n",myaddr_un.sun_path);
    fflush(where);
  }
  if (bind(s_listen,
	   (struct sockaddr *)&myaddr_un,
	   sizeof(myaddr_un)) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    fprintf(where,"could not bind to path\n");
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  chmod(myaddr_un.sun_path, 0666);

  /* what sort of sizes did we end-up with? */
  if (stream_stream_request->receive_size == 0) {
    if (lsr_size > 0) {
      recv_size = lsr_size;
    }
    else {
      recv_size = 4096;
    }
  }
  else {
    recv_size = stream_stream_request->receive_size;
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
				   stream_stream_request->recv_alignment,
				   stream_stream_request->recv_offset);

  if (debug) {
    fprintf(where,"recv_stream_stream: receive alignment and offset set...\n");
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
  addrlen = sizeof(myaddr_un);
  if (getsockname(s_listen, 
		  (struct sockaddr *)&myaddr_un,
		  &addrlen) == SOCKET_ERROR){
    netperf_response.content.serv_errno = errno;
    close(s_listen);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_un contains the path */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  strcpy(stream_stream_response->unix_path,myaddr_un.sun_path);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  stream_stream_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (stream_stream_request->measure_cpu) {
    stream_stream_response->measure_cpu = 1;
    stream_stream_response->cpu_rate = 
      calibrate_local_cpu(stream_stream_request->cpu_rate);
  }
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  stream_stream_response->send_buf_size = lss_size;
  stream_stream_response->recv_buf_size = lsr_size;
  stream_stream_response->receive_size = recv_size;

  send_response();
  
  addrlen = sizeof(peeraddr_un);
  
  if ((s_data=accept(s_listen,
		     (struct sockaddr *)&peeraddr_un,
		     &addrlen)) == INVALID_SOCKET) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    exit(1);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(stream_stream_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
#ifdef DIRTY
    /* we want to dirty some number of consecutive integers in the buffer */
    /* we are about to recv. we may also want to bring some number of */
    /* them cleanly into the cache. The clean ones will follow any dirty */
    /* ones into the cache. */

  dirty_count = stream_stream_request->dirty_count;
  clean_count = stream_stream_request->clean_count;
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

  while ((len = recv(s_data, recv_ring->buffer_ptr, recv_size, 0)) != 0) {
    if (len == SOCKET_ERROR) {
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
  
  /* The loop now exits due to zero bytes received. we will have */
  /* counted one too many messages received, so decrement the */
  /* receive_calls counter by one. raj 7/94 */
  receive_calls--;
  
  /* perform a shutdown to signal the sender that */
  /* we have received all the data sent. raj 4/93 */

  if (shutdown(s_data,1) == SOCKET_ERROR) {
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
  
  cpu_stop(stream_stream_request->measure_cpu,&elapsed_time);
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_stream_stream: got %d bytes\n",
	    bytes_received);
    fprintf(where,
	    "recv_stream_stream: got %d recvs\n",
	    receive_calls);
    fflush(where);
  }
  
  stream_stream_results->bytes_received	= bytes_received;
  stream_stream_results->elapsed_time	= elapsed_time;
  stream_stream_results->recv_calls	= receive_calls;
  
  if (stream_stream_request->measure_cpu) {
    stream_stream_results->cpu_util	= calc_cpu_util(0.0);
  };
  
  if (debug > 1) {
    fprintf(where,
	    "recv_stream_stream: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  unlink(myaddr_un.sun_path);
}


 /* this routine implements the sending (netperf) side of the STREAM_RR */
 /* test. */

void
send_stream_rr(char remote_host[])
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
  SOCKET send_socket;
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
  
  struct	sockaddr_un	server;
  
  struct	stream_rr_request_struct	*stream_rr_request;
  struct	stream_rr_response_struct	*stream_rr_response;
  struct	stream_rr_results_struct	*stream_rr_result;
  
  stream_rr_request = 
    (struct stream_rr_request_struct *)netperf_request.content.test_specific_data;
  stream_rr_response=
    (struct stream_rr_response_struct *)netperf_response.content.test_specific_data;
  stream_rr_result	=
    (struct stream_rr_results_struct *)netperf_response.content.test_specific_data;
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  
  server.sun_family = AF_UNIX;
  
  
  if ( print_headers ) {
    fprintf(where,"STREAM REQUEST/RESPONSE TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      fprintf(where,cpu_title,format_units());
    else
      fprintf(where,tput_title,format_units());
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_xferd	=	0.0;
  times_up 	= 	0;
  
  /* set-up the data buffers with the requested alignment and offset. */
  /* since this is a request/response test, default the send_width and */
  /* recv_width to 1 and not two raj 7/94 */

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
				   
  /*set up the data socket                        */
  send_socket = create_unix_socket(AF_UNIX, 
				   SOCK_STREAM);
  
  if (send_socket == INVALID_SOCKET){
    perror("netperf: send_stream_rr: stream stream data socket");
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"send_stream_rr: send_socket obtained...\n");
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
  
  netperf_request.content.request_type	=	DO_STREAM_RR;
  stream_rr_request->recv_buf_size	=	rsr_size;
  stream_rr_request->send_buf_size	=	rss_size;
  stream_rr_request->recv_alignment=	remote_recv_align;
  stream_rr_request->recv_offset	=	remote_recv_offset;
  stream_rr_request->send_alignment=	remote_send_align;
  stream_rr_request->send_offset	=	remote_send_offset;
  stream_rr_request->request_size	=	req_size;
  stream_rr_request->response_size	=	rsp_size;
  stream_rr_request->measure_cpu	=	remote_cpu_usage;
  stream_rr_request->cpu_rate	=	remote_cpu_rate;
  if (test_time) {
    stream_rr_request->test_length	=	test_time;
  }
  else {
    stream_rr_request->test_length	=	test_trans * -1;
  }
  
  if (debug > 1) {
    fprintf(where,"netperf: send_stream_rr: requesting STREAM rr test\n");
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
  /* being sent for the STREAM tests.					*/
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote listen done.\n");
    rsr_size	=	stream_rr_response->recv_buf_size;
    rss_size	=	stream_rr_response->send_buf_size;
    remote_cpu_usage=	stream_rr_response->measure_cpu;
    remote_cpu_rate = 	stream_rr_response->cpu_rate;
    /* make sure that port numbers are in network order */
    strcpy(server.sun_path,stream_rr_response->unix_path);
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    perror("netperf: remote error");
    
    exit(1);
  }
  
  /*Connect up to the remote port on the data socket  */
  if (connect(send_socket, 
	      (struct sockaddr *)&server,
	      sizeof(server)) == INVALID_SOCKET){
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
      perror("send_stream_rr: data send error");
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
	if (errno == EINTR) {
	  /* We hit the end of a timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_stream_rr: data recv error");
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
    
    nummessages++;          
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug > 3) {
      fprintf(where,
	      "Transaction %d completed\n",
	      nummessages);
      fflush(where);
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
    perror("netperf: remote error");
    
    exit(1);
  }
  
  /* We now calculate what our thruput was for the test. In the future, */
  /* we may want to include a calculation of the thruput measured by */
  /* the remote, but it should be the case that for a STREAM stream test, */
  /* that the two numbers should be *very* close... We calculate */
  /* bytes_sent regardless of the way the test length was controlled. */
  /* If it was time, we needed to, and if it was by bytes, the user may */
  /* have specified a number of bytes that wasn't a multiple of the */
  /* send_size, so we really didn't send what he asked for ;-) We use */
  /* Kbytes/s as the units of thruput for a STREAM stream test, where K = */
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
      remote_cpu_utilization = stream_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization,
						  stream_rr_result->num_cpus);
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
    /* STREAM statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
    fprintf(where,
	    ksink_fmt);
  }
  /* The test is over. Kill the data socket */
  
  if (close(send_socket) == -1) {
    perror("send_stream_rr: cannot shutdown stream stream socket");
  }
  
}

void
send_dg_stream(char remote_host[])
{
  /************************************************************************/
  /*									*/
  /*               	DG Unidirectional Send Test                    */
  /*									*/
  /************************************************************************/
  char *tput_title =
    "Socket  Message  Elapsed      Messages                \n\
Size    Size     Time         Okay Errors   Throughput\n\
bytes   bytes    secs            #      #   %s/sec\n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1 =
    "%5d   %5d    %-7.2f   %7d %6d    %7.2f\n\
%5d            %-7.2f   %7d           %7.2f\n\n";
  
  
  char *cpu_title =
    "Socket  Message  Elapsed      Messages                   CPU     Service\n\
Size    Size     Time         Okay Errors   Throughput   Util    Demand\n\
bytes   bytes    secs            #      #   %s/sec   %%       us/KB\n\n";
  
  char *cpu_fmt_0 =
    "%6.2f\n";
  
  char *cpu_fmt_1 =
    "%5d   %5d    %-7.2f   %7d %6d    %7.1f      %-6.2f  %-6.3f\n\
%5d            %-7.2f   %7d           %7.1f      %-6.2f  %-6.3f\n\n";
  
  int	messages_recvd;
  float	elapsed_time, 
  local_cpu_utilization, 
  remote_cpu_utilization;
  
  float	local_service_demand, remote_service_demand;
  double	local_thruput, remote_thruput;
  double	bytes_sent;
  double	bytes_recvd;
  
  
  int	len;
  struct ring_elt *send_ring;
  int	failed_sends;
  int	failed_cows;
  int 	messages_sent;
  SOCKET data_socket;
  
  
#ifdef WANT_INTERVALS
  int	interval_count;
#endif /* WANT_INTERVALS */
#ifdef DIRTY
  int	*message_int_ptr;
  int	i;
#endif /* DIRTY */
  
  struct	sockaddr_un	server;
  
  struct	dg_stream_request_struct	*dg_stream_request;
  struct	dg_stream_response_struct	*dg_stream_response;
  struct	dg_stream_results_struct	*dg_stream_results;
  
  dg_stream_request	= (struct dg_stream_request_struct *)netperf_request.content.test_specific_data;
  dg_stream_response	= (struct dg_stream_response_struct *)netperf_response.content.test_specific_data;
  dg_stream_results	= (struct dg_stream_results_struct *)netperf_response.content.test_specific_data;
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  
  server.sun_family = AF_UNIX;
  
  if ( print_headers ) {
    printf("DG UNIDIRECTIONAL SEND TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      printf(cpu_title,format_units());
    else
      printf(tput_title,format_units());
  }	
  
  failed_sends	= 0;
  failed_cows	= 0;
  messages_sent	= 0;
  times_up	= 0;
  
  /*set up the data socket			*/
  data_socket = create_unix_socket(AF_UNIX,
				   SOCK_DGRAM);
  
  if (data_socket == INVALID_SOCKET){
    perror("dg_send: data socket");
    exit(1);
  }
  
  /* now, we want to see if we need to set the send_size */
  if (send_size == 0) {
    if (lss_size > 0) {
      send_size = (lss_size < UNIX_LENGTH_MAX ? lss_size : UNIX_LENGTH_MAX);
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

  send_ring = allocate_buffer_ring(send_width,
				   send_size,
				   local_send_align,
				   local_send_offset);

  /* At this point, we want to do things like disable DG checksumming */
  /* and measure the cpu rate and all that so we are ready to go */
  /* immediately after the test response message is delivered. */
  
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
  
  netperf_request.content.request_type = DO_DG_STREAM;
  dg_stream_request->recv_buf_size	= rsr_size;
  dg_stream_request->message_size	= send_size;
  dg_stream_request->recv_alignment	= remote_recv_align;
  dg_stream_request->recv_offset	= remote_recv_offset;
  dg_stream_request->measure_cpu	= remote_cpu_usage;
  dg_stream_request->cpu_rate		= remote_cpu_rate;
  dg_stream_request->test_length	= test_time;
  
  send_request();
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"send_dg_stream: remote data connection done.\n");
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    perror("send_dg_stream: error on remote");
    exit(1);
  }
  
  /* Place the port number returned by the remote into the sockaddr */
  /* structure so our sends can be sent to the correct place. Also get */
  /* some of the returned socket buffer information for user display. */
  
  /* make sure that port numbers are in the proper order */
  strcpy(server.sun_path,dg_stream_response->unix_path);
  rsr_size	= dg_stream_response->recv_buf_size;
  rss_size	= dg_stream_response->send_buf_size;
  remote_cpu_rate	= dg_stream_response->cpu_rate;
  
  /* We "connect" up to the remote post to allow is to use the send */
  /* call instead of the sendto call. Presumeably, this is a little */
  /* simpler, and a little more efficient. I think that it also means */
  /* that we can be informed of certain things, but am not sure yet... */
  
  if (connect(data_socket,
	      (struct sockaddr *)&server,
	      sizeof(server)) == INVALID_SOCKET){
    perror("send_dg_stream: data socket connect failed");
    exit(1);
  }
  
  /* set up the timer to call us after test_time	*/
  start_timer(test_time);
  
  /* Get the start count for the idle counter and the start time */
  
  cpu_start(local_cpu_usage);
  
#ifdef WANT_INTERVALS
  interval_count = interval_burst;
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
      perror("dg_send: data send error");
      exit(1);
    }
    messages_sent++;          
    
    /* now we want to move our pointer to the next position in the */
    /* data buffer... */

    send_ring = send_ring->next;
    
    
#ifdef WANT_INTERVALS
    /* in this case, the interval count is the count-down couter */
    /* to decide to sleep for a little bit */
    if ((interval_burst) && (--interval_count == 0)) {
      /* call the sleep routine for some milliseconds, if our */
      /* timer popped while we were in there, we want to */
      /* break out of the loop. */
      if (msec_sleep(interval_wate)) {
	break;
      }
      interval_count = interval_burst;
    }
    
#endif
    
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
      fprintf(where,"send_dg_stream: remote results obtained\n");
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    perror("send_dg_stream: error on remote");
    exit(1);
  }
  
  bytes_sent	= send_size * messages_sent;
  local_thruput	= calc_thruput(bytes_sent);
  
  messages_recvd	= dg_stream_results->messages_recvd;
  bytes_recvd	= send_size * messages_recvd;
  
  /* we asume that the remote ran for as long as we did */
  
  remote_thruput	= calc_thruput(bytes_recvd);
  
  /* print the results for this socket and message size */
  
  if (local_cpu_usage || remote_cpu_usage) {
    /* We must now do a little math for service demand and cpu */
    /* utilization for the system(s) We pass zeros for the local */
    /* cpu utilization and elapsed time to tell the routine to use */
    /* the libraries own values for those. */
    if (local_cpu_usage) {
      if (local_cpu_rate == 0.0) {
	fprintf(where,"WARNING WARNING WARNING  WARNING WARNING WARNING  WARNING!\n");
	fprintf(where,"Local CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      
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
    
    /* The local calculations could use variables being kept by */
    /* the local netlib routines. The remote calcuations need to */
    /* have a few things passed to them. */
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,"DANGER   DANGER  DANGER   DANGER  DANGER   DANGER   DANGER!\n");
	fprintf(where,"REMOTE CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      
      remote_cpu_utilization	= dg_stream_results->cpu_util;
      remote_service_demand	= calc_service_demand(bytes_recvd,
						      0.0,
						      remote_cpu_utilization,
						      dg_stream_results->num_cpus);
    }
    else {
      remote_cpu_utilization	= -1.0;
      remote_service_demand	= -1.0;
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
      fprintf(where,
	      cpu_fmt_1,		/* the format string */
	      lss_size,		/* local sendbuf size */
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
	      remote_thruput
	      );
      break;
    }
  }
}


 /* this routine implements the receive side (netserver) of the */
 /* DG_STREAM performance test. */

void
recv_dg_stream()
{
  struct ring_elt *recv_ring;

  struct sockaddr_un myaddr_un;
  SOCKET s_data;
  int	len = 0;
  int	bytes_received = 0;
  float	elapsed_time;
  
  int	message_size;
  int	messages_recvd = 0;
  
  struct	dg_stream_request_struct	*dg_stream_request;
  struct	dg_stream_response_struct	*dg_stream_response;
  struct	dg_stream_results_struct	*dg_stream_results;
  
  dg_stream_request  = 
    (struct dg_stream_request_struct *)netperf_request.content.test_specific_data;
  dg_stream_response = 
    (struct dg_stream_response_struct *)netperf_response.content.test_specific_data;
  dg_stream_results  = 
    (struct dg_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dg_stream: entered...\n");
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
    fprintf(where,"recv_dg_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = DG_STREAM_RESPONSE;
  
  if (debug > 2) {
    fprintf(where,"recv_dg_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug > 1) {
    fprintf(where,"recv_dg_stream: requested alignment of %d\n",
	    dg_stream_request->recv_alignment);
    fflush(where);
  }

  if (recv_width == 0) recv_width = 1;

  recv_ring = allocate_buffer_ring(recv_width,
				   dg_stream_request->message_size,
				   dg_stream_request->recv_alignment,
				   dg_stream_request->recv_offset);

  if (debug > 1) {
    fprintf(where,"recv_dg_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_un,
	sizeof(myaddr_un));
  myaddr_un.sun_family      = AF_UNIX;
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug > 1) {
    fprintf(where,"recv_dg_stream: grabbing a socket...\n");
    fflush(where);
  }

  /* create_unix_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lsr_size = dg_stream_request->recv_buf_size;
    
  s_data = create_unix_socket(AF_UNIX,
			      SOCK_DGRAM);
  
  if (s_data == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  strcpy(myaddr_un.sun_path,tempnam(path_prefix,"netperf."));
  if (bind(s_data,
	   (struct sockaddr *)&myaddr_un,
	   sizeof(myaddr_un)) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }

  chmod(myaddr_un.sun_path, 0666);

  dg_stream_response->test_length = dg_stream_request->test_length;
  
  /* Now myaddr_un contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  strcpy(dg_stream_response->unix_path,myaddr_un.sun_path);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  dg_stream_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (dg_stream_request->measure_cpu) {
    /* We will pass the rate into the calibration routine. If the */
    /* user did not specify one, it will be 0.0, and we will do a */
    /* "real" calibration. Otherwise, all it will really do is */
    /* store it away... */
    dg_stream_response->measure_cpu = 1;
    dg_stream_response->cpu_rate = 
      calibrate_local_cpu(dg_stream_request->cpu_rate);
  }
  
  message_size	= dg_stream_request->message_size;
  test_time	= dg_stream_request->test_length;
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  dg_stream_response->send_buf_size = lss_size;
  dg_stream_response->recv_buf_size = lsr_size;

  send_response();
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(dg_stream_request->measure_cpu);
  
  /* The loop will exit when the timer pops, or if we happen to recv a */
  /* message of less than send_size bytes... */
  
  times_up = 0;
  start_timer(test_time + PAD_TIME);
  
  if (debug) {
    fprintf(where,"recv_dg_stream: about to enter inner sanctum.\n");
    fflush(where);
  }
  
  while (!times_up) {
    if ((len = recv(s_data, 
		    recv_ring->buffer_ptr,
		    message_size, 
		    0)) != message_size) {
      if ((len == SOCKET_ERROR) && (errno != EINTR)) {
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
    fprintf(where,"recv_dg_stream: got %d messages.\n",messages_recvd);
    fflush(where);
  }
  
  
  /* The loop now exits due timer or < send_size bytes received. */
  
  cpu_stop(dg_stream_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended on a timer, subtract the PAD_TIME */
    elapsed_time -= (float)PAD_TIME;
  }
  else {
    stop_timer();
  }
  
  if (debug) {
    fprintf(where,"recv_dg_stream: test ended in %f seconds.\n",elapsed_time);
    fflush(where);
  }
  
  
  /* We will count the "off" message that got us out of the loop */
  bytes_received = (messages_recvd * message_size) + len;
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_dg_stream: got %d bytes\n",
	    bytes_received);
    fflush(where);
  }
  
  netperf_response.content.response_type		= DG_STREAM_RESULTS;
  dg_stream_results->bytes_received	= bytes_received;
  dg_stream_results->messages_recvd	= messages_recvd;
  dg_stream_results->elapsed_time	= elapsed_time;
  if (dg_stream_request->measure_cpu) {
    dg_stream_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  else {
    dg_stream_results->cpu_util	= -1.0;
  }
  
  if (debug > 1) {
    fprintf(where,
	    "recv_dg_stream: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}

void
send_dg_rr(char remote_host[])
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
  
  float			elapsed_time;
  
  /* we add MAXALIGNMENT and MAXOFFSET to insure that there is enough */
  /* space for a maximally aligned, maximally sized message. At some */
  /* point, we may want to actually make this even larger and cycle */
  /* through the thing one piece at a time.*/
  
  int	len;
  char	*send_message_ptr;
  char	*recv_message_ptr;
  char	*temp_message_ptr;
  int	nummessages;
  SOCKET send_socket;
  int	trans_remaining;
  int	bytes_xferd;
  
  int	rsp_bytes_recvd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
#ifdef WANT_INTERVALS
  /* timing stuff */
#define	MAX_KEPT_TIMES	1024
  int	time_index = 0;
  int	unused_buckets;
  int	kept_times[MAX_KEPT_TIMES];
  int	sleep_usecs;
  unsigned	int	total_times=0;
  struct	timezone	dummy_zone;
  struct	timeval		send_time;
  struct	timeval		recv_time;
  struct	timeval		sleep_timeval;
#endif
  
  struct	sockaddr_un	server, myaddr_un;
  
  struct	dg_rr_request_struct	*dg_rr_request;
  struct	dg_rr_response_struct	*dg_rr_response;
  struct	dg_rr_results_struct	*dg_rr_result;
  
  dg_rr_request	= 
    (struct dg_rr_request_struct *)netperf_request.content.test_specific_data;
  dg_rr_response=
    (struct dg_rr_response_struct *)netperf_response.content.test_specific_data;
  dg_rr_result	=
    (struct dg_rr_results_struct *)netperf_response.content.test_specific_data;
  
  /* we want to zero out the times, so we can detect unused entries. */
#ifdef WANT_INTERVALS
  time_index = 0;
  while (time_index < MAX_KEPT_TIMES) {
    kept_times[time_index] = 0;
    time_index += 1;
  }
  time_index = 0;
#endif
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  bzero((char *)&server,
	sizeof(server));
  server.sun_family = AF_UNIX;

  bzero((char *)&myaddr_un,
	sizeof(myaddr_un));
  myaddr_un.sun_family = AF_UNIX;

  strcpy(myaddr_un.sun_path,tempnam(path_prefix,"netperf."));
  
  if ( print_headers ) {
    fprintf(where,"DG REQUEST/RESPONSE TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      fprintf(where,cpu_title,format_units());
    else
      fprintf(where,tput_title,format_units());
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_xferd	=	0;
  times_up 	= 	0;
  
  /* set-up the data buffer with the requested alignment and offset */
  temp_message_ptr = (char *)malloc(DATABUFFERLEN);
  if (temp_message_ptr == NULL) {
    printf("malloc(%d) failed!\n", DATABUFFERLEN);
    exit(1);
  }
  send_message_ptr = (char *)(( (long)temp_message_ptr + 
			(long) local_send_align - 1) &	
			~((long) local_send_align - 1));
  send_message_ptr = send_message_ptr + local_send_offset;
  temp_message_ptr = (char *)malloc(DATABUFFERLEN);
  if (temp_message_ptr == NULL) {
    printf("malloc(%d) failed!\n", DATABUFFERLEN);
    exit(1);
  }
  recv_message_ptr = (char *)(( (long)temp_message_ptr + 
			(long) local_recv_align - 1) &	
			~((long) local_recv_align - 1));
  recv_message_ptr = recv_message_ptr + local_recv_offset;
  
  /*set up the data socket                        */
  send_socket = create_unix_socket(AF_UNIX, 
				   SOCK_DGRAM);
  
  if (send_socket == INVALID_SOCKET){
    perror("netperf: send_dg_rr: dg rr data socket");
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"send_dg_rr: send_socket obtained...\n");
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
  
  netperf_request.content.request_type	=	DO_DG_RR;
  dg_rr_request->recv_buf_size	=	rsr_size;
  dg_rr_request->send_buf_size	=	rss_size;
  dg_rr_request->recv_alignment	=	remote_recv_align;
  dg_rr_request->recv_offset	=	remote_recv_offset;
  dg_rr_request->send_alignment	=	remote_send_align;
  dg_rr_request->send_offset	=	remote_send_offset;
  dg_rr_request->request_size	=	req_size;
  dg_rr_request->response_size	=	rsp_size;
  dg_rr_request->measure_cpu	=	remote_cpu_usage;
  dg_rr_request->cpu_rate	=	remote_cpu_rate;
  if (test_time) {
    dg_rr_request->test_length	=	test_time;
  }
  else {
    dg_rr_request->test_length	=	test_trans * -1;
  }
  
  if (debug > 1) {
    fprintf(where,"netperf: send_dg_rr: requesting DG request/response test\n");
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
  /* being sent for the DG tests.					*/
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote listen done.\n");
    rsr_size	=	dg_rr_response->recv_buf_size;
    rss_size	=	dg_rr_response->send_buf_size;
    remote_cpu_usage=	dg_rr_response->measure_cpu;
    remote_cpu_rate = 	dg_rr_response->cpu_rate;
    /* port numbers in proper order */
    strcpy(server.sun_path,dg_rr_response->unix_path);
  }
  else {
    Set_errno(netperf_response.content.serv_errno);
    perror("netperf: remote error");
    
    exit(1);
  }
  
  /* Connect up to the remote port on the data socket. This will set */
  /* the default destination address on this socket. we need to bind */
  /* out socket so that the remote gets something from a recvfrom  */
  if (bind(send_socket,
	   (struct sockaddr *)&myaddr_un,
	   sizeof(myaddr_un)) == SOCKET_ERROR) {
    perror("netperf: send_dg_rr");
    unlink(myaddr_un.sun_path);
    close(send_socket);
    exit(1);
  }
  
  if (connect(send_socket, 
	      (struct sockaddr *)&server,
	      sizeof(server)) == INVALID_SOCKET ) {
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
  
  /* We use an "OR" to control test execution. When the test is */
  /* controlled by time, the byte count check will always return false. */
  /* When the test is controlled by byte count, the time test will */
  /* always return false. When the test is finished, the whole */
  /* expression will go false and we will stop sending data. I think I */
  /* just arbitrarily decrement trans_remaining for the timed test, but */
  /* will not do that just yet... One other question is whether or not */
  /* the send buffer and the receive buffer should be the same buffer. */
  while ((!times_up) || (trans_remaining > 0)) {
    /* send the request */
#ifdef WANT_INTERVALS
    gettimeofday(&send_time,&dummy_zone);
#endif
    if((len=send(send_socket,
		 send_message_ptr,
		 req_size,
		 0)) != req_size) {
      if (errno == EINTR) {
	/* We likely hit */
	/* test-end time. */
	break;
      }
      perror("send_dg_rr: data send error");
      exit(1);
    }
    
    /* receive the response. with DG we will get it all, or nothing */
    
    if((rsp_bytes_recvd=recv(send_socket,
			     recv_message_ptr,
			     rsp_size,
			     0)) != rsp_size) {
      if (errno == EINTR) {
	/* Again, we have likely hit test-end time */
	break;
      }
      perror("send_dg_rr: data recv error");
      exit(1);
    }
#ifdef WANT_INTERVALS
    gettimeofday(&recv_time,&dummy_zone);
    
    /* now we do some arithmatic on the two timevals */
    if (recv_time.tv_usec < send_time.tv_usec) {
      /* we wrapped around a second */
      recv_time.tv_usec += 1000000;
      recv_time.tv_sec  -= 1;
    }
    
    /* and store it away */
    kept_times[time_index] = (recv_time.tv_sec - send_time.tv_sec) * 1000000;
    kept_times[time_index] += (recv_time.tv_usec - send_time.tv_usec);
    
    /* at this point, we may wish to sleep for some period of */
    /* time, so we see how long that last transaction just took, */
    /* and sleep for the difference of that and the interval. We */
    /* will not sleep if the time would be less than a */
    /* millisecond.  */
    if (interval_usecs > 0) {
      sleep_usecs = interval_usecs - kept_times[time_index];
      if (sleep_usecs > 1000) {
	/* we sleep */
	sleep_timeval.tv_sec = sleep_usecs / 1000000;
	sleep_timeval.tv_usec = sleep_usecs % 1000000;
	select(0,
	       0,
	       0,
	       0,
	       &sleep_timeval);
      }
    }
    
    /* now up the time index */
    time_index = (time_index +1)%MAX_KEPT_TIMES;
#endif
    nummessages++;          
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug > 3) {
      fprintf(where,"Transaction %d completed\n",nummessages);
      fflush(where);
    }
    
  }
  
  /* The test is over. Flush the buffers to the remote end. We do a */
  /* graceful release to insure that all data has been taken by the */
  /* remote. Of course, since this was a request/response test, there */
  /* should be no data outstanding on the socket ;-) */ 
  
  if (shutdown(send_socket,1) == SOCKET_ERROR) {
    perror("netperf: cannot shutdown dg stream socket");
    
    exit(1);
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
    perror("netperf: remote error");
    
    exit(1);
  }
  
  /* We now calculate what our thruput was for the test. In the future, */
  /* we may want to include a calculation of the thruput measured by */
  /* the remote, but it should be the case that for a DG stream test, */
  /* that the two numbers should be *very* close... We calculate */
  /* bytes_sent regardless of the way the test length was controlled. */
  /* If it was time, we needed to, and if it was by bytes, the user may */
  /* have specified a number of bytes that wasn't a multiple of the */
  /* send_size, so we really didn't send what he asked for ;-) We use */
  
  bytes_xferd	= (req_size * nummessages) + (rsp_size * nummessages);
  thruput		= calc_thruput(bytes_xferd);
  
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
      remote_cpu_utilization = dg_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand  = calc_service_demand((double) nummessages*1024,
						   0.0,
						   remote_cpu_utilization,
						   dg_rr_result->num_cpus);
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
    /* DG statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
#ifdef WANT_INTERVALS
    kept_times[MAX_KEPT_TIMES] = 0;
    time_index = 0;
    while (time_index < MAX_KEPT_TIMES) {
      if (kept_times[time_index] > 0) {
	total_times += kept_times[time_index];
      }
      else
	unused_buckets++;
      time_index += 1;
    }
    total_times /= (MAX_KEPT_TIMES-unused_buckets);
    fprintf(where,
	    "Average response time %d usecs\n",
	    total_times);
#endif
  }
  unlink(myaddr_un.sun_path);
}

 /* this routine implements the receive side (netserver) of a DG_RR */
 /* test. */
void
recv_dg_rr()
{
  
  struct ring_elt *recv_ring;
  struct ring_elt *send_ring;

  struct	sockaddr_un        myaddr_un,
  peeraddr_un;
  SOCKET s_data;
  int 	addrlen;
  int	trans_received = 0;
  int	trans_remaining;
  float	elapsed_time;
  
  struct	dg_rr_request_struct	*dg_rr_request;
  struct	dg_rr_response_struct	*dg_rr_response;
  struct	dg_rr_results_struct	*dg_rr_results;
  
  dg_rr_request  = 
    (struct dg_rr_request_struct *)netperf_request.content.test_specific_data;
  dg_rr_response = 
    (struct dg_rr_response_struct *)netperf_response.content.test_specific_data;
  dg_rr_results  = 
    (struct dg_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dg_rr: entered...\n");
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
    fprintf(where,"recv_dg_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = DG_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_dg_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,"recv_dg_rr: requested recv alignment of %d offset %d\n",
	    dg_rr_request->recv_alignment,
	    dg_rr_request->recv_offset);
    fprintf(where,"recv_dg_rr: requested send alignment of %d offset %d\n",
	    dg_rr_request->send_alignment,
	    dg_rr_request->send_offset);
    fflush(where);
  }

  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  recv_ring = allocate_buffer_ring(recv_width,
				   dg_rr_request->request_size,
				   dg_rr_request->recv_alignment,
				   dg_rr_request->recv_offset);

  send_ring = allocate_buffer_ring(send_width,
				   dg_rr_request->response_size,
				   dg_rr_request->send_alignment,
				   dg_rr_request->send_offset);

  if (debug) {
    fprintf(where,"recv_dg_rr: receive alignment and offset set...\n");
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_un,
	sizeof(myaddr_un));
  myaddr_un.sun_family      = AF_UNIX;
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_dg_rr: grabbing a socket...\n");
    fflush(where);
  }
  

  /* create_unix_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = dg_rr_request->send_buf_size;
  lsr_size_req = dg_rr_request->recv_buf_size;

  s_data = create_unix_socket(AF_UNIX,
			      SOCK_DGRAM);
  
  if (s_data == INVALID_SOCKET) {
    netperf_response.content.serv_errno = errno;
    send_response();
    
    exit(1);
  }
  
  /* Let's get an address assigned to this socket so we can tell the */
  /* initiator how to reach the data socket. There may be a desire to */
  /* nail this socket to a specific IP address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  strcpy(myaddr_un.sun_path,tempnam(path_prefix,"netperf."));
  if (bind(s_data,
	   (struct sockaddr *)&myaddr_un,
	   sizeof(myaddr_un)) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    unlink(myaddr_un.sun_path);
    close(s_data);
    send_response();
    
    exit(1);
  }
  
  /* Now myaddr_un contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  strcpy(dg_rr_response->unix_path,myaddr_un.sun_path);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  dg_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (dg_rr_request->measure_cpu) {
    dg_rr_response->measure_cpu = 1;
    dg_rr_response->cpu_rate = calibrate_local_cpu(dg_rr_request->cpu_rate);
  }
   
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  dg_rr_response->send_buf_size = lss_size;
  dg_rr_response->recv_buf_size = lsr_size;
 
  send_response();
  
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(dg_rr_request->measure_cpu);
  
  if (dg_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(dg_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = dg_rr_request->test_length * -1;
  }
  
  addrlen = sizeof(peeraddr_un);
  bzero((char *)&peeraddr_un, addrlen);
  
  while ((!times_up) || (trans_remaining > 0)) {
    
    /* receive the request from the other side */
    fprintf(where,"socket %d ptr %p size %d\n",
	    s_data,
	    recv_ring->buffer_ptr,
	    dg_rr_request->request_size);
    fflush(where);
    if (recvfrom(s_data,
		 recv_ring->buffer_ptr,
		 dg_rr_request->request_size,
		 0,
		 (struct sockaddr *)&peeraddr_un,
		 &addrlen) != dg_rr_request->request_size) {
      if (errno == EINTR) {
	/* we must have hit the end of test time. */
	break;
      }
      netperf_response.content.serv_errno = errno;
      fprintf(where,"error on recvfrom errno %d\n",errno);
      fflush(where);
      send_response();
      unlink(myaddr_un.sun_path);
      exit(1);
    }
    recv_ring = recv_ring->next;

    /* Now, send the response to the remote */
    if (sendto(s_data,
	       send_ring->buffer_ptr,
	       dg_rr_request->response_size,
	       0,
	       (struct sockaddr *)&peeraddr_un,
	       addrlen) != dg_rr_request->response_size) {
      if (errno == EINTR) {
	/* we have hit end of test time. */
	break;
      }
      netperf_response.content.serv_errno = errno;
      fprintf(where,"error on recvfrom errno %d\n",errno);
      fflush(where);
      unlink(myaddr_un.sun_path);
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
	      "recv_dg_rr: Transaction %d complete.\n",
	      trans_received);
      fflush(where);
    }
    
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(dg_rr_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_dg_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  dg_rr_results->bytes_received	= (trans_received * 
					   (dg_rr_request->request_size + 
					    dg_rr_request->response_size));
  dg_rr_results->trans_received	= trans_received;
  dg_rr_results->elapsed_time	= elapsed_time;
  if (dg_rr_request->measure_cpu) {
    dg_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_dg_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  unlink(myaddr_un.sun_path);
  
}
 /* this routine implements the receive (netserver) side of a STREAM_RR */
 /* test */

void
recv_stream_rr()
{
  
  struct ring_elt *send_ring;
  struct ring_elt *recv_ring;

  struct	sockaddr_un        myaddr_un,
  peeraddr_un;
  SOCKET s_listen,s_data;
  int 	addrlen;
  char	*temp_message_ptr;
  int	trans_received = 0;
  int	trans_remaining;
  int	bytes_sent;
  int	request_bytes_recvd;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	stream_rr_request_struct	*stream_rr_request;
  struct	stream_rr_response_struct	*stream_rr_response;
  struct	stream_rr_results_struct	*stream_rr_results;
  
  stream_rr_request = 
    (struct stream_rr_request_struct *)netperf_request.content.test_specific_data;
  stream_rr_response =
    (struct stream_rr_response_struct *)netperf_response.content.test_specific_data;
  stream_rr_results =
    (struct stream_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_stream_rr: entered...\n");
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
    fprintf(where,"recv_stream_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = STREAM_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_stream_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* allocate the recv and send rings with the requested alignments */
  /* and offsets. raj 7/94 */
  if (debug) {
    fprintf(where,"recv_stream_rr: requested recv alignment of %d offset %d\n",
	    stream_rr_request->recv_alignment,
	    stream_rr_request->recv_offset);
    fprintf(where,"recv_stream_rr: requested send alignment of %d offset %d\n",
	    stream_rr_request->send_alignment,
	    stream_rr_request->send_offset);
    fflush(where);
  }

  /* at some point, these need to come to us from the remote system */
  if (send_width == 0) send_width = 1;
  if (recv_width == 0) recv_width = 1;

  send_ring = allocate_buffer_ring(send_width,
				   stream_rr_request->response_size,
				   stream_rr_request->send_alignment,
				   stream_rr_request->send_offset);

  recv_ring = allocate_buffer_ring(recv_width,
				   stream_rr_request->request_size,
				   stream_rr_request->recv_alignment,
				   stream_rr_request->recv_offset);

  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  bzero((char *)&myaddr_un,
	sizeof(myaddr_un));
  myaddr_un.sun_family      = AF_UNIX;
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_stream_rr: grabbing a socket...\n");
    fflush(where);
  }

  /* create_unix_socket expects to find some things in the global */
  /* variables, so set the globals based on the values in the request. */
  /* once the socket has been created, we will set the response values */
  /* based on the updated value of those globals. raj 7/94 */
  lss_size_req = stream_rr_request->send_buf_size;
  lsr_size_req = stream_rr_request->recv_buf_size;
  
  s_listen = create_unix_socket(AF_UNIX,
				SOCK_STREAM);
  
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
  
  strcpy(myaddr_un.sun_path,tempnam(path_prefix,"netperf."));
  if (bind(s_listen,
	   (struct sockaddr *)&myaddr_un,
	   sizeof(myaddr_un)) == SOCKET_ERROR) {
    netperf_response.content.serv_errno = errno;
    unlink(myaddr_un.sun_path);
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
  
  /* Now myaddr_un contains the port and the internet address this is */
  /* returned to the sender also implicitly telling the sender that the */
  /* socket buffer sizing has been done. */
  
  strcpy(stream_rr_response->unix_path,myaddr_un.sun_path);
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  stream_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (stream_rr_request->measure_cpu) {
    stream_rr_response->measure_cpu = 1;
    stream_rr_response->cpu_rate = calibrate_local_cpu(stream_rr_request->cpu_rate);
  }
  
  
  /* before we send the response back to the initiator, pull some of */
  /* the socket parms from the globals */
  stream_rr_response->send_buf_size = lss_size;
  stream_rr_response->recv_buf_size = lsr_size;

  send_response();
  
  addrlen = sizeof(peeraddr_un);
  
  if ((s_data = accept(s_listen,
		       (struct sockaddr *)&peeraddr_un,
		       &addrlen)) == INVALID_SOCKET) {
    /* Let's just punt. The remote will be given some information */
    close(s_listen);
    
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"recv_stream_rr: accept completes on the data connection.\n");
    fflush(where);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(stream_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (stream_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(stream_rr_request->test_length + PAD_TIME);
  }
  else {
    times_up = 1;
    trans_remaining = stream_rr_request->test_length * -1;
  }
  
  while ((!times_up) || (trans_remaining > 0)) {
    temp_message_ptr = recv_ring->buffer_ptr;
    request_bytes_remaining	= stream_rr_request->request_size;
    
    /* receive the request from the other side */
    if (debug) {
      fprintf(where,"about to receive for trans %d\n",trans_received);
      fprintf(where,"temp_message_ptr is %p\n",temp_message_ptr);
      fflush(where);
    }
    while(request_bytes_remaining > 0) {
      if((request_bytes_recvd=recv(s_data,
				   temp_message_ptr,
				   request_bytes_remaining,
				   0)) == SOCKET_ERROR) {
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
      if (debug) {
	fprintf(where,"just received for trans %d\n",trans_received);
	fflush(where);
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
    if (debug) {
      fprintf(where,"about to send for trans %d\n",trans_received);
      fflush(where);
    }
    if((bytes_sent=send(s_data,
			send_ring->buffer_ptr,
			stream_rr_request->response_size,
			0)) == SOCKET_ERROR) {
      if (errno == EINTR) {
	/* the test timer has popped */
	timed_out = 1;
	fprintf(where,"yo6\n");
	fflush(where);						
	break;
      }
      netperf_response.content.serv_errno = 997;
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
	      "recv_stream_rr: Transaction %d complete\n",
	      trans_received);
      fflush(where);
    }
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(stream_rr_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_stream_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  stream_rr_results->bytes_received	= (trans_received * 
					   (stream_rr_request->request_size + 
					    stream_rr_request->response_size));
  stream_rr_results->trans_received	= trans_received;
  stream_rr_results->elapsed_time	= elapsed_time;
  if (stream_rr_request->measure_cpu) {
    stream_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_stream_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  unlink(myaddr_un.sun_path);
}

void
print_unix_usage()
{

  fwrite(unix_usage, sizeof(char), strlen(unix_usage), stdout);
  exit(1);

}
void
scan_unix_args(int argc, char *argv[])
{
#define UNIX_ARGS "hm:M:p:r:s:S:"
  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	
    arg1[BUFSIZ],  /* argument holders		*/
    arg2[BUFSIZ];
  
  init_test_vars();

  if (no_control) {
    fprintf(where,
	    "The UNIX tests do not know how to run with no control connection\n");
    exit(-1);
  }

  /* Go through all the command line arguments and break them */
  /* out. For those options that take two parms, specifying only */
  /* the first will set both to that value. Specifying only the */
  /* second will leave the first untouched. To change only the */
  /* first, use the form "first," (see the routine break_args.. */
  
  while ((c= getopt(argc, argv, UNIX_ARGS)) != EOF) {
    switch (c) {
    case '?':	
    case 'h':
      print_unix_usage();
      exit(1);
    case 'p':
      /* set the path prefix (directory) that should be used for the */
      /* pipes. at some point, there should be some error checking. */
      strcpy(path_prefix,optarg);
      break;
    case 's':
      /* set local socket sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	lss_size_req = atoi(arg1);
      if (arg2[0])
	lsr_size_req = atoi(arg2);
      break;
    case 'S':
      /* set remote socket sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	rss_size = atoi(arg1);
      if (arg2[0])
	rsr_size = atoi(arg2);
      break;
    case 'r':
      /* set the request/response sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	req_size = atoi(arg1);
      if (arg2[0])	
	rsp_size = atoi(arg2);
      break;
    case 'm':
      /* set the send size */
      send_size = atoi(optarg);
      break;
    case 'M':
      /* set the recv size */
      recv_size = atoi(optarg);
      break;
    };
  }
}
#endif /* WANT_UNIX */ 
