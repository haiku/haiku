/* -*- buffer-read-only: t -*- vi: set ro: */
/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
#line 1
/* Hook for making the close() function extensible.
   Copyright (C) 2009 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#ifndef CLOSE_HOOK_H
#define CLOSE_HOOK_H

#ifdef __cplusplus
extern "C" {
#endif


/* Currently, this entire code is only needed for the handling of sockets
   on native Windows platforms.  */
#if WINDOWS_SOCKETS


/* An element of the list of close hooks.
   The fields of this structure are considered private.  */
struct close_hook
{
  /* Doubly linked list.  */
  struct close_hook *private_next;
  struct close_hook *private_prev;
  /* Function that treats the types of FD that it knows about and calls
     execute_close_hooks (FD, REMAINING_LIST) as a fallback.  */
  int (*private_fn) (int fd, const struct close_hook *remaining_list);
};

/* This type of function closes FD, applying special knowledge for the FD
   types it knows about, and calls execute_close_hooks (FD, REMAINING_LIST)
   for the other FD types.  */
typedef int (*close_hook_fn) (int fd, const struct close_hook *remaining_list);

/* Execute the close hooks in REMAINING_LIST.
   Return 0 or -1, like close() would do.  */
extern int execute_close_hooks (int fd, const struct close_hook *remaining_list);

/* Execute all close hooks.
   Return 0 or -1, like close() would do.  */
extern int execute_all_close_hooks (int fd);

/* Add a function to the list of close hooks.
   The LINK variable points to a piece of memory which is guaranteed to be
   accessible until the corresponding call to unregister_close_hook.  */
extern void register_close_hook (close_hook_fn hook, struct close_hook *link);

/* Removes a function from the list of close hooks.  */
extern void unregister_close_hook (struct close_hook *link);


#endif


#ifdef __cplusplus
}
#endif

#endif /* CLOSE_HOOK_H */
