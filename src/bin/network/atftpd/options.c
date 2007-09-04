/* hey emacs! -*- Mode: C; c-file-style: "k&r"; indent-tabs-mode: nil -*- */
/*
 * options.c
 *    Set of functions to deal with the options structure and for parsing
 *    options in TFTP data buffer.
 *
 * $Id: options.c,v 1.16 2003/04/25 00:16:18 jp Exp $
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
#include <unistd.h>
#include <syslog.h>

#if HAVE_ARGZ
#include <argz.h>
#else
#include "argz.h"
#endif

#include <arpa/tftp.h>
#include <string.h>
#include "options.h"

/*
 * Fill a structure with the request packet of the client.
 */
int opt_parse_request(char *data, int data_size, struct tftp_opt *options)
{
     char *entry = NULL;
     char *tmp;
     struct tftphdr *tftp_data = (struct tftphdr *)data;
     size_t size = data_size - sizeof(tftp_data->th_opcode);

     /* read filename */
     entry = argz_next(tftp_data->th_stuff, size, entry);
     if (!entry)
          return ERR;
     else
          opt_set_options(options, "filename", entry);
     /* read mode */
     entry = argz_next(tftp_data->th_stuff, size, entry);
     if (!entry)
          return ERR;
     else
          opt_set_options(options, "mode", entry);
     /* scan for options */
     // FIXME: we should use opt_parse_options() here
     while ((entry = argz_next(tftp_data->th_stuff, size, entry)))
     {
          tmp = entry;
          entry = argz_next(tftp_data->th_stuff, size, entry);
          if (!entry)
               return ERR;
          else
               opt_set_options(options, tmp, entry);
     }
     return OK;
}

/*
 * Fill a structure looking only at TFTP options.
 */
int opt_parse_options(char *data, int data_size, struct tftp_opt *options)
{
     char *entry = NULL;
     char *tmp;
     struct tftphdr *tftp_data = (struct tftphdr *)data;
     size_t size = data_size - sizeof(tftp_data->th_opcode);

     while ((entry = argz_next(tftp_data->th_stuff, size, entry)))
     {
          tmp = entry;
          entry = argz_next(tftp_data->th_stuff, size, entry);
          if (!entry)
               return ERR;
          else
               opt_set_options(options, tmp, entry);
     }
     return OK;
}

/*
 * Set an option by name in the structure.
 * name is the name of the option as in tftp_def.c.
 * name is it's new value, that must comply with the rfc's.
 * When setting an option, it is marked as specified.
 * 
 */
int opt_set_options(struct tftp_opt *options, char *name, char *value)
{
     int i;

     for (i = 0; i < OPT_NUMBER; i++)
     {
          if (strncasecmp(name, options[i].option, OPT_SIZE) == 0)
          {
               options[i].specified = 1;
               if (value)
                    Strncpy(options[i].value, value, VAL_SIZE);
               else
                    Strncpy(options[i].value, tftp_default_options[i].value,
                            VAL_SIZE);
               return OK;
          }
     }
     return ERR;
}

/*
 * Return "value" for a given option name in the given option
 * structure.
 */
int opt_get_options(struct tftp_opt *options, char *name, char *value)
{
     int i;

     for (i = 0; i < OPT_NUMBER; i++)
     {
          if (strncasecmp(name, options[i].option, OPT_SIZE) == 0)
          {
               if (options[i].enabled)
                    Strncpy(value, options[i].value, VAL_SIZE);
               else
                    return ERR;
               return OK;
          }
     }
     return ERR;
}

/*
 * Disable an option by name.
 */
int opt_disable_options(struct tftp_opt *options, char *name)
{
     int i;

     for (i = 2; i < OPT_NUMBER; i++)
     {
          if (name == NULL)
               options[i].specified = 0;
          else
          {
               if (strncasecmp(name, options[i].option, OPT_SIZE) == 0)
               {
                    options[i].specified = 0;
                    return OK;
               }
          }
     }
     if (name == NULL)
          return OK;
     return ERR;
}


/*
 * Return 1 if one or more options are specified in the options structure.
 */
int opt_support_options(struct tftp_opt *options)
{
     int i;
     int support = 0;

     for (i = 2; i < OPT_NUMBER; i++)
     {
          if (options[i].specified)
               support = 1;
     }
     return support;
}

/*
 * The next few functions deal with TFTP options. Function's name are self
 * explicative.
 */

int opt_get_tsize(struct tftp_opt *options)
{
     int tsize;
     if (options[OPT_TSIZE].enabled && options[OPT_TSIZE].specified)
     {
          tsize = atoi(options[OPT_TSIZE].value);
          return tsize;
     }
     return ERR;
}

int opt_get_timeout(struct tftp_opt *options)
{
     int timeout;
     if (options[OPT_TIMEOUT].enabled && options[OPT_TIMEOUT].specified)
     {
          timeout = atoi(options[OPT_TIMEOUT].value);
          return timeout;
     }
     return ERR;
}

int opt_get_blksize(struct tftp_opt *options)
{
     int blksize;
     if (options[OPT_BLKSIZE].enabled && options[OPT_BLKSIZE].specified)
     {
          blksize = atoi(options[OPT_BLKSIZE].value);
          return blksize;
     }
     return ERR;
}

int opt_get_multicast(struct tftp_opt *options, char *addr, int *port, int *mc)
{
     char *token = NULL;
     char *string = NULL;
     char *temp = NULL;

     if (options[OPT_MULTICAST].enabled && options[OPT_MULTICAST].specified)
     {
          string = strdup(options[OPT_MULTICAST].value);
          /* get first argument */
          if ((token = strtok_r(string, ",", &temp)) == NULL)
          {
               free(string);
               return ERR;
          }
          else
               Strncpy(addr, token, IPADDRLEN);
          /* get second argument */
          if ((token = strtok_r(NULL, ",", &temp)) == NULL)
          {
               free(string);
               return ERR;
          }
          else
          {
               *port = atoi(token);
               if ((*port < 0) || (*port > 65536))
               {
                    free(string);
                    return ERR;
               }
          }
          /* get third (last) argument */
          if ((token = strtok_r(NULL, ",", &temp)) == NULL)
          {
               free(string);
               return ERR;
          }
          else
          {
               *mc = atoi(token);
               if ((*mc != 0) && (*mc != 1))
               {
                    free(string);
                    return ERR;
               }
          }
          free(string);
          return *mc;
     }
     return ERR;
}

void opt_set_tsize(int tsize, struct tftp_opt *options)
{
     snprintf(options[OPT_TSIZE].value, VAL_SIZE, "%d", tsize);
}

void opt_set_timeout(int timeout, struct tftp_opt *options)
{
     snprintf(options[OPT_TIMEOUT].value, VAL_SIZE, "%d", timeout);
}

void opt_set_blksize(int blksize, struct tftp_opt *options)
{
     snprintf(options[OPT_BLKSIZE].value, VAL_SIZE, "%d", blksize);
}

void opt_set_multicast(struct tftp_opt *options, char *addr, int port, int mc)
{
     snprintf(options[OPT_MULTICAST].value, VAL_SIZE, "%s,%d,%d", addr, port,
              mc);
}

/*
 * Format the content of the options structure (this is the content of a
 * read or write request) to a string.
 */
void opt_request_to_string(struct tftp_opt *options, char *string, int len)
{
     int i, index = 0;

     for (i = 0; i < 2; i++)
     {
          if ((index + strlen(options[i].option) + 2) < len)
          {
               Strncpy(string + index, options[i].option, len - index);
               index += strlen(options[i].option);
               Strncpy(string + index, ": ", len - index);
               index += 2;
          }
          if ((index + strlen(options[i].value) + 2) < len)
          {
               Strncpy(string + index, options[i].value, len - index);
               index += strlen(options[i].value);
               Strncpy(string + index, ", ", len - index);
               index += 2;
          }
     }
     opt_options_to_string(options, string + index, len - index);
}

/*
 * Convert the options structure to a string.
 */
void opt_options_to_string(struct tftp_opt *options, char *string, int len)
{
     int i, index = 0;

     for (i = 2; i < OPT_NUMBER; i++)
     {
          if (options[i].specified && options[i].enabled)
          {
               if ((index + strlen(options[i].option) + 2) < len)
               {
                    Strncpy(string + index, options[i].option, len - index);
                    index += strlen(options[i].option);
                    Strncpy(string + index, ": ", len - index);
                    index += 2;
               }
               if ((index + strlen(options[i].value) + 2) < len)
               {
                    Strncpy(string + index, options[i].value, len - index);
                    index += strlen(options[i].value);
                    Strncpy(string + index, ", ", len - index);
                    index += 2;
               }
          }
     }
     if (index > 0)
          string[index - 2] = 0;
     else
          string[0] = 0;
}
