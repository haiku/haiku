/*
 * \file: freecom.c
 * \brief: USB SCSI module extention for Freecom USB/IDE adaptor
 *
 * This file is a part of BeOS USB SCSI interface module project.
 * Copyright (c) 2003-2004 by Siarzhuk Zharski <imker@gmx.li>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details. 
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * This protocol extension module was developed using information from
 * "Driver for Freecom USB/IDE adaptor" in Linux usb storage driver and
 * "Mac OS X driver for the Freecom Traveller CD writer". 
 * 
 * $Source: /cvsroot/sis4be/usb_scsi/freecom/freecom.c,v $
 * $Author: zharik $
 * $Revision: 1.11 $
 * $Date: 2005/04/01 22:39:23 $
 *
 */
#include "usb_scsi.h"

#include <malloc.h>
#include "device_info.h"
#include "proto_module.h"

#define FREECOM_MODULE_NAME "freecom"
#define FREECOM_PROTOCOL_MODULE_NAME \
           MODULE_PREFIX FREECOM_MODULE_NAME PROTOCOL_SUFFIX

/**
  \struct:fcm_command
*/
typedef struct {
  uint8 type;        /* block type.        */
#define FCMT_ATAPI   0x21 /* send command. low bit indicates waiting for interrupt */
#define FCMT_STATUS  0x20 /* request for status */
#define FCMT_IN      0x81 /* request read data transfer. */
#define FCMT_OUT     0x01 /* request write data transfer */
#define FCMT_REG_IN  0xC0 /* read value from IDE regsiter */
#define FCMT_REG_OUT 0x40 /* write value to IDE register */
                          /* to use FCMT_REG_ combine it with register */
#define  FCMR_DATA   0x10 /* data register */
                          /* ... */ 
#define  FCMR_CMD    0x16 /* status-command register */ 
#define  FCMR_INT    0x17 /* interrupt-error register */ 
  uint8 timeout;    /* timeout (seconds?) */
#define  FCMTO_DEF  0xfe /* default timeout */ 
#define  FCMTO_TU   0x14 /* timeout for TEST UNIT READY command */ 
  union{
    uint8  cmd[12]; /* An ATAPI command.  */
    uint32 count;   /* number of bytes to transfer. */
    uint16 reg_val; /* Value to write to IDE register. */
    uint8  pad[62]; /* Padding Data. All FREECOM commands are 64 bytes long */
  }_PACKED data;
}_PACKED fcm_command;

/**
  \struct:fcm_status
*/
typedef struct {
    uint8    status;
#define FCMS_BSY   0x80 /* device is busy */
#define FCMS_DRDY  0x40 /*  */
#define FCMS_DMA   0x20 /*  */
#define FCMS_SEEK  0x10 /*  */
#define FCMS_DRQ   0x08 /*  */
#define FCMS_CORR  0x04 /*  */
#define FCMS_INTR  0x02 /*FREECOM specific: use obsolete bit*/
#define FCMS_CHECK 0x01 /* device is in error condition */
    uint8    reason;
#define FCMR_REL   0x04 /*  */
#define FCMR_IO    0x02 /*  */
#define FCMR_COD   0x01 /*  */
    uint16   count;
}_PACKED fcm_status;

// Timeout value (in us) for the freecom packet USB BulkRead and BulkWrite functions
#define FREECOM_USB_TIMEOUT		30000000

/**
  \fn:
  \param udi:
  \param st:
  \return:
  
*/
void trace_status(usb_device_info *udi, const fcm_status *st)
{
  char ch_status[] = "BDFSRCIE";
  char ch_reason[] = ".....RIC";
  int i = 0;
  for(; i < 8; i++){
    if(!(st->status & (1 << i)))
      ch_status[7 - i] =  '.';
    if(!(st->reason & (1 << i)))  
      ch_reason[7 - i] =  '.';
  }
  PTRACE(udi, "FCM:Status:{%s; Reason:%s; Count:%d}\n",
                                       ch_status, ch_reason, st->count);
}

/**
  \fn:freecom_initialize
  \param udi: device on wich we should perform initialization
  \return:error code if initialization failed or B_OK if it passed
  
  initialize procedure.
*/
status_t
freecom_initialize(usb_device_info *udi)
{
  status_t status = B_OK;
#define INIT_BUFFER_SIZE 0x20 
  char buffer[INIT_BUFFER_SIZE + 1];
  size_t len = 0;
  
  if(B_OK != (status = (*udi->usb_m->send_request)(udi->device,
                             USB_REQTYPE_DEVICE_IN | USB_REQTYPE_VENDOR,
                             0x4c, 0x4346, 0x0, INIT_BUFFER_SIZE,
                             buffer, &len)))
  {
    PTRACE_ALWAYS(udi, "FCM:init[%d]: init failed: %08x\n", udi->dev_num, status);
  } else {
    buffer[len] = 0;
    PTRACE(udi, "FCM:init[%d]: init '%s' OK\n", udi->dev_num, buffer);
  }

  if(B_OK != (status = (*udi->usb_m->send_request)(udi->device,
                             USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_VENDOR,
                             0x4d, 0x24d8, 0x0, 0, 0, &len)))
  {
    PTRACE_ALWAYS(udi, "FCM:init[%d]: reset on failed:%08x\n", udi->dev_num, status);
  } 

  snooze(250000);
  
  if(B_OK != (status = (*udi->usb_m->send_request)(udi->device,
                             USB_REQTYPE_DEVICE_OUT | USB_REQTYPE_VENDOR,
                             0x4d, 0x24f8, 0x0, 0, 0, &len)))
  {
    PTRACE_ALWAYS(udi, "FCM:init[%d]: reset off failed:%08x\n", udi->dev_num, status);
  } 
  
  snooze(3 * 1000000);  
  
  return status;
}
/**
  \fn:freecom_reset
  \param udi: device on wich we should perform reset
  \return:error code if reset failed or B_OK if it passed
  
  reset procedure.
*/
status_t
freecom_reset(usb_device_info *udi)
{
  status_t status = B_OK;
  /* not required ? */
  return status; 
}

/**
  \fn:usb_callback
  \param cookie:???
  \param status:???
  \param data:???
  \param actual_len:???
  \return:???
  
  ???
*/
static void usb_callback(void  *cookie,
                   uint32 status,
                   void  *data,
                   uint32 actual_len)
{
  if(cookie){
    usb_device_info *udi = (usb_device_info *)cookie;
//  PTRACE(udi, "FCM:usb_callback:status:0x%08x,len:%d\n", status, actual_len);
    udi->status = status;
    udi->data = data;
    udi->actual_len = actual_len;
    if(udi->status != B_CANCELED)
      release_sem(udi->trans_sem);
  }    
}

/**
  \fn:queue_bulk
  \param udi: device for which que_bulk request is performed
  \param buffer: data buffer, used in bulk i/o operation
  \param len: length of data buffer
  \param b_in: is "true" if input (device->host) data transfer, "false" otherwise
  \return: status of operation.
  
  performs queue_bulk USB request for corresponding pipe and handle timeout of this
  operation.
*/
static status_t
queue_bulk(usb_device_info *udi,
                     void  *buffer,
                     size_t len,
                     bool   b_in)
{
  status_t status = B_OK;
  usb_pipe pipe = b_in ? udi->pipe_in : udi->pipe_out;
  status = (*udi->usb_m->queue_bulk)(pipe, buffer, len, usb_callback, udi);
  if(status != B_OK){
    PTRACE_ALWAYS(udi, "FCM:queue_bulk:failed:%08x\n", status);
  } else {
    status = acquire_sem_etc(udi->trans_sem, 1, B_RELATIVE_TIMEOUT, FREECOM_USB_TIMEOUT/*udi->trans_timeout*/);
    if(status != B_OK){
      PTRACE_ALWAYS(udi, "FCM:queue_bulk:acquire_sem_etc failed:%08x\n", status);
      (*udi->usb_m->cancel_queued_transfers)(pipe);
    }
  }
  return status;
}

/**
  \fn:write_command
*/
static status_t
write_command(usb_device_info *udi, uint8 type, uint8 *cmd, uint8 timeout)
{
  status_t status = B_OK;
  /* initialize and fill in command block*/
  fcm_command fc = {
    .type    = type,
    .timeout = FCMTO_DEF,
  };
  if(0 != cmd){
    memcpy(fc.data.cmd, cmd, sizeof(fc.data.cmd));
    if(cmd[0] == 0x00){
      fc.timeout = FCMTO_TU; 
    }
  }
  PTRACE(udi, "FCM:FC:{Type:0x%02x; TO:%d;}\n", fc.type, fc.timeout);
  udi->trace_bytes("FCM:FC::Cmd:\n",  fc.data.cmd, sizeof(fc.data.cmd));     	
  udi->trace_bytes("FCM:FC::Pad:\n",  fc.data.pad, sizeof(fc.data.pad));     	
PTRACE(udi,"sizeof:%d\n",sizeof(fc));
  /* send command block to device */ 
  status = queue_bulk(udi, &fc, sizeof(fc), false);
  if(status != B_OK || udi->status != B_OK){
    PTRACE_ALWAYS(udi, "FCM:write_command failed:status:%08x usb status:%08x\n",
                                                                      status, udi->status);
    //(*udi->protocol_m->reset)(udi);
    status = B_CMD_WIRE_FAILED;
  }
  return status; 
}

/**
  \fn:read_status
*/
static status_t
read_status(usb_device_info *udi, fcm_status *fst)
{
  status_t status = B_OK;
  do{
    /* cleanup structure */
    memset(fst, 0, sizeof(fcm_status));
    /* read status */
    status = queue_bulk(udi, fst, sizeof(fcm_status), true);
    if(status != B_OK || udi->status != B_OK){
      PTRACE_ALWAYS(udi, "FCM:read_status failed:"
                            "status:%08x usb status:%08x\n", status, udi->status);
      //(*udi->protocol_m->reset)(udi);
      status = B_CMD_WIRE_FAILED;
      break;
    }
    if(udi->actual_len != sizeof(fcm_status)){
      PTRACE_ALWAYS(udi, "FCM:read_status failed:requested:%d, readed %d bytes\n",
                                         sizeof(fcm_status), udi->actual_len);
      status = B_CMD_WIRE_FAILED;
      break;
    }
    /* trace readed status info. if required. */
    trace_status(udi, fst);
    if(fst->status & FCMS_BSY){
      PTRACE(udi, "FCM:read_status:Timeout. Poll device with another status request.\n");
      if(B_OK != (status = write_command(udi, FCMT_STATUS, 0, 0))){
        PTRACE_ALWAYS(udi, "FCM:read_status failed:write_command_block failed %08x\n", status);
        break;
      }  
    }
  }while(fst->status & FCMS_BSY); /* repeat until device is busy */
  return status;
}

/**
  \fn:request_transfer
*/
static status_t
request_transfer(usb_device_info *udi, uint8 type, uint32 length, uint8 timeout)
{
  status_t status = B_OK;
  /* initialize and fill in Command Block */
  fcm_command fc = {
    .type    = type,
    .timeout = timeout,
  };
  fc.data.count = length;
  
  PTRACE(udi, "FCM:FC:{Type:0x%02x; TO:%d; Count:%d}\n", fc.type, fc.timeout, fc.data.count);
  udi->trace_bytes("FCM:FC::Pad:\n",  fc.data.pad, sizeof(fc.data.pad));     	
PTRACE(udi,"sizeof:%d\n",sizeof(fc));
  /* send command block to device */ 
  status = queue_bulk(udi, &fc, sizeof(fc), false);
  if(status != B_OK || udi->status != B_OK){
    PTRACE_ALWAYS(udi, "FCM:request_transfer:fc send failed:"
                                 "status:%08x usb status:%08x\n", status, udi->status);
    status = B_CMD_WIRE_FAILED;
  }
  return status; 
}


/**
  \fn:transfer_sg
  \param udi: corresponding device 
  \param data_sg: io vectors array with data to transfer
  \param sglist_count: count of entries in io vector array
  \param offset: 
  \param block_len: 
  \param b_in: direction of data transfer)

      
*/
static status_t
transfer_sg(usb_device_info *udi,
                      iovec *data_sg,
                      int32  sglist_count,
                      int32  offset,
                      int32 *block_len,
                      bool   b_in)
{ 
  status_t status = B_OK;
  usb_pipe pipe = (b_in) ? udi->pipe_in : udi->pipe_out;
  int32 off = offset;
  int32 to_xfer = *block_len;
  /* iterate through SG list */
  int i = 0;
  for(; i < sglist_count && to_xfer > 0; i++) {
    if(off < data_sg[i].iov_len) {
      int len = min(to_xfer, (data_sg[i].iov_len - off));
      char *buf = ((char *)data_sg[i].iov_base) + off;
      /* transfer linear block of data to/from device */ 
      if(B_OK == (status = (*udi->usb_m->queue_bulk)(pipe, buf, len, usb_callback, udi))) {
        status = acquire_sem_etc(udi->trans_sem, 1, B_RELATIVE_TIMEOUT, FREECOM_USB_TIMEOUT);
        if(status == B_OK){
          status = udi->status;
          if(B_OK != status){
            PTRACE_ALWAYS(udi, "FCM:transfer_sg:usb transfer status is not OK:%08x\n", status);
          }
        } else {
          PTRACE_ALWAYS(udi, "FCM:transfer_sg:acquire_sem_etc failed:%08x\n", status); 
          goto finalize;
        } 
      } else {
        PTRACE_ALWAYS(udi, "FCM:transfer_sg:queue_bulk failed:%08x\n", status);
        goto finalize;
      }
      /*check amount of transferred data*/
      if(len != udi->actual_len){
        PTRACE_ALWAYS(udi, "FCM:transfer_sg:WARNING!!!Length reported:%d required:%d\n",
                                                                      udi->actual_len, len);
      }
      /* handle offset logic */
      to_xfer -= len;      
      off = 0;
    } else {
      off -= data_sg[i].iov_len;
    }
  }
finalize:
  *block_len -= to_xfer;    
  return (B_OK != status) ? B_CMD_WIRE_FAILED : status;  
}

static status_t
transfer_data(usb_device_info *udi,
                        iovec *data_sg,
                        int32  sglist_count,
                        int32  transfer_length,
                        int32 *residue,
                        fcm_status *fst, 
                        bool   b_in)
{
  status_t status  = B_OK;
  int32 readed_len = 0;
  uint8 xfer_type  = b_in ? FCMT_IN : FCMT_OUT;
  int32 block_len  = (FCMR_COD == (fst->reason & FCMR_COD)) ?
                                                         transfer_length : fst->count;
  /* Strange situation with SCSIProbe - device say about 6 bytes available for 5-bytes
     request. Read 5 and all is ok. Hmm... */
  if(block_len > transfer_length){
    PTRACE_ALWAYS(udi, "FCM:transfer_data:TRUNCATED! "
                    "requested:%d available:%d.\n", transfer_length, block_len);
    block_len  = transfer_length; 
  }
  /* iterate until data will be transferred */
  while(readed_len < transfer_length && block_len > 0){
    /* send transfer block */
    if(B_OK != (status = request_transfer(udi, xfer_type, block_len, 0))){
      goto finalize;
    }
    /* check if data available/awaited */
    if(FCMS_DRQ != (fst->status & FCMS_DRQ)){
      PTRACE_ALWAYS(udi, "FCM:transfer_data:device doesn't want to transfer anything!!!\n");
      status = B_CMD_FAILED;
      goto finalize;
    }
    /* check the device state */
    if(!((fst->reason & FCMR_REL) == 0 &&
         (fst->reason & FCMR_IO)      == ((b_in) ? FCMR_IO : 0) &&
         (fst->reason & FCMR_COD)     == 0))
    {
      PTRACE_ALWAYS(udi, "FCM:transfer_data:device doesn't ready to transfer?\n");
      status = B_CMD_FAILED;
      goto finalize;
    }
    /* transfer scatter/gather safely only for length bytes at specified offset */       
    if(B_OK != (status = transfer_sg(udi, data_sg,
                                     sglist_count, readed_len, &block_len, b_in)))
    {
      goto finalize;
    }
    /* read status */
    if(B_OK != (status = read_status(udi, fst))){
      goto finalize;
    }
    /* check device error state */
    if(fst->status & FCMS_CHECK){
       PTRACE(udi, "FCM:transfer_data:data transfer failed?\n", fst->status);
       status = B_CMD_FAILED;
       goto finalize;
    }
    /* we have read block successfully */
    readed_len += block_len;
    /* check device state and amount of data */
    if((fst->reason & FCMR_COD) == FCMR_COD){
#if 0 /* too frequently reported - disabled to prevent log trashing */   
      if(readed_len < transfer_length){ 
        PTRACE_ALWAYS(udi, "FCM:transfer_data:Device had LESS data that we wanted.\n");
      }
#endif      
      break; /* exit iterations - device has finished the talking */  
    } else {
      if(readed_len >= transfer_length){
        PTRACE_ALWAYS(udi, "FCM:transfer_data:Device had MORE data that we wanted.\n");
        break;
      }
      block_len = fst->count; /* still have something to read */
    } 
  }
  /* calculate residue */
  *residue -= readed_len;
  if(*residue < 0)
    *residue = 0; /* TODO: consistency between SG length and transfer length -
                           in MODE_SENSE_10 converted commands f.example ... */
  PTRACE(udi,"FCM:transfer_data:was requested:%d, residue:%d\n", transfer_length, *residue);  
finalize:
  return status; /* TODO: MUST return B_CMD_*** error codes !!!!! */
}

/**
  \fn:freecom_transfer
  \param udi: corresponding device 
  \param cmd: SCSI command to be performed on USB device
  \param cmdlen: length of SCSI command
  \param data_sg: io vectors array with data to transfer
  \param sglist_count: count of entries in io vector array
  \param transfer_len: overall length of data to be transferred
  \param dir: direction of data transfer
  \param ccbio: CCB_SCSIIO struct for original SCSI command
  \param cb: callback to handle of final stage of command performing (autosense \
             request etc.)
    
  transfer procedure for bulk-only protocol. Performs  SCSI command on USB device
  [2]
*/
void
freecom_transfer(usb_device_info *udi,
                             uint8 *cmd,
                             uint8  cmdlen,
                             iovec*sg_data,
                             int32 sg_count,
                             int32  transfer_len,
                        EDirection  dir,
                        CCB_SCSIIO *ccbio,
               ud_transfer_callback cb)
{
  status_t command_status = B_CMD_WIRE_FAILED;
  int32 residue           = transfer_len;
  fcm_status   fst = {0};
  
  /* Current BeOS scsi_cd driver bombs us with lot of
     MODE_SENSE/MODE_SELECT commands. Unfortunately some of them
     hangs the Freecom firmware. Try to ignore translated ones */
#if 1
  if((MODE_SENSE_10  == cmd[0] && 0x0e == cmd[2]) ||
     (MODE_SELECT_10 == cmd[0] && 0x10 == cmd[1]))
  {
    uint8 *cmd_org = 0;
     /* set the command data pointer */    
    if(ccbio->cam_ch.cam_flags & CAM_CDB_POINTER){
      cmd_org = ccbio->cam_cdb_io.cam_cdb_ptr;
    }else{
      cmd_org = ccbio->cam_cdb_io.cam_cdb_bytes;
    }
    if(cmd_org[0] != cmd[0]){
      if(MODE_SENSE_10 == cmd[0]){ /* emulate answer - return empty pages */
        scsi_mode_param_header_10 *hdr = (scsi_mode_param_header_10*)sg_data[0].iov_base;
        memset(hdr, 0, sizeof(scsi_mode_param_header_10));
        hdr->mode_data_len[1] = 6;
        PTRACE(udi, "FCM:freecom_transfer:MODE_SENSE_10 ignored %02x->02x\n", cmd_org[0], cmd[0]);
      } else {
        PTRACE(udi, "FCM:freecom_transfer:MODE_SELECT_10 ignored %02x->%02x\n", cmd_org[0], cmd[0]);
      }
      command_status = B_OK; /* just say that command processed OK */
      goto finalize;
    }  
  }
#endif  
  /* write command to device */
  if(B_OK != (command_status = write_command(udi, FCMT_ATAPI, cmd, 0))){
    PTRACE_ALWAYS(udi, "FCM:freecom_transfer:"
                             "send command failed. Status:%08x\n", command_status);
    goto finalize;
  }
  /* obtain status */
  if(B_OK != (command_status = read_status(udi, &fst))){
    PTRACE_ALWAYS(udi, "FCM:freecom_transfer:"
                            "read status failed. Status:%08x\n", command_status);
    goto finalize;
  }
  /* check device error state */
  if(fst.status & FCMS_CHECK){
    PTRACE_ALWAYS(udi, "FCM:freecom_transfer:FST.Status is not OK\n");
    command_status = B_CMD_FAILED;
    goto finalize;
  }
  /* transfer data */  
  if(transfer_len != 0x0 && dir != eDirNone){
    command_status = transfer_data(udi, sg_data, sg_count, transfer_len,
                                                   &residue, &fst, (eDirIn == dir));
  }
finalize:  
  /* finalize transfer */
  cb(udi, ccbio, residue, command_status);
}

static status_t
std_ops(int32 op, ...)
{
  switch(op) {
  case B_MODULE_INIT:
    return B_OK;
  case B_MODULE_UNINIT:
    return B_OK;
  default:
    return B_ERROR;
  }
}

static protocol_module_info freecom_protocol_module = {
  { FREECOM_PROTOCOL_MODULE_NAME, 0, std_ops },
  freecom_initialize,
  freecom_reset,
  freecom_transfer,
};

_EXPORT protocol_module_info *modules[] = {
  &freecom_protocol_module,
  NULL
};
