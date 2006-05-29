/*
 * Copyright (c) 2003-2005 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the BSD License.
 * 
 */
#ifndef _PROTO_COMMON_H_ 
  #define _PROTO_COMMON_H_

#ifndef _DEVICE_INFO_H_
  #include "device_info.h"
#endif /* _DEVICE_INFO_H_ */

void bulk_callback(void  *cookie,
                   uint32 status,
                   void  *data,
                   uint32 actual_len);
                   
status_t process_data_io(usb_device_info *udi, iovec *sg_data, int32 sg_count/*sg_buffer *sgb*/, EDirection dir);
 
void transfer_callback(usb_device_info *udi,
                                    CCB_SCSIIO *ccbio,
                                         int32 residue,
                                     status_t status);
void sense_callback(usb_device_info *udi,
                                    CCB_SCSIIO *ccbio,
                                         int32 residue,
                                     status_t status);
#endif /*_PROTO_COMMON_H_*/ 
