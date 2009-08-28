/* sockets.c --- wrappers for Windows socket functions

   Copyright (C) 2008-2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Simon Josefsson */

#include <config.h>

/* Specification.  */
#include "sockets.h"

#if WINDOWS_SOCKETS

/* This includes winsock2.h on MinGW. */
#include <sys/socket.h>

#include "close-hook.h"

/* Get set_winsock_errno, FD_TO_SOCKET etc. */
#include "w32sock.h"

static int
close_fd_maybe_socket (int fd, const struct close_hook *remaining_list)
{
  SOCKET sock;
  WSANETWORKEVENTS ev;

  /* Test whether fd refers to a socket.  */
  sock = FD_TO_SOCKET (fd);
  ev.lNetworkEvents = 0xDEADBEEF;
  WSAEnumNetworkEvents (sock, NULL, &ev);
  if (ev.lNetworkEvents != 0xDEADBEEF)
    {
      /* fd refers to a socket.  */
      /* FIXME: other applications, like squid, use an undocumented
	 _free_osfhnd free function.  But this is not enough: The 'osfile'
	 flags for fd also needs to be cleared, but it is hard to access it.
	 Instead, here we just close twice the file descriptor.  */
      if (closesocket (sock))
	{
	  set_winsock_errno ();
	  return -1;
	}
      else
	{
	  /* This call frees the file descriptor and does a
	     CloseHandle ((HANDLE) _get_osfhandle (fd)), which fails.  */
	  _close (fd);
	  return 0;
	}
    }
  else
    /* Some other type of file descriptor.  */
    return execute_close_hooks (fd, remaining_list);
}

static struct close_hook close_sockets_hook;

static int initialized_sockets_version /* = 0 */;

#endif

int
gl_sockets_startup (int version)
{
#if WINDOWS_SOCKETS
  if (version > initialized_sockets_version)
    {
      WSADATA data;
      int err;

      err = WSAStartup (version, &data);
      if (err != 0)
	return 1;

      if (data.wVersion < version)
	return 2;

      if (initialized_sockets_version == 0)
	register_close_hook (close_fd_maybe_socket, &close_sockets_hook);

      initialized_sockets_version = version;
    }
#endif

  return 0;
}

int
gl_sockets_cleanup (void)
{
#if WINDOWS_SOCKETS
  int err;

  initialized_sockets_version = 0;

  unregister_close_hook (&close_sockets_hook);

  err = WSACleanup ();
  if (err != 0)
    return 1;
#endif

  return 0;
}
