// OBSOLETE /* Simulator header for fr30.
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
// OBSOLETE #ifndef FR30_ARCH_H
// OBSOLETE #define FR30_ARCH_H
// OBSOLETE 
// OBSOLETE #define TARGET_BIG_ENDIAN 1
// OBSOLETE 
// OBSOLETE /* Enum declaration for model types.  */
// OBSOLETE typedef enum model_type {
// OBSOLETE   MODEL_FR30_1, MODEL_MAX
// OBSOLETE } MODEL_TYPE;
// OBSOLETE 
// OBSOLETE #define MAX_MODELS ((int) MODEL_MAX)
// OBSOLETE 
// OBSOLETE /* Enum declaration for unit types.  */
// OBSOLETE typedef enum unit_type {
// OBSOLETE   UNIT_NONE, UNIT_FR30_1_U_STM, UNIT_FR30_1_U_LDM, UNIT_FR30_1_U_STORE
// OBSOLETE  , UNIT_FR30_1_U_LOAD, UNIT_FR30_1_U_CTI, UNIT_FR30_1_U_EXEC, UNIT_MAX
// OBSOLETE } UNIT_TYPE;
// OBSOLETE 
// OBSOLETE #define MAX_UNITS (3)
// OBSOLETE 
// OBSOLETE #endif /* FR30_ARCH_H */
