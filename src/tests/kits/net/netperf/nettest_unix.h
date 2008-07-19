/*
        Copyright (C) 1993-2004 Hewlett-Packard Company
*/

 /* This file contains the test-specific definitions for netperf's */
 /* DLPI tests */

struct	stream_stream_request_struct {
  int	recv_buf_size;
  int	send_buf_size;
  int	receive_size;   /* how many bytes do we want to */
                        /* receive at one time? */
  int	recv_alignment; /* what is the alignment of the */
                        /* receive buffer? */
  int	recv_offset;    /* and at what offset from that */
                        /* alignment? */
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int	measure_cpu;	/* does the client want server cpu */
                        /* utilization measured? */
  float	cpu_rate;	/* do we know how fast the cpu is */
                        /* already? */ 
  int	test_length;	/* how long is the test?		*/
  int   dirty_count;    /* how many integers in the receive buffer */
			/* should be made dirty before calling recv? */
  int   clean_count;    /* how many integers should be read from the */
			/* recv buffer before calling recv? */ 
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct	stream_stream_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	receive_size;
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct stream_stream_results_struct {
  int	bytes_received;	/* ignored initially */
  int	recv_calls;	/* ignored initially */
  float	elapsed_time;	/* how long the test ran */
  float	cpu_util;	/* -1 if not measured */
  float	serv_dem;	/* -1 if not measured */
  int   num_cpus;
};

struct	stream_rr_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct	stream_rr_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  float	cpu_rate;		/* could we measure	*/
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path to the dlpi device */
};

struct stream_rr_results_struct {
  int	bytes_received;	/* ignored initially */
  int	recv_calls;	/* ignored initially */
  int	trans_received;	/* not ignored  */
  float	elapsed_time;	/* how long the test ran */
  float	cpu_util;	/* -1 if not measured */
  float	serv_dem;	/* -1 if not measured */
  int   num_cpus;
};

struct	dg_stream_request_struct {
  int	recv_buf_size;
  int	message_size;
  int	recv_alignment;
  int	recv_offset;
  int	measure_cpu;
  float	cpu_rate;
  int	test_length;
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct	dg_stream_response_struct {
  int	recv_buf_size;
  int	send_buf_size;
  int	measure_cpu;
  int	test_length;
  float	cpu_rate;
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct	dg_stream_results_struct {
  int	messages_recvd;
  int	bytes_received;
  float	elapsed_time;
  float	cpu_util;
  int   num_cpus;
};


struct	dg_rr_request_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive */
			/* copies? */ 
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct	dg_rr_response_struct {
  int	recv_buf_size;	/* how big does the client want it	*/
  int	send_buf_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
  int   path_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  unix_path[32]; /* the path */
};

struct dg_rr_results_struct {
  int	bytes_received;	/* ignored initially */
  int	recv_calls;	/* ignored initially */
  int	trans_received;	/* not ignored  */
  float	elapsed_time;	/* how long the test ran */
  float	cpu_util;	/* -1 if not measured */
  float	serv_dem;	/* -1 if not measured */
  int   num_cpus;
};

extern void scan_unix_args(int argc, char *argv[]);

extern void send_stream_stream(char remote_host[]);
extern void send_stream_rr(char remote_host[]);
extern void send_dg_stream(char remote_host[]);
extern void send_dg_rr(char remote_host[]);

extern void recv_stream_stream();
extern void recv_stream_rr();
extern void recv_dg_stream();
extern void recv_dg_rr();
