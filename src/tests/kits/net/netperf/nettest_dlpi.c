#ifdef lint
#define DO_DLPI
#define DIRTY
#define INTERVALS
#endif /* lint */
/****************************************************************/
/*								*/
/*	nettest_dlpi.c						*/
/*								*/
/*	the actual test routines...				*/
/*								*/
/*	send_dlpi_co_stream()	perform a CO DLPI stream test	*/
/*	recv_dlpi_co_stream()					*/
/*	send_dlpi_co_rr()	perform a CO DLPI req/res	*/
/*	recv_dlpi_co_rr()					*/
/*	send_dlpi_cl_stream()	perform a CL DLPI stream test	*/
/*	recv_dlpi_cl_stream()					*/
/*	send_dlpi_cl_rr()	perform a CL DLPI req/res	*/
/*	recv_dlpi_cl_rr()					*/
/*								*/
/****************************************************************/

#ifdef DO_DLPI
char	nettest_dlpi_id[]="\
@(#)nettest_dlpi.c (c) Copyright 1993,1995 Hewlett-Packard Co. Version 2.2pl1";

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <malloc.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#ifdef __osf__
#include <sys/dlpihdr.h>
#else /* __osf__ */
#include <sys/dlpi.h>
#ifdef __hpux__
#include <sys/dlpi_ext.h>
#endif /* __hpux__ */
#endif /* __osf__ */

#include "netlib.h"
#include "netsh.h"
#include "nettest_dlpi.h"

/* these are some variables global to all the DLPI tests. declare */
/* them static to make them global only to this file */

static int 
  rsw_size,		/* remote send window size	*/
  rrw_size,		/* remote recv window size	*/
  lsw_size,		/* local  send window size 	*/
  lrw_size,		/* local  recv window size 	*/
  req_size = 100,	/* request size                   	*/
  rsp_size = 200,	/* response size			*/
  send_size,		/* how big are individual sends		*/
  recv_size;		/* how big are individual receives	*/

int
  loc_ppa = 4,          /* the ppa for the local interface, */
  /* as shown as the NM Id in lanscan */
  rem_ppa = 4,          /* the ppa for the remote interface */
  dlpi_sap = 84;        /* which 802.2 SAP should we use?   */

char loc_dlpi_device[32] = "/dev/dlpi";
char rem_dlpi_device[32] = "/dev/dlpi";

char dlpi_usage[] = "\n\
Usage: netperf [global options] -- [test options] \n\
\n\
CO/CL DLPI Test Options:\n\
    -D dev[,dev]      Set the local/remote DLPI device file name\n\
    -h                Display this text\n\
    -M bytes          Set the recv size (DLCO_STREAM, DLCL_STREAM)\n\
    -m bytes          Set the send size (DLCO_STREAM, DLCL_STREAM)\n\
    -p loc[,rem]      Set the local/remote PPA for the test\n\
    -R bytes          Set response size (DLCO_RR, DLCL_RR)\n\
    -r bytes          Set request size (DLCO_RR, DLCL_RR)\n\
    -s sap            Set the 802.2 sap for the test\n\
    -W send[,recv]    Set remote send/recv window sizes\n\
    -w send[,recv]    Set local send/recv window sizes\n\
\n\
For those options taking two parms, at least one must be specified;\n\
specifying one value without a comma will set both parms to that\n\
value, specifying a value with a leading comma will set just the second\n\
parm, a value with a trailing comma will set just the first. To set\n\
each parm to unique values, specify both and separate them with a\n\
comma.\n"; 


/* This routine implements the CO unidirectional data transfer test */
/* (a.k.a. stream) for the sockets interface. It receives its */
/* parameters via global variables from the shell and writes its */
/* output to the standard output. */


void 
  send_dlpi_co_stream()
{
  
  char *tput_title = "\
Recv   Send    Send                          \n\
Window Window  Message  Elapsed              \n\
Size   Size    Size     Time     Throughput  \n\
frames frames  bytes    secs.    %s/sec  \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1 =
    "%5d  %5d  %6d    %-6.2f   %7.2f   \n";
  
  char *cpu_title = "\
Recv   Send    Send                          Utilization    Service Demand\n\
Window Window  Message  Elapsed              Send   Recv    Send    Recv\n\
Size   Size    Size     Time     Throughput  local  remote  local   remote\n\
frames frames  bytes    secs.    %-8.8s/s  %%      %%       us/KB   us/KB\n\n";
  
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
  
#ifdef INTERVALS
  int interval_count;
#endif /* INTERVALS */
  
  /* what we want is to have a buffer space that is at least one */
  /* send-size greater than our send window. this will insure that we */
  /* are never trying to re-use a buffer that may still be in the hands */
  /* of the transport. This buffer will be malloc'd after we have found */
  /* the size of the local senc socket buffer. We will want to deal */
  /* with alignment and offset concerns as well. */
  
  struct ring_elt *send_ring;
  char	*message_base;
  char	*message_ptr;
  struct strbuf send_message;
  char  dlsap[BUFSIZ];
  int   dlsap_len;
  int	*message_int_ptr;
  int	message_offset;
  int	malloc_size;
  
  int	len;
  int	nummessages;
  int	send_descriptor;
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
  
  struct	dlpi_co_stream_request_struct	*dlpi_co_stream_request;
  struct	dlpi_co_stream_response_struct	*dlpi_co_stream_response;
  struct	dlpi_co_stream_results_struct	*dlpi_co_stream_result;
  
  dlpi_co_stream_request	= 
    (struct dlpi_co_stream_request_struct *)netperf_request.content.test_specific_data;
  dlpi_co_stream_response	=
    (struct dlpi_co_stream_response_struct *)netperf_response.content.test_specific_data;
  dlpi_co_stream_result	        = 
    (struct dlpi_co_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if ( print_headers ) {
    fprintf(where,"DLPI CO STREAM TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      fprintf(where,cpu_title,format_units());
    else
      fprintf(where,tput_title,format_units());
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_sent	=	0.0;
  times_up 	= 	0;
  
  /*set up the data descriptor                        */
  send_descriptor = dl_open(loc_dlpi_device,loc_ppa);  
  if (send_descriptor < 0){
    perror("netperf: send_dlpi_co_stream: dlpi stream data descriptor");
    exit(1);
  }
  
  /* bind the puppy and get the assigned dlsap */
  dlsap_len = BUFSIZ;
  if (dl_bind(send_descriptor, 
              dlpi_sap, DL_CODLS, dlsap, &dlsap_len) != 0) {
    fprintf(where,"send_dlpi_co_rr: bind failure\n");
    fflush(where);
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"send_dlpi_co_stream: send_descriptor obtained...\n");
  }
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  if (lsw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_co_stream: window send size altered from system default...\n");
      fprintf(where,"                          send: %d\n",lsw_size);
    }
  }
  if (lrw_size > 0) {
    if (debug > 1) {
      fprintf(where,
	      "netperf: send_dlpi_co_stream: window recv size altered from system default...\n");
      fprintf(where,"                          recv: %d\n",lrw_size);
    }
  }
  
  
  /* Now, we will find-out what the size actually became, and report */
  /* that back to the user. If the call fails, we will just report a -1 */
  /* back to the initiator for the recv buffer size. */
  
  
  if (debug) {
    fprintf(where,
	    "netperf: send_dlpi_co_stream: window sizes determined...\n");
    fprintf(where,"         send: %d recv: %d\n",lsw_size,lrw_size);
    ffluch(where);
  }
  
#else /* DL_HP_SET_LOCAL_WIN_REQ */
  
  lsw_size = -1;
  lrw_size = -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
  /* we should pick a default send_size, it should not be larger than */
  /* the min of the two interface MTU's, and should perhaps default to */
  /* the Interface MTU, but for now, we will default it to 1024... if */
  /* someone wants to change this, the should change the corresponding */
  /* lines in the recv_dlpi_co_stream routine */
  
  if (send_size == 0) {
    send_size = 1024;
  }
  
  /* set-up the data buffer with the requested alignment and offset. */
  /* After we have calculated the proper starting address, we want to */
  /* put that back into the message_base variable so we go back to the */
  /* proper place. note that this means that only the first send is */
  /* guaranteed to be at the alignment specified by the -a parameter. I */
  /* think that this is a little more "real-world" than what was found */
  /* in previous versions. note also that we have allocated a quantity */
  /* of memory that is at least one send-size greater than our socket */
  /* buffer size. We want to be sure that there are at least two */
  /* buffers allocated - this can be a bit of a problem when the */
  /* send_size is bigger than the socket size, so we must check... the */
  /* user may have wanted to explicitly set the "width" of our send */
  /* buffers, we should respect that wish... */
  if (send_width == 0) {
    send_width = (lsw_size/send_size) + 1;
    if (send_width == 1) send_width++;
  }
  
  send_ring = allocate_buffer_ring(send_width,
				   send_size,
				   local_send_align,
				   local_send_offset);
  
  send_message.maxlen = send_size;
  send_message.len = send_size;
  send_message.buf = send_ring->buffer_ptr;
  
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
  /* default should be used. */
  
  netperf_request.content.request_type	 =	DO_DLPI_CO_STREAM;
  dlpi_co_stream_request->send_win_size =	rsw_size;
  dlpi_co_stream_request->recv_win_size =	rrw_size;
  dlpi_co_stream_request->receive_size	 =	recv_size;
  dlpi_co_stream_request->recv_alignment=	remote_recv_align;
  dlpi_co_stream_request->recv_offset	 =	remote_recv_offset;
  dlpi_co_stream_request->measure_cpu	 =	remote_cpu_usage;
  dlpi_co_stream_request->cpu_rate	 =	remote_cpu_rate;
  dlpi_co_stream_request->ppa           =      rem_ppa;
  dlpi_co_stream_request->sap           =      dlpi_sap;
  dlpi_co_stream_request->dev_name_len  =      strlen(rem_dlpi_device);
  strcpy(dlpi_co_stream_request->dlpi_device,
	 rem_dlpi_device);
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I didn't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_co_stream_request->dlpi_device;
    lastword = initword + ((strlen(rem_dlpi_device) + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = ntohl(*charword);
    }
  }
#endif /* __alpha */
  
  if (test_time) {
    dlpi_co_stream_request->test_length	=	test_time;
  }
  else {
    dlpi_co_stream_request->test_length	=	test_bytes;
  }
#ifdef DIRTY
  dlpi_co_stream_request->dirty_count       =       rem_dirty_count;
  dlpi_co_stream_request->clean_count       =       rem_clean_count;
#endif /* DIRTY */
  
  
  if (debug > 1) {
    fprintf(where,
	    "netperf: send_dlpi_co_stream: requesting DLPI CO stream test\n");
  }
  
  send_request();
  
  /* The response from the remote will contain all of the relevant 	*/
  /* parameters for this test type. We will put them back into 	*/
  /* the variables here so they can be displayed if desired.  The	*/
  /* remote will have calibrated CPU if necessary, and will have done	*/
  /* all the needed set-up we will have calibrated the cpu locally	*/
  /* before sending the request, and will grab the counter value right	*/
  /* after the connect returns. The remote will grab the counter right	*/
  /* after the accept call. This saves the hassle of extra messages	*/
  /* being sent for the TCP tests.					*/
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote listen done.\n");
    rrw_size	=	dlpi_co_stream_response->recv_win_size;
    rsw_size	=	dlpi_co_stream_response->send_win_size;
    remote_cpu_usage=	dlpi_co_stream_response->measure_cpu;
    remote_cpu_rate = 	dlpi_co_stream_response->cpu_rate;
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
    exit(1);
  }
  
  /* Connect up to the remote port on the data descriptor */
  if(dl_connect(send_descriptor,
		dlpi_co_stream_response->station_addr,
		dlpi_co_stream_response->station_addr_len) != 0) {
    fprintf(where,"recv_dlpi_co_stream: connect failure\n");
    fflush(where);
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
#endif /* DIRTY */
  
  while ((!times_up) || (bytes_remaining > 0)) {
    
#ifdef DIRTY
    /* we want to dirty some number of consecutive integers in the buffer */
    /* we are about to send. we may also want to bring some number of */
    /* them cleanly into the cache. The clean ones will follow any dirty */
    /* ones into the cache. */
    message_int_ptr = (int *)message_ptr;
    for (i = 0; i < loc_dirty_count; i++) {
      *message_int_ptr = rand();
      message_int_ptr++;
    }
    for (i = 0; i < loc_clean_count; i++) {
      loc_dirty_count = *message_int_ptr;
      message_int_ptr++;
    }
#endif /* DIRTY */
    
    if((putmsg(send_descriptor,
	       0,
	       &send_message,
	       0)) != 0) {
      if (errno == EINTR)
	break;
      perror("netperf: data send error");
      exit(1);
    }
    send_ring = send_ring->next;
    send_message.buf = send_ring->buffer_ptr;
#ifdef INTERVALS
    for (interval_count = 0;
	 interval_count < interval_wate;
	 interval_count++);
#endif /* INTERVALS */
    
    if (debug > 4) {
      fprintf(where,"netperf: send_clpi_co_stream: putmsg called ");
      fprintf(where,"len is %d\n",send_message.len);
      fflush(where);
    }
    
    nummessages++;          
    if (bytes_remaining) {
      bytes_remaining -= send_size;
    }
  }
  
  /* The test is over. Flush the buffers to the remote end. We do a */
  /* graceful release to insure that all data has been taken by the */
  /* remote. this needs a little work - there is no three-way */
  /* handshake with type two as there is with TCP, so there really */
  /* should be a message exchange here. however, we will finesse it by */
  /* saying that the tests shoudl run for a while. */ 
  
  if (debug) {
    fprintf(where,"sending test end signal \n");
    fflush(where);
  }
  
  send_message.len = (send_size - 1);
  if (send_message.len == 0) send_message.len = 2;
  
  if((putmsg(send_descriptor,
	     0,
	     &send_message,
	     0)) != 0) {
    perror("netperf: data send error");
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
  
  bytes_sent	= ((double) send_size * (double) nummessages) + (double) len;
  thruput		= calc_thruput(bytes_sent);
  
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
      local_cpu_utilization	= calc_cpu_util(0.0);
      local_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      0.0);
    }
    else {
      local_cpu_utilization	= -1.0;
      local_service_demand	= -1.0;
    }
    
    if (remote_cpu_usage) {
      if (remote_cpu_rate == 0.0) {
	fprintf(where,
		"DANGER   DANGER  DANGER   DANGER   DANGER  DANGER   DANGER!\n");
	fprintf(where,
		"Remote CPU usage numbers based on process information only!\n");
	fflush(where);
      }
      remote_cpu_utilization	= dlpi_co_stream_result->cpu_util;
      remote_service_demand	= calc_service_demand(bytes_sent,
						      0.0,
						      remote_cpu_utilization);
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
	      rrw_size,		/* remote recvbuf size */
	      lsw_size,		/* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time,		/* how long was the test */
	      thruput, 		/* what was the xfer rate */
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
	      rrw_size, 		/* remote recvbuf size */
	      lsw_size, 		/* local sendbuf size */
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
	    bytes_sent / (double)dlpi_co_stream_result->recv_calls,
	    dlpi_co_stream_result->recv_calls);
  }
  
}


/* This is the server-side routine for the tcp stream test. It is */
/* implemented as one routine. I could break things-out somewhat, but */
/* didn't feel it was necessary. */

int 
  recv_dlpi_co_stream()
{
  
  int	data_descriptor;
  int	flags = 0;
  int	measure_cpu;
  int	bytes_received;
  int	receive_calls;
  float	elapsed_time;
  
  struct ring_elt *recv_ring;
  char	*message_ptr;
  char	*message_base;
  int   *message_int_ptr;
  struct strbuf recv_message;
  int   dirty_count;
  int   clean_count;
  int   i;
  
  struct	dlpi_co_stream_request_struct	*dlpi_co_stream_request;
  struct	dlpi_co_stream_response_struct	*dlpi_co_stream_response;
  struct	dlpi_co_stream_results_struct	*dlpi_co_stream_results;
  
  dlpi_co_stream_request	= (struct dlpi_co_stream_request_struct *)netperf_request.content.test_specific_data;
  dlpi_co_stream_response	= (struct dlpi_co_stream_response_struct *)netperf_response.content.test_specific_data;
  dlpi_co_stream_results	= (struct dlpi_co_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dlpi_co_stream: entered...\n");
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
  
  netperf_response.content.response_type = DLPI_CO_STREAM_RESPONSE;
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug > 1) {
    fprintf(where,"recv_dlpi_co_stream: requested alignment of %d\n",
	    dlpi_co_stream_request->recv_alignment);
    fflush(where);
  }
  
  
  /* Grab a descriptor to listen on, and then listen on it. */
  
  if (debug > 1) {
    fprintf(where,"recv_dlpi_co_stream: grabbing a descriptor...\n");
    fflush(where);
  }
  
  
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_co_stream_request->dlpi_device;
    lastword = initword + ((dlpi_co_stream_request->dev_name_len + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = htonl(*charword);
    }
  }
#endif /* __alpha */
  
  data_descriptor = dl_open(dlpi_co_stream_request->dlpi_device,
			    dlpi_co_stream_request->ppa);
  if (data_descriptor < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  /* Let's get an address assigned to this descriptor so we can tell the */
  /* initiator how to reach the data descriptor. There may be a desire to */
  /* nail this descriptor to a specific address in a multi-homed, */
  /* multi-connection situation, but for now, we'll ignore the issue */
  /* and concentrate on single connection testing. */
  
  /* bind the sap and retrieve the dlsap assigned by the system  */
  dlpi_co_stream_response->station_addr_len = 14; /* arbitrary */
  if (dl_bind(data_descriptor,
	      dlpi_co_stream_request->sap,
	      DL_CODLS,
	      (char *)dlpi_co_stream_response->station_addr,
	      &dlpi_co_stream_response->station_addr_len) != 0) {
    fprintf(where,"recv_dlpi_co_stream: bind failure\n");
    fflush(where);
    exit(1);
  }
  
  /* The initiator may have wished-us to modify the socket buffer */
  /* sizes. We should give it a shot. If he didn't ask us to change the */
  /* sizes, we should let him know what sizes were in use at this end. */
  /* If none of this code is compiled-in, then we will tell the */
  /* initiator that we were unable to play with the socket buffer by */
  /* setting the size in the response to -1. */
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  
  if (dlpi_co_stream_request->recv_win_size) {
  }
  /* Now, we will find-out what the size actually became, and report */
  /* that back to the user. If the call fails, we will just report a -1 */
  /* back to the initiator for the recv buffer size. */
  
#else /* the system won't let us play with the buffers */
  
  dlpi_co_stream_response->recv_win_size	= -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
  /* what sort of sizes did we end-up with? */
  /* this bit of code whould default to the Interface MTU */
  if (dlpi_co_stream_request->receive_size == 0) {
    recv_size = 1024;
  }
  else {
    recv_size = dlpi_co_stream_request->receive_size;
  }
  
  /* tell the other fellow what our receive size became */
  dlpi_co_stream_response->receive_size = recv_size;
  
  /* just a little prep work for when we may have to behave like the */
  /* sending side... */
  message_base = (char *)malloc(recv_size * 2);
  message_ptr = (char *)(((long)message_base + 
			  (long)dlpi_co_stream_request->recv_alignment -1) &
			 ~((long)dlpi_co_stream_request->recv_alignment - 1));
  message_ptr = message_ptr + dlpi_co_stream_request->recv_offset;
  recv_message.maxlen = recv_size;
  recv_message.len = 0;
  recv_message.buf = message_ptr;
  
  if (debug > 1) {
    fprintf(where,
	    "recv_dlpi_co_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  dlpi_co_stream_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (dlpi_co_stream_request->measure_cpu) {
    dlpi_co_stream_response->measure_cpu = 1;
    dlpi_co_stream_response->cpu_rate = 
      calibrate_local_cpu(dlpi_co_stream_request->cpu_rate);
  }
  
  send_response();
  
  /* accept a connection on this file descriptor. at some point, */
  /* dl_accept will "do the right thing" with the last two parms, but */
  /* for now it ignores them, so we will pass zeros. */
  
  if(dl_accept(data_descriptor, 0, 0) != 0) {
    fprintf(where,
	    "recv_dlpi_co_stream: error in accept, errno %d\n",
	    errno);
    fflush(where);
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"netserver:recv_dlpi_co_stream: connection accepted\n");
    fflush(where);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(dlpi_co_stream_request->measure_cpu);
  
#ifdef DIRTY
  /* we want to dirty some number of consecutive integers in the buffer */
  /* we are about to recv. we may also want to bring some number of */
  /* them cleanly into the cache. The clean ones will follow any dirty */
  /* ones into the cache. */
  
  dirty_count = dlpi_co_stream_request->dirty_count;
  clean_count = dlpi_co_stream_request->clean_count;
  message_int_ptr = (int *)message_ptr;
  for (i = 0; i < dirty_count; i++) {
    *message_int_ptr = rand();
    message_int_ptr++;
  }
  for (i = 0; i < clean_count; i++) {
    dirty_count = *message_int_ptr;
    message_int_ptr++;
  }
#endif /* DIRTY */
  
  recv_message.len = recv_size; 
  while (recv_message.len == recv_size) {
    if (getmsg(data_descriptor, 
	       0,
	       &recv_message, 
	       &flags) != 0) {
      netperf_response.content.serv_errno = errno;
      send_response();
      exit(1);
    }
    bytes_received += recv_message.len;
    receive_calls++;
    
    if (debug) {
      fprintf(where,
	      "netserver:recv_dlpi_co_stream: getmsg accepted %d bytes\n",
	      recv_message.len);
      fflush(where);
    }
    
    
#ifdef DIRTY
    message_int_ptr = (int *)message_ptr;
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
  
  /* The loop now exits due to zero bytes received. */
  /* should perform a disconnect to signal the sender that */
  /* we have received all the data sent. */
  
  if (close(data_descriptor) == -1) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  cpu_stop(dlpi_co_stream_request->measure_cpu,&elapsed_time);
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_co_stream: got %d bytes\n",
	    bytes_received);
    fprintf(where,
	    "recv_dlpi_co_stream: got %d recvs\n",
	    receive_calls);
    fflush(where);
  }
  
  dlpi_co_stream_results->bytes_received	= bytes_received;
  dlpi_co_stream_results->elapsed_time	= elapsed_time;
  dlpi_co_stream_results->recv_calls		= receive_calls;
  
  if (dlpi_co_stream_request->measure_cpu) {
    dlpi_co_stream_results->cpu_util	= calc_cpu_util(0.0);
  };
  
  if (debug > 1) {
    fprintf(where,
	    "recv_dlpi_co_stream: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
}

/*********************************/

int send_dlpi_co_rr(remote_host)
     char	remote_host[];
{
  
  char *tput_title = "\
 Local /Remote\n\
 Window Size   Request  Resp.   Elapsed  Trans.\n\
 Send   Recv   Size     Size    Time     Rate         \n\
 frames frames bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
 %-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
 %-6d %-6d\n";
  
  char *cpu_title = "\
 Local /Remote\n\
 Window Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
 Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
 frames frames bytes   bytes  secs.   per sec  %%      %%      us/Tr   us/Tr\n\n";
  
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
 %5d  %5d   %5d  %5d";
  
  
  int			timed_out = 0;
  float			elapsed_time;
  int	    dlsap_len;
  char      dlsap[BUFSIZ];
  
  int   flags = 0;
  char	*send_message_ptr;
  char	*recv_message_ptr;
  char	*temp_message_ptr;
  struct strbuf send_message;
  struct strbuf recv_message;
  
  int	nummessages;
  int	send_descriptor;
  int	trans_remaining;
  double	bytes_xferd;
  
  int	rsp_bytes_left;
  
  /* we assume that station adresses fit within two ints */
  unsigned int   remote_address[1];
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  double	thruput;
  
  struct	dlpi_co_rr_request_struct	*dlpi_co_rr_request;
  struct	dlpi_co_rr_response_struct	*dlpi_co_rr_response;
  struct	dlpi_co_rr_results_struct	*dlpi_co_rr_result;
  
  dlpi_co_rr_request	= 
    (struct dlpi_co_rr_request_struct *)netperf_request.content.test_specific_data;
  dlpi_co_rr_response	= 
    (struct dlpi_co_rr_response_struct *)netperf_response.content.test_specific_data;
  dlpi_co_rr_result	= 
    (struct dlpi_co_rr_results_struct *)netperf_response.content.test_specific_data;
  
  /* since we are now disconnected from the code that established the */
  /* control socket, and since we want to be able to use different */
  /* protocols and such, we are passed the name of the remote host and */
  /* must turn that into the test specific addressing information. */
  
  if ( print_headers ) {
    fprintf(where,"DLPI CO REQUEST/RESPONSE TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      fprintf(where,cpu_title,format_units());
    else
      fprintf(where,tput_title,format_units());
  }
  
  /* initialize a few counters */
  
  nummessages	=	0;
  bytes_xferd	=	0.0;
  times_up 	= 	0;
  
  /* set-up the data buffers with the requested alignment and offset */
  temp_message_ptr = (char *)malloc(req_size+MAXALIGNMENT+MAXOFFSET);
  send_message_ptr = (char *)(( (long) temp_message_ptr + 
			       (long) local_send_align - 1) &	
			      ~((long) local_send_align - 1));
  send_message_ptr = send_message_ptr + local_send_offset;
  send_message.maxlen = req_size+MAXALIGNMENT+MAXOFFSET;
  send_message.len    = req_size;
  send_message.buf    = send_message_ptr;
  
  temp_message_ptr = (char *)malloc(rsp_size+MAXALIGNMENT+MAXOFFSET);
  recv_message_ptr = (char *)(( (long) temp_message_ptr + 
			       (long) local_recv_align - 1) &	
			      ~((long) local_recv_align - 1));
  recv_message_ptr = recv_message_ptr + local_recv_offset;
  recv_message.maxlen = rsp_size+MAXALIGNMENT+MAXOFFSET;
  recv_message.len    = 0;
  recv_message.buf    = send_message_ptr;
  
  /*set up the data socket                        */
  
  send_descriptor = dl_open(loc_dlpi_device,loc_ppa);
  if (send_descriptor < 0){
    perror("netperf: send_dlpi_co_rr: tcp stream data descriptor");
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"send_dlpi_co_rr: send_descriptor obtained...\n");
  }
  
  /* bind the puppy and get the assigned dlsap */
  
  dlsap_len = BUFSIZ;
  if (dl_bind(send_descriptor, 
	      dlpi_sap, DL_CODLS, dlsap, &dlsap_len) != 0) {
    fprintf(where,"send_dlpi_co_rr: bind failure\n");
    fflush(where);
    exit(1);
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
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  if (lsw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_co_rr: socket send size altered from system default...\n");
      fprintf(where,"                          send: %d\n",lsw_size);
    }
  }
  if (lrw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_co_rr: socket recv size altered from system default...\n");
      fprintf(where,"                          recv: %d\n",lrw_size);
    }
  }
  
  
  /* Now, we will find-out what the size actually became, and report */
  /* that back to the user. If the call fails, we will just report a -1 */
  /* back to the initiator for the recv buffer size. */
  
  
  if (debug) {
    fprintf(where,"netperf: send_dlpi_co_rr: socket sizes determined...\n");
    fprintf(where,"         send: %d recv: %d\n",lsw_size,lrw_size);
  }
  
#else /* DL_HP_SET_LOCAL_WIN_REQ */
  
  lsw_size = -1;
  lrw_size = -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
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
  
  netperf_request.content.request_type	        =	DO_DLPI_CO_RR;
  dlpi_co_rr_request->recv_win_size	=	rrw_size;
  dlpi_co_rr_request->send_win_size	=	rsw_size;
  dlpi_co_rr_request->recv_alignment	=	remote_recv_align;
  dlpi_co_rr_request->recv_offset	=	remote_recv_offset;
  dlpi_co_rr_request->send_alignment	=	remote_send_align;
  dlpi_co_rr_request->send_offset	=	remote_send_offset;
  dlpi_co_rr_request->request_size	=	req_size;
  dlpi_co_rr_request->response_size	=	rsp_size;
  dlpi_co_rr_request->measure_cpu	=	remote_cpu_usage;
  dlpi_co_rr_request->cpu_rate	        =	remote_cpu_rate;
  dlpi_co_rr_request->ppa               =       rem_ppa;
  dlpi_co_rr_request->sap               =       dlpi_sap;
  dlpi_co_rr_request->dev_name_len      =       strlen(rem_dlpi_device);
  strcpy(dlpi_co_rr_request->dlpi_device,
	 rem_dlpi_device);
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_co_rr_request->dlpi_device;
    lastword = initword + ((strlen(rem_dlpi_device) + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = ntohl(*charword);
    }
  }
#endif /* __alpha */
  
  if (test_time) {
    dlpi_co_rr_request->test_length	=	test_time;
  }
  else {
    dlpi_co_rr_request->test_length	=	test_trans * -1;
  }
  
  if (debug > 1) {
    fprintf(where,"netperf: send_dlpi_co_rr: requesting TCP stream test\n");
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
    if (debug)
      fprintf(where,"remote listen done.\n");
    rrw_size	=	dlpi_co_rr_response->recv_win_size;
    rsw_size	=	dlpi_co_rr_response->send_win_size;
    remote_cpu_usage=	dlpi_co_rr_response->measure_cpu;
    remote_cpu_rate = 	dlpi_co_rr_response->cpu_rate;
    
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
    
    exit(1);
  }
  
  /*Connect up to the remote port on the data descriptor  */
  
  if(dl_connect(send_descriptor,
		dlpi_co_rr_response->station_addr,
		dlpi_co_rr_response->station_addr_len) != 0) {
    fprintf(where,"send_dlpi_co_rr: connect failure\n");
    fflush(where);
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
    if((putmsg(send_descriptor,
	       0,
	       &send_message,
	       0)) != 0) {
      if (errno == EINTR) {
	/* we hit the end of a */
	/* timed test. */
	timed_out = 1;
	break;
      }
      perror("send_dlpi_co_rr: putmsg error");
      exit(1);
    }
    
    if (debug) {
      fprintf(where,"recv_message.len %d\n",recv_message.len);
      fprintf(where,"send_message.len %d\n",send_message.len);
      fflush(where);
    }
    
    /* receive the response */
    /* this needs some work with streams buffers if we are going to */
    /* support requests and responses larger than the MTU of the */
    /* network, but this can wait until later */
    rsp_bytes_left = rsp_size;
    recv_message.len = rsp_size;
    while(rsp_bytes_left > 0) {
      if((getmsg(send_descriptor,
		 0,
		 &recv_message,
		 &flags)) < 0) {
	if (errno == EINTR) {
	  /* We hit the end of a timed test. */
	  timed_out = 1;
	  break;
	}
	perror("send_dlpi_co_rr: data recv error");
	exit(1);
      }
      rsp_bytes_left -= recv_message.len;
    }	
    
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
  
  /* At this point we used to call shutdown onthe data socket to be */
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
						  0.0);
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
      remote_cpu_utilization = dlpi_co_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand = calc_service_demand((double) nummessages*1024,
						  0.0,
						  remote_cpu_utilization);
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
	      lsw_size,		/* local sendbuf size */
	      lrw_size,
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
	      rsw_size,
	      rrw_size);
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
	      lsw_size,
	      lrw_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      nummessages/elapsed_time);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rsw_size, 		/* remote recvbuf size */
	      rrw_size);
      
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
	    ksink_fmt);
  }
  /* The test is over. Kill the data descriptor */
  
  if (close(send_descriptor) == -1) {
    perror("send_dlpi_co_rr: cannot shutdown tcp stream descriptor");
  }
  
}

void
  send_dlpi_cl_stream(remote_host)
char	remote_host[];
{
  /************************************************************************/
  /*									*/
  /*               	UDP Unidirectional Send Test                    */
  /*									*/
  /************************************************************************/
  char *tput_title =
    "Window  Message  Elapsed      Messages                \n\
Size    Size     Time         Okay Errors   Throughput\n\
frames  bytes    secs            #      #   %s/sec\n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1 =
    "%5d   %5d    %-7.2f   %7d %6d    %7.2f\n\
%5d            %-7.2f   %7d           %7.2f\n\n";
  
  
  char *cpu_title =
    "Window  Message  Elapsed      Messages                   CPU     Service\n\
Size    Size     Time         Okay Errors   Throughput   Util    Demand\n\
frames  bytes    secs            #      #   %s/sec   %%       us/KB\n\n";
  
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
  
  
  int	*message_int_ptr;
  char	*message_ptr;
  char	*message_base;
  char  sctl_data[BUFSIZ];
  struct strbuf send_message;
  struct strbuf sctl_message;
  dl_unitdata_req_t *data_req = (dl_unitdata_req_t *)sctl_data;
  
  char dlsap[BUFSIZ];
  int  dlsap_len;
  int	message_offset;
  int	message_max_offset;
  int	failed_sends;
  int	failed_cows;
  int 	messages_sent;
  int 	data_descriptor;
  
  
#ifdef INTERVALS
  int	interval_count;
#endif /* INTERVALS */
#ifdef DIRTY
  int	i;
#endif /* DIRTY */
  
  struct	dlpi_cl_stream_request_struct	*dlpi_cl_stream_request;
  struct	dlpi_cl_stream_response_struct	*dlpi_cl_stream_response;
  struct	dlpi_cl_stream_results_struct	*dlpi_cl_stream_results;
  
  dlpi_cl_stream_request	= (struct dlpi_cl_stream_request_struct *)netperf_request.content.test_specific_data;
  dlpi_cl_stream_response	= (struct dlpi_cl_stream_response_struct *)netperf_response.content.test_specific_data;
  dlpi_cl_stream_results	= (struct dlpi_cl_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if ( print_headers ) {
    printf("DLPI CL UNIDIRECTIONAL SEND TEST\n");
    if (local_cpu_usage || remote_cpu_usage)
      printf(cpu_title,format_units());
    else
      printf(tput_title,format_units());
  }	
  
  failed_sends	= 0;
  messages_sent	= 0;
  times_up	= 0;
  
  /*set up the data descriptor			*/
  
  data_descriptor = dl_open(loc_dlpi_device,loc_ppa);
  if (data_descriptor < 0){
    perror("send_dlpi_cl_stream: data descriptor");
    exit(1);
  }
  
  /* bind the puppy and get the assigned dlsap */
  dlsap_len = BUFSIZ;
  if (dl_bind(data_descriptor, 
              dlpi_sap, DL_CLDLS, dlsap, &dlsap_len) != 0) {
    fprintf(where,"send_dlpi_cl_stream: bind failure\n");
    fflush(where);
    exit(1);
  }
  
  /* Modify the local socket size (SNDBUF size)    */
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  if (lsw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_cl_stream: descriptor send size altered from system default...\n");
      fprintf(where,"                          send: %d\n",lsw_size);
    }
  }
  if (lrw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_cl_stream: descriptor recv size altered from system default...\n");
      fprintf(where,"                          recv: %d\n",lrw_size);
    }
  }
  
  
  /* Now, we will find-out what the size actually became, and report */
  /* that back to the user. If the call fails, we will just report a -1 */
  /* back to the initiator for the recv buffer size. */
  
#else /* DL_HP_SET_LOCAL_WIN_REQ */
  
  lsw_size = -1;
  lrw_size = -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
  /* now, we want to see if we need to set the send_size */
  if (send_size == 0) {
    send_size = 1024;
  }
  
  
  /* set-up the data buffer with the requested alignment and offset, */
  /* most of the numbers here are just a hack to pick something nice */
  /* and big in an attempt to never try to send a buffer a second time */
  /* before it leaves the node...unless the user set the width */
  /* explicitly. */
  if (send_width == 0) send_width = 32;
  message_base = (char *)malloc(send_size * (send_width + 1) + local_send_align + local_send_offset);
  message_ptr = (char *)(( (long) message_base + 
			  (long) local_send_align - 1) &	
			 ~((long) local_send_align - 1));
  message_ptr = message_ptr + local_send_offset;
  message_base = message_ptr;
  send_message.maxlen = send_size;
  send_message.len = send_size;
  send_message.buf = message_base;
  
  sctl_message.maxlen = BUFSIZ;
  sctl_message.len    = 0;
  sctl_message.buf    = sctl_data;
  
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
  
  netperf_request.content.request_type                 = DO_DLPI_CL_STREAM;
  dlpi_cl_stream_request->recv_win_size	        = rrw_size;
  dlpi_cl_stream_request->message_size	        = send_size;
  dlpi_cl_stream_request->recv_alignment	= remote_recv_align;
  dlpi_cl_stream_request->recv_offset		= remote_recv_offset;
  dlpi_cl_stream_request->measure_cpu		= remote_cpu_usage;
  dlpi_cl_stream_request->cpu_rate		= remote_cpu_rate;
  dlpi_cl_stream_request->ppa                   = rem_ppa;
  dlpi_cl_stream_request->sap                   = dlpi_sap;
  dlpi_cl_stream_request->dev_name_len          = strlen(rem_dlpi_device);
  strcpy(dlpi_cl_stream_request->dlpi_device,
	 rem_dlpi_device);
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_cl_stream_request->dlpi_device;
    lastword = initword + ((strlen(rem_dlpi_device) + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = ntohl(*charword);
    }
  }
#endif /* __alpha */
  
  if (test_time) {
    dlpi_cl_stream_request->test_length	=	test_time;
  }
  else {
    dlpi_cl_stream_request->test_length	=	test_bytes * -1;
  }
  
  
  send_request();
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"send_dlpi_cl_stream: remote data connection done.\n");
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("send_dlpi_cl_stream: error on remote");
    exit(1);
  }
  
  /* place some of the remote's addressing information into the send */
  /* structure so our sends can be sent to the correct place. Also get */
  /* some of the returned socket buffer information for user display. */
  
  /* set-up the destination addressing control info */
  data_req->dl_primitive = DL_UNITDATA_REQ;
  bcopy((char *)(dlpi_cl_stream_response->station_addr),
	((char *)data_req + sizeof(dl_unitdata_req_t)),
	dlpi_cl_stream_response->station_addr_len);
  data_req->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
  data_req->dl_dest_addr_length = dlpi_cl_stream_response->station_addr_len;
  /* there is a dl_priority structure too, but I am ignoring it for */
  /* the time being. */
  sctl_message.len = sizeof(dl_unitdata_req_t) + 
    data_req->dl_dest_addr_length;
  
  rrw_size	        = dlpi_cl_stream_response->recv_win_size;
  rsw_size	        = dlpi_cl_stream_response->send_win_size;
  remote_cpu_rate	= dlpi_cl_stream_response->cpu_rate;
  
  
  /* set up the timer to call us after test_time	*/
  start_timer(test_time);
  
  /* Get the start count for the idle counter and the start time */
  
  cpu_start(local_cpu_usage);
  
#ifdef INTERVALS
  interval_count = interval_burst;
#endif /* INTERVALS */
  
  /* Send datagrams like there was no tomorrow */
  while (!times_up) {
#ifdef DIRTY
    /* we want to dirty some number of consecutive integers in the buffer */
    /* we are about to send. we may also want to bring some number of */
    /* them cleanly into the cache. The clean ones will follow any dirty */
    /* ones into the cache. */
    message_int_ptr = (int *)message_ptr;
    for (i = 0; i < loc_dirty_count; i++) {
      *message_int_ptr = 4;
      message_int_ptr++;
    }
    for (i = 0; i < loc_clean_count; i++) {
      loc_dirty_count = *message_int_ptr;
      message_int_ptr++;
    }
#endif /* DIRTY */
    if (putmsg(data_descriptor,
	       &sctl_message,
	       &send_message,
	       0)  != 0) {
      if (errno == EINTR) {
	break;
      }
      if (errno == ENOBUFS) {
	/* we might not ever hit this with STREAMS, it would probably */
	/* be better to do a getinfo request at the end of the test to */
	/* get all sorts of gory statistics. in the meantime, we will */
	/* keep this code in place. */
	failed_sends++;
	continue;
      }
      perror("send_dlpi_cl_stream: data send error");
      if (debug) {
	fprintf(where,"messages_sent %u\n",messages_sent);
	fflush(where);
      }
      exit(1);
    }
    messages_sent++;          
    
    /* now we want to move our pointer to the next position in the */
    /* data buffer...since there was a successful send */
    
    
#ifdef INTERVALS
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
      fprintf(where,"send_dlpi_cl_stream: remote results obtained\n");
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("send_dlpi_cl_stream: error on remote");
    exit(1);
  }
  
  bytes_sent	= send_size * messages_sent;
  local_thruput	= calc_thruput(bytes_sent);
  
  messages_recvd	= dlpi_cl_stream_results->messages_recvd;
  bytes_recvd	= send_size * messages_recvd;
  
  /* we asume that the remote ran for as long as we did */
  
  remote_thruput	= calc_thruput(bytes_recvd);
  
  /* print the results for this descriptor and message size */
  
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
						      0.0);
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
      
      remote_cpu_utilization	= dlpi_cl_stream_results->cpu_util;
      remote_service_demand	= calc_service_demand(bytes_recvd,
						      0.0,
						      remote_cpu_utilization);
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
	      lsw_size,		/* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time,		/* how long was the test */
	      messages_sent,
	      failed_sends,
	      local_thruput, 		/* what was the xfer rate */
	      local_cpu_utilization,	/* local cpu */
	      local_service_demand,	/* local service demand */
	      rrw_size,
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
	      lsw_size, 		/* local sendbuf size */
	      send_size,		/* how large were the sends */
	      elapsed_time, 		/* how long did it take */
	      messages_sent,
	      failed_sends,
	      local_thruput,
	      rrw_size, 		/* remote recvbuf size */
	      elapsed_time,
	      messages_recvd,
	      remote_thruput
	      );
      break;
    }
  }
}

int
  recv_dlpi_cl_stream()
{
  
  char message[MAXMESSAGESIZE+MAXALIGNMENT+MAXOFFSET];
  int	data_descriptor;
  int	len;
  char	*message_ptr;
  char  rctl_data[BUFSIZ];
  struct strbuf recv_message;
  struct strbuf rctl_message;
  int flags = 0;
  /* these are to make reading some of the DLPI control messages easier */
  dl_unitdata_ind_t *data_ind = (dl_unitdata_ind_t *)rctl_data;
  dl_uderror_ind_t  *uder_ind = (dl_uderror_ind_t *)rctl_data;
  
  int	bytes_received = 0;
  float	elapsed_time;
  
  int	message_size;
  int	messages_recvd = 0;
  int	measure_cpu;
  
  struct	dlpi_cl_stream_request_struct	*dlpi_cl_stream_request;
  struct	dlpi_cl_stream_response_struct	*dlpi_cl_stream_response;
  struct	dlpi_cl_stream_results_struct	*dlpi_cl_stream_results;
  
  dlpi_cl_stream_request	= (struct dlpi_cl_stream_request_struct *)netperf_request.content.test_specific_data;
  dlpi_cl_stream_response	= (struct dlpi_cl_stream_response_struct *)netperf_response.content.test_specific_data;
  dlpi_cl_stream_results	= (struct dlpi_cl_stream_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dlpi_cl_stream: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen descriptor with all the desired */
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
    fprintf(where,"recv_dlpi_cl_stream: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = DLPI_CL_STREAM_RESPONSE;
  
  if (debug > 2) {
    fprintf(where,"recv_dlpi_cl_stream: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variable to be at the desired */
  /* alignment with the desired offset. */
  
  if (debug > 1) {
    fprintf(where,"recv_dlpi_cl_stream: requested alignment of %d\n",
	    dlpi_cl_stream_request->recv_alignment);
    fflush(where);
  }
  message_ptr = (char *)(((long)message + 
			  (long ) dlpi_cl_stream_request->recv_alignment -1) & 
			 ~((long) dlpi_cl_stream_request->recv_alignment - 1));
  message_ptr = message_ptr + dlpi_cl_stream_request->recv_offset;

  if (dlpi_cl_stream_request->message_size > 0) {
    recv_message.maxlen = dlpi_cl_stream_request->message_size;
  }
  else {
    recv_message.maxlen = 4096;
  }
  recv_message.len    = 0;
  recv_message.buf    = message_ptr;
  
  rctl_message.maxlen = BUFSIZ;
  rctl_message.len    = 0;
  rctl_message.buf    = rctl_data;
  
  if (debug > 1) {
    fprintf(where,
	    "recv_dlpi_cl_stream: receive alignment and offset set...\n");
    fflush(where);
  }
  
  if (debug > 1) {
    fprintf(where,"recv_dlpi_cl_stream: grabbing a descriptor...\n");
    fflush(where);
  }
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_cl_stream_request->dlpi_device;
    lastword = initword + ((dlpi_cl_stream_request->dev_name_len + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = htonl(*charword);
    }
  }
#endif /* __alpha */
  
  data_descriptor = dl_open(dlpi_cl_stream_request->dlpi_device,
			    dlpi_cl_stream_request->ppa);
  if (data_descriptor < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  /* The initiator may have wished-us to modify the window */
  /* sizes. We should give it a shot. If he didn't ask us to change the */
  /* sizes, we should let him know what sizes were in use at this end. */
  /* If none of this code is compiled-in, then we will tell the */
  /* initiator that we were unable to play with the sizes by */
  /* setting the size in the response to -1. */
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  
  if (dlpi_cl_stream_request->recv_win_size) {
    dlpi_cl_stream_response->recv_win_size	= -1;
  }
  
#else /* the system won't let us play with the buffers */
  
  dlpi_cl_stream_response->recv_win_size	= -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
  dlpi_cl_stream_response->test_length = dlpi_cl_stream_request->test_length;
  
  /* bind the sap and retrieve the dlsap assigned by the system  */
  dlpi_cl_stream_response->station_addr_len = 14; /* arbitrary */
  if (dl_bind(data_descriptor,
	      dlpi_cl_stream_request->sap,
	      DL_CLDLS,
	      (char *)dlpi_cl_stream_response->station_addr,
	      &dlpi_cl_stream_response->station_addr_len) != 0) {
    fprintf(where,"send_dlpi_cl_stream: bind failure\n");
    fflush(where);
    exit(1);
  }
  
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a -1 to */
  /* the initiator. */
  
  dlpi_cl_stream_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (dlpi_cl_stream_request->measure_cpu) {
    /* We will pass the rate into the calibration routine. If the */
    /* user did not specify one, it will be 0.0, and we will do a */
    /* "real" calibration. Otherwise, all it will really do is */
    /* store it away... */
    dlpi_cl_stream_response->measure_cpu = 1;
    dlpi_cl_stream_response->cpu_rate = calibrate_local_cpu(dlpi_cl_stream_request->cpu_rate);
  }
  
  message_size	= dlpi_cl_stream_request->message_size;
  test_time	= dlpi_cl_stream_request->test_length;
  
  send_response();
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(dlpi_cl_stream_request->measure_cpu);
  
  /* The loop will exit when the timer pops, or if we happen to recv a */
  /* message of less than send_size bytes... */
  
  times_up = 0;
  start_timer(test_time + PAD_TIME);
  
  if (debug) {
    fprintf(where,"recv_dlpi_cl_stream: about to enter inner sanctum.\n");
    fflush(where);
  }
  
  while (!times_up) {
    if((getmsg(data_descriptor, 
	       &rctl_message,    
	       &recv_message,
	       &flags) != 0) || 
       (data_ind->dl_primitive != DL_UNITDATA_IND)) {
      if (errno == EINTR) {
	/* Again, we have likely hit test-end time */
	break;
      }
      fprintf(where,
	      "dlpi_recv_cl_stream: getmsg failure: errno %d primitive 0x%x\n",
	      errno,
	      data_ind->dl_primitive);
      fflush(where);
      netperf_response.content.serv_errno = 996;
      send_response();
      exit(1);
    }
    messages_recvd++;
  }
  
  if (debug) {
    fprintf(where,"recv_dlpi_cl_stream: got %d messages.\n",messages_recvd);
    fflush(where);
  }
  
  
  /* The loop now exits due timer or < send_size bytes received. */
  
  cpu_stop(dlpi_cl_stream_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended on a timer, subtract the PAD_TIME */
    elapsed_time -= (float)PAD_TIME;
  }
  else {
    alarm(0);
  }
  
  if (debug) {
    fprintf(where,"recv_dlpi_cl_stream: test ended in %f seconds.\n",elapsed_time);
    fflush(where);
  }
  
  
  /* We will count the "off" message */
  bytes_received = (messages_recvd * message_size) + len;
  
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_cl_stream: got %d bytes\n",
	    bytes_received);
    fflush(where);
  }
  
  netperf_response.content.response_type		= DLPI_CL_STREAM_RESULTS;
  dlpi_cl_stream_results->bytes_received	= bytes_received;
  dlpi_cl_stream_results->messages_recvd	= messages_recvd;
  dlpi_cl_stream_results->elapsed_time	= elapsed_time;
  if (dlpi_cl_stream_request->measure_cpu) {
    dlpi_cl_stream_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  else {
    dlpi_cl_stream_results->cpu_util	= -1.0;
  }
  
  if (debug > 1) {
    fprintf(where,
	    "recv_dlpi_cl_stream: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}

int send_dlpi_cl_rr(remote_host)
     char	remote_host[];
{
  
  char *tput_title = "\
Local /Remote\n\
Window Size   Request  Resp.   Elapsed  Trans.\n\
Send   Recv   Size     Size    Time     Rate         \n\
frames frames bytes    bytes   secs.    per sec   \n\n";
  
  char *tput_fmt_0 =
    "%7.2f\n";
  
  char *tput_fmt_1_line_1 = "\
%-6d %-6d %-6d   %-6d  %-6.2f   %7.2f   \n";
  char *tput_fmt_1_line_2 = "\
%-6d %-6d\n";
  
  char *cpu_title = "\
Local /Remote\n\
Window Size   Request Resp.  Elapsed Trans.   CPU    CPU    S.dem   S.dem\n\
Send   Recv   Size    Size   Time    Rate     local  remote local   remote\n\
frames frames bytes   bytes  secs.   per sec  %%      %%      us/Tr   us/Tr\n\n";
  
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
%5d  %5d   %5d  %5d";
  
  
  float			elapsed_time;
  
  int   dlsap_len;
  int   flags = 0;
  char	*send_message_ptr;
  char	*recv_message_ptr;
  char	*temp_message_ptr;
  char  sctl_data[BUFSIZ];
  char  rctl_data[BUFSIZ];
  char  dlsap[BUFSIZ];
  struct strbuf send_message;
  struct strbuf recv_message;
  struct strbuf sctl_message;
  struct strbuf rctl_message;
  
  /* these are to make reading some of the DLPI control messages easier */
  dl_unitdata_ind_t *data_ind = (dl_unitdata_ind_t *)rctl_data;
  dl_unitdata_req_t *data_req = (dl_unitdata_req_t *)sctl_data;
  dl_uderror_ind_t  *uder_ind = (dl_uderror_ind_t *)rctl_data;
  
  int	nummessages;
  int	send_descriptor;
  int	trans_remaining;
  int	bytes_xferd;
  
  float	local_cpu_utilization;
  float	local_service_demand;
  float	remote_cpu_utilization;
  float	remote_service_demand;
  float	thruput;
  
#ifdef INTERVALS
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
#endif /* INTERVALS */
  
  struct	dlpi_cl_rr_request_struct	*dlpi_cl_rr_request;
  struct	dlpi_cl_rr_response_struct	*dlpi_cl_rr_response;
  struct	dlpi_cl_rr_results_struct	*dlpi_cl_rr_result;
  
  dlpi_cl_rr_request	= 
    (struct dlpi_cl_rr_request_struct *)netperf_request.content.test_specific_data;
  dlpi_cl_rr_response	= 
    (struct dlpi_cl_rr_response_struct *)netperf_response.content.test_specific_data;
  dlpi_cl_rr_result	= 
    (struct dlpi_cl_rr_results_struct *)netperf_response.content.test_specific_data;
  
  /* we want to zero out the times, so we can detect unused entries. */
#ifdef INTERVALS
  time_index = 0;
  while (time_index < MAX_KEPT_TIMES) {
    kept_times[time_index] = 0;
    time_index += 1;
  }
  time_index = 0;
#endif /* INTERVALS */
  
  if (print_headers) {
    fprintf(where,"DLPI CL REQUEST/RESPONSE TEST\n");
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
  temp_message_ptr = (char *)malloc(req_size+MAXALIGNMENT+MAXOFFSET);
  send_message_ptr = (char *)(( (long)temp_message_ptr + 
			       (long) local_send_align - 1) &	
			      ~((long) local_send_align - 1));
  send_message_ptr = send_message_ptr + local_send_offset;
  send_message.maxlen = req_size;
  send_message.len    = req_size;
  send_message.buf    = send_message_ptr;
  
  temp_message_ptr = (char *)malloc(rsp_size+MAXALIGNMENT+MAXOFFSET);
  recv_message_ptr = (char *)(( (long)temp_message_ptr + 
			       (long) local_recv_align - 1) &	
			      ~((long) local_recv_align - 1));
  recv_message_ptr = recv_message_ptr + local_recv_offset;
  recv_message.maxlen = rsp_size;
  recv_message.len    = 0;
  recv_message.buf    = recv_message_ptr;
  
  sctl_message.maxlen = BUFSIZ;
  sctl_message.len    = 0;
  sctl_message.buf    = sctl_data;
  
  rctl_message.maxlen = BUFSIZ;
  rctl_message.len    = 0;
  rctl_message.buf    = rctl_data;
  
  /* lets get ourselves a file descriptor */
  
  send_descriptor = dl_open(loc_dlpi_device,loc_ppa);
  if (send_descriptor < 0){
    perror("netperf: send_dlpi_cl_rr: dlpi cl rr send descriptor");
    exit(1);
  }
  
  if (debug) {
    fprintf(where,"send_dlpi_cl_rr: send_descriptor obtained...\n");
  }
  
  /* bind the sap to the descriptor and get the dlsap */
  dlsap_len = BUFSIZ;
  if (dl_bind(send_descriptor,
	      dlpi_sap,
	      DL_CLDLS,
	      dlsap,
	      &dlsap_len) != 0) {
    fprintf(where,"send_dlpi_cl_rr: bind failure\n");
    fflush(where);
    exit(1);
  }
  
  /* Modify the local socket size. If the user has not requested that */
  /* the socket buffers be altered, we will try to find-out what their */
  /* values are. If we cannot touch the socket buffer in any way, we */
  /* will set the values to -1 to indicate that.  The receive socket */
  /* must have enough space to hold addressing information so += a */
  /* sizeof struct sockaddr_in to it. */ 
  
  /* this is actually nothing code, and should be replaced with the */
  /* alalagous calls in the STREAM test where the window size is set */
  /* with the HP DLPI Extension. raj 8/94 */
#ifdef SO_SNDBUF
  if (lsw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_cl_rr: local window size altered from system default...\n");
      fprintf(where,"                          window: %d\n",lsw_size);
    }
  }
  if (lrw_size > 0) {
    if (debug > 1) {
      fprintf(where,"netperf: send_dlpi_cl_rr: remote window size altered from system default...\n");
      fprintf(where,"                          remote: %d\n",lrw_size);
    }
  }
  
  
  /* Now, we will find-out what the size actually became, and report */
  /* that back to the user. If the call fails, we will just report a -1 */
  /* back to the initiator for the recv buffer size. */
  
  if (debug) {
    fprintf(where,"netperf: send_dlpi_cl_rr: socket sizes determined...\n");
    fprintf(where,"         send: %d recv: %d\n",lsw_size,lrw_size);
  }
  
#else /* SO_SNDBUF */
  
  lsw_size = -1;
  lrw_size = -1;
  
#endif /* SO_SNDBUF */
  
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
  
  netperf_request.content.request_type	        =	DO_DLPI_CL_RR;
  dlpi_cl_rr_request->recv_win_size	=	rrw_size;
  dlpi_cl_rr_request->send_win_size	=	rsw_size;
  dlpi_cl_rr_request->recv_alignment	=	remote_recv_align;
  dlpi_cl_rr_request->recv_offset	=	remote_recv_offset;
  dlpi_cl_rr_request->send_alignment	=	remote_send_align;
  dlpi_cl_rr_request->send_offset	=	remote_send_offset;
  dlpi_cl_rr_request->request_size	=	req_size;
  dlpi_cl_rr_request->response_size	=	rsp_size;
  dlpi_cl_rr_request->measure_cpu	=	remote_cpu_usage;
  dlpi_cl_rr_request->cpu_rate	        =	remote_cpu_rate;
  dlpi_cl_rr_request->ppa               =       rem_ppa;
  dlpi_cl_rr_request->sap               =       dlpi_sap;
  dlpi_cl_rr_request->dev_name_len      =       strlen(rem_dlpi_device);
  strcpy(dlpi_cl_rr_request->dlpi_device,
	 rem_dlpi_device);
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_cl_rr_request->dlpi_device;
    lastword = initword + ((strlen(rem_dlpi_device) + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = ntohl(*charword);
    }
  }
#endif /* __alpha */
  
  if (test_time) {
    dlpi_cl_rr_request->test_length	=	test_time;
  }
  else {
    dlpi_cl_rr_request->test_length	=	test_trans * -1;
  }
  
  if (debug > 1) {
    fprintf(where,"netperf: send_dlpi_cl_rr: requesting DLPI CL request/response test\n");
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
  /* being sent for the tests.                                          */
  
  recv_response();
  
  if (!netperf_response.content.serv_errno) {
    if (debug)
      fprintf(where,"remote listen done.\n");
    rrw_size	=	dlpi_cl_rr_response->recv_win_size;
    rsw_size	=	dlpi_cl_rr_response->send_win_size;
    remote_cpu_usage=	dlpi_cl_rr_response->measure_cpu;
    remote_cpu_rate = 	dlpi_cl_rr_response->cpu_rate;
    
    /* set-up the destination addressing control info */
    data_req->dl_primitive = DL_UNITDATA_REQ;
    bcopy((char *)(dlpi_cl_rr_response->station_addr),
	  ((char *)data_req + sizeof(dl_unitdata_req_t)),
	  dlpi_cl_rr_response->station_addr_len);
    data_req->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
    data_req->dl_dest_addr_length = dlpi_cl_rr_response->station_addr_len;
    /* there is a dl_priority structure too, but I am ignoring it for */
    /* the time being. */
    sctl_message.len = sizeof(dl_unitdata_req_t) + 
      data_req->dl_dest_addr_length;
  }
  else {
    errno = netperf_response.content.serv_errno;
    perror("netperf: remote error");
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
#ifdef INTERVALS
    gettimeofday(&send_time,&dummy_zone);
#endif /* INTERVALS */
    if(putmsg(send_descriptor,
	      &sctl_message,
	      &send_message,
	      0) != 0) {
      if (errno == EINTR) {
	/* We likely hit */
	/* test-end time. */
	break;
      }
      /* there is more we could do here, but it can wait */
      perror("send_dlpi_cl_rr: data send error");
      exit(1);
    }
    
    /* receive the response. at some point, we will need to handle */
    /* sending responses which are greater than the datalink MTU. we */
    /* may also want to add some DLPI error checking, but for now we */
    /* will ignore that and just let errors stop the test with little */
    /* indication of what might actually be wrong. */
    
    if((getmsg(send_descriptor, 
	       &rctl_message,    
	       &recv_message,
	       &flags) != 0) || 
       (data_ind->dl_primitive != DL_UNITDATA_IND)) {
      if (errno == EINTR) {
	/* Again, we have likely hit test-end time */
	break;
      }
      fprintf(where,
	      "send_dlpi_cl_rr: recv error: errno %d primitive 0x%x\n",
	      errno,
	      data_ind->dl_primitive);
      fflush(where);
      exit(1);
    }
#ifdef INTERVALS
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
#endif /* INTERVALS */
    nummessages++;          
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug > 3) {
      fprintf(where,"Transaction %d completed\n",nummessages);
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
  /* the remote, but it should be the case that for a UDP stream test, */
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
						  0.0);
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
      remote_cpu_utilization = dlpi_cl_rr_result->cpu_util;
      /* since calc_service demand is doing ms/Kunit we will */
      /* multiply the number of transaction by 1024 to get */
      /* "good" numbers */
      remote_service_demand  = calc_service_demand((double) nummessages*1024,
						   0.0,
						   remote_cpu_utilization);
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
	      lsw_size,		/* local sendbuf size */
	      lrw_size,
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
	      rsw_size,
	      rrw_size);
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
	      lsw_size,
	      lrw_size,
	      req_size,		/* how large were the requests */
	      rsp_size,		/* how large were the responses */
	      elapsed_time, 		/* how long did it take */
	      nummessages/elapsed_time);
      fprintf(where,
	      tput_fmt_1_line_2,
	      rsw_size, 		/* remote recvbuf size */
	      rrw_size);
      
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
    /* UDP statistics, the alignments of the sends and receives */
    /* and all that sort of rot... */
    
#ifdef INTERVALS
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
}

int 
  recv_dlpi_cl_rr()
{
  
  char message[MAXMESSAGESIZE+MAXALIGNMENT+MAXOFFSET];
  int	data_descriptor;
  int   flags = 0;
  int	measure_cpu;
  
  char	*recv_message_ptr;
  char	*send_message_ptr;
  char  sctl_data[BUFSIZ];
  char  rctl_data[BUFSIZ];
  char  dlsap[BUFSIZ];
  struct strbuf send_message;
  struct strbuf recv_message;
  struct strbuf sctl_message;
  struct strbuf rctl_message;
  
  /* these are to make reading some of the DLPI control messages easier */
  dl_unitdata_ind_t *data_ind = (dl_unitdata_ind_t *)rctl_data;
  dl_unitdata_req_t *data_req = (dl_unitdata_req_t *)sctl_data;
  dl_uderror_ind_t  *uder_ind = (dl_uderror_ind_t *)rctl_data;
  
  int	trans_received;
  int	trans_remaining;
  float	elapsed_time;
  
  struct	dlpi_cl_rr_request_struct	*dlpi_cl_rr_request;
  struct	dlpi_cl_rr_response_struct	*dlpi_cl_rr_response;
  struct	dlpi_cl_rr_results_struct	*dlpi_cl_rr_results;
  
  dlpi_cl_rr_request  = 
    (struct dlpi_cl_rr_request_struct *)netperf_request.content.test_specific_data;
  dlpi_cl_rr_response = 
    (struct dlpi_cl_rr_response_struct *)netperf_response.content.test_specific_data;
  dlpi_cl_rr_results  = 
    (struct dlpi_cl_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dlpi_cl_rr: entered...\n");
    fflush(where);
  }
  
  /* We want to set-up the listen descriptor with all the desired */
  /* parameters and then let the initiator know that all is ready. If */
  /* socket size defaults are to be used, then the initiator will have */
  /* sent us 0's. If the descriptor sizes cannot be changed, then we will */
  /* send-back what they are. If that information cannot be determined, */
  /* then we send-back -1's for the sizes. If things go wrong for any */
  /* reason, we will drop back ten yards and punt. */
  
  /* If anything goes wrong, we want the remote to know about it. It */
  /* would be best if the error that the remote reports to the user is */
  /* the actual error we encountered, rather than some bogus unexpected */
  /* response type message. */
  
  if (debug) {
    fprintf(where,"recv_dlpi_cl_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = DLPI_CL_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_dlpi_cl_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_cl_rr: requested recv alignment of %d offset %d\n",
	    dlpi_cl_rr_request->recv_alignment,
	    dlpi_cl_rr_request->recv_offset);
    fprintf(where,
	    "recv_dlpi_cl_rr: requested send alignment of %d offset %d\n",
	    dlpi_cl_rr_request->send_alignment,
	    dlpi_cl_rr_request->send_offset);
    fflush(where);
  }
  recv_message_ptr = (char *)(( (long)message + 
			       (long) dlpi_cl_rr_request->recv_alignment -1) & 
			      ~((long) dlpi_cl_rr_request->recv_alignment - 1));
  recv_message_ptr = recv_message_ptr + dlpi_cl_rr_request->recv_offset;
  recv_message.maxlen = dlpi_cl_rr_request->request_size;
  recv_message.len    = 0;
  recv_message.buf    = recv_message_ptr;
  
  send_message_ptr = (char *)(( (long)message + 
			       (long) dlpi_cl_rr_request->send_alignment -1) & 
			      ~((long) dlpi_cl_rr_request->send_alignment - 1));
  send_message_ptr = send_message_ptr + dlpi_cl_rr_request->send_offset;
  send_message.maxlen = dlpi_cl_rr_request->response_size;
  send_message.len    = dlpi_cl_rr_request->response_size;
  send_message.buf    = send_message_ptr;
  
  sctl_message.maxlen = BUFSIZ;
  sctl_message.len    = 0;
  sctl_message.buf    = sctl_data;
  
  rctl_message.maxlen = BUFSIZ;
  rctl_message.len    = 0;
  rctl_message.buf    = rctl_data;
  
  if (debug) {
    fprintf(where,"recv_dlpi_cl_rr: receive alignment and offset set...\n");
    fprintf(where,"recv_dlpi_cl_rr: grabbing a socket...\n");
    fflush(where);
  }
  
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_cl_rr_request->dlpi_device;
    lastword = initword + ((dlpi_cl_rr_request->dev_name_len + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = htonl(*charword);
    }
  }
#endif /* __alpha */
  
  data_descriptor = dl_open(dlpi_cl_rr_request->dlpi_device,
			    dlpi_cl_rr_request->ppa);
  if (data_descriptor < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  
  /* The initiator may have wished-us to modify the window */
  /* sizes. We should give it a shot. If he didn't ask us to change the */
  /* sizes, we should let him know what sizes were in use at this end. */
  /* If none of this code is compiled-in, then we will tell the */
  /* initiator that we were unable to play with the sizes by */
  /* setting the size in the response to -1. */
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  
  if (dlpi_cl_rr_request->recv_win_size) {
  }
  
  if (dlpi_cl_rr_request->send_win_size) {
  }
  
  /* Now, we will find-out what the sizes actually became, and report */
  /* them back to the user. If the calls fail, we will just report a -1 */
  /* back to the initiator for the buffer size. */
  
#else /* the system won't let us play with the buffers */
  
  dlpi_cl_rr_response->recv_win_size	= -1;
  dlpi_cl_rr_response->send_win_size	= -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
  /* bind the sap and retrieve the dlsap assigned by the system  */
  dlpi_cl_rr_response->station_addr_len = 14; /* arbitrary */
  if (dl_bind(data_descriptor,
	      dlpi_cl_rr_request->sap,
	      DL_CLDLS,
	      (char *)dlpi_cl_rr_response->station_addr,
	      &dlpi_cl_rr_response->station_addr_len) != 0) {
    fprintf(where,"send_dlpi_cl_rr: bind failure\n");
    fflush(where);
    exit(1);
  }
  
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  dlpi_cl_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (dlpi_cl_rr_request->measure_cpu) {
    dlpi_cl_rr_response->measure_cpu = 1;
    dlpi_cl_rr_response->cpu_rate = calibrate_local_cpu(dlpi_cl_rr_request->cpu_rate);
  }
  
  send_response();
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start receiving. */
  
  cpu_start(dlpi_cl_rr_request->measure_cpu);
  
  if (dlpi_cl_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(dlpi_cl_rr_request->test_length + PAD_TIME);
  }
else {
  times_up = 1;
  trans_remaining = dlpi_cl_rr_request->test_length * -1;
}
  
  while ((!times_up) || (trans_remaining > 0)) {
    
    /* receive the request from the other side. at some point we need */
    /* to handle "logical" requests and responses which are larger */
    /* than the data link MTU */
    
    if((getmsg(data_descriptor, 
	       &rctl_message,    
	       &recv_message,
	       &flags) != 0) || 
       (data_ind->dl_primitive != DL_UNITDATA_IND)) {
      if (errno == EINTR) {
	/* Again, we have likely hit test-end time */
	break;
      }
      fprintf(where,
	      "dlpi_recv_cl_rr: getmsg failure: errno %d primitive 0x%x\n",
	      errno,
	      data_ind->dl_primitive);
      fprintf(where,
	      "                 recevied %u transactions\n",
	      trans_received);
      fflush(where);
      netperf_response.content.serv_errno = 995;
      send_response();
      exit(1);
    }
    
    /* Now, send the response to the remote. first copy the dlsap */
    /* information from the receive to the sending control message */
    
    data_req->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
    bcopy((char *)data_ind + data_ind->dl_src_addr_offset,
	  (char *)data_req + data_req->dl_dest_addr_offset,
	  data_ind->dl_src_addr_length);
    data_req->dl_dest_addr_length = data_ind->dl_src_addr_length;
    data_req->dl_primitive = DL_UNITDATA_REQ;
    sctl_message.len = sizeof(dl_unitdata_req_t) +
      data_ind->dl_src_addr_length;
    if(putmsg(data_descriptor,
	      &sctl_message,
	      &send_message,
	      0) != 0) {
      if (errno == EINTR) {
	/* We likely hit */
	/* test-end time. */
	break;
      }
      /* there is more we could do here, but it can wait */
      fprintf(where,
	      "dlpi_recv_cl_rr: putmsg failure: errno %d\n",
	      errno);
      fflush(where);
      netperf_response.content.serv_errno = 993;
      send_response();
      exit(1);
    }
    
    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug) {
      fprintf(where,
	      "recv_dlpi_cl_rr: Transaction %d complete.\n",
	      trans_received);
      fflush(where);
    }
    
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(dlpi_cl_rr_request->measure_cpu,&elapsed_time);
  
  if (times_up) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_cl_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  dlpi_cl_rr_results->bytes_received	= (trans_received * 
					   (dlpi_cl_rr_request->request_size + 
					    dlpi_cl_rr_request->response_size));
  dlpi_cl_rr_results->trans_received	= trans_received;
  dlpi_cl_rr_results->elapsed_time	= elapsed_time;
  if (dlpi_cl_rr_request->measure_cpu) {
    dlpi_cl_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_cl_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}

int 
  recv_dlpi_co_rr()
{
  
  char message[MAXMESSAGESIZE+MAXALIGNMENT+MAXOFFSET];
  int	s_listen,data_descriptor;
  
  int	measure_cpu;
  
  int   flags = 0;
  char	*recv_message_ptr;
  char	*send_message_ptr;
  struct strbuf send_message;
  struct strbuf recv_message;
  
  int	trans_received;
  int	trans_remaining;
  int	request_bytes_remaining;
  int	timed_out = 0;
  float	elapsed_time;
  
  struct	dlpi_co_rr_request_struct	*dlpi_co_rr_request;
  struct	dlpi_co_rr_response_struct	*dlpi_co_rr_response;
  struct	dlpi_co_rr_results_struct	*dlpi_co_rr_results;
  
  dlpi_co_rr_request	= (struct dlpi_co_rr_request_struct *)netperf_request.content.test_specific_data;
  dlpi_co_rr_response	= (struct dlpi_co_rr_response_struct *)netperf_response.content.test_specific_data;
  dlpi_co_rr_results	= (struct dlpi_co_rr_results_struct *)netperf_response.content.test_specific_data;
  
  if (debug) {
    fprintf(where,"netserver: recv_dlpi_co_rr: entered...\n");
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
    fprintf(where,"recv_dlpi_co_rr: setting the response type...\n");
    fflush(where);
  }
  
  netperf_response.content.response_type = DLPI_CO_RR_RESPONSE;
  
  if (debug) {
    fprintf(where,"recv_dlpi_co_rr: the response type is set...\n");
    fflush(where);
  }
  
  /* We now alter the message_ptr variables to be at the desired */
  /* alignments with the desired offsets. */
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_co_rr: requested recv alignment of %d offset %d\n",
	    dlpi_co_rr_request->recv_alignment,
	    dlpi_co_rr_request->recv_offset);
    fprintf(where,
	    "recv_dlpi_co_rr: requested send alignment of %d offset %d\n",
	    dlpi_co_rr_request->send_alignment,
	    dlpi_co_rr_request->send_offset);
    fflush(where);
  }
  recv_message_ptr = (char *)(( (long)message + 
			       (long) dlpi_co_rr_request->recv_alignment -1) & 
			      ~((long) dlpi_co_rr_request->recv_alignment - 1));
  recv_message_ptr = recv_message_ptr + dlpi_co_rr_request->recv_offset;
  recv_message.maxlen = dlpi_co_rr_request->request_size;
  recv_message.len    = 0;
  recv_message.buf    = recv_message_ptr;
  
  send_message_ptr = (char *)(( (long)message + 
			       (long) dlpi_co_rr_request->send_alignment -1) & 
			      ~((long) dlpi_co_rr_request->send_alignment - 1));
  send_message_ptr = send_message_ptr + dlpi_co_rr_request->send_offset;
  send_message.maxlen = dlpi_co_rr_request->response_size;
  send_message.len    = dlpi_co_rr_request->response_size;
  send_message.buf    = send_message_ptr;
  
  if (debug) {
    fprintf(where,"recv_dlpi_co_rr: receive alignment and offset set...\n");
    fprintf(where,"recv_dlpi_co_rr: send_message.buf %x .len %d .maxlen %d\n",
	    send_message.buf,send_message.len,send_message.maxlen);
    fprintf(where,"recv_dlpi_co_rr: recv_message.buf %x .len %d .maxlen %d\n",
	    recv_message.buf,recv_message.len,recv_message.maxlen);
    fflush(where);
  }
  
  /* Let's clear-out our sockaddr for the sake of cleanlines. Then we */
  /* can put in OUR values !-) At some point, we may want to nail this */
  /* socket to a particular network-level address, but for now, */
  /* INADDR_ANY should be just fine. */
  
  /* Grab a socket to listen on, and then listen on it. */
  
  if (debug) {
    fprintf(where,"recv_dlpi_co_rr: grabbing a socket...\n");
    fflush(where);
  }
  
  /* lets grab a file descriptor for a particular link */
  
#ifdef __alpha
  
  /* ok - even on a DEC box, strings are strings. I din't really want */
  /* to ntohl the words of a string. since I don't want to teach the */
  /* send_ and recv_ _request and _response routines about the types, */
  /* I will put "anti-ntohl" calls here. I imagine that the "pure" */
  /* solution would be to use XDR, but I am still leary of being able */
  /* to find XDR libs on all platforms I want running netperf. raj */
  {
    int *charword;
    int *initword;
    int *lastword;
    
    initword = (int *) dlpi_co_rr_request->dlpi_device;
    lastword = initword + ((dlpi_co_rr_request->dev_name_len + 3) / 4);
    
    for (charword = initword;
	 charword < lastword;
	 charword++) {
      
      *charword = htonl(*charword);
    }
  }
#endif /* __alpha */
  
  if ((data_descriptor = dl_open(dlpi_co_rr_request->dlpi_device,
				 dlpi_co_rr_request->ppa)) < 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  /* bind the file descriptor to a sap and get the resultant dlsap */
  dlpi_co_rr_response->station_addr_len = 14; /*arbitrary needs fixing */
  if (dl_bind(data_descriptor, 
              dlpi_co_rr_request->sap, 
              DL_CODLS,
              (char *)dlpi_co_rr_response->station_addr,
              &dlpi_co_rr_response->station_addr_len) != 0) {
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  /* The initiator may have wished-us to modify the socket buffer */
  /* sizes. We should give it a shot. If he didn't ask us to change the */
  /* sizes, we should let him know what sizes were in use at this end. */
  /* If none of this code is compiled-in, then we will tell the */
  /* initiator that we were unable to play with the socket buffer by */
  /* setting the size in the response to -1. */
  
#ifdef DL_HP_SET_LOCAL_WIN_REQ
  
  if (dlpi_co_rr_request->recv_win_size) {
    /* SMOP */
  }
  
  if (dlpi_co_rr_request->send_win_size) {
    /* SMOP */
  }
  
  /* Now, we will find-out what the sizes actually became, and report */
  /* them back to the user. If the calls fail, we will just report a -1 */
  /* back to the initiator for the buffer size. */
  
#else /* the system won't let us play with the buffers */
  
  dlpi_co_rr_response->recv_win_size	= -1;
  dlpi_co_rr_response->send_win_size	= -1;
  
#endif /* DL_HP_SET_LOCAL_WIN_REQ */
  
  /* we may have been requested to enable the copy avoidance features. */
  /* can we actually do this with DLPI, the world wonders */
  
  if (dlpi_co_rr_request->so_rcvavoid) {
#ifdef SO_RCV_COPYAVOID
    dlpi_co_rr_response->so_rcvavoid = 0;
#else
    /* it wasn't compiled in... */
    dlpi_co_rr_response->so_rcvavoid = 0;
#endif
  }
  
  if (dlpi_co_rr_request->so_sndavoid) {
#ifdef SO_SND_COPYAVOID
    dlpi_co_rr_response->so_sndavoid = 0;
#else
    /* it wasn't compiled in... */
    dlpi_co_rr_response->so_sndavoid = 0;
#endif
  }
  
  netperf_response.content.serv_errno   = 0;
  
  /* But wait, there's more. If the initiator wanted cpu measurements, */
  /* then we must call the calibrate routine, which will return the max */
  /* rate back to the initiator. If the CPU was not to be measured, or */
  /* something went wrong with the calibration, we will return a 0.0 to */
  /* the initiator. */
  
  dlpi_co_rr_response->cpu_rate = 0.0; 	/* assume no cpu */
  if (dlpi_co_rr_request->measure_cpu) {
    dlpi_co_rr_response->measure_cpu = 1;
    dlpi_co_rr_response->cpu_rate = calibrate_local_cpu(dlpi_co_rr_request->cpu_rate);
  }
  
  send_response();
  
  /* accept a connection on this file descriptor. at some point, */
  /* dl_accept will "do the right thing" with the last two parms, but */
  /* for now it ignores them, so we will pass zeros. */
  
  if(dl_accept(data_descriptor, 0, 0) != 0) {
    fprintf(where,
	    "recv_dlpi_co_rr: error in accept, errno %d\n",
	    errno);
    fflush(where);
    netperf_response.content.serv_errno = errno;
    send_response();
    exit(1);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_co_rr: accept completes on the data connection.\n");
    fflush(where);
  }
  
  /* Now it's time to start receiving data on the connection. We will */
  /* first grab the apropriate counters and then start grabbing. */
  
  cpu_start(dlpi_co_rr_request->measure_cpu);
  
  /* The loop will exit when the sender does a shutdown, which will */
  /* return a length of zero   */
  
  if (dlpi_co_rr_request->test_length > 0) {
    times_up = 0;
    trans_remaining = 0;
    start_timer(dlpi_co_rr_request->test_length + PAD_TIME);
  }
else {
  times_up = 1;
  trans_remaining = dlpi_co_rr_request->test_length * -1;
}
  
  while ((!times_up) || (trans_remaining > 0)) {
    request_bytes_remaining	= dlpi_co_rr_request->request_size;
    
    /* receive the request from the other side. there needs to be some */
    /* more login in place for handling messages larger than link mtu, */
    /* but that can wait for later */
    while(request_bytes_remaining > 0) {
      if((getmsg(data_descriptor,
		 0,
		 &recv_message,
		 &flags)) < 0) {
	if (errno == EINTR) {
	  /* the timer popped */
	  timed_out = 1;
	  break;
	}
	
        if (debug) {
	  fprintf(where,"failed getmsg call errno %d\n",errno);
	  fprintf(where,"recv_message.len %d\n",recv_message.len);
	  fprintf(where,"send_message.len %d\n",send_message.len);
	  fflush(where);
        }
	
	netperf_response.content.serv_errno = errno;
	send_response();
	exit(1);
      }
      else {
	request_bytes_remaining -= recv_message.len;
      }
    }
    
    if (timed_out) {
      /* we hit the end of the test based on time - lets bail out of */
      /* here now... */ 
      break;
    }
    
    if (debug) {
      fprintf(where,"recv_message.len %d\n",recv_message.len);
      fprintf(where,"send_message.len %d\n",send_message.len);
      fflush(where);
    }
    
    /* Now, send the response to the remote */
    if((putmsg(data_descriptor,
	       0,
	       &send_message,
	       0)) != 0) {
      if (errno == EINTR) {
	/* the test timer has popped */
	timed_out = 1;
	break;
      }
      netperf_response.content.serv_errno = 994;
      send_response();
      exit(1);
    }
    
    trans_received++;
    if (trans_remaining) {
      trans_remaining--;
    }
    
    if (debug) {
      fprintf(where,
	      "recv_dlpi_co_rr: Transaction %d complete\n",
	      trans_received);
      fflush(where);
    }
  }
  
  
  /* The loop now exits due to timeout or transaction count being */
  /* reached */
  
  cpu_stop(dlpi_co_rr_request->measure_cpu,&elapsed_time);
  
  if (timed_out) {
    /* we ended the test by time, which was at least 2 seconds */
    /* longer than we wanted to run. so, we want to subtract */
    /* PAD_TIME from the elapsed_time. */
    elapsed_time -= PAD_TIME;
  }
  /* send the results to the sender			*/
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_co_rr: got %d transactions\n",
	    trans_received);
    fflush(where);
  }
  
  dlpi_co_rr_results->bytes_received	= (trans_received * 
					   (dlpi_co_rr_request->request_size + 
					    dlpi_co_rr_request->response_size));
  dlpi_co_rr_results->trans_received	= trans_received;
  dlpi_co_rr_results->elapsed_time	= elapsed_time;
  if (dlpi_co_rr_request->measure_cpu) {
    dlpi_co_rr_results->cpu_util	= calc_cpu_util(elapsed_time);
  }
  
  if (debug) {
    fprintf(where,
	    "recv_dlpi_co_rr: test complete, sending results.\n");
    fflush(where);
  }
  
  send_response();
  
}

/* this routine will display the usage string for the DLPI tests */
void
  print_dlpi_usage()

{
  printf("%s",dlpi_usage);
}


/* this routine will scan the command line for DLPI test arguments */
void
  scan_dlpi_args(argc, argv)
int	argc;
char	*argv[];

{
  extern int	optind, opterrs;  /* index of first unused arg 	*/
  extern char	*optarg;	  /* pointer to option string	*/
  
  int		c;
  
  char	arg1[BUFSIZ],  /* argument holders		*/
  arg2[BUFSIZ];
  
  /* Go through all the command line arguments and break them */
  /* out. For those options that take two parms, specifying only */
  /* the first will set both to that value. Specifying only the */
  /* second will leave the first untouched. To change only the */
  /* first, use the form first, (see the routine break_args.. */
  
#define DLPI_ARGS "D:hM:m:p:r:s:W:w:"
  
  while ((c= getopt(argc, argv, DLPI_ARGS)) != EOF) {
    switch (c) {
    case '?':	
    case 'h':
      print_dlpi_usage();
      exit(1);
    case 'D':
      /* set the dlpi device file name(s) */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	strcpy(loc_dlpi_device,arg1);
      if (arg2[0])
	strcpy(rem_dlpi_device,arg2);
      break;
    case 'm':
      /* set the send size */
      send_size = atoi(optarg);
      break;
    case 'M':
      /* set the recv size */
      recv_size = atoi(optarg);
      break;
    case 'p':
      /* set the local/remote ppa */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	loc_ppa = atoi(arg1);
      if (arg2[0])
	rem_ppa = atoi(arg2);
      break;
    case 'r':
      /* set the request/response sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	req_size = atoi(arg1);
      if (arg2[0])	
	rsp_size = atoi(arg2);
      break;
    case 's':
      /* set the 802.2 sap for the test */
      dlpi_sap = atoi(optarg);
      break;
    case 'w':
      /* set local window sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	lsw_size = atoi(arg1);
      if (arg2[0])
	lrw_size = atoi(arg2);
      break;
    case 'W':
      /* set remote window sizes */
      break_args(optarg,arg1,arg2);
      if (arg1[0])
	rsw_size = atoi(arg1);
      if (arg2[0])
	rrw_size = atoi(arg2);
      break;
    };
  }
}


#endif /* DO_DLPI */
