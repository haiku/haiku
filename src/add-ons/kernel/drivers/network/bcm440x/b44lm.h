
/******************************************************************************/
/*                                                                            */
/* Broadcom BCM4400 Linux Network Driver, Copyright (c) 2002 Broadcom         */
/* Corporation.                                                               */
/* All rights reserved.                                                       */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation, located in the file LICENSE.                 */
/*                                                                            */
/* History:                                                                   */
/*    02/25/00 Hav Khauv        Initial version.                              */
/******************************************************************************/

#ifndef B44LM_H
#define B44LM_H

#include "b44queue.h"



/******************************************************************************/
/* Basic types. */
/******************************************************************************/

typedef char           LM_CHAR,    *PLM_CHAR;
typedef unsigned int   LM_UINT,    *PLM_UINT;
typedef unsigned char  LM_UINT8,   *PLM_UINT8;
typedef unsigned short LM_UINT16,  *PLM_UINT16;
typedef unsigned int   LM_UINT32,  *PLM_UINT32;
typedef unsigned long  LM_COUNTER, *PLM_COUNTER;
typedef void           LM_VOID,    *PLM_VOID;
typedef char           LM_BOOL,    *PLM_BOOL;

typedef LM_UINT32 LM_PHYSICAL_ADDRESS, *PLM_PHYSICAL_ADDRESS;

#ifndef TRUE
#define TRUE           1
#endif /* TRUE */

#ifndef FALSE
#define FALSE          0
#endif /* FALSE */

#ifndef NULL
#define NULL                ((void *) 0)
#endif /* NULL */

#ifndef OFFSETOF
#define OFFSETOF(_s, _m)    (B44_MM_PTR(&(((_s *) 0)->_m)))
#endif /* OFFSETOF */



/******************************************************************************/
/* Simple macros. */
/******************************************************************************/

#define IS_ETH_BROADCAST(_pEthAddr)                                         \
    (((unsigned char *) (_pEthAddr))[0] == ((unsigned char) 0xff))

#define IS_ETH_MULTICAST(_pEthAddr)                                         \
    (((unsigned char *) (_pEthAddr))[0] & ((unsigned char) 0x01))

#define IS_ETH_ADDRESS_EQUAL(_pEtherAddr1, _pEtherAddr2)                    \
    ((((unsigned char *) (_pEtherAddr1))[0] ==                              \
    ((unsigned char *) (_pEtherAddr2))[0]) &&                               \
    (((unsigned char *) (_pEtherAddr1))[1] ==                               \
    ((unsigned char *) (_pEtherAddr2))[1]) &&                               \
    (((unsigned char *) (_pEtherAddr1))[2] ==                               \
    ((unsigned char *) (_pEtherAddr2))[2]) &&                               \
    (((unsigned char *) (_pEtherAddr1))[3] ==                               \
    ((unsigned char *) (_pEtherAddr2))[3]) &&                               \
    (((unsigned char *) (_pEtherAddr1))[4] ==                               \
    ((unsigned char *) (_pEtherAddr2))[4]) &&                               \
    (((unsigned char *) (_pEtherAddr1))[5] ==                               \
    ((unsigned char *) (_pEtherAddr2))[5]))

#define COPY_ETH_ADDRESS(_Src, _Dst)                                        \
    ((unsigned char *) (_Dst))[0] = ((unsigned char *) (_Src))[0];          \
    ((unsigned char *) (_Dst))[1] = ((unsigned char *) (_Src))[1];          \
    ((unsigned char *) (_Dst))[2] = ((unsigned char *) (_Src))[2];          \
    ((unsigned char *) (_Dst))[3] = ((unsigned char *) (_Src))[3];          \
    ((unsigned char *) (_Dst))[4] = ((unsigned char *) (_Src))[4];          \
    ((unsigned char *) (_Dst))[5] = ((unsigned char *) (_Src))[5];



/******************************************************************************/
/* Constants. */
/******************************************************************************/

#define ETHERNET_ADDRESS_SIZE           6
#define ETHERNET_PACKET_HEADER_SIZE     14
#define MIN_ETHERNET_PACKET_SIZE        64      /* with 4 byte crc. */
#define MAX_ETHERNET_PACKET_SIZE        1518    /* with 4 byte crc. */
#define MIN_ETHERNET_PACKET_SIZE_NO_CRC 60
#define MAX_ETHERNET_PACKET_SIZE_NO_CRC 1514
#define MAX_ETHERNET_PACKET_BUFFER_SIZE 1536    /* A nice even number. */

#ifndef LM_MAX_MC_TABLE_SIZE
#define LM_MAX_MC_TABLE_SIZE            63
#endif /* LM_MAX_MC_TABLE_SIZE */
#define LM_MC_ENTRY_SIZE                (ETHERNET_ADDRESS_SIZE+1)
#define LM_MC_INSTANCE_COUNT_INDEX      (LM_MC_ENTRY_SIZE-1)


/* Receive filter masks. */
#define LM_ACCEPT_UNICAST               0x0001
#define LM_ACCEPT_MULTICAST             0x0002
#define LM_ACCEPT_ALL_MULTICAST         0x0004
#define LM_ACCEPT_BROADCAST             0x0008
#define LM_ACCEPT_ERROR_PACKET          0x0010

#define LM_PROMISCUOUS_MODE             0x10000



/******************************************************************************/
/* PCI registers. */
/******************************************************************************/

#define PCI_VENDOR_ID_REG               0x00
#define PCI_DEVICE_ID_REG               0x02

#define PCI_COMMAND_REG                 0x04
#define PCI_IO_SPACE_ENABLE             0x0001
#define PCI_MEM_SPACE_ENABLE            0x0002
#define PCI_BUSMASTER_ENABLE            0x0004
#define PCI_MEMORY_WRITE_INVALIDATE     0x0010
#define PCI_PARITY_ERROR_ENABLE         0x0040
#define PCI_SYSTEM_ERROR_ENABLE         0x0100
#define PCI_FAST_BACK_TO_BACK_ENABLE    0x0200

#define PCI_STATUS_REG                  0x06
#define PCI_REV_ID_REG                  0x08

#define PCI_CACHE_LINE_SIZE_REG         0x0c

#define PCI_IO_BASE_ADDR_REG            0x10
#define PCI_IO_BASE_ADDR_MASK           0xfffffff0

#define PCI_MEM_BASE_ADDR_LOW           0x10
#define PCI_MEM_BASE_ADDR_HIGH          0x14

#define PCI_SUBSYSTEM_VENDOR_ID_REG     0x2c
#define PCI_SUBSYSTEM_ID_REG            0x2e
#define PCI_INT_LINE_REG                0x3c

#define PCIX_CAP_REG                    0x40
#define PCIX_ENABLE_RELAXED_ORDERING    BIT_17

/******************************************************************************/
/* Fragment structure. */
/******************************************************************************/

typedef struct {
    LM_UINT32 FragSize;
    LM_PHYSICAL_ADDRESS FragBuf;
} LM_FRAG, *PLM_FRAG;

typedef struct {
    /* FragCount is initialized for the caller to the maximum array size, on */
    /* return FragCount is the number of the actual fragments in the array. */
    LM_UINT32 FragCount;

    /* Total buffer size. */
    LM_UINT32 TotalSize;

    /* Fragment array buffer. */
    LM_FRAG Fragments[1];
} LM_FRAG_LIST, *PLM_FRAG_LIST;

#define DECLARE_FRAG_LIST_BUFFER_TYPE(_FRAG_LIST_TYPE_NAME, _MAX_FRAG_COUNT) \
    typedef struct {                                                         \
        LM_FRAG_LIST FragList;                                               \
        LM_FRAG FragListBuffer[_MAX_FRAG_COUNT-1];                           \
    } _FRAG_LIST_TYPE_NAME, *P##_FRAG_LIST_TYPE_NAME



/******************************************************************************/
/* Status codes. */
/******************************************************************************/

#define LM_STATUS_SUCCESS                                       0
#define LM_STATUS_FAILURE                                       1

#define LM_STATUS_INTERRUPT_ACTIVE                              2
#define LM_STATUS_INTERRUPT_NOT_ACTIVE                          3

#define LM_STATUS_LINK_ACTIVE                                   4
#define LM_STATUS_LINK_DOWN                                     5
#define LM_STATUS_LINK_SETTING_MISMATCH                         6

#define LM_STATUS_TOO_MANY_FRAGMENTS                            7
#define LM_STATUS_TRANSMIT_ABORTED                              8
#define LM_STATUS_TRANSMIT_ERROR                                9
#define LM_STATUS_RECEIVE_ABORTED                               10
#define LM_STATUS_RECEIVE_ERROR                                 11
#define LM_STATUS_INVALID_PACKET_SIZE                           12
#define LM_STATUS_OUT_OF_MAP_REGISTERS                          13
#define LM_STATUS_UNKNOWN_ADAPTER                               14

typedef LM_UINT LM_STATUS, *PLM_STATUS;


/******************************************************************************/
/* Line speed. */
/******************************************************************************/

#define LM_LINE_SPEED_UNKNOWN                                   0
#define LM_LINE_SPEED_AUTO                  LM_LINE_SPEED_UNKNOWN
#define LM_LINE_SPEED_10MBPS                                    10
#define LM_LINE_SPEED_100MBPS                                   100

typedef LM_UINT32 LM_LINE_SPEED, *PLM_LINE_SPEED;



/******************************************************************************/
/* Duplex mode. */
/******************************************************************************/

#define LM_DUPLEX_MODE_UNKNOWN                                  0
#define LM_DUPLEX_MODE_HALF                                     1
#define LM_DUPLEX_MODE_FULL                                     2

typedef LM_UINT32 LM_DUPLEX_MODE, *PLM_DUPLEX_MODE;



/******************************************************************************/
/* Power state. */
/******************************************************************************/

#define LM_POWER_STATE_D0       0
#define LM_POWER_STATE_D1       1
#define LM_POWER_STATE_D2       2
#define LM_POWER_STATE_D3       3

typedef LM_UINT32 LM_POWER_STATE, *PLM_POWER_STATE;



/******************************************************************************/
/* Task offloading. */
/******************************************************************************/

#define LM_TASK_OFFLOAD_NONE                    0x0000
#define LM_TASK_OFFLOAD_TX_IP_CHECKSUM          0x0001
#define LM_TASK_OFFLOAD_RX_IP_CHECKSUM          0x0002
#define LM_TASK_OFFLOAD_TX_TCP_CHECKSUM         0x0004
#define LM_TASK_OFFLOAD_RX_TCP_CHECKSUM         0x0008
#define LM_TASK_OFFLOAD_TX_UDP_CHECKSUM         0x0010
#define LM_TASK_OFFLOAD_RX_UDP_CHECKSUM         0x0020
#define LM_TASK_OFFLOAD_TCP_SEGMENTATION        0x0040

typedef LM_UINT32 LM_TASK_OFFLOAD, *PLM_TASK_OFFLOAD;



/******************************************************************************/
/* Flow control. */
/******************************************************************************/

#define LM_FLOW_CONTROL_NONE                    0x00
#define LM_FLOW_CONTROL_RECEIVE_PAUSE           0x01
#define LM_FLOW_CONTROL_TRANSMIT_PAUSE          0x02
#define LM_FLOW_CONTROL_RX_TX_PAUSE (LM_FLOW_CONTROL_RECEIVE_PAUSE | \
    LM_FLOW_CONTROL_TRANSMIT_PAUSE)

/* This value can be or-ed with RECEIVE_PAUSE and TRANSMIT_PAUSE.  If the */
/* auto-negotiation is disabled and the RECEIVE_PAUSE and TRANSMIT_PAUSE */
/* bits are set, then flow control is enabled regardless of link partner's */
/* flow control capability. */
#define LM_FLOW_CONTROL_AUTO_PAUSE              0x80000000

typedef LM_UINT32 LM_FLOW_CONTROL, *PLM_FLOW_CONTROL;



/******************************************************************************/
/* Wake up mode. */
/******************************************************************************/

#define LM_WAKE_UP_MODE_NONE                    0
#define LM_WAKE_UP_MODE_MAGIC_PACKET            1
#define LM_WAKE_UP_MODE_NWUF                    2
#define LM_WAKE_UP_MODE_LINK_CHANGE             4

typedef LM_UINT32 LM_WAKE_UP_MODE, *PLM_WAKE_UP_MODE;



/******************************************************************************/
/* Counters. */
/******************************************************************************/

#define LM_COUNTER_FRAMES_XMITTED_OK                            0
#define LM_COUNTER_FRAMES_RECEIVED_OK                           1
#define LM_COUNTER_ERRORED_TRANSMIT_COUNT                       2
#define LM_COUNTER_ERRORED_RECEIVE_COUNT                        3
#define LM_COUNTER_RCV_CRC_ERROR                                4
#define LM_COUNTER_ALIGNMENT_ERROR                              5
#define LM_COUNTER_SINGLE_COLLISION_FRAMES                      6
#define LM_COUNTER_MULTIPLE_COLLISION_FRAMES                    7
#define LM_COUNTER_FRAMES_DEFERRED                              8
#define LM_COUNTER_MAX_COLLISIONS                               9
#define LM_COUNTER_RCV_OVERRUN                                  10
#define LM_COUNTER_XMIT_UNDERRUN                                11
#define LM_COUNTER_UNICAST_FRAMES_XMIT                          12
#define LM_COUNTER_MULTICAST_FRAMES_XMIT                        13
#define LM_COUNTER_BROADCAST_FRAMES_XMIT                        14
#define LM_COUNTER_UNICAST_FRAMES_RCV                           15
#define LM_COUNTER_MULTICAST_FRAMES_RCV                         16
#define LM_COUNTER_BROADCAST_FRAMES_RCV                         17

typedef LM_UINT32 LM_COUNTER_TYPE, *PLM_COUNTER_TYPE;



/******************************************************************************/
/* Forward definition. */
/******************************************************************************/

typedef struct _LM_DEVICE_BLOCK *PLM_DEVICE_BLOCK;
typedef struct _LM_PACKET *PLM_PACKET;



/******************************************************************************/
/* Function prototypes. */
/******************************************************************************/

LM_STATUS b44_LM_GetAdapterInfo(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_InitializeAdapter(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_ResetAdapter(PLM_DEVICE_BLOCK pDevice, LM_BOOL full);
LM_STATUS b44_LM_DisableInterrupt(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_EnableInterrupt(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_SendPacket(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket);
LM_STATUS b44_LM_ServiceInterrupts(PLM_DEVICE_BLOCK pDevice);
#ifdef BCM_NAPI_RXPOLL
int b44_LM_ServiceRxPoll(PLM_DEVICE_BLOCK pDevice, int limit);
#endif
LM_STATUS b44_LM_QueueRxPackets(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_SetReceiveMask(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Mask);
LM_STATUS b44_LM_Halt(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_Abort(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_MulticastAdd(PLM_DEVICE_BLOCK pDevice, PLM_UINT8 pMcAddress);
LM_STATUS b44_LM_MulticastDel(PLM_DEVICE_BLOCK pDevice, PLM_UINT8 pMcAddress);
LM_STATUS b44_LM_MulticastClear(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_SetMacAddress(PLM_DEVICE_BLOCK pDevice, PLM_UINT8 pMacAddress);
LM_STATUS b44_LM_LoopbackAddress(PLM_DEVICE_BLOCK pDevice, PLM_UINT8 pAddress);

LM_WAKE_UP_MODE b44_LM_PMCapabilities(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_SetPowerState(PLM_DEVICE_BLOCK pDevice, LM_POWER_STATE PowerLevel);

LM_VOID b44_LM_ReadPhy(PLM_DEVICE_BLOCK pDevice, LM_UINT32 PhyReg,
    PLM_UINT32 pData32);
LM_VOID b44_LM_WritePhy(PLM_DEVICE_BLOCK pDevice, LM_UINT32 PhyReg,
    LM_UINT32 Data32);

LM_STATUS b44_LM_SetupPhy(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_ResetPhy(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_GetStats(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_NvramRead(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT32 *pData);
LM_STATUS b44_LM_NvramWriteBlock(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT32 *pData, LM_UINT32 Size);
void b44_LM_PollLink(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_StatsUpdate(PLM_DEVICE_BLOCK pDevice);
#ifdef BCM_WOL
LM_VOID b44_LM_pmset(PLM_DEVICE_BLOCK pDevice);
#endif
LM_VOID b44_LM_PowerDownPhy(PLM_DEVICE_BLOCK pDevice);

/******************************************************************************/
/* These are the OS specific functions called by LMAC. */
/******************************************************************************/

LM_STATUS b44_MM_ReadConfig16(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT16 *pValue16);
LM_STATUS b44_MM_WriteConfig16(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT16 Value16);
LM_STATUS b44_MM_ReadConfig32(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT32 *pValue32);
LM_STATUS b44_MM_WriteConfig32(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Offset,
    LM_UINT32 Value32);
LM_STATUS b44_MM_MapMemBase(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_MM_MapIoBase(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_MM_IndicateRxPackets(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_MM_IndicateTxPackets(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_MM_StartTxDma(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket);
LM_STATUS b44_MM_CompleteTxDma(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket);
LM_STATUS b44_MM_AllocateMemory(PLM_DEVICE_BLOCK pDevice, LM_UINT32 BlockSize, 
    PLM_VOID *pMemoryBlockVirt);
LM_STATUS b44_MM_AllocateSharedMemory(PLM_DEVICE_BLOCK pDevice, LM_UINT32 BlockSize,
    PLM_VOID *pMemoryBlockVirt, PLM_PHYSICAL_ADDRESS pMemoryBlockPhy);
LM_STATUS b44_MM_GetConfig(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_MM_IndicateStatus(PLM_DEVICE_BLOCK pDevice, LM_STATUS Status);
LM_STATUS b44_MM_InitializeUmPackets(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_MM_FreeRxBuffer(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket);
#ifdef BCM_NAPI_RXPOLL
LM_STATUS b44_MM_ScheduleRxPoll(PLM_DEVICE_BLOCK pDevice);
#endif


#endif /* B44LM_H */

