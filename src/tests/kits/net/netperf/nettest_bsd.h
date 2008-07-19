/*
        Copyright (C) 1993-2004 Hewlett-Packard Company
*/

 /* This file contains the test-specific definitions for netperf's BSD */
 /* sockets tests */

/* well boys and girls, seems that while AF_INET is "2" and AF_UNSPEC
   is "0" the world over, AF_INET6 is different values depending on
   the platform... grrr.  On HP-UX 11i it is "22" and on Linux 2.6 it
   is "10" sooooo... we have to define our own space for netperf to
   enable us to pass values around from machine to machine. raj
   2005-02-08 */
#define NF_UNSPEC 0
#define NF_INET   4
#define NF_INET6  6

struct	tcp_stream_request_struct {
  int	send_buf_size;
  int	recv_buf_size;	/* how big does the client want it - the */
			/* receive socket buffer that is */ 
  int	receive_size;   /* how many bytes do we want to receive at one */
			/* time? */ 
  int	recv_alignment; /* what is the alignment of the receive */
			/* buffer? */ 
  int	recv_offset;    /* and at what offset from that alignment? */ 
  int	no_delay;       /* do we disable the nagle algorithm for send */
			/* coalescing? */ 
  int	measure_cpu;	/* does the client want server cpu utilization */
			/* measured? */ 
  float	cpu_rate;	/* do we know how fast the cpu is already? */ 
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid copies on */
			/* receives? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   dirty_count;    /* how many integers in the receive buffer */
			/* should be made dirty before calling recv? */  
  int   clean_count;    /* how many integers should be read from the */
			/* recv buffer before calling recv? */ 
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;       /* the address family of ipaddress */
};

struct	tcp_stream_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	receive_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */ 
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct tcp_stream_results_struct {
  double         bytes_received;
  unsigned int	 recv_calls;	
  float	         elapsed_time;	/* how long the test ran */
  float	         cpu_util;	/* -1 if not measured */
  float	         serv_dem;	/* -1 if not measured */
  int            cpu_method;    /* how was cpu util measured? */
  int            num_cpus;      /* how many CPUs had the remote? */
};

struct	tcp_maerts_request_struct {
  int	send_buf_size;
  int	recv_buf_size;	/* how big does the client want it - the */
			/* receive socket buffer that is */ 
  int	send_size;      /* how many bytes do we want netserver to send
			   at one time? */
  int	send_alignment; /* what is the alignment of the send */
			/* buffer? */ 
  int	send_offset;    /* and at what offset from that alignment? */ 
  int	no_delay;       /* do we disable the nagle algorithm for send */
			/* coalescing? */ 
  int	measure_cpu;	/* does the client want server cpu utilization */
			/* measured? */ 
  float	cpu_rate;	/* do we know how fast the cpu is already? */ 
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid copies on */
			/* receives? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   dirty_count;    /* how many integers in the send buffer */
			/* should be made dirty before calling recv? */  
  int   clean_count;    /* how many integers should be read from the */
			/* recv buffer before calling recv? */ 
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
};

struct	tcp_maerts_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */ 
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct tcp_maerts_results_struct {
  double         bytes_sent;
  unsigned int	 send_calls;	
  float	         elapsed_time;	/* how long the test ran */
  float	         cpu_util;	/* -1 if not measured */
  float	         serv_dem;	/* -1 if not measured */
  int            cpu_method;    /* how was cpu util measured? */
  int            num_cpus;      /* how many CPUs had the remote? */
};

struct	tcp_rr_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
};

struct	tcp_rr_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct tcp_rr_results_struct {
  unsigned int  bytes_received;	/* ignored initially */
  unsigned int	recv_calls;	/* ignored initially */
  unsigned int	trans_received;	/* not ignored  */
  float	        elapsed_time;	/* how long the test ran */
  float	        cpu_util;	/* -1 if not measured */
  float	        serv_dem;	/* -1 if not measured */
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */
};

struct	tcp_conn_rr_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
};


struct	tcp_conn_rr_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct tcp_conn_rr_results_struct {
  unsigned int	bytes_received;	/* ignored initially */
  unsigned int	recv_calls;	/* ignored initially */
  unsigned int	trans_received;	/* not ignored  */
  float	        elapsed_time;	/* how long the test ran */
  float	        cpu_util;	/* -1 if not measured */
  float	        serv_dem;	/* -1 if not measured */
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */
};

struct	tcp_tran_rr_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
};


struct	tcp_tran_rr_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct tcp_tran_rr_results_struct {
  unsigned int	bytes_received;	/* ignored initially */
  unsigned int	recv_calls;	/* ignored initially */
  unsigned int	trans_received;	/* not ignored  */
  float	        elapsed_time;	/* how long the test ran */
  float	        cpu_util;	/* -1 if not measured */
  float	        serv_dem;	/* -1 if not measured */
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */

};

struct	udp_stream_request_struct {
  int	recv_buf_size;
  int	message_size;
  int   recv_connected;
  int	recv_alignment;
  int	recv_offset;
  int	checksum_off;
  int	measure_cpu;
  float	cpu_rate;
  int	test_length;
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
  
};

struct	udp_stream_response_struct {
  int	recv_buf_size;
  int	send_buf_size;
  int	measure_cpu;
  int	test_length;
  int	data_port_number;
  float	cpu_rate;
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct	udp_stream_results_struct {
  unsigned int	messages_recvd;
  unsigned int	bytes_received;
  float	        elapsed_time;
  float	        cpu_util;
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */
};


struct	udp_rr_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
};

struct	udp_rr_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct udp_rr_results_struct {
  unsigned int	bytes_received;	/* ignored initially */
  unsigned int	recv_calls;	/* ignored initially */
  unsigned int	trans_received;	/* not ignored  */
  float	        elapsed_time;	/* how long the test ran */
  float	        cpu_util;	/* -1 if not measured */
  float	        serv_dem;	/* -1 if not measured */
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */
};

struct	tcp_cc_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   port;           /* the port to which the recv side should bind
			   to allow netperf to run through those evil
			   firewall things */
  int   ipfamily;
};


struct	tcp_cc_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	send_buf_size;
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
};

struct tcp_cc_results_struct {
  unsigned int	bytes_received;	/* ignored initially */
  unsigned int	recv_calls;	/* ignored initially */
  unsigned int	trans_received;	/* not ignored  */
  float	        elapsed_time;	/* how long the test ran */
  float	        cpu_util;	/* -1 if not measured */
  float	        serv_dem;	/* -1 if not measured */
  int           cpu_method;    /* how was cpu util measured? */
  int           num_cpus;      /* how many CPUs had the remote? */
};

extern int   rss_size_req,     /* requested remote socket send buffer size */
	     rsr_size_req,     /* requested remote socket recv buffer size */
	     rss_size,         /* remote socket send buffer size       */
	     rsr_size,         /* remote socket recv buffer size       */
	     lss_size_req,     /* requested local socket send buffer size */
	     lsr_size_req,     /* requested local socket recv buffer size */
	     lss_size,         /* local  socket send buffer size       */
	     lsr_size,         /* local  socket recv buffer size       */
	     req_size,         /* request size                         */
	     rsp_size,         /* response size                        */
	     send_size,        /* how big are individual sends         */
	     recv_size,        /* how big are individual receives      */
	     loc_nodelay,          /* don't/do use NODELAY locally         */
	     rem_nodelay,          /* don't/do use NODELAY remotely        */
	     loc_sndavoid,         /* avoid send copies locally            */
	     loc_rcvavoid,         /* avoid recv copies locally            */
	     rem_sndavoid,         /* avoid send copies remotely           */
	     rem_rcvavoid;         /* avoid recv_copies remotely           */


extern void scan_sockets_args(int argc, char *argv[]);
extern struct addrinfo *complete_addrinfo(char *controlhost, 
				   char *data_address, 
				   char *port, 
				   int family, 
				   int type, 
				   int protocol, 
				   int flags);
extern void complete_addrinfos(struct addrinfo **remote,
			       struct addrinfo **local, 
			       char remote_host[], 
			       int type, 
			       int protocol, 
			       int flags);
extern int af_to_nf(int af);
extern int nf_to_af(int nf);
extern void print_top_test_header(char test_name[], 
				  struct addrinfo *source, 
				  struct addrinfo *destination);
extern void set_port_number(struct addrinfo *res, 
			    unsigned short port);
extern void set_hostname_and_port(char *hostname, 
				  char *portstr, 
				  int family, 
				  int port);
extern void send_tcp_stream(char remote_host[]);
extern void send_tcp_maerts(char remote_host[]);
extern void send_tcp_rr(char remote_host[]);
extern void send_tcp_conn_rr(char remote_host[]);
extern void send_tcp_cc(char remote_host[]);
extern void send_udp_stream(char remote_host[]);
extern void send_udp_rr(char remote_host[]);

extern void recv_tcp_stream();
extern void recv_tcp_maerts();
extern void recv_tcp_rr();
extern void recv_tcp_conn_rr();
extern void recv_tcp_cc();
extern void recv_udp_stream();
extern void recv_udp_rr();

extern void loc_cpu_rate();
extern void rem_cpu_rate();

#ifdef HAVE_ICSC_EXS
extern void send_exs_tcp_stream(char remotehost[]);
#endif /* HAVE_ICSC_EXS */

#ifdef HAVE_SENDFILE
extern void sendfile_tcp_stream(char remotehost[]);
#endif /* HAVE_SENDFILE */

#if !defined(HAVE_STRUCT_SOCKADDR_STORAGE) && !defined(sockaddr_storage)
#define sockaddr_storage sockaddr_in
#endif

#ifdef DO_NBRR
extern void send_tcp_nbrr(char remote_host[]);

extern void recv_tcp_nbrr();
#endif

