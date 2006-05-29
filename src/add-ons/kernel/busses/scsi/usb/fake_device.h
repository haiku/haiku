/*
 * Copyright (c) 2003-2005 by Siarzhuk Zharski <imker@gmx.li>
 * Distributed under the terms of the BSD License.
 * 
 */
#ifndef _FAKE_DEVICE_H_ 
  #define _FAKE_DEVICE_H_

void fake_inquiry_request(usb_device_info *udi, CCB_SCSIIO *ccbio);
void fake_test_unit_ready_request(CCB_SCSIIO *ccbio);
status_t fake_scsi_io(CCB_SCSIIO *ccbio);
  
#endif /* _FAKE_DEVICE_H_ */
