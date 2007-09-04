/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftp_io.h
 *
 * $Id: tftp_io.h,v 1.18 2004/02/13 03:16:09 jp Exp $
 *
 * Copyright (c) 2000 Jean-Pierre Lefebvre <helix@step.polymtl.ca>
 *                and Remi Lefebvre <remi@debian.org>
 *
 * atftp is free software; you can redistribute them and/or modify them
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 */

#ifndef tftp_io_h
#define tftp_io_h

#include <arpa/tftp.h>
#include <sys/socket.h>
#include "tftp_def.h"
#include "options.h"

/* missing from <arpa/tftp.h> */
/* new opcode */
#define OACK   06
/* new error code */
#define EOPTNEG 8               /* error in option negociation */

/* return value of tftp_get_packet */
#define GET_DISCARD 0
#define GET_TIMEOUT 1
#define GET_RRQ     2
#define GET_WRQ     3
#define GET_ACK     4
#define GET_OACK    5
#define GET_ERROR   6
#define GET_DATA    7

/* functions prototype */
int tftp_send_request(int socket, struct sockaddr_in *s_inn, short type,
                      char *data_buffer, int data_buffer_size,
                      struct tftp_opt *tftp_options);
int tftp_send_ack(int socket, struct sockaddr_in *s_inn, short block_number);
int tftp_send_oack(int socket, struct sockaddr_in *s_inn, struct tftp_opt *tftp_options,
                   char *buffer, int buffer_size);
int tftp_send_error(int socket, struct sockaddr_in *s_inn, short err_code,
                    char *buffer, int buffer_size);
int tftp_send_data(int socket, struct sockaddr_in *s_inn, short block_number,
                   int size, char *data);
int tftp_get_packet(int sock1, int sock2, int *sock, struct sockaddr_in *sa,
                    struct sockaddr_in *from, struct sockaddr_in *to,
                    int timeout, int *size, char *data);
int tftp_file_read(FILE *fp, char *buffer, int buffer_size, int block_number, int convert,
                   int *prev_block_number, int *prev_file_pos, int *temp);
int tftp_file_write(FILE *fp, char *data_buffer, int data_buffer_size, int block_number,
                    int data_size, int convert, int *prev_block_number, int *temp);
#endif
