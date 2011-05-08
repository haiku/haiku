/* d4lib.c 
 * Copyright (C) 2001 Jean-Jacques Sarton jj.sarton@t-online.de
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* this file will be a library, which will allow to use the EPSON
 * Stylus Scanner and also allow to send commands to the
 * Stylus Color 480/580.
 *
 * At this stage a lot of things are not implemented and the
 * first goal is to get the Stylus Scanner printing.
 * This may be reached if this file is compiled with the TEST
 * option set.
 *
 * I donÅ¥t own a Stylus Scanner and I am not able to test this
 * code, as desired. Printing on a Stylus Photo 1290 work fine
 * with this.
 *
 * The best way to get the Stylus Scanner working is to test
 * this and also correct my possibly errors.
 * Programming knowledge will be helpfull for this.
 *
 */
 

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "d4lib.h"


#ifndef RDTIMEOUT
#define RDTIMEOUT     10000
#define WRTIMEOUT     10000
#define RDDATATIMEOUT 1000
#define MICROTIMEOUT  1
#endif

int d4WrTimeout     = WRTIMEOUT;
int d4RdTimeout     = RDTIMEOUT;
int d4RdDataTimeout = RDDATATIMEOUT;
int d4MicroTimeout  = 1;
int ppid        = 0;

int debugD4     = 1;

typedef void (*signalHandler_t)(int);
static signalHandler_t sig;
static int timeoutGot = 0;
static int _readData(int fd, unsigned char *buf, int len);

static int d4Errno = 0;

/* commands for the D4 protocol

Transaction    Cmd    Reply
-------------- ----   -----    
Init           0x00   0x80
OpenChannel    0x01   0x81
CloseChannel   0x02   0x82
Credit         0x03   0x83
CreditRequest  0x04   0x84
Exit           0x08   0x88
GetSocketID    0x09   0x89
GetServiceName 0x0a   0x8a
Error          0x7f   -

*/

typedef struct cmdHeader_s
{
   unsigned char psid;
   unsigned char ssid;
   unsigned char lengthH;
   unsigned char lengthL;
   unsigned char credit;
   unsigned char control;
   unsigned char command;
} cmdHeader_t;

typedef struct replyHeader_s
{
   unsigned char psid;
   unsigned char ssid;
   unsigned char lengthH;
   unsigned char lengthL;
   unsigned char credit;
   unsigned char control;
   unsigned char command;
   unsigned char result;
} replyHeader_t;

typedef struct init_s
{
   cmdHeader_t head;
   unsigned char          revision;
} init_t;

typedef struct initReply_s
{
   replyHeader_t head;
   unsigned char            revision;
} initReply_t;

typedef struct error_s
{
   replyHeader_t head;
   unsigned char            epsid;
   unsigned char            essid;
   unsigned char            ecode;
} error_t;

/* results */
typedef struct errorMessage_s
{
   unsigned char    result;
   const char *message;
   int   errorClass;
}  errorMessage_t;

#define RECOVERABLE 0
#define FATAL       1

static errorMessage_t errorMessage[] =
{
   { 0x01, "Unable to begin conversation, try later."              ,0 },
   { 0x02, "Protocol revision not supported."                      ,1 },
   { 0x03, "Transaction channel canÅ¥t be closed."                  ,0 },
   { 0x04, "No sufficient resources available now."                ,0 },
   { 0x05, "Connection denied."                                    ,1 },
   { 0x06, "Channel allready open."                                ,0 },
   { 0x07, "Credit overflow, previous credit remain valid."        ,0 },
   { 0x08, "Channel is not open."                                  ,1 },
   { 0x09, "Service not available on specified socket."            ,1 },
   { 0x0a, "Service name to socket ID failed."                     ,1 },
   { 0x0b, "Init transaction failed."                              ,1 },
   { 0x0c, "Invalid packet size."                                  ,1 },
   { 0x0d, "Requested packed size is 0, no data can be transfered.",0 },
   { 0x80, "Malformed packet, ignored."                            ,1 },
   { 0x81, "No credit for received packet, ignored"                ,0 },
   { 0x82, "Reply donÅ¥t match with outstanding command, ignored."  ,1 },
   { 0x83, "Packet size greater as negotiated size."               ,1 },
   { 0x84, "Data received for a non opened channel."               ,1 },
   { 0x85, "Reply with unknown result value received."             ,1 },
   { 0x86, "Piggybacked credit in data packet cause overflow."     ,1 },
   { 0x87, "Unknown 1284.4 Reply."                                 ,0 },
   { 0x00, NULL                                                    ,0 }
};

#define RESET_TIMER(ti,oti) { signal(SIGALRM, sig); \
                              memset((void*)&ti,0,sizeof(ti)); \
                              memset((void*)&oti,0,sizeof(oti)); \
                              setitimer( ITIMER_REAL ,&ti, &oti); \
                            }

#define SET_TIMER(ti,oti,val) { memset((void*)&ti,0,sizeof(ti)); \
                                memset((void*)&oti,0,sizeof(oti)); \
                                ti.it_value.tv_sec  = val/1000; \
                                ti.it_value.tv_usec = (val%1000)*1000; \
                                setitimer( ITIMER_REAL ,&ti, &oti); \
                                sig = signal(SIGALRM, sigAlarm); \
                              }

/*******************************************************************/
/* Function printHexValues                                         */
/*                                                                 */
/* Print hex code contained in the passed buffer                   */
/*                                                                 */
/*******************************************************************/

static void printHexValues(const char *dir, const unsigned char *buf, int len)
{
   int i, j;
   int printable_count = 0;
   int longest_printable_run = 0;
   int current_printable_run = 0;
   int print_strings = 0;
   int blocks = (len + 15) / 16;
#if 0
   len = len > 30 ? 30 : len;
#endif
   printf("%s\n",dir);
   for (i = 0; i < len; i++)
     {
       if (isprint(buf[i]))
	 {
	   if (!isspace(buf[i]))
	     printable_count++;
	   current_printable_run++;
	 }
       else
	 {
	   if (current_printable_run > longest_printable_run)
	     longest_printable_run = current_printable_run;
	 }
     }
   if (current_printable_run > longest_printable_run)
     longest_printable_run = current_printable_run;
   if (longest_printable_run >= 8 ||
       ((float) printable_count / (float) len > .75))
     print_strings = 1;
   if (print_strings)
     {
       for (i = 0; i < len; i++)
	 {
	   printf("%c",isprint(buf[i])||isspace(buf[i])?buf[i]:'*');
	   if (buf[i] == ';' && i < len - 1)
	     printf("\n");
	 }
       printf("\n");
     }
   for (j = 0; j < blocks; j++)
     {
       int baseidx = j * 16;
       int count = len;
       if (count > baseidx + 16)
	 count =  baseidx + 16;
       printf("%4d: ", baseidx);
       for ( i = baseidx; i < count;i++)
	 {
	   if (i % 4 == 0)
	     printf(" ");
	   printf(" %02x",buf[i]);
	 }
       if (print_strings)
	 {
	   printf("\n      ");
	   for ( i = baseidx; i < count;i++)
	     {
	       if (i % 4 == 0)
		 printf(" ");
	       printf("  %c",
		       isprint(buf[i]) && !isspace(buf[i]) ? buf[i] : ' ');
	     }
	 }
       printf("\n");
     }
}

int SafeWrite(int fd, const void *data, int len)
{
  int status;
  int retries=30;
  if (debugD4)
    printHexValues("SafeWrite: ", data, len);
  do
    {
      status = write(fd, data, len);
      if(status < len)
	usleep(d4WrTimeout);
      retries--;
    }
  while ((status < len) && (retries > 0));
  return(status);
}

/*******************************************************************/
/* Function sigAlarm(int code)                                     */
/*                                                                 */
/* do nothing, avoid printing of undesired messages and/or         */
/* other not desirable actions                                     */
/*                                                                 */
/*******************************************************************/

static void sigAlarm(int code)
{
   timeoutGot = -1;
}


/*******************************************************************/
/* Function printError()                                           */
/*    print an error message on stdout                             */
/*                                                                 */
/* Input:  unsigned char errorNb the error number                  */
/*                                                                 */
/* Return: fatal = 1 or recoverable = 0                            */
/*                                                                 */
/*******************************************************************/

static int printError(unsigned char errorNb)
{
   errorMessage_t *msg = errorMessage;
   if ( errorNb == 0 )
   {
      return 0;
   }
   while (  msg->result )
   {
      if ( msg->result == errorNb )
      {
	 if (debugD4)
	   printf("%s\n", msg->message);
         return msg->errorClass;
      }
      msg++;
   }
   printf("Unknown IEEE 1284.4 error number %d\n",errorNb);
   return 0;
}


/*******************************************************************/
/* Function printCmdType()                                         */
/*        print on stdout the command name                         */
/* Input:  unsigned char *cmd   the data are to be put here        */
/*                                                                 */
/* Return: -                                                       */
/*                                                                 */
/*******************************************************************/

static void printCmdType(unsigned char *cmd)
{
   if (cmd[6] & 0x80)
     printf(">>>");
   if ( cmd[0] == 0 && cmd[1] == 0 )
   {
      switch(cmd[6] & 0x7f)
      {
         case    0: printf("--- Init           ---\n");break;
         case    1: printf("--- OpenChannel    ---\n");break;
         case    2: printf("--- CloseChannel   ---\n");break;
         case    3: printf("--- Credit         ---\n");break;
         case    4: printf("--- CreditRequest  ---\n");break;
         case    8: printf("--- Exit           ---\n");break;
         case    9: printf("--- GetSocketID    ---\n");break;
         case   10: printf("--- GetServiceName ---\n");break;
         case 0x45: printf("--- EnterD4Mode    ---\n");break;
         case 0x7f: printf("--- Error          ---\n");break;
         default:   printf("--- ?????????????? ---\n");break;
      }
   }
   else
   {
      printf("--- Send Data      ---\n");
   }
}


/*******************************************************************/
/* Function writeCmd()                                             */
/*        write a commmand                                         */
/* Input:  int   fd    file handle                                 */
/*         char *cmd   the data are to be put here                 */
/*         int   len   the number of bytes to read                 */
/*                                                                 */
/* Return: number of bytes write or -1                             */
/*                                                                 */
/*******************************************************************/

static int writeCmd(int fd, unsigned char *cmd, int len)
{
   int w;
   int i = 0;
   struct  itimerval ti, oti;

# if PTIME
   struct timeval beg, end;
   long dt;
# endif
   if ( debugD4 )
   {
      printCmdType(cmd);
# if PTIME
      gettimeofday(&beg, NULL);
# endif
      if ( cmd[0] == 0 && cmd[1] == 0 )
      {
         printHexValues("Send: ", cmd, len);
      }
      else
      {
         printHexValues("Send: ", cmd, 6);
      }
   }

   /* according to Glen Steward, this will solve problems  */
   /* for the cartridge exchange with the Stylus Color 580 */
   usleep(d4MicroTimeout);

   timeoutGot = 0;
   errno = 0;
   while ( i < len )
   {
      SET_TIMER(ti,oti,d4WrTimeout);         
      w = SafeWrite(fd, cmd+i,len-i);
      RESET_TIMER(ti,oti);
      if ( w < 0 )
      {
         if ( debugD4 )
         {
	   perror("Write error");
         }
         i= -1;
         break;
      }
      else
         i += w;
   }

   if ( debugD4 )
   {
# if PTIME
      gettimeofday(&end, NULL);
      dt = (end.tv_sec  - beg.tv_sec) * 1000000;
      dt += end.tv_usec - beg.tv_usec;
      printf("Write time %5.3f s\n",(double)dt/1000000);
# endif
   }

   if ( timeoutGot )
      return -1;
   return i;
}

/*******************************************************************/
/* Function readAnswer()                                           */
/*        Read the datas returned by the printer                   */
/* Input:  int   fd    file handle                                 */
/*         char *buf   the data are to be put here                 */
/*         int   len   the number of bytes to read                 */
/*                                                                 */
/* Return: number of bytes read. -1 on error                       */
/*                                                                 */
/*******************************************************************/

int readAnswer(int fd, unsigned char *buf, int len, int allowExtra)
{
   int rd    = 0;
   int total = 0;
   struct timeval beg, end;
   struct itimerval ti, oti;
   long dt;
   int count = 0;
   int first_read = 1;
   int excess = 0;
   /* wait a little bit before reading an answer */
   usleep(d4RdTimeout);

   /* for error handling in case of timeout */
   timeoutGot = 0;

   /* set errno to 0 in order to get correct informations */
   /* in case of error                                    */
   errno = 0;

   gettimeofday(&beg, NULL);

   if (debugD4)
     printf("length: %i\n", len);
   while ( total < len )
   {
      SET_TIMER(ti,oti, d4RdTimeout);
      rd = read(fd, buf+total, len-total);
      if (debugD4)
	{
	  if (first_read)
	    {
	      printf("read: ");
	      first_read = 0;
	    }
	  if (rd < 0)
	    {
	      printf("%i %s\n", rd, errno != 0 ?strerror(errno) : "");
	      first_read = 1;
	    }
	  else
	    printf("%i ", rd);
	}
      RESET_TIMER(ti,oti);
      if ( rd <= 0 )
      {
         gettimeofday(&end, NULL);
         dt  = (end.tv_sec  - beg.tv_sec) * 1000;
         dt += (end.tv_usec - beg.tv_usec) / 1000;
         if ( dt > d4RdTimeout * 2 )
         {
            if ( debugD4 )
               printf("Timeout 1 at readAnswer() rcv %d bytes\n",total);
            timeoutGot = 1;
            break;
         }
         count++;
         if ( count >= 100 )
         {
             timeoutGot = 1;
             if ( rd == 0 )
                errno = -1; /* tell that there is an abnormal condition */
             break;
         }
         errno = 0;
      } else {
         total += rd;
         if ( total > 3 )
         {
            /* the bytes idx 2 and 3 contain the length */
            /* in case of errors this may differ from   */
            /* the expected lenght. Setting len to this */
            /* value will avoid waiting for timeout     */
	    int newlen = (buf[2] << 8) + buf[3];
	    if (len > newlen)
	      {
		if (debugD4)
		  printf("Changing len from %d to %d\n", len, newlen);
		len = newlen;
	      }
	    else if (len < newlen)
	      {
		excess = newlen - len;
		if (debugD4)
		  printf("Expected %d, getting %d, %sflushing %d\n",
			  len, newlen, allowExtra ? "not " : "", excess);
	      }
         }
      }
      usleep(d4RdTimeout);
   }
   if (! allowExtra)
     {
       int retry_count = 0;
       while (excess > 0)
	 {
	   char wastebuf[256];
	   int bytes = excess > 256 ? 256 : excess;
	   int status = read(fd, wastebuf, bytes);
	   if (status < 0)
	     break;
	   else if (status == 0 && retry_count > 2)
	     break;
	   else if (status == 0)
	     retry_count++;
	   else
	     retry_count = 0;
	   if (status < bytes)
	     usleep(d4RdTimeout);
	   if (debugD4)
	     printHexValues("waste", (const unsigned char *) wastebuf, status);
	   excess -= status;
	 }
     }
   
   if ( debugD4 )
   {
      printCmdType(buf);
#  if PTIME
      gettimeofday(&end, NULL);
#  endif
      printf("total: %i\n", total);
      printHexValues("Recv: ",buf,total);
#  if PTIME
      dt = (end.tv_sec  - beg.tv_sec) * 1000000;
      dt += end.tv_usec - beg.tv_usec;
      printf("Read time %5.3f s\n",(double)dt/1000000);
#  endif
   }
   if ( timeoutGot )
   {
      if ( debugD4 )
         printf("Timeout 2 at readAnswer()\n");
      return -1;
   }
   return total;
}

static void _flushData(int fd)
{
   int rd    = 0;
   struct itimerval ti, oti;
   char buf[1024];
   int len = 1023;
   int count = 200;
   usleep(d4RdTimeout);

   /* for error handling in case of timeout */
   timeoutGot = 0;

   /* set errno to 0 in order to get correct informations */
   /* in case of error                                    */
   errno = 0;

   if (debugD4)
     printf("flush data: length: %i\n", len);
   do
     {
       usleep(d4RdTimeout);
       SET_TIMER(ti,oti, d4RdTimeout);
       rd = read(fd, buf, len);
       if (debugD4)
	 printf("flush: read: %i %s\n", rd,
		 rd < 0 && errno != 0 ?strerror(errno) : "");
       RESET_TIMER(ti,oti);
       count--;
     } while ( count > 0 && (rd > 0 || (rd < 0 && errno == EAGAIN)));
}

/*******************************************************************/
/* Function _readData()                                            */
/*        Read the datas returned by the printer                   */
/* Input:  int   fd    file handle                                 */
/*         char *buf   the data are to be put here                 */
/*         int   len   the number of bytes to read                 */
/*                                                                 */
/* Return: number of bytes read. -1 on error                       */
/*                                                                 */
/*******************************************************************/

static int _readData(int fd, unsigned char *buf, int len)
{
   int rd    = 0;
   int total = 0;
   int toGet = 0;
   unsigned char  header[6];
   struct timeval beg, end;
   long dt;
   struct itimerval ti, oti;

   /* set errno to 0 in order to get correct informations */
   /* in case of error                                    */
   errno = 0;

   /* read the first 6 bytes */
   gettimeofday(&beg, NULL);
   while ( total < 6 )
   {
      SET_TIMER(ti,oti, d4RdTimeout);
      rd = read(fd, header+total, 6-total);
      RESET_TIMER(ti,oti);
      if ( rd <= 0 )
      {
         gettimeofday(&end, NULL);
         dt  = (end.tv_sec  - beg.tv_sec) * 1000;
         dt += (end.tv_usec - beg.tv_usec) / 1000;
         if ( dt > d4RdTimeout*3 )
         {
            if ( debugD4 )
               printf("Timeout at _readData(), dt = %ld ms\n", dt);
            return -1;
            break;
         }
         continue;
      }
      else
      {
         total += rd;
      }
   }

   if ( debugD4 )
      printHexValues("Recv: ",header,total);

   if ( total == 6 )
   {
      toGet = (header[2] >> 8) + header[3] - 6;
      if (debugD4)
	printf("toGet: %i\n", toGet);
      total = 0;
      gettimeofday(&beg, NULL);
      while ( total < toGet )
      {
         SET_TIMER(ti,oti, d4RdTimeout);
         rd = read(fd, buf+total, toGet-total);
         RESET_TIMER(ti,oti);
         if ( rd <= 0 )
         {
            gettimeofday(&end, NULL);
            dt  = (end.tv_sec  - beg.tv_sec) * 1000;
            dt += (end.tv_usec - beg.tv_usec) / 1000;
            if ( dt > d4RdTimeout*3 )
            {
               if ( debugD4 )
                  printf("Timeout at _readData(), dt = %ld ms\n",dt);
               return -1;
               break;
            }
            continue;
         }
         else
         {
            total += rd;
         }
      }
      if ( debugD4 )
         printHexValues("Recv: ",buf,total);
      return total;
   }

   return -1;
}

/*******************************************************************/
/* Function sendReceiveCmd()                                       */
/*        send a command and get the answer.                       */
/* Input:  int   fd    file handle                                 */
/*         char *buf   the data are to be put here                 */
/*         int   len   the number of bytes to read                 */
/*                                                                 */
/* Return: number of bytes read                                    */
/*                                                                 */
/*******************************************************************/

static int sendReceiveCmd(int fd, unsigned char *cmd, int len, unsigned char *answer, int expectedlen, int allowExtra)
{
   int rd;
   d4Errno = 0;
   if ( (rd = writeCmd(fd, cmd, len ) ) != len )
   {
      if ( rd < 0 ) return -1;
      return 0;
   }
   rd = readAnswer(fd, answer, expectedlen, allowExtra );
   if ( rd == 0 )
   {
      /* no answer from device */
      return 0;
   }
   else if ( rd < 0 )
   {
      /* interrupted write call */
      if ( debugD4 )
         printf("interrupt received\n");
      return -1;
   }
   else
   {
      /* check result */
      if ( answer[6] == 0x7f )
      {
         printError(answer[9]);
	 d4Errno = answer[9];
         return -1;
      }
      else if (  answer[7] != 0 )
      {
	 d4Errno = answer[7];
         if ( printError(answer[7]) )
         {
            return -1;
         }
         return 0;
      }
      else
      {
         return rd;
      }
   }
}

/*******************************************************************/
/* Function EnterIEEE()                                            */
/*        send a command and get the answer.                       */
/* Input:  int   fd    file handle                                 */
/*                                                                 */
/* Return: 0 on error 1 if all is OK                               */
/*                                                                 */
/*******************************************************************/

int EnterIEEE(int fd)
{
   unsigned char buf[200];
   unsigned char cmd[] = 
   {
      0x00, 0x00, 0x00, 0x1b, 0x01, '@', 'E', 'J', 'L', ' ',
      '1', '2', '8', '4', '.', '4', 0x0a, '@', 'E', 'J',
      'L', 0x0a, '@', 'E', 'J', 'L', 0x0a
   };
   int rd;
   memset(buf, 0, sizeof(buf));
Loop:
   if ( writeCmd(fd, cmd, sizeof(cmd) ) != sizeof(cmd) )
   {
      return 0;
   }
   rd = readAnswer(fd, buf, 8, 0);
   if ( rd == 0 )
   {
      /* no answer from device */
      if (debugD4)
        printf(">>>No answer from printer\n");
      return 0;
   }
   else if ( rd < 0 )
   {
      if (debugD4)
        printf(">>>Interrupted write\n");
      /* interrupted write call */
      return 0;
   }
   else
   {
      int i;
      /* check result */
      for (i=0; i < rd; i++ )
        if ( buf[i] != 0 )
           break;
      if ( i == rd ) goto Loop;
      return 1;
   }
}

/*******************************************************************/
/* Function Init()                                                 */
/*        handle the init command                                  */
/* Input:  int   fd    file handle                                 */
/*                                                                 */
/* Return: 0 on error 1 if all is OK                               */
/*                                                                 */
/*******************************************************************/

int Init(int fd)
{
   unsigned char buf[20];
   init_t cmd;
   int rd;
   
   cmd.head.psid     = 0;
   cmd.head.ssid     = 0;
   cmd.head.lengthH  = 0;
   cmd.head.lengthL  = 8;
   cmd.head.credit   = 1;
   cmd.head.control  = 0;
   cmd.head.command  = 0;
   cmd.revision      = 0x10;
    
   rd = sendReceiveCmd(fd, (unsigned char*)&cmd, sizeof(cmd), buf, 9, 0 );
   return rd == 9 ? 1 : 0;
}

/*******************************************************************/
/* Function Exit()                                                 */
/*        handle the Exit command                                  */
/* Input:  int   fd    file handle                                 */
/*                                                                 */
/* Return: 0 on error 1 if all is OK                               */
/*                                                                 */
/*******************************************************************/

int Exit(int fd)
{
   int rd;
   unsigned char  buf[20];
   cmdHeader_t cmd;
   cmd.psid     = 0;
   cmd.ssid     = 0;
   cmd.lengthH  = 0;
   cmd.lengthL  = 7;
   cmd.credit   = 1;
   cmd.control  = 0;
   cmd.command  = 8;

   rd = sendReceiveCmd(fd, (unsigned char*)&cmd, sizeof(cmd), buf, 8, 0 );
   return rd > 0 ? 1 : rd;
}

/*******************************************************************/
/* Function GetSocketID()                                          */
/*        handle the GetSocketID command                           */
/* Input:  int   fd    file handle                                 */
/*         char *serviceName  name of wanted service               */
/*                                                                 */
/* Return: 0 on error else the socket ID                           */
/*                                                                 */
/*******************************************************************/

int GetSocketID(int fd, const char *serviceName)
{
   /* the service name may not be longer as 40 bytes */
   int len = sizeof(cmdHeader_t) + strlen(serviceName);
   char buf[100];
   unsigned char rBuf[100];
   int rd;
   cmdHeader_t *cmd = (cmdHeader_t*)buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = len & 0xff;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 0x09;
   strcpy(buf + sizeof(cmdHeader_t), serviceName);

   rd = sendReceiveCmd(fd, (unsigned char*)buf, len, rBuf, len + 2, 0);
   if ( rd > 0 )
   {
      return rBuf[8];
   }
   else
   {
      return 0;
   }
}

/*******************************************************************/
/* Function OpenChannel()                                          */
/*        handle the OpenChannel command                           */
/* Input:  int   fd    file handle                                 */
/* I/O:    int  *sndSz The size for sendig of datas                */
/*         int  *recSz The size for receiving of datas             */
/*                                                                 */
/* Return: -1 on error or 1 if all is OK                           */
/*                                                                 */
/*******************************************************************/

int OpenChannel(int fd, unsigned char sockId, int *sndSz, int *rcvSz)
{
   unsigned char  cmd[17];
   unsigned char  buf[20];
   int rd;
   int i;

   for(i = 0; i < 5; i++)	/* Retry count */
   {
      cmd[0]  = 0;       /* transaction sockets */
      cmd[1]  = 0;
      cmd[2]  = 0;       /* len */
      cmd[3]  = 17;      /* len */
      cmd[4]  = 1;       /* credit */
      cmd[5]  = 0;       /* control */
      cmd[6]  = 1;       /* command */
      cmd[7]  = sockId;  /* sockets # */
      cmd[8]  = sockId;  /* sockets # */
      cmd[9]  = *sndSz >> 8;   /* packet size in send dir */
      cmd[10] = *sndSz & 0xff;
      cmd[11] = *rcvSz >> 8;   /* packet size in recv dir */
      cmd[12] = *rcvSz & 0xff;
      cmd[13] = 0;    /* max outstanding Credit, must be 0 */
      cmd[14] = 0;
      cmd[15] = 0;    /* initial credit for us ? */
      cmd[16] = 0;

      rd = sendReceiveCmd(fd, cmd, 17, buf, 16, 0);
      if ( rd == -1 )
      {
	if (debugD4)
	  printf("OpenChannel %d fails, error %d\n", sockId, d4Errno);
	if (d4Errno == 6)	/* channel already open */
	  {
	    if ( debugD4 )
	      printf("Channel %d already open, closing\n", sockId);
	    CloseChannel(fd, sockId);
	    continue;
	  }
	else
	  return -1;
      }
      else if ( rd == 16 )
      {
         if ( buf[7] == 4 )
         {
            /* device canÅ¥t allocate resources now */
            continue;
         }
         else if ( buf[7] != 0 )
         {
	   if (debugD4)
	     printf("OpenChannel %d fails, hard error\n", sockId);
            /* hard error */
            return -1;
         }
         *sndSz = (buf[10]<<8) + buf[11];
         *rcvSz = (buf[12]<<8) + buf[13];
         break;
      }
      else
      {
	if (d4Errno == 6)	/* channel already open */
	  {
	    if ( debugD4 )
	      printf("Channel %d already open, closing\n", sockId);
	    CloseChannel(fd, sockId);
	    continue;
	  }
         /* at this stage we can only have an error */
	if (debugD4)
	  printf("OpenChannel %d fails, wrong count %d\n", sockId, rd);
         return -1;
      }
   }
   return 1;                 
}

/*******************************************************************/
/* Function CloseChannel()                                         */
/*        handle the CloseChannel command                          */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  he socket to close                      */
/*                                                                 */
/* Return: -1 on error or 1 if all is OK                           */
/*                                                                 */
/*******************************************************************/

int CloseChannel(int fd, unsigned char socketID)
{
   unsigned char buf[100];
   int           rd;
   cmdHeader_t *cmd = (cmdHeader_t *)buf;
   cmd->psid     =  0;
   cmd->ssid     =  0;
   cmd->lengthH  =  0;
   cmd->lengthL  = 10;
   cmd->credit   =  1;
   cmd->control  =  0;
   cmd->command  =  2;
   buf[sizeof(cmdHeader_t)+0] = socketID;
   buf[sizeof(cmdHeader_t)+1] = socketID;
   buf[sizeof(cmdHeader_t)+2] = 0;
   rd = sendReceiveCmd(fd, buf,10, buf, 10, 0);
   return rd == 10 ? 1 : rd;
}

/*******************************************************************/
/* Function CreditRequest()                                        */
/*        handle the CreditRequest command                         */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  he socket to close                      */
/*                                                                 */
/* Return: -1 on error else the credit got                         */
/*                                                                 */
/*******************************************************************/

int CreditRequest(int fd, unsigned char socketID)
{
   int           rd;
   unsigned char            buf[100];
   unsigned char            rBuf[100];
   cmdHeader_t *cmd = (cmdHeader_t *)buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = 13;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 4;
   buf[sizeof(cmdHeader_t)+0] = socketID;
   buf[sizeof(cmdHeader_t)+1] = socketID;
   buf[sizeof(cmdHeader_t)+2] = 0x00;
   buf[sizeof(cmdHeader_t)+3] = 0x80;
   buf[sizeof(cmdHeader_t)+4] = 0xff;
   buf[sizeof(cmdHeader_t)+5] = 0xff;
   rd = sendReceiveCmd(fd, buf, 13, rBuf, 12, 0);
   if ( rd == 12 )
   {
      /* this is the credit */
      return (rBuf[10]*256)+rBuf[11];
   }
   else
   {
      return rd > 0 ? 0 : rd; /* there was an error */
   } 
}

/*******************************************************************/
/* Function Credit()                                               */
/*        give credit to the attached device                       */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  the socket to close                     */
/*                                                                 */
/* Return: -1 on error or 1 if all is OK                           */
/*                                                                 */
/*******************************************************************/

/* needed for sending of commands (channel 2) or scanning */
int Credit(int fd, unsigned char socketID, int credit)
{
   int rd;
   unsigned char buf[100];
   unsigned char rBuf[100];
   cmdHeader_t *cmd = (cmdHeader_t*)buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = 0x0b;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 0x03;

   buf[sizeof(cmdHeader_t)+0] = socketID;
   buf[sizeof(cmdHeader_t)+1] = socketID;
   buf[sizeof(cmdHeader_t)+2] = credit >> 8;
   buf[sizeof(cmdHeader_t)+3] = credit & 0xff;
   rd = sendReceiveCmd(fd, buf, 11, rBuf, 10, 0);
   if ( rd == 10 )
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

/*******************************************************************/
/* Function askForCredit()                                         */
/*        Convenience function                                     */
/*        handle the CreditRequest command                         */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID                               */
/* IN/Out  int  *sndSize  for error handling                       */
/* IN/Out  int  *rcvSize  for error handling                       */
/*                                                                 */
/* Return: credit                                                  */
/*                                                                 */
/* Remark: CreditRequest() will be called in a loop as long as     */
/*         the returned credit is 0                                */
/*                                                                 */
/*******************************************************************/
#define MAX_CREDIT_REQUEST 2
int askForCredit(int fd, unsigned char socketID, int *sndSize, int *rcvSize)
{
   int credit = 0;
   int count  = 0;
   int retries = 10;
   
   while (credit == 0 && retries-- >= 0 )
   {
      while((credit=CreditRequest(fd,socketID)) == 0  && count < MAX_CREDIT_REQUEST && retries-- >= 0)
         usleep(d4RdTimeout);

      if ( credit == -1 )
      {
         if ( errno == ENODEV || count == MAX_CREDIT_REQUEST )
         {
            break;
         }
         credit = 0;
         /* init printer and reopen the printer channel */
         CloseChannel(fd, socketID);
	 socketID = GetSocketID(fd, "EPSON-CTRL");
         if ( Init(fd) )
         {
	   if (debugD4)
	     printf("askForCredit init succeeded, now try to open\n");
            OpenChannel(fd, socketID, sndSize, rcvSize);
         }
      }
      /* if the parent died, live this loop if credit not got */ 
      if ( credit == 0 && getppid() == ppid )
         return 0;
      count++;
   }
   return credit;
}

/*******************************************************************/
/* Function writeData()                                            */
/*        Convenience function                                     */
/*        write the data to the device                             */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  the deetination socket       */
/*         unsigned char   *buf       the datas to be send         */
/*         int   len       how many datas are to we send           */
/*         int   eoj       set out of band flag if eoj set         */
/*                                                                 */
/* Return: number of bytes written or -1;                          */
/*                                                                 */
/*******************************************************************/

int writeData(int fd, unsigned char socketID, const unsigned char *buf, int len, int eoj)
{
   unsigned char  cmd[6];
   int wr = 0;
   int ret = 0;
   struct  itimerval ti, oti;
   struct timeval beg;
   static unsigned char *buffer = NULL;
   static int bLen   = 0;
   if ( debugD4 )
   {
      printf("--- Send Data      ---\n");
      gettimeofday(&beg, NULL);
   }
   len += 6;
   if ( len > bLen )
   {
      if ( buffer == NULL )
         buffer = (unsigned char*)malloc(len);
      else
         buffer = (unsigned char*)realloc(buffer, len);
      if ( buffer == NULL )
         return -1;
      bLen = len;
   }
   cmd[0] = socketID;
   cmd[1] = socketID;
   cmd[2] = len >> 8;
   cmd[3] = len & 0xff;
   cmd[4] = 0;
   cmd[5] = eoj ? 1 : 0;

   memcpy(buffer, cmd, 6);
   memcpy(buffer + 6, buf, len - 6 );
   while( ret > -1 && wr != len )
   {
      SET_TIMER(ti,oti,d4WrTimeout);
      ret = SafeWrite(fd, buffer+wr, len-wr );
      RESET_TIMER(ti,oti);
      if ( ret == -1 )
      {
         perror("write: ");
      }
      else
      {
         wr += ret;
      }
   }

   if ( debugD4 )
   {
# if PTIME
      gettimeofday(&end, NULL);
      dt = (end.tv_sec  - beg.tv_sec) * 1000000;
      dt += end.tv_usec - beg.tv_usec;
# endif  
      printf("Send: ");
      for ( ret = 0; (wr > 0) && (ret < ((wr > 20) ? 20 : wr)) ; ret++ )
         printf("%02x ", buffer[ret]);
      printf("\n      ");
      for ( ret = 0; (wr > 0) && (ret < ((wr > 20) ? 20 : wr)) ; ret++ )
         printf("%c  ", isprint(buffer[ret])&&!isspace(buffer[ret])?buffer[ret]:' ');
      printf("\n");
# if PTIME
       printf("Write time %5.3f s\n",(double)dt/1000000);
# endif
   }

   if (  wr > 6 )
      wr -= 6;
   else
      wr = -1;
   return wr;

}

/*******************************************************************/
/* Function readData()                                             */
/*        Convenience function                                     */
/*        give credit and read then the expected datas             */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  the destination socket       */
/*         unsigned char   *buf       the datas to be send         */
/*         int   len       howmany datas are to we send            */
/*                                                                 */
/* Return: number of bytes read or -1;                             */
/*                                                                 */
/*******************************************************************/

int readData(int fd, unsigned char socketID, unsigned char *buf, int len)
{
   int ret;
   /* give credit */
   if ( Credit(fd, socketID, 1) == 1 )
   {
      /* wait a little bit */
      usleep(d4RdDataTimeout);
      ret = _readData(fd, buf, len);
      return ret; 
   }
   else
   {
      return -1;
   }
}

/*******************************************************************/
/* Function writeAndReadData()                                     */
/*        Convenience function                                     */
/*        give credit and read then the expected datas             */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  the destination socket       */
/*         unsigned char   *cmd       the datas to be send         */
/*         int   cmd_len       howmany datas are to we send        */
/*         int   eoj       set out of band flag if eoj set         */
/*         unsigned char   *buf       the datas to be send         */
/*         int   *sndSz    Send buffer size                        */
/*         int   *rcvSz    Receive buffer size                     */
/*         int   len       how many datas are to we send           */
/*         fptr  test      function to verify buffer contents      */ 
/*                                                                 */
/* Return: number of bytes read or -1;                             */
/*                                                                 */
/* This allows us to give credit before sending the command.       */
/* Sending the command and then giving credit sometimes causes     */
/* the actual data to be sent as a reply to the credit command.    */
/*                                                                 */
/*******************************************************************/

int writeAndReadData(int fd, unsigned char socketID,
		     const unsigned char *cmd, int cmd_len, int eoj,
		     unsigned char *buf, int len, int *sndSz, int *rcvSz,
		     int (*test)(const unsigned char *buf))
{
   int ret;
   int retry = 5;
   int credit = askForCredit(fd, socketID, sndSz, rcvSz);
   if (credit < 0)
     return -1;
   /* give credit */
   if ( Credit(fd, socketID, 1) == 1 )
     {
       if (writeData(fd, socketID, cmd, cmd_len, eoj) <= 0)
	 return -1;
       /* wait a little bit */
       do
	 {
	   usleep(d4RdDataTimeout);
	   ret = _readData(fd, buf, len);
	   if (ret < 0)
	     return ret;
	 } while (retry-- >= 0 && (!test || !(*test)(buf)));
       return ret;
     }
   else
     return -1;
}

/*******************************************************************/
/* Function readData()                                             */
/*        Convenience function                                     */
/*        give credit and read then the expected datas             */
/* Input:  int   fd    file handle                                 */
/*         unsigned char    socketID  the destination socket                  */
/*         unsigned char   *buf       the datas to be send                    */
/*         int   len       howmany datas are to we send            */
/*                                                                 */
/* Return: number of bytes written or -1;                          */
/*                                                                 */
/*******************************************************************/

void flushData(int fd, unsigned char socketID)
{
  if (debugD4)
    printf("flushData %d\n", socketID);
   /* give credit */
   if (socketID != (unsigned char) -1)
     {
       if ( Credit(fd, socketID, 1) == 1 )
	 {
	   /* wait a little bit */
	   usleep(d4RdDataTimeout);
	   _flushData(fd);
	 }
     }
   else
     _flushData(fd);
}

/*******************************************************************/
/* Function clearSndBuf()                                          */
/*        Convenience function                                     */
/*                                                                 */
/* Input:  int   fd    file handle                                 */
/*                                                                 */
/*                                                                 */
/*******************************************************************/

static void clearSndBuf(int fd)
{
   char             buf[256];
   struct itimerval ti, oti;
   
   SET_TIMER(ti,oti, d4RdTimeout);
   while ( read(fd, buf, sizeof(buf) ) > 0 )
      SET_TIMER(ti,oti, d4RdTimeout);
   RESET_TIMER(ti,oti);
}

void setDebug(int debug)
{
  debugD4 = debug;
}

#if 0 /* implementation later ? */
int InitReply(int fd)
{
   unsigned char buf[20];
   initReply_t cmd;

   cmd.head.psid     = 0;
   cmd.head.ssid     = 0;
   cmd.head.lengthH  = 0;
   cmd.head.lengthL  = 9;
   cmd.head.credit   = 1;
   cmd.head.control  = 0;
   cmd.head.command  = 0x80;
   cmd.head.result   = 0;   /* put the correct value here, 0,1,2 or 0x0b */
   cmd.revision      = 0x10;
    
}

int ExitReply(int fd)
{
   replyHeader_t cmd;
   cmd.psid     = 0;
   cmd.ssid     = 0;
   cmd.lengthH  = 0;
   cmd.lengthL  = 7;
   cmd.credit   = 1;    /* always ignored */
   cmd.control  = 0;
   cmd.command  = 0x88;
   cmd.result   = 0x0;  /* put the correct value here always 0 */
}

int Error(int fd)
{
   char
   error_t cmd;
   cmd.head.psid     = 0;
   cmd.head.ssid     = 0;
   cmd.head.lengthH  = 0;
   cmd.head.lengthL  = 0x0a;
   cmd.head.credit   = 0;
   cmd.head.control  = 0;
   cmd.head.command  = 0x7f;
   cmd.epsid         = psid;
   cmd.essid         = ssid;
   cmd.ecode         = 0x80; /* + 1...7 */ 
}

int GetSocketIDReply(int fd, unsigned char socketID)
{
   /* the service name may not be longer as 40 bytes */
   int len = sizeof(replyHeader_t) + 1 + strlen(serviceName);
   unsigned char buf[100];
   replyHeader_t *cmd = buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = len & 0xff;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 0x89;
   cmd->result   = 0; /* put the correct vale here 0 or 0x0a */
   buf[sizeof(cmdHeader_t)] = socketID;
   strcpy(buf+1+sizeof(cmdHeader_t), serviceName);
   
}

int GetServiceName(inf fd, unsigned char socketID)
{
   int rd;
   unsigned char buf[100];
   cmdHeader_t *cmd = buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = 8;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 0x0a;
   buf[sizeof(cmdHeader_t)] = socketID;
}

/* as GetSocketIDReply but with command 0x8a instead 0f 0x89 */
int GetServiceNameReply(inf fd)
{
   /* the service name may not be longer as 40 bytes */
   int len = sizeof(replyHeader_t) + 1 + strlen(serviceName);
   unsigned char buf[100];
   replyHeader_t *cmd = buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = len & 0xff;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 0x8a;
   cmd->result   = 0; /* put the correct vale here 0 or 0x0a */
   buf[sizeof(replyHeader_t)] = socketID;
   strcpy(buf+1+sizeof(replyHeader_t), serviceName);
   
}  

int CreditReply(int fd, unsigned char socketID, int credit)
{
   unsigned char buf[100];
   replyHeader_t *cmd = buf;
   cmd->psid     = 0;
   cmd->ssid     = 0;
   cmd->lengthH  = 0;
   cmd->lengthL  = 0x0b;
   cmd->credit   = 1;
   cmd->control  = 0;
   cmd->command  = 0x83;
   buf[sizeof(replyHeader_t)]   = socketID;
   buf[sizeof(replyHeader_t)+1] = socketID;
}

#endif

