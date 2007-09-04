/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * tftpd_mcast.c
 *    support routine for multicast server
 *
 * $Id: tftpd_mcast.c,v 1.6 2003/04/25 00:16:19 jp Exp $
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "tftpd.h"
#include "tftp_def.h"
#include "logger.h"

#define START    0
#define GET_ENUM 1
#define GET_GRP  2
#define EXPAND   3

pthread_mutex_t mcast_tid_list = PTHREAD_MUTEX_INITIALIZER;

int parse_ip(char *string, char **res_ip);
int parse_port(char *string, char **res_port);

struct tid {
     char *addr;
     short port;
     int used;
     struct tid *next;
};

struct tid *tid_list = NULL;

/*
 * Return a free IP/Port for the multicast transfer
 */
int tftpd_mcast_get_tid(char **addr, short *port)
{
     struct tid *current = tid_list;

     pthread_mutex_lock(&mcast_tid_list);
     /* walk the list for a free tid */
     while (current != NULL)
     {
	  if (current->used == 0)
	  {
	       *addr = current->addr;
	       *port = current->port;
	       current->used = 1;
	       pthread_mutex_unlock(&mcast_tid_list);
	       return OK;
	  }
	  else
	       current = current->next;
     }
     pthread_mutex_unlock(&mcast_tid_list);
     return ERR;
}

int tftpd_mcast_free_tid(char *addr, short port)
{
     struct tid *current = tid_list;

     pthread_mutex_lock(&mcast_tid_list);
     while (current != NULL)
     {
	  if ((current->used == 1) && (current->port == port) &&
	      (strcmp(current->addr, addr) == 0))
	  {
	       current->used = 0;
	       pthread_mutex_unlock(&mcast_tid_list);
	       return OK;
	  }
	  else
	       current = current->next;
     }
     pthread_mutex_unlock(&mcast_tid_list);
     return ERR;
}

/* valid address specification:
   239.255.0.0-239.255.0.20
   239.255.0.0-20
   239.255.0.0,1,2,3,8,10
*/
/* valid port specification
   1758
   1758-1780
   1758,1760,4000
*/
int tftpd_mcast_parse_opt(char *addr, char *ports)
{
     char *ip;
     char *port;
     struct tid *current = NULL;
     struct tid *tmp = NULL;

     while (1)
     {
	  if (parse_ip(addr, &ip) !=  OK)
	       return ERR;
	  if (ip == NULL)
	       return OK;
	  while (1)
	  {
	       if (parse_port(ports, &port) != OK)
		    return ERR;
	       if (port == NULL)
		    break;
	       /* add this ip/port to the tid list */
	       tmp = malloc(sizeof(struct tid));
	       tmp->addr = strdup(ip);
	       tmp->port = (short)atoi(port);
	       tmp->used = 0;
	       tmp->next = NULL;
	       if (tid_list == NULL)
	       {
		    tid_list = tmp;
		    current = tid_list;
	       }
	       else
	       {
		    current->next = tmp;
		    current = tmp;
	       }
	  }
     }
}

void tftpd_mcast_clean(void)
{


}

int parse_ip(char *string, char **res_ip)
{
     static int state = START;

     static char s[MAXLEN];
     static char *saveptr;
     static char s2[MAXLEN];
     static char *saveptr2;

     static int ip[4];
     static int tmp_ip[4];

     static int i;
     
     int res;
     char *tmp = NULL, *tmp2 = NULL;
     static char out[MAXLEN];

     *res_ip = NULL;

     while (1)
     {
	  switch (state)
	  {
	  case START:
	       Strncpy(s, string, MAXLEN);
	       tmp = strtok_r(s, ",", &saveptr);
	       if (tmp == NULL)
	       {
		    state = START;
		    return ERR;
	       }
	       else
		    state = GET_GRP;
			 break;
	  case GET_ENUM:
	       tmp = strtok_r(NULL, ",", &saveptr);
	       if (tmp == NULL)
	       {
		    state = START;
		    return OK;
	       }
	       else
		    state = GET_GRP;
			 break;
	  case GET_GRP:
	       Strncpy(s2, tmp, MAXLEN);
	       tmp = strtok_r(s2, "-", &saveptr2);
	       if (tmp == NULL)
	       {
		    state = START;
		    return ERR;
	       }
	       res = sscanf(tmp, "%d.%d.%d.%d", &tmp_ip[0], &tmp_ip[1],
			    &tmp_ip[2], &tmp_ip[3]);
	       if (res == 4)
	       {
		    for (i=0; i < 4; i++)
			 ip[i] = tmp_ip[i];
	       }
	       else
	       {
		    if (res == 1)
		    {
			 ip[3] = tmp_ip[0];
		    }
		    else
		    {
			 state = START;
			 return ERR;
		    }
	       }
	       tmp2 = strtok_r(NULL, "-", &saveptr2);
	       if (tmp2 == NULL)
	       {
		    state = GET_ENUM;
		    snprintf(out, sizeof(out), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
		    *res_ip = out;
		    return OK;
	       }
	       else
	       {
		    sscanf(tmp2, "%d", &tmp_ip[0]);
		    i = ip[3];
		    if (i >= tmp_ip[0])
		    {
			 logger(LOG_ERR, "Bad address range: %d.%d.%d.%d-%d",
				ip[0], ip[1], ip[2], ip[3], tmp_ip[0]);
			 return ERR;
		    }
		    state = EXPAND;
	       }
	       break;
	  case EXPAND:
	       if (i > tmp_ip[0])
	       {
		    state = GET_ENUM;
		    break;
	       }
	       snprintf(out, sizeof(out), "%d.%d.%d.%d", ip[0], ip[1], ip[2], i);
	       i++;
	       *res_ip = out;
	       return OK;
	       break;
	  }
     }
}

int parse_port(char *string, char **res_port)
{
     static int state = START;

     static char s[MAXLEN];
     static char *saveptr;
     static char s2[MAXLEN];
     static char *saveptr2;

     static int port;
     static int tmp_port;

     static int i;
     
     int res;
     char *tmp = NULL, *tmp2 = NULL;
     static char out[MAXLEN];

     *res_port = NULL;

     while (1)
     {
	  switch (state)
	  {
	  case START:
	       Strncpy(s, string, MAXLEN);
	       tmp = strtok_r(s, ",", &saveptr);
	       if (tmp == NULL)
	       {
		    state = START;
		    return ERR;
	       }
	       else
		    state = GET_GRP;
			 break;
	  case GET_ENUM:
	       tmp = strtok_r(NULL, ",", &saveptr);
	       if (tmp == NULL)
	       {
		    state = START;
		    return OK;
	       }
	       else
		    state = GET_GRP;
			 break;
	  case GET_GRP:
	       Strncpy(s2, tmp, MAXLEN);
	       tmp = strtok_r(s2, "-", &saveptr2);
	       if (tmp == NULL)
	       {
		    state = START;
		    return ERR;
	       }
	       res = sscanf(tmp, "%d", &port);
	       if (res != 1)
	       {
		    state = START;
		    return ERR;
	       }
	       tmp2 = strtok_r(NULL, "-", &saveptr2);
	       if (tmp2 == NULL)
	       {
		    state = GET_ENUM;
		    snprintf(out, sizeof(out), "%d", port);
		    *res_port = out;
		    return OK;
	       }
	       else
	       {
		    sscanf(tmp2, "%d", &tmp_port);
		    i = port;
		    if (i >= tmp_port)
		    {
			 logger(LOG_ERR, "Bad port range: %d-%d", i, tmp_port);
			 return ERR;
		    }
		    state = EXPAND;
	       }
	       break;
	  case EXPAND:
	       if (i > tmp_port)
	       {
		    state = GET_ENUM;
		    break;
	       }
	       snprintf(out, sizeof(out), "%d", i);
	       i++;
	       *res_port = out;
	       return OK;
	       break;
	  }
     }
}
