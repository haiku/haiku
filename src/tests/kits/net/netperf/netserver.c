#ifdef NEED_MAKEFILE_EDIT
#error you must first edit and customize the makefile to your platform
#endif /* NEED_MAKEFILE_EDIT */

/*
 
	   Copyright (C) 1993-2003 Hewlett-Packard Company
                         ALL RIGHTS RESERVED.
 
  The enclosed software and documention includes copyrighted works of
  Hewlett-Packard Co. For as long as you comply with the following
  limitations, you are hereby authorized to (i) use, reproduce, and
  modify the software and documentation, and to (ii) distribute the
  software and documentation, including modifications, for
  non-commercial purposes only.
      
  1.  The enclosed software and documentation is made available at no
      charge in order to advance the general development of
      high-performance networking products.
 
  2.  You may not delete any copyright notices contained in the
      software or documentation. All hard copies, and copies in
      source code or object code form, of the software or
      documentation (including modifications) must contain at least
      one of the copyright notices.
 
  3.  The enclosed software and documentation has not been subjected
      to testing and quality control and is not a Hewlett-Packard Co.
      product. At a future time, Hewlett-Packard Co. may or may not
      offer a version of the software and documentation as a product.
  
  4.  THE SOFTWARE AND DOCUMENTATION IS PROVIDED "AS IS".
      HEWLETT-PACKARD COMPANY DOES NOT WARRANT THAT THE USE,
      REPRODUCTION, MODIFICATION OR DISTRIBUTION OF THE SOFTWARE OR
      DOCUMENTATION WILL NOT INFRINGE A THIRD PARTY'S INTELLECTUAL
      PROPERTY RIGHTS. HP DOES NOT WARRANT THAT THE SOFTWARE OR
      DOCUMENTATION IS ERROR FREE. HP DISCLAIMS ALL WARRANTIES,
      EXPRESS AND IMPLIED, WITH REGARD TO THE SOFTWARE AND THE
      DOCUMENTATION. HP SPECIFICALLY DISCLAIMS ALL WARRANTIES OF
      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
  
  5.  HEWLETT-PACKARD COMPANY WILL NOT IN ANY EVENT BE LIABLE FOR ANY
      DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
      (INCLUDING LOST PROFITS) RELATED TO ANY USE, REPRODUCTION,
      MODIFICATION, OR DISTRIBUTION OF THE SOFTWARE OR DOCUMENTATION.
 
*/
char	netserver_id[]="\
@(#)netserver.c (c) Copyright 1993-2003 Hewlett-Packard Co. Version 2.2pl3";

 /***********************************************************************/
 /*									*/
 /*	netserver.c							*/
 /*									*/
 /*	This is the server side code for the netperf test package. It	*/
 /* will operate either stand-alone, or as a child of inetd. In this	*/
 /* way, we insure that it can be installed on systems with or without	*/
 /* root permissions (editing inetd.conf). Essentially, this code is	*/
 /* the analog to the netsh.c code.					*/
 /*									*/
 /***********************************************************************/


/************************************************************************/
/*									*/
/*	Global include files						*/
/*									*/
/************************************************************************/
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#if !defined(WIN32) && !defined(__VMS)
#include <sys/ipc.h>
#endif /* !defined(WIN32) && !defined(__VMS) */
#include <fcntl.h>
#ifdef WIN32
#include <time.h>
#include <windows.h>
#include <winsock.h>
#else
#ifndef MPE
#include <sys/time.h>
#endif /* MPE */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#ifndef DONT_WAIT
#include <sys/wait.h>
#endif /* DONT_WAIT */
#endif /* WIN32 */
#include <string.h>
#include <stdlib.h>
#ifdef __VMS
#include <tcpip$inetdef.h> 
#include <unixio.h> 
#endif /* __VMS */
#include "netlib.h"
#include "nettest_bsd.h"

#ifdef DO_UNIX
#include "nettest_unix.h"
#endif /* DO_UNIX */

#ifdef DO_DLPI
#include "nettest_dlpi.h"
#endif /* DO_DLPI */

#ifdef DO_IPV6
#include "nettest_ipv6.h"
#endif /* DO_IPV6 */

#ifdef DO_DNS
#include "nettest_dns.h"
#endif /* DO_DNS */

#include "netsh.h"

#ifndef DEBUG_LOG_FILE
#define DEBUG_LOG_FILE "/tmp/netperf.debug"
#endif /* DEBUG_LOG_FILE */

 /* some global variables */

FILE	*afp;
short	listen_port_num;
extern	char	*optarg;
extern	int	optind, opterr;

#define SERVER_ARGS "dn:p:46"

 /* This routine implements the "main event loop" of the netperf	*/
 /* server code. Code above it will have set-up the control connection	*/
 /* so it can just merrily go about its business, which is to		*/
 /* "schedule" performance tests on the server.				*/

void 
process_requests()
{
  
  float	temp_rate;
  
  
  while (1) {
    recv_request();
    if (debug)
      dump_request();
    
    switch (netperf_request.content.request_type) {
      
    case DEBUG_ON:
      netperf_response.content.response_type = DEBUG_OK;
      if (!debug)
	debug++;
      dump_request();
      send_response();
      break;
      
    case DEBUG_OFF:
      if (debug)
	debug--;
      netperf_response.content.response_type = DEBUG_OK;
      fclose(where);
      send_response();
      break;
      
    case CPU_CALIBRATE:
      netperf_response.content.response_type = CPU_CALIBRATE;
      temp_rate = calibrate_local_cpu(0.0);
      bcopy((char *)&temp_rate,
	    (char *)netperf_response.content.test_specific_data,
	    sizeof(temp_rate));
      if (debug) {
	fprintf(where,"netserver: sending CPU information:");
	fprintf(where,"rate is %g\n",temp_rate);
	fflush(where);
      }

      /* we need the cpu_start, cpu_stop in the looper case to kill the */
      /* child proceses raj 7/95 */

#ifdef USE_LOOPER
      cpu_start(1);
      cpu_stop(1,&temp_rate);
#endif /* USE_LOOPER */

      send_response();
      break;
      
    case DO_TCP_STREAM:
      recv_tcp_stream();
      break;
      
    case DO_TCP_MAERTS:
      recv_tcp_maerts();
      break;
      
    case DO_TCP_RR:
      recv_tcp_rr();
      break;
      
    case DO_TCP_CRR:
      recv_tcp_conn_rr();
      break;
      
    case DO_TCP_CC:
      recv_tcp_cc();
      break;
      
#ifdef DO_1644
    case DO_TCP_TRR:
      recv_tcp_tran_rr();
      break;
#endif /* DO_1644 */
      
#ifdef DO_NBRR
    case DO_TCP_NBRR:
      recv_tcp_nbrr();
      break;
#endif /* DO_NBRR */
      
    case DO_UDP_STREAM:
      recv_udp_stream();
      break;
      
    case DO_UDP_RR:
      recv_udp_rr();
      break;
      
#ifdef DO_DLPI

    case DO_DLPI_CO_RR:
      recv_dlpi_co_rr();
      break;
      
    case DO_DLPI_CL_RR:
      recv_dlpi_cl_rr();
      break;

    case DO_DLPI_CO_STREAM:
      recv_dlpi_co_stream();
      break;

    case DO_DLPI_CL_STREAM:
      recv_dlpi_cl_stream();
      break;

#endif /* DO_DLPI */

#ifdef DO_UNIX

    case DO_STREAM_STREAM:
      recv_stream_stream();
      break;
      
    case DO_STREAM_RR:
      recv_stream_rr();
      break;
      
    case DO_DG_STREAM:
      recv_dg_stream();
      break;
      
    case DO_DG_RR:
      recv_dg_rr();
      break;
      
#endif /* DO_UNIX */

#ifdef DO_FORE

    case DO_FORE_STREAM:
      recv_fore_stream();
      break;
      
    case DO_FORE_RR:
      recv_fore_rr();
      break;
      
#endif /* DO_FORE */

#ifdef DO_HIPPI

    case DO_HIPPI_STREAM:
      recv_hippi_stream();
      break;
      
    case DO_HIPPI_RR:
      recv_hippi_rr();
      break;
      
#endif /* DO_HIPPI */

#ifdef DO_XTI
    case DO_XTI_TCP_STREAM:
      recv_xti_tcp_stream();
      break;
      
    case DO_XTI_TCP_RR:
      recv_xti_tcp_rr();
      break;
      
    case DO_XTI_UDP_STREAM:
      recv_xti_udp_stream();
      break;
      
    case DO_XTI_UDP_RR:
      recv_xti_udp_rr();
      break;

#endif /* DO_XTI */
#ifdef DO_LWP
    case DO_LWPSTR_STREAM:
      recv_lwpstr_stream();
      break;
      
    case DO_LWPSTR_RR:
      recv_lwpstr_rr();
      break;
      
    case DO_LWPDG_STREAM:
      recv_lwpdg_stream();
      break;
      
    case DO_LWPDG_RR:
      recv_lwpdg_rr();
      break;

#endif /* DO_LWP */
#ifdef DO_IPV6
    case DO_TCPIPV6_STREAM:
      recv_tcpipv6_stream();
      break;
      
    case DO_TCPIPV6_RR:
      recv_tcpipv6_rr();
      break;
      
    case DO_TCPIPV6_CRR:
      recv_tcpipv6_conn_rr();
      break;
      
    case DO_UDPIPV6_STREAM:
      recv_udpipv6_stream();
      break;
      
    case DO_UDPIPV6_RR:
      recv_udpipv6_rr();
      break;

#endif /* DO_IPV6 */

#ifdef DO_DNS
    case DO_DNS_RR:
      recv_dns_rr();
      break;
#endif /* DO_DNS */

    default:
      fprintf(where,"unknown test number %d\n",
	      netperf_request.content.request_type);
      fflush(where);
      netperf_response.content.serv_errno=998;
      send_response();
      break;
      
    }
  }
}

/*********************************************************************/
/*				       		                     */
/*	set_up_server()						     */
/*								     */
/* set-up the server listen socket. we only call this routine if the */
/* user has specified a port number on the command line.             */
/*								     */
/*********************************************************************/
/*KC*/

void set_up_server(int af)
{ 
  struct sockaddr 	*server;
  struct sockaddr_in 	server4;
  struct sockaddr 	peeraddr;
#ifdef DO_IPV6
  struct sockaddr_in6 	server6;
#endif
  
  int server_control;
  int sockaddr_len;
  int on=1;

  if (af == AF_INET) {
	server4.sin_port = htons(listen_port_num);
	server4.sin_addr.s_addr = INADDR_ANY;
	server4.sin_family = AF_INET;
 	sockaddr_len = sizeof(struct sockaddr_in);
	server = (struct sockaddr *) &server4;
  }
#ifdef DO_IPV6
  else {
	server6.sin6_port = htons(listen_port_num);
	server6.sin6_family = AF_INET6;
#ifndef IN6_CLEAR_IN6ADDR 
#define IN6_CLEAR_IN6ADDR(x) 	memset(&(x), 0, sizeof(struct in6_addr))
#endif
	IN6_CLEAR_IN6ADDR(server6.sin6_addr);
 	sockaddr_len = sizeof(struct sockaddr_in6);
	server = (struct sockaddr *) &server6;
  }
#else
  else {
	fprintf(stderr,
		"netserver: IPv6 is not supported\n");
	fflush(stderr);
	exit(1);
  }
#endif

  printf("Starting netserver at port %d\n",listen_port_num);

  server_control = socket(server->sa_family,SOCK_STREAM,0);

#ifdef WIN32
  if (server_control == INVALID_SOCKET)
#else
  if (server_control < 0)
#endif /* WIN32 */
    {
      perror("server_set_up: creating the socket");
      exit(1);
    }
  if (setsockopt(server_control, 
		 SOL_SOCKET, 
		 SO_REUSEADDR, 
		 &on , 
		 sizeof(on)) == -1)
    {
      perror("server_set_up: SO_REUSEADDR");
      exit(1);
    }

  if (bind (server_control, server, sockaddr_len) == -1)
    {
      perror("server_set_up: binding the socket");
      exit (1);
    }
  if (listen (server_control,5) == -1)
    {
      perror("server_set_up: listening");
      exit(1);
    }
  
  /*
    setpgrp();
    */

#if !defined(WIN32) && !defined(MPE) && !defined(__VMS)
  switch (fork())
    {
    case -1:  	
      perror("netperf server error");
      exit(1);
      
    case 0:	
      /* stdin/stderr should use fclose */
      fclose(stdin);
      fclose(stderr);
 
#ifndef NO_SETSID
      setsid();
#else
      setpgrp();
#endif /* NO_SETSID */

 /* some OS's have SIGCLD defined as SIGCHLD */
#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif /* SIGCLD */

      signal(SIGCLD, SIG_IGN);
      
#endif /* WIN32 */

      for (;;)
	{
	  if ((server_sock=accept(server_control,
				  &peeraddr,
				  &sockaddr_len)) == -1)
	    {
	      printf("server_control: accept failed\n");
	      exit(1);
	    }
#if defined(WIN32) || defined(MPE) || defined(__VMS)
	  /*
	   * Since we cannot fork this process , we cant fire any threads
	   * as they all share the same global data . So we better allow
	   * one request at at time 
	   */
	  process_requests() ;
	}
#else
      signal(SIGCLD, SIG_IGN);
	  
	  switch (fork())
	    {
	    case -1:
	      /* something went wrong */
	      exit(1);
	    case 0:
	      /* we are the child process */
	      close(server_control);
	      process_requests();
	      exit(0);
	      break;
	    default:
	      /* we are the parent process */
	      close(server_sock);
	      /* we should try to "reap" some of our children. on some */
	      /* systems they are being left as defunct processes. we */
	      /* will call waitpid, looking for any child process, */
	      /* with the WNOHANG feature. when waitpid return a zero, */
	      /* we have reaped all the children there are to reap at */
	      /* the moment, so it is time to move on. raj 12/94 */
#ifndef DONT_WAIT
	      while(waitpid(-1, NULL, WNOHANG) > 0) { }
#endif /* DONT_WAIT */
	      break;
	    }
	} /*for*/
      break; /*case 0*/
      
    default: 
      exit (0);
      
    }
#endif /* WIN32 */  
}


int
main(argc, argv)
     int argc;
     char *argv[];
{

  int	c;
  int	af = AF_INET;

struct sockaddr name;
  int namelen = sizeof(name);
  

#ifdef WIN32
	WSADATA	wsa_data ;

	/* Initialise the wsock lib ( version 1.1 ) */
	if ( WSAStartup(0x0101,&wsa_data) == SOCKET_ERROR ){
		printf("WSAStartup() fauled : %d\n",GetLastError()) ;
		return 1 ;
	}
#endif /* WIN32 */

  netlib_init();
  
  /* Scan the command line to see if we are supposed to set-up our own */
  /* listen socket instead of relying on inetd. */
  
  while ((c = getopt(argc, argv, SERVER_ARGS)) != EOF) {
    switch (c) {
    case '?':
    case 'h':
      print_netserver_usage();
      exit(1);
    case 'd':
      /* we want to set the debug file name sometime */
      debug++;
      break;
    case 'p':
      /* we want to open a listen socket at a */
      /* specified port number */
      listen_port_num = atoi(optarg);
      break;
    case 'n':
      shell_num_cpus = atoi(optarg);
      if (shell_num_cpus > MAXCPUS) {
	fprintf(stderr,
		"netserver: This version can only support %d CPUs. Please",
		MAXCPUS);
	fprintf(stderr,
		"           increase MAXCPUS in netlib.h and recompile.\n");
	fflush(stderr);
	exit(1);
      }
      break;
#ifdef DO_IPV6
    case '4':
      af = AF_INET;
      break;
    case '6':
      af = AF_INET6;
      break;
#endif
    }
  }

/*  unlink(DEBUG_LOG_FILE); */
  
  if ((where = fopen(DEBUG_LOG_FILE, "w")) == NULL) {
    perror("netserver: debug file");
    exit(1);
  }
  
  chmod(DEBUG_LOG_FILE,0644);
  
  /* if we were given a port number, then we should open a */
  /* socket and hang listens off of it. otherwise, we should go */
  /* straight into processing requests. the do_listen() routine */
  /* will sit in an infinite loop accepting connections and */
  /* forking child processes. the child processes will call */
  /* process_requests */
  
  /* If fd 0 is not a socket then assume we're not being called */
  /* from inetd and start server socket on the default port. */
  /* this enhancement comes from vwelch@ncsa.uiuc.edu (Von Welch) */

  if (listen_port_num) {
    /* the user specified a port number on the command line */
    set_up_server(af);
  }
#if !defined(__VMS)
  else if (getsockname(0, &name, &namelen) == -1) {
    /* we may not be a child of inetd */
#ifdef WIN32
	  if (WSAGetLastError() == WSAENOTSOCK) {
#else 
	  if (errno == ENOTSOCK) {
#endif /* WIN32 */
	  listen_port_num = TEST_PORT;
      set_up_server(af);
    }
  }
#endif /* !defined(__VMS) */
  else {
    /* we are probably a child of inetd, or are being invoked via the
       VMS auxilliarly server mechanism */
#if !defined(__VMS)
    server_sock = 0;
#else
    if ( (server_sock = socket(TCPIP$C_AUXS, SOCK_STREAM, 0)) < 0 ) 
    { 
      perror("Failed to grab aux server socket" ); 
      exit(1); 
    } 
  
#endif /* !defined(__VMS) */
    process_requests();
  }
  return(0);
}
