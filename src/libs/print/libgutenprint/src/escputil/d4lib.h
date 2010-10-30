/*
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

#ifndef D4LIB_H

#define D4LIB_H

extern int debugD4;   /* allow printout of debug informations */

extern int EnterIEEE(int fd);
extern int Init(int fd);
extern int Exit(int fd);
extern int GetSocketID(int fd, const char *serviceName);
extern int OpenChannel(int fd, unsigned char sockId, int *sndSz, int *rcvSz);
extern int CloseChannel(int fd, unsigned char socketID);
extern int CreditRequest(int fd, unsigned char socketID);
extern int Credit(int fd, unsigned char socketID, int credit);

/* convenience function */
extern int SafeWrite(int fd, const void *data, int len);
extern int askForCredit(int fd, unsigned char socketID, int *sndSz, int *rcvSz);
extern int writeData(int fd, unsigned char socketID, const unsigned char *buf, int len, int eoj);
extern int readData(int fd, unsigned char socketID, unsigned char *buf, int len);
extern int writeAndReadData(int fd, unsigned char socketID,
			    const unsigned char *cmd, int cmd_len, int eoj,
			    unsigned char *buf, int len, int *sndSz, int *rcvSz,
			    int (*test)(const unsigned char *buf));
extern int readAnswer(int fd, unsigned char *buf, int len, int allowExtra);
extern void flushData(int fd, unsigned char socketID);
extern void setDebug(int debug);

extern int d4WrTimeout;
extern int d4RdTimeout;
extern int ppid;

#if D4_DEBUG
#define DEBUG 1
#endif

#endif

