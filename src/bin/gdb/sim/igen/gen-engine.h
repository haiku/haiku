/* The IGEN simulator generator for GDB, the GNU Debugger.

   Copyright 2002 Free Software Foundation, Inc.

   Contributed by Andrew Cagney.

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
   Boston, MA 02111-1307, USA.  */

extern void gen_engine_h
  (lf *file, gen_table *gen, insn_table *isa, cache_entry *cache_rules);

extern void gen_engine_c
  (lf *file, gen_table *gen, insn_table *isa, cache_entry *cache_rules);

extern void print_engine_run_function_header
  (lf *file, char *processor, function_decl_type decl_type);
