/*
 * Copyright (c) 2003-2005 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the BSD License.
 * 
 */

/** decalration of SCSI commands transformation procedures */

#ifndef _TRANSFORM_PROCS_H_ 
  #define _TRANSFORM_PROCS_H_

#ifndef _PROTO_MODULE_H_ 
#include "proto_module.h"
#endif /* _PROTO_MODULE_H_ */ 
/* transforms SCSI commands */
extern transform_module_info scsi_transform_m;
/* transforms RBC commands */
extern transform_module_info rbc_transform_m;
/* transforms UFI commands */
extern transform_module_info ufi_transform_m;
/* transforms ATAPI commands */
extern transform_module_info atapi_transform_m;
/* transforms QIC-157 commands */
extern transform_module_info qic157_transform_m;
#endif /*_TRANSFORM_PROCS_H_*/ 
