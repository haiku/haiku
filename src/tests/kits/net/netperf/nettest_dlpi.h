/*
        Copyright (C) 1993, Hewlett-Packard Company
*/

 /* This file contains the test-specific definitions for netperf's */
 /* DLPI tests */

#define PAD_TIME 2

struct	dlpi_co_stream_request_struct {
  int	recv_win_size;
  int	send_win_size;
  int	receive_size;   /* how many bytes do we want to */
                        /* receive at one time? */
  int	recv_alignment; /* what is the alignment of the */
                        /* receive buffer? */
  int	recv_offset;    /* and at what offset from that */
                        /* alignment? */
  int	measure_cpu;	/* does the client want server cpu */
                        /* utilization measured? */
  float	cpu_rate;	/* do we know how fast the cpu is */
                        /* already? */ 
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid */
                        /* copies on receives? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   dirty_count;    /* how many integers in the receive buffer */
			/* should be made dirty before calling recv? */
  int   clean_count;    /* how many integers should be read from the */
			/* recv buffer before calling recv? */ 
  int   sap;            /* */
  int   ppa;            /* which device do we wish to use? */
  int   dev_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  dlpi_device[32]; /* the path to the dlpi device */
};

struct	dlpi_co_stream_response_struct {
  int	recv_win_size;	/* how big does the client want it	*/
  int	send_win_size;
  int	receive_size;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
  int   station_addr_len;
  int   station_addr[1];/* what is the station address for the */
			/* specified ppa? */
};

struct dlpi_co_stream_results_struct {
  int	bytes_received;	/* ignored initially */
  int	recv_calls;	/* ignored initially */
  float	elapsed_time;	/* how long the test ran */
  float	cpu_util;	/* -1 if not measured */
  float	serv_dem;	/* -1 if not measured */
  int   cpu_method;     /* how was CPU util measured? */
  int   num_cpus;       /* how many CPUs were there? */
};

struct	dlpi_co_rr_request_struct {
  int	recv_win_size;	/* how big does the client want it	*/
  int	send_win_size;
  int	recv_alignment;
  int	recv_offset;
  int	send_alignment;
  int	send_offset;
  int	request_size;
  int	response_size;
  int	measure_cpu;	/* does the client want server cpu	*/
  float	cpu_rate;	/* do we know how fast the cpu is?	*/
  int	test_length;	/* how long is the test?		*/
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   ppa;            /* which device do we wish to use? */
  int   sap;            /* which sap should be used? */
  int   dev_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  dlpi_device[32]; /* the path to the dlpi device */
};

struct	dlpi_co_rr_response_struct {
  int	recv_win_size;	/* how big does the client want it	*/
  int	send_win_size;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
  int   station_addr_len;    /* the length of the station address */
  int   station_addr[1];     /* the remote's station address */
};

struct dlpi_co_rr_results_struct {
  int	bytes_received;	/* ignored initially */
  int	recv_calls;	/* ignored initially */
  int	trans_received;	/* not ignored  */
  float	elapsed_time;	/* how long the test ran */
  float	cpu_util;	/* -1 if not measured */
  float	serv_dem;	/* -1 if not measured */
  int   cpu_method;     /* how was CPU util measured? */
  int   num_cpus;       /* how many CPUs were there? */
};

struct	dlpi_cl_stream_request_struct {
  int	recv_win_size;
  int	message_size;
  int	recv_alignment;
  int	recv_offset;
  int	checksum_off;
  int	measure_cpu;
  float	cpu_rate;
  int	test_length;
  int	so_rcvavoid;    /* do we want the remote to avoid receive copies? */
  int	so_sndavoid;    /* do we want the remote to avoid send copies? */
  int   ppa;            /* which device do we wish to use? */
  int   sap;
  int   dev_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  dlpi_device[32]; /* the path to the dlpi device */
};

struct	dlpi_cl_stream_response_struct {
  int	recv_win_size;
  int	send_win_size;
  int	measure_cpu;
  int	test_length;
  int	data_port_number;
  float	cpu_rate;
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
  int   station_addr_len;    /* the length of the station address */
  int   station_addr[1];     /* the remote's station address */
};

struct	dlpi_cl_stream_results_struct {
  int	messages_recvd;
  int	bytes_received;
  float	elapsed_time;
  float	cpu_util;
};


struct	dlpi_cl_rr_request_struct {
  int	recv_win_size;	/* how big does the client want it	*/
  int	send_win_size;
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
  int   ppa;            /* which device do we wish to use? */
  int   sap;            /* which sap? */
  int   dev_name_len;   /* the length of the device name string. this */
			/* is used to put it into the proper order on */
			/* @#$% byte-swapped boxes... */
  char  dlpi_device[32]; /* the path to the dlpi device */
};

struct	dlpi_cl_rr_response_struct {
  int	recv_win_size;	/* how big does the client want it	*/
  int	send_win_size;
  int	no_delay;
  int	measure_cpu;	/* does the client want server cpu	*/
  int	test_length;	/* how long is the test?		*/
  int	data_port_number;	/* connect to me here	*/
  float	cpu_rate;		/* could we measure	*/
  int	so_rcvavoid;	/* could the remote avoid receive copies? */
  int	so_sndavoid;	/* could the remote avoid send copies? */
  int   station_addr_len;    /* the length of the station address */
  int   station_addr[1];     /* the remote's station address */
};

struct dlpi_cl_rr_results_struct {
  int	bytes_received;	/* ignored initially */
  int	recv_calls;	/* ignored initially */
  int	trans_received;	/* not ignored  */
  float	elapsed_time;	/* how long the test ran */
  float	cpu_util;	/* -1 if not measured */
  float	serv_dem;	/* -1 if not measured */
  int   cpu_method;     /* how was CPU util measured? */
  int   num_cpus;       /* how many CPUs were there? */
};
