/* GDB Notifications to Observers.

   Copyright 2004 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   --

   This file was generated using observer.sh and observer.texi.  */

#ifndef OBSERVER_H
#define OBSERVER_H

struct observer;
struct bpstats;
struct so_list;

/* normal_stop notifications.  */

typedef void (observer_normal_stop_ftype) (struct bpstats *bs);

extern struct observer *observer_attach_normal_stop (observer_normal_stop_ftype *f);
extern void observer_detach_normal_stop (struct observer *observer);
extern void observer_notify_normal_stop (struct bpstats *bs);

/* target_changed notifications.  */

typedef void (observer_target_changed_ftype) (struct target_ops *target);

extern struct observer *observer_attach_target_changed (observer_target_changed_ftype *f);
extern void observer_detach_target_changed (struct observer *observer);
extern void observer_notify_target_changed (struct target_ops *target);

/* inferior_created notifications.  */

typedef void (observer_inferior_created_ftype) (struct target_ops *objfile, int from_tty);

extern struct observer *observer_attach_inferior_created (observer_inferior_created_ftype *f);
extern void observer_detach_inferior_created (struct observer *observer);
extern void observer_notify_inferior_created (struct target_ops *objfile, int from_tty);

/* solib_unloaded notifications.  */

typedef void (observer_solib_unloaded_ftype) (struct so_list *solib);

extern struct observer *observer_attach_solib_unloaded (observer_solib_unloaded_ftype *f);
extern void observer_detach_solib_unloaded (struct observer *observer);
extern void observer_notify_solib_unloaded (struct so_list *solib);

#endif /* OBSERVER_H */
