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


/* normal_stop notifications.  */

static struct observer_list *normal_stop_subject = NULL;

struct normal_stop_args { struct bpstats *bs; };

static void
observer_normal_stop_notification_stub (const void *data, const void *args_data)
{
  observer_normal_stop_ftype *notify = (observer_normal_stop_ftype *) data;
  const struct normal_stop_args *args = args_data;
  notify (args->bs);
}

struct observer *
observer_attach_normal_stop (observer_normal_stop_ftype *f)
{
  return generic_observer_attach (&normal_stop_subject,
				  &observer_normal_stop_notification_stub,
				  (void *) f);
}

void
observer_detach_normal_stop (struct observer *observer)
{
  generic_observer_detach (&normal_stop_subject, observer);
}

void
observer_notify_normal_stop (struct bpstats *bs)
{
  struct normal_stop_args args;
  args.bs = bs;
  if (observer_debug)
    fprintf_unfiltered (gdb_stdlog, "observer_notify_normal_stop() called\n");
  generic_observer_notify (normal_stop_subject, &args);
}

/* target_changed notifications.  */

static struct observer_list *target_changed_subject = NULL;

struct target_changed_args { struct target_ops *target; };

static void
observer_target_changed_notification_stub (const void *data, const void *args_data)
{
  observer_target_changed_ftype *notify = (observer_target_changed_ftype *) data;
  const struct target_changed_args *args = args_data;
  notify (args->target);
}

struct observer *
observer_attach_target_changed (observer_target_changed_ftype *f)
{
  return generic_observer_attach (&target_changed_subject,
				  &observer_target_changed_notification_stub,
				  (void *) f);
}

void
observer_detach_target_changed (struct observer *observer)
{
  generic_observer_detach (&target_changed_subject, observer);
}

void
observer_notify_target_changed (struct target_ops *target)
{
  struct target_changed_args args;
  args.target = target;
  if (observer_debug)
    fprintf_unfiltered (gdb_stdlog, "observer_notify_target_changed() called\n");
  generic_observer_notify (target_changed_subject, &args);
}

/* inferior_created notifications.  */

static struct observer_list *inferior_created_subject = NULL;

struct inferior_created_args { struct target_ops *objfile; int from_tty; };

static void
observer_inferior_created_notification_stub (const void *data, const void *args_data)
{
  observer_inferior_created_ftype *notify = (observer_inferior_created_ftype *) data;
  const struct inferior_created_args *args = args_data;
  notify (args->objfile, args->from_tty);
}

struct observer *
observer_attach_inferior_created (observer_inferior_created_ftype *f)
{
  return generic_observer_attach (&inferior_created_subject,
				  &observer_inferior_created_notification_stub,
				  (void *) f);
}

void
observer_detach_inferior_created (struct observer *observer)
{
  generic_observer_detach (&inferior_created_subject, observer);
}

void
observer_notify_inferior_created (struct target_ops *objfile, int from_tty)
{
  struct inferior_created_args args;
  args.objfile = objfile, args.from_tty = from_tty;
  if (observer_debug)
    fprintf_unfiltered (gdb_stdlog, "observer_notify_inferior_created() called\n");
  generic_observer_notify (inferior_created_subject, &args);
}

/* solib_unloaded notifications.  */

static struct observer_list *solib_unloaded_subject = NULL;

struct solib_unloaded_args { struct so_list *solib; };

static void
observer_solib_unloaded_notification_stub (const void *data, const void *args_data)
{
  observer_solib_unloaded_ftype *notify = (observer_solib_unloaded_ftype *) data;
  const struct solib_unloaded_args *args = args_data;
  notify (args->solib);
}

struct observer *
observer_attach_solib_unloaded (observer_solib_unloaded_ftype *f)
{
  return generic_observer_attach (&solib_unloaded_subject,
				  &observer_solib_unloaded_notification_stub,
				  (void *) f);
}

void
observer_detach_solib_unloaded (struct observer *observer)
{
  generic_observer_detach (&solib_unloaded_subject, observer);
}

void
observer_notify_solib_unloaded (struct so_list *solib)
{
  struct solib_unloaded_args args;
  args.solib = solib;
  if (observer_debug)
    fprintf_unfiltered (gdb_stdlog, "observer_notify_solib_unloaded() called\n");
  generic_observer_notify (solib_unloaded_subject, &args);
}
