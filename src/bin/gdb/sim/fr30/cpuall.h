// OBSOLETE /* Simulator CPU header for fr30.
// OBSOLETE 
// OBSOLETE THIS FILE IS MACHINE GENERATED WITH CGEN.
// OBSOLETE 
// OBSOLETE Copyright 1996, 1997, 1998, 1999, 2000, 2001 Free Software Foundation, Inc.
// OBSOLETE 
// OBSOLETE This file is part of the GNU simulators.
// OBSOLETE 
// OBSOLETE This program is free software; you can redistribute it and/or modify
// OBSOLETE it under the terms of the GNU General Public License as published by
// OBSOLETE the Free Software Foundation; either version 2, or (at your option)
// OBSOLETE any later version.
// OBSOLETE 
// OBSOLETE This program is distributed in the hope that it will be useful,
// OBSOLETE but WITHOUT ANY WARRANTY; without even the implied warranty of
// OBSOLETE MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// OBSOLETE GNU General Public License for more details.
// OBSOLETE 
// OBSOLETE You should have received a copy of the GNU General Public License along
// OBSOLETE with this program; if not, write to the Free Software Foundation, Inc.,
// OBSOLETE 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// OBSOLETE 
// OBSOLETE */
// OBSOLETE 
// OBSOLETE #ifndef FR30_CPUALL_H
// OBSOLETE #define FR30_CPUALL_H
// OBSOLETE 
// OBSOLETE /* Include files for each cpu family.  */
// OBSOLETE 
// OBSOLETE #ifdef WANT_CPU_FR30BF
// OBSOLETE #include "eng.h"
// OBSOLETE #include "cgen-engine.h"
// OBSOLETE #include "cpu.h"
// OBSOLETE #include "decode.h"
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE extern const MACH fr30_mach;
// OBSOLETE 
// OBSOLETE #ifndef WANT_CPU
// OBSOLETE /* The ARGBUF struct.  */
// OBSOLETE struct argbuf {
// OBSOLETE   /* These are the baseclass definitions.  */
// OBSOLETE   IADDR addr;
// OBSOLETE   const IDESC *idesc;
// OBSOLETE   char trace_p;
// OBSOLETE   char profile_p;
// OBSOLETE   /* ??? Temporary hack for skip insns.  */
// OBSOLETE   char skip_count;
// OBSOLETE   char unused;
// OBSOLETE   /* cpu specific data follows */
// OBSOLETE };
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #ifndef WANT_CPU
// OBSOLETE /* A cached insn.
// OBSOLETE 
// OBSOLETE    ??? SCACHE used to contain more than just argbuf.  We could delete the
// OBSOLETE    type entirely and always just use ARGBUF, but for future concerns and as
// OBSOLETE    a level of abstraction it is left in.  */
// OBSOLETE 
// OBSOLETE struct scache {
// OBSOLETE   struct argbuf argbuf;
// OBSOLETE };
// OBSOLETE #endif
// OBSOLETE 
// OBSOLETE #endif /* FR30_CPUALL_H */
