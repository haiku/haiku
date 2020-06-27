/*
 * \file: datafab.c
 * \brief: USB SCSI module extention for Datafab USB readers
 *
 * This file is a part of BeOS USB SCSI interface module project.
 * Copyright (c) 2005 by Siarzhuk Zharski <imker@gmx.li>
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
 * "Driver for Datafab USB Compact Flash reader" in Linux usb storage driver. 
 * 
 * $Source: /cvsroot/sis4be/usb_scsi/datafab/datafab.c,v $
 * $Author: zharik $
 * $Revision: 1.3 $
 * $Date: 2005/03/12 21:18:48 $
 *
 */
#include "usb_scsi.h"

#include <string.h>
#include "device_info.h"
#include "proto_module.h"

#define DATAFAB_MODULE_NAME "datafab"
#define DATAFAB_PROTOCOL_MODULE_NAME \
           MODULE_PREFIX DATAFAB_MODULE_NAME PROTOCOL_SUFFIX

typedef struct {
  usb_device_info *udi;
  uint8 *cmd;
  uint8  cmdlen;
  iovec*sg_data;
  int32 sg_count;
  int32  transfer_len;
  EDirection  dir;
  CCB_SCSIIO *ccbio;
  int32 residue;
} usb_scsi_transport_info;

typedef struct{
/*
0 M General configuration bit-significant information:
  F 15 0 = ATA device
  X 14-8 Retired
  F 7 1 = removable media device
  X 6 Obsolete
  X 5-3 Retired
  V 2 Response incomplete
  X 1 Retired
  F 0 Reserved
*/
  uint16 info;
#define NON_ATA_DEVICE   0x
#define REMOVABLE_DEVICE 0x
/*
1 X Obsolete
2 O V Specific configuration
3 X Obsolete
4-5 X Retired
6 X Obsolete
7-8 O V Reserved for assignment by the CompactFlash™ Association
9 X Retired
*/
  uint16 pad1[9];
  /*
10-19 M F Serial number (20 ASCII characters)
*/
  uint16 serial[10];
/*
20-21 X Retired
22 X Obsolete
*/
  uint16 pad2[3];
  /*
23-26 M F Firmware revision (8 ASCII characters)
*/
  uint16 firmware_rev[4];
  /*
27-46 M F Model number (40 ASCII characters)
*/
  uint16 model_num[20];
/*
47 M F 15-8 80h
  F 7-0 00h = Reserved
  F 01h-FFh = Maximum number of sectors that shall be transferred per interrupt on
  READ/WRITE MULTIPLE commands
*//*
48 F Reserved
*//*
49 M Capabilities
  F 15-14 Reserved for the IDENTIFY PACKET DEVICE command.
  F 13 1 = Standby timer values as specified in this standard are supported
  0 = Standby timer values shall be managed by the device
  F 12 Reserved for the IDENTIFY PACKET DEVICE command.
  F 11 1 = IORDY supported
  0 = IORDY may be supported
  F 10 1 = IORDY may be disabled
  F 9 1 = LBA supported
  F 8 1 = DMA supported.
  X 7-0 Retired
*//*
50 M Capabilities
  F 15 Shall be cleared to zero.
  F 14 Shall be set to one.
  F 13-2 Reserved.
  X 1 Obsolete
  F 0 Shall be set to one to indicate a device specific Standby timer value minimum.
*//*
51-52 X Obsolete
*//*
53 M F 15-3 Reserved
  F 2 1 = the fields reported in word 88 are valid
  0 = the fields reported in word 88 are not valid
  F 1 1 = the fields reported in words (70:64) are valid
  0 = the fields reported in words (70:64) are not valid
  X 0 Obsolete
*//*
54-58 X Obsolete
*//*
59 M F 15-9 Reserved
  V 8 1 = Multiple sector setting is valid
  V 7-0 xxh = Current setting for number of sectors that shall be transferred per interrupt on
  R/W Multiple command
*/
  uint16 pad3[13];
/*
60-61 M F Total number of user addressable sectors
*/
  uint32 total_secs;
/*
62 X Obsolete
*//*
63 M F 15-11 Reserved
  V 10 1 = Multiword DMA mode 2 is selected
  0 = Multiword DMA mode 2 is not selected
  V 9 1 = Multiword DMA mode 1 is selected
  0 = Multiword DMA mode 1 is not selected
  V 8 1 = Multiword DMA mode 0 is selected
  0 = Multiword DMA mode 0 is not selected
  F 7-3 Reserved
  F 2 1 = Multiword DMA mode 2 and below are supported
  F 1 1 = Multiword DMA mode 1 and below are supported
  F 0 1 = Multiword DMA mode 0 is supported
64 M F 15-8 Reserved
  F 7-0 PIO modes supported
65 M Minimum Multiword DMA transfer cycle time per word
  F 15-0 Cycle time in nanoseconds
66 M Manufacturer’s recommended Multiword DMA transfer cycle time
  F 15-0 Cycle time in nanoseconds
67 M Minimum PIO transfer cycle time without flow control
  F 15-0 Cycle time in nanoseconds
68 M Minimum PIO transfer cycle time with IORDY flow control
  F 15-0 Cycle time in nanoseconds
69-70 F Reserved (for future command overlap and queuing)
71-74 F Reserved for IDENTIFY PACKET DEVICE command.
75 O Queue depth
  F 15-5 Reserved
  F 4-0 Maximum queue depth – 1
76-79 F Reserved
80 M Major version number
  0000h or FFFFh = device does not report version
  F 15 Reserved
  F 14 Reserved for ATA/ATAPI-14
  F 13 Reserved for ATA/ATAPI-13
  F 12 Reserved for ATA/ATAPI-12
  F 11 Reserved for ATA/ATAPI-11
  F 10 Reserved for ATA/ATAPI-10
  F 9 Reserved for ATA/ATAPI-9
  F 8 Reserved for ATA/ATAPI-8
  F 7 Reserved for ATA/ATAPI-7
  F 6 1 = supports ATA/ATAPI-6
  F 5 1 = supports ATA/ATAPI-5
  F 4 1 = supports ATA/ATAPI-4
  F 3 1 = supports ATA-3
  X 2 Obsolete
  X 1 Obsolete
  F 0 Reserved
81 M F Minor version number
  0000h or FFFFh = device does not report version
  0001h-FFFEh = see 8.15.41
82 M Command set supported.
  X 15 Obsolete
  F 14 1 = NOP command supported
  F 13 1 = READ BUFFER command supported
  F 12 1 = WRITE BUFFER command supported
  X 11 Obsolete
  F 10 1 = Host Protected Area feature set supported
  F 9 1 = DEVICE RESET command supported
  F 8 1 = SERVICE interrupt supported
  F 7 1 = release interrupt supported
  F 6 1 = look-ahead supported
  F 5 1 = write cache supported
  F 4 Shall be cleared to zero to indicate that the PACKET Command feature set is not
  supported.
  F 3 1 = mandatory Power Management feature set supported
  F 2 1 = Removable Media feature set supported
  F 1 1 = Security Mode feature set supported
  F 0 1 = SMART feature set supported
83 M Command sets supported.
  F 15 Shall be cleared to zero
  F 14 Shall be set to one
  F 13 1 = FLUSH CACHE EXT command supported
  F 12 1 = mandatory FLUSH CACHE command supported
  F 11 1 = Device Configuration Overlay feature set supported
  F 10 1 = 48-bit Address feature set supported
  F 9 1 = Automatic Acoustic Management feature set supported
  F 8 1 = SET MAX security extension supported
  F 7 See Address Offset Reserved Area Boot, NCITS TR27:2001
  F 6 1 = SET FEATURES subcommand required to spinup after power-up
  F 5 1 = Power-Up In Standby feature set supported
  F 4 1 = Removable Media Status Notification feature set supported
  F 3 1 = Advanced Power Management feature set supported
  F 2 1 = CFA feature set supported
  F 1 1 = READ/WRITE DMA QUEUED supported
  F 0 1 = DOWNLOAD MICROCODE command supported
84 M Command set/feature supported extension.
  F 15 Shall be cleared to zero
  F 14 Shall be set to one
  F 13-6 Reserved
  F 5 1 = General Purpose Logging feature set supported
  F 4 Reserved
  F 3 1 = Media Card Pass Through Command feature set supported
  F 2 1 = Media serial number supported
  F 1 1 = SMART self-test supported
  F 0 1 = SMART error logging supported
85 M Command set/feature enabled.
  X 15 Obsolete
  F 14 1 = NOP command enabled
  F 13 1 = READ BUFFER command enabled
  F 12 1 = WRITE BUFFER command enabled
  X 11 Obsolete
  V 10 1 = Host Protected Area feature set enabled
  F 9 1 = DEVICE RESET command enabled
  V 8 1 = SERVICE interrupt enabled
  V 7 1 = release interrupt enabled
  V 6 1 = look-ahead enabled
  V 5 1 = write cache enabled
  F 4 Shall be cleared to zero to indicate that the PACKET Command feature set is not
  supported.
  F 3 1 = Power Management feature set enabled
  F 2 1 = Removable Media feature set enabled
  V 1 1 = Security Mode feature set enabled
  V 0 1 = SMART feature set enabled
86 M Command set/feature enabled.
  F 15-14 Reserved
  F 13 1 = FLUSH CACHE EXT command supported
  F 12 1 = FLUSH CACHE command supported
  F 11 1 = Device Configuration Overlay supported
  F 10 1 = 48-bit Address features set supported
  V 9 1 = Automatic Acoustic Management feature set enabled
  F 8 1 = SET MAX security extension enabled by SET MAX SET PASSWORD
  F 7 See Address Offset Reserved Area Boot, NCITS TR27:2001
  F 6 1 = SET FEATURES subcommand required to spin-up after power-up
  V 5 1 = Power-Up In Standby feature set enabled
  V 4 1 = Removable Media Status Notification feature set enabled
  V 3 1 = Advanced Power Management feature set enabled
  F 2 1 = CFA feature set enabled
  F 1 1 = READ/WRITE DMA QUEUED command supported
  F 0 1 = DOWNLOAD MICROCODE command supported
87 M Command set/feature default.
  F 15 Shall be cleared to zero
  F 14 Shall be set to one
  F 13-6 Reserved
  F 5 General Purpose Logging feature set supported
  V 4 Reserved
  V 3 1 = Media Card Pass Through Command feature set enabled
  V 2 1 = Media serial number is valid
  F 1 1 = SMART self-test supported
  F 0 1 = SMART error logging supported
88 O F 15-14 Reserved
  V 13 1 = Ultra DMA mode 5 is selected
  0 = Ultra DMA mode 5 is not selected
  V 12 1 = Ultra DMA mode 4 is selected
  0 = Ultra DMA mode 4 is not selected
  V 11 1 = Ultra DMA mode 3 is selected
  0 = Ultra DMA mode 3 is not selected
  V 10 1 = Ultra DMA mode 2 is selected
  0 = Ultra DMA mode 2 is not selected
  V 9 1 = Ultra DMA mode 1 is selected
  0 = Ultra DMA mode 1 is not selected
  V 8 1 = Ultra DMA mode 0 is selected
  0 = Ultra DMA mode 0 is not selected
  F 7-6 Reserved
  F 5 1 = Ultra DMA mode 5 and below are supported
  F 4 1 = Ultra DMA mode 4 and below are supported
  F 3 1 = Ultra DMA mode 3 and below are supported
  F 2 1 = Ultra DMA mode 2 and below are supported
  F 1 1 = Ultra DMA mode 1 and below are supported
  F 0 1 = Ultra DMA mode 0 is supported
89 O F Time required for security erase unit completion
90 O F Time required for Enhanced security erase completion
91 O V Current advanced power management value
92 O V Master Password Revision Code
93 * Hardware reset result. The contents of bits (12:0) of this word shall change only during the
  execution of a hardware reset.
  F 15 Shall be cleared to zero.
  F 14 Shall be set to one.
  V 13 1 = device detected CBLID- above ViH
  0 = device detected CBLID- below ViL
  12-8 Device 1 hardware reset result. Device 0 shall clear these bits to zero. Device 1
  shall set these bits as follows:
  F 12 Reserved.
  V 11 0 = Device 1 did not assert PDIAG-.
  1 = Device 1 asserted PDIAG-.
  V 10-9 These bits indicate how Device 1 determined the device number:
  00 = Reserved.
  01 = a jumper was used.
  10 = the CSEL signal was used.
  11 = some other method was used or the method is unknown.
  8 Shall be set to one.
  7-0 Device 0 hardware reset result. Device 1 shall clear these bits to zero. Device 0
  shall set these bits as follows:
  F 7 Reserved.
  F 6 0 = Device 0 does not respond when Device 1 is selected.
  1 = Device 0 responds when Device 1 is selected.
  V 5 0 = Device 0 did not detect the assertion of DASP-.
  1 = Device 0 detected the assertion of DASP-.
  V 4 0 = Device 0 did not detect the assertion of PDIAG-.
  1 = Device 0 detected the assertion of PDIAG-.
  V 3 0 = Device 0 failed diagnostics.
  1 = Device 0 passed diagnostics.
  V 2-1 These bits indicate how Device 0 determined the device number:
  00 = Reserved.
  01 = a jumper was used.
  10 = the CSEL signal was used.
  11 = some other method was used or the method is unknown.
  F 0 Shall be set to one.
94 O V 15-8 Vendor’s recommended acoustic management value.
  V 7-0 Current automatic acoustic management value.
95-99 F Reserved
100-103 O V Maximum user LBA for 48-bit Address feature set.
104-126 F Reserved
127 O Removable Media Status Notification feature set support
  F 15-2 Reserved
  F 1-0 00 = Removable Media Status Notification feature set not supported
  01 = Removable Media Status Notification feature supported
  10 = Reserved
  11 = Reserved
128 O Security status
  F 15-9 Reserved
  V 8 Security level 0 = High, 1 = Maximum
  F 7-6 Reserved
  F 5 1 = Enhanced security erase supported
  V 4 1 = Security count expired
  V 3 1 = Security frozen
  V 2 1 = Security locked
  V 1 1 = Security enabled
  F 0 1 = Security supported
129-159 X Vendor specific
160 O CFA power mode 1
  F 15 Word 160 supported
  F 14 Reserved
  F 13 CFA power mode 1 is required for one or more commands implemented by the
  device
  V 12 CFA power mode 1 disabled
  F 11-0 Maximum current in ma
161-175 X Reserved for assignment by the CompactFlash™ Association
176-205 O V Current media serial number
206-254 F Reserved
255 M X Integrity word
  15-8 Checksum
  7-0 Signature
*/
  uint16 pad4[24];
  uint16 pad5[25];
  uint16 pad6[25];
  uint16 pad7[20];
  uint16 pad8[25];
  uint16 pad9[25];
  uint16 padA[25];
  uint16 padB[25];
}ATA_DEVICE_INFO;

#define DEVICE_INFO_SIZE 512

typedef struct {
  uint8 feature;      /* R: error, W:feature */
  uint8 sector_count; /* R: IReason W: Sectors Count*/
  uint8 address[3];   /* part of address */
  uint8 addr_dev;     /* part of address and device info*/
#define ADDR_MASK  0x0F
#define DEV_MASK   0xF0
#define DEV_ON     0xA0
#define DEV_LBA    0x40
#define DEV_MASTER 0x00
#define DEV_SLAVE  0x10
  uint8 command;      /* R: status W:command */
  uint8 intr_reset;   /* interrupt/reset register */
#define IR_IRQ_ENABLE 0x01
#define IR_RESET      0x02
}command_registers;

typedef struct {
  uint8 reg1;
  uint8 reg2;
}status_registers;

#define IDE_CMD_IDENTIFY_DEVICE 0xEC
#define IDE_CMD_READ            0x20
#define IDE_CMD_WRITE           0x30

/* duplication! */
#define INQ_VENDOR_LEN    0x08
#define INQ_PRODUCT_LEN   0x10
#define INQ_REVISION_LEN  0x04


/*
    B_DEV_INVALID_IOCTL = B_DEVICE_ERROR_BASE,
	B_DEV_NO_MEMORY,
	B_DEV_BAD_DRIVE_NUM,
	B_DEV_NO_MEDIA,
	B_DEV_UNREADABLE,
	B_DEV_FORMAT_ERROR,
	B_DEV_TIMEOUT,
	B_DEV_RECALIBRATE_ERROR,
	B_DEV_SEEK_ERROR,
	B_DEV_ID_ERROR,
	B_DEV_READ_ERROR,               a
	B_DEV_WRITE_ERROR,
	B_DEV_NOT_READY,
	B_DEV_MEDIA_CHANGED,
	B_DEV_MEDIA_CHANGE_REQUESTED,
	B_DEV_RESOURCE_CONFLICT,
	B_DEV_CONFIGURATION_ERROR,        10
	B_DEV_DISABLED_BY_USER,
	B_DEV_DOOR_OPEN,
	
	B_DEV_INVALID_PIPE,    13
	B_DEV_CRC_ERROR, 
	B_DEV_STALLED,              15
	B_DEV_BAD_PID,
	B_DEV_UNEXPECTED_PID,
	B_DEV_DATA_OVERRUN,           18
	B_DEV_DATA_UNDERRUN,
	B_DEV_FIFO_OVERRUN,           1a
	B_DEV_FIFO_UNDERRUN,
	B_DEV_PENDING,
	B_DEV_MULTIPLE_ERRORS,           1d
	B_DEV_TOO_LATE
*/

/**
  \fn:datafab_initialize
  \param udi: device on wich we should perform initialization
  \return:error code if initialization failed or B_OK if it passed
  
  initialize procedure for bulk only protocol devices.
*/
status_t
datafab_initialize(usb_device_info *udi)
{
  status_t status = B_OK;
/*TODO*/
  return status;
}
/**
  \fn:datafab_reset
  \param udi: device on wich we should perform reset
  \return:error code if reset failed or B_OK if it passed
  
  reset procedure for bulk only protocol devices. Tries to send
  BulkOnlyReset USB request and clear USB_FEATURE_ENDPOINT_HALT features on
  input and output pipes. ([2] 3.1)
*/
status_t
datafab_reset(usb_device_info *udi)
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
    PTRACE_ALWAYS(udi, "datafab_queue_bulk:failed:%08x\n", status);
  } else {
    status = acquire_sem_etc(udi->trans_sem, 1, B_RELATIVE_TIMEOUT,
                           /*DATAFAB_USB_TIMEOUT*/ udi->trans_timeout);
    if(status != B_OK){
      PTRACE_ALWAYS(udi, "datafab_queue_bulk:acquire_sem_etc failed:%08x\n", status);
      (*udi->usb_m->cancel_queued_transfers)(pipe);
    }
  }
  return status;
}

/**
  \fn:handle_INQUIRY
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles INQUIRY SCSI command
*/
static status_t 
handle_INQUIRY(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  uint8 *data = usti->ccbio->cam_data_ptr;
  if(usti->ccbio->cam_ch.cam_flags & CAM_SCATTER_VALID){
    PTRACE(usti->udi,"handle_INQUIRY: problems!!! scatter gatter ....=-(\n");
  } else {
    command_registers cr = {
      .feature = 0,
      .sector_count = 1,
      .address = {0},
      .addr_dev = 0xa0,
      .command = 0xec,
      .intr_reset = 0x01
    };
    
    ATA_DEVICE_INFO adi = {0};
    
    PTRACE(usti->udi, "ATA_DEVICE_INFO sizeof:%d pipe_in:%x, pipe_out:%x\n", sizeof(adi),
               usti->udi->pipe_in, usti->udi->pipe_out);

    PTRACE(usti->udi, "command_registers sizeof:%d\n", sizeof(cr));
    
    memset(data, 0, usti->ccbio->cam_dxfer_len);
//TODO!!!
    /* data[0] = 0x1F;*/ /* we can play here with type of device */
 /*   data[1] = 0x80;
    data[2] = 0x02;
    data[3] = 0x02;
    data[4] = (0 != usti->udi) ? 5 : 31; / * udi != 0 - mean FIX_NO_INQUIRY * /
    if(usti->ccbio->cam_dxfer_len >= 0x24){
      strncpy(&data[8],  "USB SCSI", INQ_VENDOR_LEN);
      strncpy(&data[16], "Reserved", INQ_PRODUCT_LEN);
      strncpy(&data[32], "N/A", INQ_REVISION_LEN); 
    } */
    status = queue_bulk(usti->udi, &cr, sizeof(cr), false);
    if(B_OK != status){
      PTRACE(usti->udi, "write command 1 status:%08x\n", status);
      //goto finalize;
    }
    
    status = queue_bulk(usti->udi, &adi, sizeof(adi), true);
    if(B_OK != status){
      PTRACE(usti->udi, "write command 2 status:%08x\n", status);
      goto finalize;
    }

    PTRACE(usti->udi, "ADI::info:%08x\n", adi.info);
    usti->udi->trace_bytes("ADI::pad1:",  (char*)adi.pad1, sizeof(adi.pad1));     	
    usti->udi->trace_bytes("ADI::serial:",  (char*)adi.serial, sizeof(adi.serial));     	
    usti->udi->trace_bytes("ADI::pad2:",  (char*)adi.pad2, sizeof(adi.pad2));     	
    usti->udi->trace_bytes("ADI::firmware_rev:",  (char*)adi.firmware_rev, sizeof(adi.firmware_rev));     	
    usti->udi->trace_bytes("ADI::model_num:",  (char*)adi.model_num, sizeof(adi.model_num));     	
    usti->udi->trace_bytes("ADI::pad3:",  (char*)adi.pad3, sizeof(adi.pad3));     	
    usti->udi->trace_bytes("ADI::total_secs:",  (char*)&adi.total_secs, sizeof(adi.total_secs));     	
    usti->udi->trace_bytes("ADI::pad4:",  (char*)adi.pad4, sizeof(adi.pad4));     	
    usti->udi->trace_bytes("ADI::pad5:",  (char*)adi.pad5, sizeof(adi.pad5));     	
    usti->udi->trace_bytes("ADI::pad6:",  (char*)adi.pad6, sizeof(adi.pad6));     	
    usti->udi->trace_bytes("ADI::pad7:",  (char*)adi.pad7, sizeof(adi.pad7));     	
    usti->udi->trace_bytes("ADI::pad8:",  (char*)adi.pad8, sizeof(adi.pad8));     	
    usti->udi->trace_bytes("ADI::pad9:",  (char*)adi.pad9, sizeof(adi.pad9));     	
    usti->udi->trace_bytes("ADI::padA:",  (char*)adi.padA, sizeof(adi.padA));     	
    usti->udi->trace_bytes("ADI::padB:",  (char*)adi.padB, sizeof(adi.padB));     	
    
finalize:    
    status = B_CMD_WIRE_FAILED;
  }
  return status;
}

/**
  \fn:handle_TEST_UNIT_READY
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles TEST_UNIT_READY SCSI command
*/
static status_t 
handle_TEST_UNIT_READY(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_READ_CAPACITY
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles READ_CAPACITY SCSI command
*/
static status_t 
handle_READ_CAPACITY(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_REQUEST_SENSE
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles REQUEST_SENSE SCSI command
*/
static status_t 
handle_REQUEST_SENSE(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_MODE_SENSE
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles MODE_SENSE SCSI command
*/
static status_t 
handle_MODE_SENSE(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_MODE_SELECT
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles MODE_SELECT SCSI command
*/
static status_t 
handle_MODE_SELECT(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_READ
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles READ SCSI command
*/
static status_t 
handle_READ(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_WRITE
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles WRITE SCSI command
*/
static status_t 
handle_WRITE(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:handle_UnknownCommand
  \param usti: pointer to usb_scsi_transport_info sutruct containing request 
               information
  \return: command execution status

  handles "Unknown" SCSI command that were not handled
*/
static status_t 
handle_UnknownCommand(usb_scsi_transport_info *usti)
{
  status_t status = B_CMD_FAILED;
  return status;
}

/**
  \fn:datafab_transfer
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
datafab_transfer(usb_device_info *udi,
                             uint8 *cmd,
                             uint8  cmdlen,
                             iovec*sg_data,
                             int32 sg_count,
                             int32  transfer_len,
                        EDirection  dir,
                        CCB_SCSIIO *ccbio,
               ud_transfer_callback cb)
{
  status_t command_status = B_CMD_WIRE_FAILED;//B_OK;
  int32 residue           = transfer_len;
  usb_scsi_transport_info usti = {
   .udi          = udi,
   .cmd          = cmd,
   .cmdlen       = cmdlen,
   .sg_data      = sg_data,
   .sg_count     = sg_count,
   .transfer_len = transfer_len,
   .dir          = dir,
   .ccbio        = ccbio,
   .residue      = transfer_len
  };

  switch(cmd[0]){
  case TEST_UNIT_READY:
    command_status = handle_TEST_UNIT_READY(&usti);
    break;
  case INQUIRY:
    command_status = handle_INQUIRY(&usti);
    break;
  case READ_CAPACITY:
    command_status = handle_READ_CAPACITY(&usti);
    break;
  case REQUEST_SENSE:
    command_status = handle_REQUEST_SENSE(&usti);
    break;
  case MODE_SENSE_6:
  case MODE_SENSE_10:
    command_status = handle_MODE_SENSE(&usti);
    break;
  case MODE_SELECT_6:
  case MODE_SELECT_10:
    command_status = handle_MODE_SELECT(&usti);
    break;
/*  case ALLOW_MEDIUM_REMOVAL:
    command_status = handle_ALLOW_MEDIUM_REMOVAL();
    break;*/
  case READ_6:
  case READ_10:
  case READ_12:
    command_status = handle_READ(&usti);
    break;
  case WRITE_6:
  case WRITE_10:
  case WRITE_12:
    command_status = handle_WRITE(&usti);
    break;
  default:
    command_status = handle_UnknownCommand(&usti);
    break;
  }
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

static protocol_module_info datafab_protocol_module = {
  { DATAFAB_PROTOCOL_MODULE_NAME, 0, std_ops },
  datafab_initialize,
  datafab_reset,
  datafab_transfer,
};

_EXPORT protocol_module_info *modules[] = {
  &datafab_protocol_module,
  NULL
};
