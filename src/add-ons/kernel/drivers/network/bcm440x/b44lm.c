/******************************************************************************/
/*                                                                            */
/* Broadcom BCM4400 Linux Network Driver, Copyright (c) 2000 Broadcom         */
/* Corporation.                                                               */
/* All rights reserved.                                                       */
/*                                                                            */
/* This program is free software; you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation, located in the file LICENSE.                 */
/*                                                                            */
/* History:                                                                   */
/******************************************************************************/

#include "b44mm.h"



/******************************************************************************/
/* Local functions. */
/******************************************************************************/

LM_STATUS b44_LM_Abort(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_QueueRxPackets(PLM_DEVICE_BLOCK pDevice);
LM_STATUS b44_LM_SetFlowControl(PLM_DEVICE_BLOCK pDevice,
    LM_UINT32 LocalPhyAd, LM_UINT32 RemotePhyAd);
static LM_UINT32 b44_GetPhyAdFlowCntrlSettings(PLM_DEVICE_BLOCK pDevice);

STATIC LM_STATUS b44_LM_ResetChip(PLM_DEVICE_BLOCK pDevice);
STATIC LM_STATUS b44_LM_DisableChip(PLM_DEVICE_BLOCK pDevice);
void b44_LM_ClearStats(LM_DEVICE_BLOCK *pDevice);
void b44_LM_WriteCam(LM_DEVICE_BLOCK *pDevice, LM_UINT8 *ea,
    LM_UINT32 camindex);

LM_UINT32 b44_LM_getsbaddr(LM_DEVICE_BLOCK *pDevice, LM_UINT32 id,
    LM_UINT32 coreunit);
void b44_LM_sb_core_disable(LM_DEVICE_BLOCK *pDevice);
LM_UINT32 b44_LM_sb_pci_setup(LM_DEVICE_BLOCK *pDevice, LM_UINT32 cores);
LM_UINT32 b44_LM_sb_coreunit(LM_DEVICE_BLOCK *pDevice);
void b44_LM_sb_core_reset(LM_DEVICE_BLOCK *pDevice);
LM_UINT32 b44_LM_sb_coreid(LM_DEVICE_BLOCK *pDevice);
LM_UINT32 b44_LM_sb_corerev(LM_DEVICE_BLOCK *pDevice);
LM_UINT32 b44_LM_sb_iscoreup(LM_DEVICE_BLOCK *pDevice);
#ifdef BCM_WOL
static void b44_LM_ftwrite(LM_DEVICE_BLOCK *pDevice, LM_UINT32 *b,
    LM_UINT32 nbytes, LM_UINT32 ftaddr);
#endif

#define BCM4710_PCI_DMA		0x40000000	/* Client Mode PCI memory access space (1 GB) */
#define BCM4710_ENUM		0x18000000	/* Beginning of core enumeration space */

struct sbmap bcm4402[] = {
	{SBID_PCI_DMA,		0,	BCM4710_PCI_DMA},
	{SBID_ENUM,		0,	BCM4710_ENUM},
	{SBID_REG_EMAC,		0,	0x18000000},
	{SBID_REG_CODEC,	0,	0x18001000},
	{SBID_REG_PCI,		0,	0x18002000}
};

#ifdef B44_DEBUG
int b44_reset_count = 0;
#endif

/******************************************************************************/
/* External functions. */
/******************************************************************************/

LM_UINT32
b44_LM_ByteSwap(LM_UINT32 Value32)
{
    return ((Value32 & 0xff) << 24)| ((Value32 & 0xff00) << 8)|
        ((Value32 & 0xff0000) >> 8) | ((Value32 >> 24) & 0xff);
}

/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_QueueRxPackets(PLM_DEVICE_BLOCK pDevice)
{
	PLM_PACKET pPacket;
	LM_PHYSICAL_ADDRESS pa;
	LM_UINT32 rxout = pDevice->rxout;
	LM_UINT32 ctrl;

	pPacket = (PLM_PACKET) QQ_PopHead(&pDevice->RxPacketFreeQ.Container);
	while(pPacket) {

		/* Initialize the receive buffer pointer */
		b44_MM_MapRxDma(pDevice, pPacket, &pa);

		*((LM_UINT32 *) pPacket->u.Rx.pRxBufferVirt) = 0;

		/* prep the descriptor control value */
		ctrl = pPacket->u.Rx.RxBufferSize;
		if (rxout == (pDevice->MaxRxPacketDescCnt - 1))
			ctrl |= CTRL_EOT;

		/* init the rx descriptor */
		pDevice->pRxDesc[rxout].ctrl = ctrl;
		pDevice->pRxDesc[rxout].addr = pa + pDevice->dataoffset;

		pDevice->RxPacketArr[rxout] = pPacket;
		rxout = (rxout + 1) & (pDevice->MaxRxPacketDescCnt - 1);

		pPacket = (PLM_PACKET)
			QQ_PopHead(&pDevice->RxPacketFreeQ.Container);
	} /* while */

	pDevice->rxout = rxout;
	MM_WMB();

	REG_WR(pDevice, dmaregs.rcvptr, rxout * sizeof(dmadd_t));
	return LM_STATUS_SUCCESS;
} /* LM_QueueRxPackets */


/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
STATIC LM_STATUS
b44_LM_EepromReadBlock(PLM_DEVICE_BLOCK pDevice,
    LM_UINT32 offset, LM_UINT32 *pData, LM_UINT32 size)
{
	int off, nw;
//	LM_UINT8 chk8;
	int i;
	LM_UINT32 *buf;

	off = offset;
	nw = ROUNDUP(size, 4);
	buf = (LM_UINT32 *) pData;

	/* read the sprom */
	for (i = 0; i < nw; i += 4)
		buf[i/4] = REG_RD_OFFSET(pDevice, 4096 + off + i);

	return LM_STATUS_SUCCESS;
} /* b44_LM_EepromRead */


/******************************************************************************/
/* Description:                                                               */
/*    This routine initializes default parameters and reads the PCI           */
/*    configurations.                                                         */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_GetAdapterInfo(
PLM_DEVICE_BLOCK pDevice)
{
    LM_UINT32 eprom_dw[32];
    LM_UINT8 *eprom = (LM_UINT8 *) eprom_dw;
    LM_STATUS Status;
    LM_UINT32 Value32;

    /* Get Device Id and Vendor Id */
    Status = b44_MM_ReadConfig32(pDevice, PCI_VENDOR_ID_REG, &Value32);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }
    pDevice->PciVendorId = (LM_UINT16) Value32;
    pDevice->PciDeviceId = (LM_UINT16) (Value32 >> 16);

    Status = b44_MM_ReadConfig32(pDevice, PCI_REV_ID_REG, &Value32);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }
    pDevice->PciRevId = (LM_UINT8) Value32;

    /* Get subsystem vendor. */
    Status = b44_MM_ReadConfig32(pDevice, PCI_SUBSYSTEM_VENDOR_ID_REG, &Value32);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }
    pDevice->PciSubvendorId = (LM_UINT16) Value32;

    /* Get PCI subsystem id. */
    pDevice->PciSubsystemId = (LM_UINT16) (Value32 >> 16);

    Status = b44_MM_MapMemBase(pDevice);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }
    /* Initialize the memory view pointer. */
    pDevice->pMemView = (bcmenetregs_t *) pDevice->pMappedMemBase;

    b44_LM_EepromReadBlock(pDevice, 0, eprom_dw, sizeof(eprom_dw));

    /* check sprom version */
    if ((eprom[126] != 1) && (eprom[126] != 0x10))
        return LM_STATUS_FAILURE;

    pDevice->PermanentNodeAddress[0] = eprom[79];
    pDevice->PermanentNodeAddress[1] = eprom[78];
    pDevice->PermanentNodeAddress[2] = eprom[81];
    pDevice->PermanentNodeAddress[3] = eprom[80];
    pDevice->PermanentNodeAddress[4] = eprom[83];
    pDevice->PermanentNodeAddress[5] = eprom[82];

    memcpy(pDevice->NodeAddress, pDevice->PermanentNodeAddress, 6);

    pDevice->PhyAddr = eprom[90] & 0x1f;
    pDevice->MdcPort = (eprom[90] >> 14) & 0x1;

    /* Initialize the default values. */
    pDevice->TxPacketDescCnt = DEFAULT_TX_PACKET_DESC_COUNT;
    pDevice->RxPacketDescCnt = DEFAULT_RX_PACKET_DESC_COUNT;
    pDevice->MaxRxPacketDescCnt = DMAMAXRINGSZ / sizeof(dmadd_t);
    pDevice->MaxTxPacketDescCnt = DMAMAXRINGSZ / sizeof(dmadd_t);
    pDevice->rxoffset = 30;
    pDevice->lazyrxfc = 1;
    pDevice->lazyrxmult = 0;
    pDevice->lazytxfc = 0;
    pDevice->lazytxmult = 0;
    pDevice->intmask = DEF_INTMASK;
    pDevice->LinkStatus = LM_STATUS_LINK_DOWN;

#ifdef BCM_WOL
    pDevice->WakeUpMode = LM_WAKE_UP_MODE_NONE;
#endif

    /* Change driver parameters. */
    Status = b44_MM_GetConfig(pDevice);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }

#if 0
    /* Calling SetupPhy will cause target aborts if the chip has not */
    /*  been reset */
    b44_LM_SetupPhy(pDevice);
#endif
    ASSERT(b44_LM_sb_coreid(pDevice) == SB_ENET);

    pDevice->corerev = b44_LM_sb_corerev(pDevice);

    pDevice->sbmap = bcm4402;

    pDevice->coreunit = b44_LM_sb_coreunit(pDevice);

    ASSERT((pDevice->coreunit == 0) || (pDevice->coreunit == 1));

    /* supports link change interrupt */
    if (pDevice->corerev >= 7)
        pDevice->intmask |= I_LS;

    return LM_STATUS_SUCCESS;
} /* LM_GetAdapterInfo */


/******************************************************************************/
/* Description:                                                               */
/*    This routine sets up receive/transmit buffer descriptions queues.       */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_InitializeAdapter(PLM_DEVICE_BLOCK pDevice)
{
    LM_PHYSICAL_ADDRESS MemPhy, MemBasePhy;
    LM_UINT8 *pMemVirt, *pMemBase;
    PLM_PACKET pPacket;
    LM_STATUS Status;
    LM_UINT32 Size;
    LM_UINT32 align, j;

    /* Intialize the queues. */
    QQ_InitQueue(&pDevice->RxPacketReceivedQ.Container, 
        MAX_RX_PACKET_DESC_COUNT);
    QQ_InitQueue(&pDevice->RxPacketFreeQ.Container,
        MAX_RX_PACKET_DESC_COUNT);

    QQ_InitQueue(&pDevice->TxPacketFreeQ.Container,MAX_TX_PACKET_DESC_COUNT);
    QQ_InitQueue(&pDevice->TxPacketXmittedQ.Container,MAX_TX_PACKET_DESC_COUNT);

    /* Allocate memory for packet descriptors. */
    Size = (pDevice->RxPacketDescCnt + 
        pDevice->TxPacketDescCnt) * B44_MM_PACKET_DESC_SIZE;
    Status = b44_MM_AllocateMemory(pDevice, Size, (PLM_VOID *) &pPacket);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }

    for(j = 0; j < pDevice->TxPacketDescCnt; j++)
    {
        QQ_PushTail(&pDevice->TxPacketFreeQ.Container, pPacket);

        pPacket = (PLM_PACKET) ((PLM_UINT8) pPacket + B44_MM_PACKET_DESC_SIZE);
    } /* for(j.. */

    for(j = 0; j < pDevice->RxPacketDescCnt; j++)
    {
        /* Receive buffer size. */
        pPacket->u.Rx.RxBufferSize = 1522 + pDevice->rxoffset;

        /* Add the descriptor to RxPacketFreeQ. */
        QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);

        pPacket = (PLM_PACKET) ((PLM_UINT8) pPacket + B44_MM_PACKET_DESC_SIZE);
    } /* for */

    /* Initialize the rest of the packet descriptors. */
    Status = b44_MM_InitializeUmPackets(pDevice);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    } /* if */

    /* Make the Tx ring size power of 2 */
    pDevice->MaxTxPacketDescCnt = DMAMAXRINGSZ / sizeof(dmadd_t);
    while ((pDevice->MaxTxPacketDescCnt >> 1) > pDevice->TxPacketDescCnt)
        pDevice->MaxTxPacketDescCnt >>= 1;

    Size = (pDevice->MaxRxPacketDescCnt + pDevice->MaxTxPacketDescCnt) *
        sizeof(dmadd_t) + DMARINGALIGN;

    Status = b44_MM_AllocateSharedMemory(pDevice, Size, (PLM_VOID) &pMemBase, &MemBasePhy);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }

    MemPhy = (MemBasePhy + (DMARINGALIGN - 1)) & ~(DMARINGALIGN - 1);
    align = MemPhy - MemBasePhy;
    pMemVirt = pMemBase + align;

    pDevice->pRxDesc = (dmadd_t *) pMemVirt;
    pDevice->RxDescPhy = MemPhy;

    pMemVirt += pDevice->MaxRxPacketDescCnt * sizeof(dmadd_t);
    MemPhy += pDevice->MaxRxPacketDescCnt * sizeof(dmadd_t);

    pDevice->pTxDesc = (dmadd_t *) pMemVirt;
    pDevice->TxDescPhy = MemPhy;

    /* Initialize the hardware. */
    Status = b44_LM_ResetAdapter(pDevice, TRUE);
    if(Status != LM_STATUS_SUCCESS)
    {
        return Status;
    }

    /* We are done with initialization. */
    pDevice->InitDone = TRUE;

    return LM_STATUS_SUCCESS;
} /* LM_InitializeAdapter */


LM_STATUS
b44_LM_DisableChip(PLM_DEVICE_BLOCK pDevice)
{

    /* disable emac */
    REG_WR(pDevice, enetcontrol, EC_ED);
    SPINWAIT((REG_RD(pDevice, enetcontrol) & EC_ED), 200);

    REG_WR(pDevice, dmaregs.xmtcontrol, 0);
    REG_WR(pDevice, dmaregs.rcvcontrol, 0);
    b44_MM_Wait(10);

    return LM_STATUS_SUCCESS;
}

/******************************************************************************/
/* Description:                                                               */
/*    This function reinitializes the adapter.                                */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_ResetAdapter(PLM_DEVICE_BLOCK pDevice, LM_BOOL full)
{

    /* Disable interrupt. */
    if (pDevice->InitDone)
    {
        b44_LM_DisableInterrupt(pDevice);
    }

    /* Disable transmit and receive DMA engines.  Abort all pending requests. */
    b44_LM_Abort(pDevice);

    pDevice->ShuttingDown = FALSE;

    /* enable pci interrupts, bursts, and prefetch */
    pDevice->pcirev = b44_LM_sb_pci_setup(pDevice,
        ((pDevice->coreunit == 0)? SBIV_ENET0: SBIV_ENET1));

    pDevice->ddoffset = pDevice->dataoffset =
        b44_LM_getsbaddr(pDevice, SBID_PCI_DMA, 0);

    b44_LM_ResetChip(pDevice);

#if 1
    if (pDevice->InitDone != TRUE) {
        if (pDevice->MdcPort == pDevice->coreunit) {
            b44_LM_ResetPhy(pDevice);
            b44_LM_SetupPhy(pDevice);
        }
    }
#endif

    b44_LM_SetMacAddress(pDevice, pDevice->NodeAddress);

    /* enable crc32 generation  and set proper LED modes */
    REG_WR(pDevice, emaccontrol, EMC_CG | (0x7 << EMC_LC_SHIFT));

    REG_WR(pDevice, intrecvlazy, (pDevice->lazyrxfc << IRL_FC_SHIFT));
    if (pDevice->lazyrxfc > 1)
    {
        REG_OR(pDevice, intrecvlazy, (pDevice->lazyrxmult * pDevice->lazyrxfc));
    }

    /* enable 802.3x tx flow control (honor received PAUSE frames) */
//    REG_WR(pDevice, rxconfig, ERC_FE | ERC_UF);

    b44_LM_SetReceiveMask(pDevice, pDevice->ReceiveMask);

    /* set max frame lengths - account for possible vlan tag */
    REG_WR(pDevice, rxmaxlength, MAX_ETHERNET_PACKET_SIZE + 32);
    REG_WR(pDevice, txmaxlength, MAX_ETHERNET_PACKET_SIZE + 32);

    /* set tx watermark */
    REG_WR(pDevice, txwatermark, 56);

    if (full)
    {
        /* initialize the tx and rx dma channels */
        /* clear tx descriptor ring */
        memset((void*)pDevice->pTxDesc, 0, (pDevice->MaxTxPacketDescCnt *
            sizeof(dmadd_t)));

        REG_WR(pDevice, dmaregs.xmtcontrol, XC_XE);
        REG_WR(pDevice, dmaregs.xmtaddr, (pDevice->TxDescPhy +
            pDevice->ddoffset));

        /* clear rx descriptor ring */
        memset((void*)pDevice->pRxDesc, 0, (pDevice->MaxRxPacketDescCnt *
            sizeof(dmadd_t)));

        REG_WR(pDevice, dmaregs.rcvcontrol, ((pDevice->rxoffset <<
            RC_RO_SHIFT) | RC_RE));

        REG_WR(pDevice, dmaregs.rcvaddr, (pDevice->RxDescPhy +
            pDevice->ddoffset));

        /* Queue Rx packet buffers. */
        b44_LM_QueueRxPackets(pDevice);

        MM_ATOMIC_SET(&pDevice->SendDescLeft, pDevice->TxPacketDescCnt - 1);
    }
    else
    {
        REG_WR(pDevice, dmaregs.rcvcontrol, ((pDevice->rxoffset <<
            RC_RO_SHIFT) | RC_RE));
    }

    /* turn on the emac */
    REG_OR(pDevice, enetcontrol, EC_EE);

    return LM_STATUS_SUCCESS;
} /* LM_ResetAdapter */


/******************************************************************************/
/* Description:                                                               */
/*    This routine disables the adapter from generating interrupts.           */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_DisableInterrupt(
    PLM_DEVICE_BLOCK pDevice)
{
	REG_WR(pDevice, intmask, 0);
	(void) REG_RD(pDevice, intmask);	/* sync readback */
	return LM_STATUS_SUCCESS;
}



/******************************************************************************/
/* Description:                                                               */
/*    This routine enables the adapter to generate interrupts.                */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_EnableInterrupt(
    PLM_DEVICE_BLOCK pDevice)
{
	REG_WR(pDevice, intmask, pDevice->intmask);
	return LM_STATUS_SUCCESS;
}



/******************************************************************************/
/* Description:                                                               */
/*    This routine puts a packet on the wire if there is a transmit DMA       */
/*    descriptor available; otherwise the packet is queued for later          */
/*    transmission.  If the second argue is NULL, this routine will put       */
/*    the queued packet on the wire if possible.                              */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_SendPacket(PLM_DEVICE_BLOCK pDevice, PLM_PACKET pPacket)
{
	LM_UINT32 fragcount;
	LM_UINT32 len, txout, ctrl;
	LM_PHYSICAL_ADDRESS pa;
	int first, next;

	txout = pDevice->txout;

	pDevice->TxPacketArr[txout] = pPacket;
	for(fragcount = 0, first = 1, next = 1; next;
		first = 0, fragcount++) {

		ctrl = 0;
		b44_MM_MapTxDma(pDevice, pPacket, &pa, &len, fragcount);
		ctrl = len & CTRL_BC_MASK;

		if (first)
			ctrl |= CTRL_SOF;
		if (fragcount == (pPacket->u.Tx.FragCount - 1)) {
			ctrl |= CTRL_EOF;
			next = 0;
		}
		if (txout == (pDevice->MaxTxPacketDescCnt - 1)) {
			ctrl |= CTRL_EOT;
		}
		ctrl |= CTRL_IOC;

		/* init the tx descriptor */
		pDevice->pTxDesc[txout].ctrl = ctrl;
		pDevice->pTxDesc[txout].addr = pa + pDevice->dataoffset;

		txout = (txout + 1) & (pDevice->MaxTxPacketDescCnt - 1);

	}

	MM_ATOMIC_SUB(&pDevice->SendDescLeft, pPacket->u.Tx.FragCount);

	pDevice->txout = txout;

	MM_WMB();

	REG_WR(pDevice, dmaregs.xmtptr, (txout * sizeof(dmadd_t)));

	return LM_STATUS_SUCCESS;
}


/******************************************************************************/
/* Description:                                                               */
/*    This routine sets the receive control register according to ReceiveMask */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_SetReceiveMask(PLM_DEVICE_BLOCK pDevice, LM_UINT32 Mask)
{
    LM_UINT32 ReceiveMask;
    LM_UINT32 j;
    LM_UINT32 idx = 0;
    LM_UINT8 zero[6] = {0,0,0,0,0,0};

    ReceiveMask = Mask;

    if(Mask & LM_ACCEPT_UNICAST)
    {
        Mask &= ~LM_ACCEPT_UNICAST;
    }

    if(Mask & LM_ACCEPT_MULTICAST)
    {
        Mask &= ~LM_ACCEPT_MULTICAST;
    }

    if(Mask & LM_ACCEPT_ALL_MULTICAST)
    {
        Mask &= ~LM_ACCEPT_ALL_MULTICAST;
    }

    if(Mask & LM_ACCEPT_BROADCAST)
    {
        Mask &= ~LM_ACCEPT_BROADCAST;
    }

    if(Mask & LM_PROMISCUOUS_MODE)
    {
        Mask &= ~LM_PROMISCUOUS_MODE;
    }

    /* Make sure all the bits are valid before committing changes. */
    if(Mask)
    {
        return LM_STATUS_FAILURE;
    }

	if (ReceiveMask & LM_PROMISCUOUS_MODE)
		REG_OR(pDevice, rxconfig, ERC_PE);
	else {
		REG_WR(pDevice, camcontrol, 0);
		/* our local address */
		b44_LM_WriteCam(pDevice, pDevice->NodeAddress, idx++);

		/* allmulti or a list of discrete multicast addresses */
		if (ReceiveMask & LM_ACCEPT_ALL_MULTICAST)
			REG_OR(pDevice, rxconfig, ERC_AM);
		else if (ReceiveMask & LM_ACCEPT_MULTICAST) {
			for(j = 0; j < pDevice->McEntryCount; j++) {
				b44_LM_WriteCam(pDevice, pDevice->McTable[j],
					idx++);
			}
		}

		for (; idx < 64; idx++) {
			b44_LM_WriteCam(pDevice, zero, idx);
		}

		/* enable cam */
		REG_OR(pDevice, camcontrol, CC_CE);
	}

	return LM_STATUS_SUCCESS;
} /* LM_SetReceiveMask */



/******************************************************************************/
/* Description:                                                               */
/*    Disable the interrupt and put the transmitter and receiver engines in   */
/*    an idle state.  Also aborts all pending send requests and receive       */
/*    buffers.                                                                */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_Abort(
PLM_DEVICE_BLOCK pDevice)
{
    PLM_PACKET pPacket;
    LM_UINT32 rxin, txin, txdmask, txcurr;

    if (!pDevice->InitDone)
    {
        return LM_STATUS_SUCCESS;
    }

    b44_LM_DisableInterrupt(pDevice);

    txcurr = (REG_RD(pDevice, dmaregs.xmtstatus) & XS_CD_MASK);
    txcurr = txcurr / sizeof(dmadd_t);
    /* Allow tx packets to drain */
    if (pDevice->txout != txcurr)
    {
        b44_MM_Wait(20);
    }
    REG_WR(pDevice, dmaregs.xmtcontrol, 0);
    b44_MM_Wait(120);

    b44_LM_DisableChip(pDevice);

    txdmask = pDevice->MaxTxPacketDescCnt - 1;
    for (txin = pDevice->txin; txin != pDevice->txout;
        txin = (txin + 1) & txdmask)
    {
        if ((pPacket = pDevice->TxPacketArr[txin])) {
            QQ_PushTail(&pDevice->TxPacketXmittedQ.Container, pPacket);
            pDevice->TxPacketArr[txin] = 0;
        }
    }

    if(!pDevice->ShuttingDown)
    {
        /* Indicate packets to the protocol. */
        b44_MM_IndicateTxPackets(pDevice);

        /* Indicate received packets to the protocols. */
        b44_MM_IndicateRxPackets(pDevice);
    }
    else
    {
        /* Move the receive packet descriptors in the ReceivedQ to the */
        /* free queue. */
        for(; ;)
        {
            pPacket = (PLM_PACKET) QQ_PopHead(
                &pDevice->RxPacketReceivedQ.Container);
            if(pPacket == NULL)
            {
                break;
            }
            QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);
        }
    }

    /* Clean up the Receive desc ring. */

    rxin = pDevice->rxin;
    while(rxin != pDevice->rxout) {
        pPacket = pDevice->RxPacketArr[rxin];

        QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);

        rxin = (rxin + 1) & (pDevice->MaxRxPacketDescCnt - 1);
    } /* while */

    pDevice->rxin = rxin;
    return LM_STATUS_SUCCESS;
} /* LM_Abort */



/******************************************************************************/
/* Description:                                                               */
/*    Disable the interrupt and put the transmitter and receiver engines in   */
/*    an idle state.  Aborts all pending send requests and receive buffers.   */
/*    Also free all the receive buffers.                                      */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_Halt(PLM_DEVICE_BLOCK pDevice)
{
    PLM_PACKET pPacket;
    LM_UINT32 EntryCnt;

    b44_LM_Abort(pDevice);

    /* Get the number of entries in the queue. */
    EntryCnt = QQ_GetEntryCnt(&pDevice->RxPacketFreeQ.Container);

    /* Make sure all the packets have been accounted for. */
    for(EntryCnt = 0; EntryCnt < pDevice->RxPacketDescCnt; EntryCnt++)
    {
        pPacket = (PLM_PACKET) QQ_PopHead(&pDevice->RxPacketFreeQ.Container);
        if (pPacket == 0)
            break;

        b44_MM_FreeRxBuffer(pDevice, pPacket);

        QQ_PushTail(&pDevice->RxPacketFreeQ.Container, pPacket);
    }

    b44_LM_ResetChip(pDevice);

    /* Reprogram the MAC address. */
    b44_LM_SetMacAddress(pDevice, pDevice->NodeAddress);

    return LM_STATUS_SUCCESS;
} /* LM_Halt */


STATIC LM_STATUS
b44_LM_ResetChip(LM_DEVICE_BLOCK *pDevice)
{
	if (!b44_LM_sb_iscoreup(pDevice)) {
		b44_LM_sb_pci_setup(pDevice, 
			((pDevice->coreunit == 0)? SBIV_ENET0: SBIV_ENET1));	
		/* power on reset: reset the enet core */
		b44_LM_sb_core_reset(pDevice);

		goto chipinreset;
	}

	/* read counters before resetting the chip */
	if (pDevice->mibgood)
		b44_LM_StatsUpdate(pDevice);

	REG_WR(pDevice, intrecvlazy, 0);

	/* disable emac */
	REG_WR(pDevice, enetcontrol, EC_ED);
	SPINWAIT((REG_RD(pDevice, enetcontrol) & EC_ED), 200);

	/* reset the dma engines */
	REG_WR(pDevice, dmaregs.xmtcontrol, 0);
	pDevice->txin = pDevice->txout = 0;

	if (REG_RD(pDevice, dmaregs.rcvstatus) & RS_RE_MASK) {
		/* wait until channel is idle or stopped */
		SPINWAIT(!(REG_RD(pDevice, dmaregs.rcvstatus) & RS_RS_IDLE),
			100);
	}

	REG_WR(pDevice, dmaregs.rcvcontrol, 0);
	pDevice->rxin = pDevice->rxout = 0;

	REG_WR(pDevice, enetcontrol, EC_ES);

	b44_LM_sb_core_reset(pDevice);

chipinreset:
	if (pDevice->InitDone == FALSE)
		b44_LM_ClearStats(pDevice);

	/*
	 * We want the phy registers to be accessible even when
	 * the driver is "downed" so initialize MDC preamble, frequency,
	 * and whether internal or external phy here.
	 */
		/* 4402 has 62.5Mhz SB clock and internal phy */
	REG_WR(pDevice, mdiocontrol, 0x8d);

	/* some chips have internal phy, some don't */
	if (!(REG_RD(pDevice, devcontrol) & DC_IP)) {
		REG_WR(pDevice, enetcontrol, EC_EP);
	} else if (REG_RD(pDevice, devcontrol) & DC_ER) {
		REG_AND(pDevice, devcontrol, ~DC_ER);

		b44_MM_Wait(100);
	}

	/* clear persistent sw intstatus */
	pDevice->intstatus = 0;
	return LM_STATUS_SUCCESS;
}

void
b44_LM_ClearStats(LM_DEVICE_BLOCK *pDevice)
{
	/* must clear mib registers by hand */
	REG_WR(pDevice, mibcontrol, EMC_RZ);
	(void) REG_RD(pDevice, mib.tx_good_octets);
	(void) REG_RD(pDevice, mib.tx_good_pkts);
	(void) REG_RD(pDevice, mib.tx_octets);
	(void) REG_RD(pDevice, mib.tx_pkts);
	(void) REG_RD(pDevice, mib.tx_broadcast_pkts);
	(void) REG_RD(pDevice, mib.tx_multicast_pkts);
	(void) REG_RD(pDevice, mib.tx_len_64);
	(void) REG_RD(pDevice, mib.tx_len_65_to_127);
	(void) REG_RD(pDevice, mib.tx_len_128_to_255);
	(void) REG_RD(pDevice, mib.tx_len_256_to_511);
	(void) REG_RD(pDevice, mib.tx_len_512_to_1023);
	(void) REG_RD(pDevice, mib.tx_len_1024_to_max);
	(void) REG_RD(pDevice, mib.tx_jabber_pkts);
	(void) REG_RD(pDevice, mib.tx_oversize_pkts);
	(void) REG_RD(pDevice, mib.tx_fragment_pkts);
	(void) REG_RD(pDevice, mib.tx_underruns);
	(void) REG_RD(pDevice, mib.tx_total_cols);
	(void) REG_RD(pDevice, mib.tx_single_cols);
	(void) REG_RD(pDevice, mib.tx_multiple_cols);
	(void) REG_RD(pDevice, mib.tx_excessive_cols);
	(void) REG_RD(pDevice, mib.tx_late_cols);
	(void) REG_RD(pDevice, mib.tx_defered);
	(void) REG_RD(pDevice, mib.tx_carrier_lost);
	(void) REG_RD(pDevice, mib.tx_pause_pkts);
	(void) REG_RD(pDevice, mib.rx_good_octets);
	(void) REG_RD(pDevice, mib.rx_good_pkts);
	(void) REG_RD(pDevice, mib.rx_octets);
	(void) REG_RD(pDevice, mib.rx_pkts);
	(void) REG_RD(pDevice, mib.rx_broadcast_pkts);
	(void) REG_RD(pDevice, mib.rx_multicast_pkts);
	(void) REG_RD(pDevice, mib.rx_len_64);
	(void) REG_RD(pDevice, mib.rx_len_65_to_127);
	(void) REG_RD(pDevice, mib.rx_len_128_to_255);
	(void) REG_RD(pDevice, mib.rx_len_256_to_511);
	(void) REG_RD(pDevice, mib.rx_len_512_to_1023);
	(void) REG_RD(pDevice, mib.rx_len_1024_to_max);
	(void) REG_RD(pDevice, mib.rx_jabber_pkts);
	(void) REG_RD(pDevice, mib.rx_oversize_pkts);
	(void) REG_RD(pDevice, mib.rx_fragment_pkts);
	(void) REG_RD(pDevice, mib.rx_missed_pkts);
	(void) REG_RD(pDevice, mib.rx_crc_align_errs);
	(void) REG_RD(pDevice, mib.rx_undersize);
	(void) REG_RD(pDevice, mib.rx_crc_errs);
	(void) REG_RD(pDevice, mib.rx_align_errs);
	(void) REG_RD(pDevice, mib.rx_symbol_errs);
	(void) REG_RD(pDevice, mib.rx_pause_pkts);
	(void) REG_RD(pDevice, mib.rx_nonpause_pkts);
	pDevice->mibgood = TRUE;
}

#ifdef BCM_NAPI_RXPOLL
int
b44_LM_ServiceRxPoll(PLM_DEVICE_BLOCK pDevice, int limit)
{
	LM_UINT32 rxin, curr, rxdmask;
	unsigned int len;
	int skiplen = 0;
	bcmenetrxh_t *rxh;
	LM_PACKET *pPacket;
	int received = 0;

	curr = (REG_RD(pDevice, dmaregs.rcvstatus) & RS_CD_MASK);
	curr = curr / sizeof(dmadd_t);
	rxdmask = pDevice->MaxRxPacketDescCnt - 1;
	for (rxin = pDevice->rxin; rxin != curr; rxin = (rxin + 1) & rxdmask)
	{
		pPacket = pDevice->RxPacketArr[rxin];
		if (skiplen > 0) {
			pPacket->PacketStatus = LM_STATUS_FAILURE;
			skiplen -= pPacket->u.Rx.RxBufferSize;
			if (skiplen < 0)
				skiplen = 0;
			goto rx_err;
		}
		rxh = (bcmenetrxh_t *) pPacket->u.Rx.pRxBufferVirt;
		len = MM_SWAP_LE16(rxh->len);
		if (len > (pPacket->u.Rx.RxBufferSize - pDevice->rxoffset)) {
			pPacket->PacketStatus = LM_STATUS_FAILURE;
			skiplen = len - (pPacket->u.Rx.RxBufferSize -
				pDevice->rxoffset);
		}
		else {
			int i = 0;

			if (len == 0) {
				while ((len == 0) && (i < 5)) {
					b44_MM_Wait(2);
					len = MM_SWAP_LE16(rxh->len);
					i++;
				}
				if (len == 0) {
					pPacket->PacketStatus =
						LM_STATUS_FAILURE;
					goto rx_err;
				}
			}
			if (MM_SWAP_LE16(rxh->flags) & RXF_ERRORS) {
				pPacket->PacketStatus = LM_STATUS_FAILURE;
			}
			else {
				pPacket->PacketStatus = LM_STATUS_SUCCESS;
			}
			pPacket->PacketSize = len - 4;
		}
rx_err:
		QQ_PushTail(&pDevice->RxPacketReceivedQ.Container,
			pPacket);

		if (++received >= limit)
		{
			rxin = (rxin + 1) & rxdmask;
			break;
		}
		curr = (REG_RD(pDevice, dmaregs.rcvstatus) & RS_CD_MASK) /
			sizeof(dmadd_t);
	}
	pDevice->rxin = rxin;
	return received;
}
#endif

void
b44_LM_ServiceRxInterrupt(LM_DEVICE_BLOCK *pDevice)
{
	LM_UINT32 curr;
#ifndef BCM_NAPI_RXPOLL
	LM_UINT32 rxin, rxdmask;
	unsigned int len;
	int skiplen = 0;
	bcmenetrxh_t *rxh;
	LM_PACKET *pPacket;
#endif

	curr = (REG_RD(pDevice, dmaregs.rcvstatus) & RS_CD_MASK);
	curr = curr / sizeof(dmadd_t);
#ifdef BCM_NAPI_RXPOLL
	if (!pDevice->RxPoll)
	{
		if (pDevice->rxin != curr)
		{
			if (b44_MM_ScheduleRxPoll(pDevice) == LM_STATUS_SUCCESS)
			{
				pDevice->RxPoll = TRUE;
				pDevice->intmask &= ~(I_RI | I_RU | I_RO);
				REG_WR(pDevice, intmask, pDevice->intmask);
			}
		}
	}
#else
	rxdmask = pDevice->MaxRxPacketDescCnt - 1;
	for (rxin = pDevice->rxin; rxin != curr; rxin = (rxin + 1) & rxdmask)
	{
		pPacket = pDevice->RxPacketArr[rxin];
		if (skiplen > 0) {
			pPacket->PacketStatus = LM_STATUS_FAILURE;
			skiplen -= pPacket->u.Rx.RxBufferSize;
			if (skiplen < 0)
				skiplen = 0;
			goto rx_err;
		}
		rxh = (bcmenetrxh_t *) pPacket->u.Rx.pRxBufferVirt;
		len = MM_SWAP_LE16(rxh->len);
		if (len > (pPacket->u.Rx.RxBufferSize - pDevice->rxoffset)) {
			pPacket->PacketStatus = LM_STATUS_FAILURE;
			skiplen = len - (pPacket->u.Rx.RxBufferSize -
				pDevice->rxoffset);
		}
		else {
			int i = 0;

			if (len == 0) {
				while ((len == 0) && (i < 5)) {
					b44_MM_Wait(2);
					len = MM_SWAP_LE16(rxh->len);
					i++;
				}
				if (len == 0) {
					pPacket->PacketStatus =
						LM_STATUS_FAILURE;
					goto rx_err;
				}
			}
			if (MM_SWAP_LE16(rxh->flags) & RXF_ERRORS) {
				pPacket->PacketStatus = LM_STATUS_FAILURE;
			}
			else {
				pPacket->PacketStatus = LM_STATUS_SUCCESS;
			}
			pPacket->PacketSize = len - 4;
		}
rx_err:
		QQ_PushTail(&pDevice->RxPacketReceivedQ.Container,
			pPacket);
		curr = (REG_RD(pDevice, dmaregs.rcvstatus) & RS_CD_MASK) /
			sizeof(dmadd_t);
	}
	pDevice->rxin = curr;
#endif
}

void
b44_LM_ServiceTxInterrupt(LM_DEVICE_BLOCK *pDevice)
{
	LM_UINT32 txin, curr, txdmask;
	LM_PACKET *pPacket;

	curr = (REG_RD(pDevice, dmaregs.xmtstatus) & XS_CD_MASK);
	curr = curr / sizeof(dmadd_t);
	txdmask = pDevice->MaxTxPacketDescCnt - 1;
	for (txin = pDevice->txin; txin != curr; txin = (txin + 1) & txdmask)
	{
		if ((pPacket = pDevice->TxPacketArr[txin])) {
			QQ_PushTail(&pDevice->TxPacketXmittedQ.Container,
				pPacket);
			pDevice->TxPacketArr[txin] = 0;
			MM_ATOMIC_ADD(&pDevice->SendDescLeft,
				pPacket->u.Tx.FragCount);
		}
	}
	pDevice->txin = curr;
}

/******************************************************************************/
/* Description:                                                               */
/*    This is the interrupt event handler routine. It acknowledges all        */
/*    pending interrupts and process all pending events.                      */
/*                                                                            */
/* Return:                                                                    */
/*    LM_STATUS_SUCCESS                                                       */
/******************************************************************************/
LM_STATUS
b44_LM_ServiceInterrupts(PLM_DEVICE_BLOCK pDevice)
{
	LM_UINT32 intstatus, intmask;

	while (1) {
		intstatus = REG_RD(pDevice, intstatus);
		intmask = REG_RD(pDevice, intmask);

		/* defer unsolicited interrupts */
		intstatus &= intmask;

		/* if not for us */
		if (intstatus == 0)
			return 12;

		/* clear the interrupt */
		REG_WR(pDevice, intstatus, intstatus);

		if (intstatus & I_LS) {
			b44_LM_PollLink(pDevice);
		}
		if (intstatus & I_RI) {
			b44_LM_ServiceRxInterrupt(pDevice);
		}

		if (intstatus & (I_XI | I_TO)) {
			b44_LM_ServiceTxInterrupt(pDevice);
			REG_WR(pDevice, gptimer, 0);
		}
		if (intstatus & I_ERRORS) {
#ifdef B44_DEBUG
			b44_reset_count++;
#endif
			pDevice->InReset = TRUE;
			b44_LM_ResetAdapter(pDevice, TRUE);
			pDevice->InReset = FALSE;
			b44_LM_EnableInterrupt(pDevice);
		}
#ifndef BCM_NAPI_RXPOLL
		if (!QQ_Empty(&pDevice->RxPacketReceivedQ.Container)) {
			b44_MM_IndicateRxPackets(pDevice);
		}
#endif
		if (!QQ_Empty(&pDevice->TxPacketXmittedQ.Container)) {
			b44_MM_IndicateTxPackets(pDevice);
		}
	}
	return LM_STATUS_SUCCESS;
} /* LM_ServiceInterrupts */



/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_MulticastAdd(
PLM_DEVICE_BLOCK pDevice,
PLM_UINT8 pMcAddress) {
    PLM_UINT8 pEntry;
    LM_UINT32 j;

    pEntry = pDevice->McTable[0];
    for(j = 0; j < pDevice->McEntryCount; j++)
    {
        if(IS_ETH_ADDRESS_EQUAL(pEntry, pMcAddress))
        {
            /* Found a match, increment the instance count. */
            pEntry[LM_MC_INSTANCE_COUNT_INDEX] += 1;

            return LM_STATUS_SUCCESS;
        }

        pEntry += LM_MC_ENTRY_SIZE;
    }
    
    if(pDevice->McEntryCount >= LM_MAX_MC_TABLE_SIZE)
    {
        return LM_STATUS_FAILURE;
    }

    pEntry = pDevice->McTable[pDevice->McEntryCount];

    COPY_ETH_ADDRESS(pMcAddress, pEntry);
    pEntry[LM_MC_INSTANCE_COUNT_INDEX] = 1;

    pDevice->McEntryCount++;

    b44_LM_SetReceiveMask(pDevice, pDevice->ReceiveMask | LM_ACCEPT_MULTICAST);

    return LM_STATUS_SUCCESS;
} /* b44_LM_MulticastAdd */



/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_MulticastDel(PLM_DEVICE_BLOCK pDevice, PLM_UINT8 pMcAddress)
{
    PLM_UINT8 pEntry;
    LM_UINT32 j;

    pEntry = pDevice->McTable[0];
    for(j = 0; j < pDevice->McEntryCount; j++)
    {
        if(IS_ETH_ADDRESS_EQUAL(pEntry, pMcAddress))
        {
            /* Found a match, decrement the instance count. */
            pEntry[LM_MC_INSTANCE_COUNT_INDEX] -= 1;

            /* No more instance left, remove the address from the table. */
            /* Move the last entry in the table to the delete slot. */
            if(pEntry[LM_MC_INSTANCE_COUNT_INDEX] == 0 &&
                pDevice->McEntryCount > 1)
            {

                COPY_ETH_ADDRESS(
                    pDevice->McTable[pDevice->McEntryCount-1], pEntry);
                pEntry[LM_MC_INSTANCE_COUNT_INDEX] =
                    pDevice->McTable[pDevice->McEntryCount-1]
                    [LM_MC_INSTANCE_COUNT_INDEX];
            }
            pDevice->McEntryCount--;

            /* Update the receive mask if the table is empty. */
            if(pDevice->McEntryCount == 0)
            {
                b44_LM_SetReceiveMask(pDevice, 
                    pDevice->ReceiveMask & ~LM_ACCEPT_MULTICAST);
            }

            return LM_STATUS_SUCCESS;
        }

        pEntry += LM_MC_ENTRY_SIZE;
    }

    return LM_STATUS_FAILURE;
} /* b44_LM_MulticastDel */



/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_MulticastClear(
PLM_DEVICE_BLOCK pDevice) {
    pDevice->McEntryCount = 0;

    b44_LM_SetReceiveMask(pDevice, pDevice->ReceiveMask & ~LM_ACCEPT_MULTICAST);

    return LM_STATUS_SUCCESS;
} /* b44_LM_MulticastClear */



/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_SetMacAddress(PLM_DEVICE_BLOCK pDevice, PLM_UINT8 pMacAddress)
{
    return LM_STATUS_SUCCESS;
}


/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_SetFlowControl(
    PLM_DEVICE_BLOCK pDevice,
    LM_UINT32 LocalPhyAd,
    LM_UINT32 RemotePhyAd)
{
    LM_FLOW_CONTROL FlowCap;

    /* Resolve flow control. */
    FlowCap = LM_FLOW_CONTROL_NONE;

    /* See Table 28B-3 of 802.3ab-1999 spec. */
    if(pDevice->FlowControlCap & LM_FLOW_CONTROL_AUTO_PAUSE)
    {
        if(LocalPhyAd & PHY_AN_AD_PAUSE_CAPABLE)
        {
            if(LocalPhyAd & PHY_AN_AD_ASYM_PAUSE)
            {
                if(RemotePhyAd & PHY_LINK_PARTNER_PAUSE_CAPABLE)
                {
                    FlowCap = LM_FLOW_CONTROL_TRANSMIT_PAUSE |
                        LM_FLOW_CONTROL_RECEIVE_PAUSE;
                }
                else if(RemotePhyAd & PHY_LINK_PARTNER_ASYM_PAUSE)
                {
                    FlowCap = LM_FLOW_CONTROL_RECEIVE_PAUSE;
                }
            }
            else
            {
                if(RemotePhyAd & PHY_LINK_PARTNER_PAUSE_CAPABLE)
                {
                    FlowCap = LM_FLOW_CONTROL_TRANSMIT_PAUSE |
                        LM_FLOW_CONTROL_RECEIVE_PAUSE;
                }
            }
        }
        else if(LocalPhyAd & PHY_AN_AD_ASYM_PAUSE)
        {
            if((RemotePhyAd & PHY_LINK_PARTNER_PAUSE_CAPABLE) &&
                (RemotePhyAd & PHY_LINK_PARTNER_ASYM_PAUSE))
            {
                FlowCap = LM_FLOW_CONTROL_TRANSMIT_PAUSE;
            }
        }
    }
    else
    {
        FlowCap = pDevice->FlowControlCap;
    }

    /* Enable/disable rx PAUSE. */
    if(FlowCap & LM_FLOW_CONTROL_RECEIVE_PAUSE &&
        (pDevice->FlowControlCap == LM_FLOW_CONTROL_AUTO_PAUSE ||
        pDevice->FlowControlCap & LM_FLOW_CONTROL_RECEIVE_PAUSE))
    {
        pDevice->FlowControl |= LM_FLOW_CONTROL_RECEIVE_PAUSE;
        REG_WR(pDevice, rxconfig, ERC_EF);

    }

    /* Enable/disable tx PAUSE. */
    if(FlowCap & LM_FLOW_CONTROL_TRANSMIT_PAUSE &&
        (pDevice->FlowControlCap == LM_FLOW_CONTROL_AUTO_PAUSE ||
        pDevice->FlowControlCap & LM_FLOW_CONTROL_TRANSMIT_PAUSE))
    {
        pDevice->FlowControl |= LM_FLOW_CONTROL_TRANSMIT_PAUSE;
        REG_WR(pDevice, emacflowcontrol, EMF_PG | (0xc0 & EMF_RFH_MASK));

    }

    return LM_STATUS_SUCCESS;
}



/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_STATUS
b44_LM_SetupPhy(
    PLM_DEVICE_BLOCK pDevice)
{
	LM_UINT32 Value32;
	LM_UINT32 Adv, FCAdv, Ctrl, NewCtrl;
	int RestartAuto = 0;
	int i;

	/* enable activity led */
	b44_LM_ReadPhy(pDevice, 26, &Value32);
	b44_LM_WritePhy(pDevice, 26, Value32 & 0x7fff);

	/* enable traffic meter led mode */
	b44_LM_ReadPhy(pDevice, 27, &Value32);
	b44_LM_WritePhy(pDevice, 27, Value32 | (1 << 6));
	if (!pDevice->DisableAutoNeg) {
		if (pDevice->RequestedLineSpeed == LM_LINE_SPEED_AUTO) {
			Adv = PHY_AN_AD_ALL_SPEEDS;
		}
		else if (pDevice->RequestedLineSpeed == LM_LINE_SPEED_10MBPS) {
			if (pDevice->RequestedDuplexMode ==
				LM_DUPLEX_MODE_FULL) {
				Adv = PHY_AN_AD_10BASET_FULL;
			}
			else {
				Adv = PHY_AN_AD_10BASET_HALF;
			}
		}
		else if (pDevice->RequestedLineSpeed == LM_LINE_SPEED_100MBPS) {
			if (pDevice->RequestedDuplexMode ==
				LM_DUPLEX_MODE_FULL) {
				Adv = PHY_AN_AD_100BASETX_FULL;
			}
			else {
				Adv = PHY_AN_AD_100BASETX_HALF;
			}
		}
		else {
			Adv = PHY_AN_AD_ALL_SPEEDS;
		}

		if ((pDevice->RequestedLineSpeed == LM_LINE_SPEED_AUTO) ||
			(pDevice->RequestedDuplexMode == LM_DUPLEX_MODE_FULL)) {
			FCAdv = b44_GetPhyAdFlowCntrlSettings(pDevice);
			Value32 &= PHY_AN_AD_ASYM_PAUSE |
				PHY_AN_AD_PAUSE_CAPABLE;
			if (FCAdv != Value32) {
				RestartAuto = 1;
				Adv |= FCAdv;
				goto restart_auto_neg;
			}
		}

		b44_LM_ReadPhy(pDevice, PHY_CTRL_REG, &Ctrl);
		if (!(Ctrl & PHY_CTRL_AUTO_NEG_ENABLE)) {
			RestartAuto = 1;
			goto restart_auto_neg;
		}
		b44_LM_ReadPhy(pDevice, PHY_AN_AD_REG, &Value32);
		if ((Value32 & PHY_AN_AD_ALL_SPEEDS) != Adv) {
			RestartAuto = 1;
		}
restart_auto_neg:
		if (RestartAuto) {
			Adv |= PHY_AN_AD_PROTOCOL_802_3_CSMA_CD;
			b44_LM_WritePhy(pDevice, PHY_AN_AD_REG, Adv);
			b44_LM_WritePhy(pDevice, PHY_CTRL_REG,
				PHY_CTRL_AUTO_NEG_ENABLE |
				PHY_CTRL_RESTART_AUTO_NEG);
		}
		pDevice->Advertising = Adv;
	}
	else {
		b44_LM_ReadPhy(pDevice, PHY_CTRL_REG, &Ctrl);
		NewCtrl = Ctrl & (~(PHY_CTRL_SPEED_SELECT_100MBPS |
			PHY_CTRL_FULL_DUPLEX_MODE | PHY_CTRL_AUTO_NEG_ENABLE));
		if (pDevice->RequestedLineSpeed == LM_LINE_SPEED_100MBPS) {
			NewCtrl |= PHY_CTRL_SPEED_SELECT_100MBPS;
		}
		if (pDevice->RequestedDuplexMode == LM_DUPLEX_MODE_FULL) {
			NewCtrl |= PHY_CTRL_FULL_DUPLEX_MODE;
			REG_OR(pDevice, txcontrol, EXC_FD);
		}
		else {
			REG_AND(pDevice, txcontrol, ~EXC_FD);
		}
		if (NewCtrl != Ctrl) {
			/* force a link down */
			b44_LM_WritePhy(pDevice, PHY_CTRL_REG,
				PHY_CTRL_LOOPBACK_MODE);
			i = 0;
			do {
				b44_LM_ReadPhy(pDevice, PHY_STATUS_REG,
					&Value32);
				b44_MM_Wait(100);
				i++;
			} while ((Value32 & PHY_STATUS_LINK_PASS) && (i < 800));
			b44_LM_ResetPhy(pDevice);
			b44_MM_Wait(100);
			b44_LM_WritePhy(pDevice, PHY_CTRL_REG, NewCtrl);
			b44_LM_ReadPhy(pDevice, 26, &Value32);
			b44_LM_WritePhy(pDevice, 26, Value32 & 0x7fff);
		}
		if (pDevice->RequestedDuplexMode == LM_DUPLEX_MODE_FULL) {
			pDevice->FlowControlCap &= ~LM_FLOW_CONTROL_AUTO_PAUSE;
			b44_LM_SetFlowControl(pDevice, 0, 0);
		}
	}
	return LM_STATUS_SUCCESS;
}

LM_STATUS
b44_LM_ResetPhy(LM_DEVICE_BLOCK *pDevice)
{
	LM_UINT32 value32;

	b44_LM_WritePhy(pDevice, 0, PHY_CTRL_PHY_RESET);
	b44_MM_Wait(100);
	b44_LM_ReadPhy(pDevice, 0, &value32);
	if (value32 & PHY_CTRL_PHY_RESET) {
		printf("Phy reset not complete\n");
	}
	return LM_STATUS_SUCCESS;
}

/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_VOID
b44_LM_ReadPhy(
PLM_DEVICE_BLOCK pDevice, LM_UINT32 PhyReg, LM_UINT32 *pData32)
{
	/* clear mii_int */
	REG_WR(pDevice, emacintstatus, EI_MII);

	/* issue the read */
	REG_WR(pDevice, mdiodata, (MD_SB_START | MD_OP_READ |
		(pDevice->PhyAddr << MD_PMD_SHIFT)
		| (PhyReg << MD_RA_SHIFT) | MD_TA_VALID));

	/* wait for it to complete */
	SPINWAIT(((REG_RD(pDevice, emacintstatus) & EI_MII) == 0), 100);
	if ((REG_RD(pDevice, emacintstatus) & EI_MII) == 0) {
		printf("LM_ReadPhy: did not complete\n");
	}

	*pData32 = REG_RD(pDevice, mdiodata) & MD_DATA_MASK;
} /* LM_ReadPhy */



/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
LM_VOID
b44_LM_WritePhy(
PLM_DEVICE_BLOCK pDevice, LM_UINT32 PhyReg, LM_UINT32 Data32)
{
	/* clear mii_int */
	REG_WR(pDevice, emacintstatus, EI_MII);
	ASSERT((REG_RD(pDevice, emacintstatus) & EI_MII) == 0);

	/* issue the write */
	REG_WR(pDevice, mdiodata, (MD_SB_START | MD_OP_WRITE |
		(pDevice->PhyAddr << MD_PMD_SHIFT)
		| (PhyReg << MD_RA_SHIFT) | MD_TA_VALID | Data32));

	/* wait for it to complete */
	SPINWAIT(((REG_RD(pDevice, emacintstatus) & EI_MII) == 0), 100);
	if ((REG_RD(pDevice, emacintstatus) & EI_MII) == 0) {
		printf("b44_LM_WritePhy: did not complete\n");
	}
} /* LM_WritePhy */


LM_STATUS
b44_LM_StatsUpdate(LM_DEVICE_BLOCK *pDevice)
{
	pDevice->tx_good_octets += REG_RD(pDevice, mib.tx_good_octets);
	pDevice->tx_good_pkts += REG_RD(pDevice, mib.tx_good_pkts);
	pDevice->tx_octets += REG_RD(pDevice, mib.tx_octets);
	pDevice->tx_pkts += REG_RD(pDevice, mib.tx_pkts);
	pDevice->tx_broadcast_pkts += REG_RD(pDevice, mib.tx_broadcast_pkts);
	pDevice->tx_multicast_pkts += REG_RD(pDevice, mib.tx_multicast_pkts);
	pDevice->tx_len_64 += REG_RD(pDevice, mib.tx_len_64);
	pDevice->tx_len_65_to_127 += REG_RD(pDevice, mib.tx_len_65_to_127);
	pDevice->tx_len_128_to_255 += REG_RD(pDevice, mib.tx_len_128_to_255);
	pDevice->tx_len_256_to_511 += REG_RD(pDevice, mib.tx_len_256_to_511);
	pDevice->tx_len_512_to_1023 += REG_RD(pDevice, mib.tx_len_512_to_1023);
	pDevice->tx_len_1024_to_max += REG_RD(pDevice, mib.tx_len_1024_to_max);
	pDevice->tx_jabber_pkts += REG_RD(pDevice, mib.tx_jabber_pkts);
	pDevice->tx_oversize_pkts += REG_RD(pDevice, mib.tx_oversize_pkts);
	pDevice->tx_fragment_pkts += REG_RD(pDevice, mib.tx_fragment_pkts);
	pDevice->tx_underruns += REG_RD(pDevice, mib.tx_underruns);
	pDevice->tx_total_cols += REG_RD(pDevice, mib.tx_total_cols);
	pDevice->tx_single_cols += REG_RD(pDevice, mib.tx_single_cols);
	pDevice->tx_multiple_cols += REG_RD(pDevice, mib.tx_multiple_cols);
	pDevice->tx_excessive_cols += REG_RD(pDevice, mib.tx_excessive_cols);
	pDevice->tx_late_cols += REG_RD(pDevice, mib.tx_late_cols);
	pDevice->tx_defered += REG_RD(pDevice, mib.tx_defered);
/*	pDevice->tx_carrier_lost += REG_RD(pDevice, mib.tx_carrier_lost);*/
	/* carrier counter is sometimes bogus, so disable it for now */
	REG_RD(pDevice, mib.tx_carrier_lost);
	pDevice->tx_pause_pkts += REG_RD(pDevice, mib.tx_pause_pkts);

	pDevice->rx_good_octets += REG_RD(pDevice, mib.rx_good_octets);
	pDevice->rx_good_pkts += REG_RD(pDevice, mib.rx_good_pkts);
	pDevice->rx_octets += REG_RD(pDevice, mib.rx_octets);
	pDevice->rx_pkts += REG_RD(pDevice, mib.rx_pkts);
	pDevice->rx_broadcast_pkts += REG_RD(pDevice, mib.rx_broadcast_pkts);
	pDevice->rx_multicast_pkts += REG_RD(pDevice, mib.rx_multicast_pkts);
	pDevice->rx_len_64 += REG_RD(pDevice, mib.rx_len_64);
	pDevice->rx_len_65_to_127 += REG_RD(pDevice, mib.rx_len_65_to_127);
	pDevice->rx_len_128_to_255 += REG_RD(pDevice, mib.rx_len_128_to_255);
	pDevice->rx_len_256_to_511 += REG_RD(pDevice, mib.rx_len_256_to_511);
	pDevice->rx_len_512_to_1023 += REG_RD(pDevice, mib.rx_len_512_to_1023);
	pDevice->rx_len_1024_to_max += REG_RD(pDevice, mib.rx_len_1024_to_max);
	pDevice->rx_jabber_pkts += REG_RD(pDevice, mib.rx_jabber_pkts);
	pDevice->rx_oversize_pkts += REG_RD(pDevice, mib.rx_oversize_pkts);
	pDevice->rx_fragment_pkts += REG_RD(pDevice, mib.rx_fragment_pkts);
	pDevice->rx_missed_pkts += REG_RD(pDevice, mib.rx_missed_pkts);
	pDevice->rx_crc_align_errs += REG_RD(pDevice, mib.rx_crc_align_errs);
	pDevice->rx_undersize += REG_RD(pDevice, mib.rx_undersize);
	pDevice->rx_crc_errs += REG_RD(pDevice, mib.rx_crc_errs);
	pDevice->rx_align_errs += REG_RD(pDevice, mib.rx_align_errs);
	pDevice->rx_symbol_errs += REG_RD(pDevice, mib.rx_symbol_errs);
	pDevice->rx_pause_pkts += REG_RD(pDevice, mib.rx_pause_pkts);
	pDevice->rx_nonpause_pkts += REG_RD(pDevice, mib.rx_nonpause_pkts);

	return LM_STATUS_SUCCESS;
}

void
b44_LM_WriteCam(LM_DEVICE_BLOCK *pDevice, LM_UINT8 *ea, LM_UINT32 camindex)
{
	LM_UINT32 w;

	w = ((LM_UINT32)ea[2] << 24) | ((LM_UINT32)ea[3] << 16) |
		((LM_UINT32) ea[4] << 8) | ea[5];
	REG_WR(pDevice, camdatalo, w);
	w = CD_V | ((LM_UINT32)ea[0] << 8) | ea[1];
	REG_WR(pDevice, camdatahi, w);
	REG_WR(pDevice, camcontrol, (((LM_UINT32) camindex << CC_INDEX_SHIFT) |
		CC_WR));

	/* spin until done */
	SPINWAIT((REG_RD(pDevice, camcontrol) & CC_CB), 100);
}

/******************************************************************************/
/* Description:                                                               */
/*                                                                            */
/* Return:                                                                    */
/******************************************************************************/
static LM_UINT32
b44_GetPhyAdFlowCntrlSettings(
    PLM_DEVICE_BLOCK pDevice)
{
    LM_UINT32 Value32;

    Value32 = 0;

    /* Auto negotiation flow control only when autonegotiation is enabled. */
    if(pDevice->DisableAutoNeg == FALSE ||
        pDevice->RequestedLineSpeed == LM_LINE_SPEED_AUTO)
    {
        /* Please refer to Table 28B-3 of the 802.3ab-1999 spec. */
        if((pDevice->FlowControlCap == LM_FLOW_CONTROL_AUTO_PAUSE) ||
            ((pDevice->FlowControlCap & LM_FLOW_CONTROL_RECEIVE_PAUSE) &&
            (pDevice->FlowControlCap & LM_FLOW_CONTROL_TRANSMIT_PAUSE)))
        {
            Value32 |= PHY_AN_AD_PAUSE_CAPABLE;
        }
        else if(pDevice->FlowControlCap & LM_FLOW_CONTROL_TRANSMIT_PAUSE)
        {
            Value32 |= PHY_AN_AD_ASYM_PAUSE;
        }
        else if(pDevice->FlowControlCap & LM_FLOW_CONTROL_RECEIVE_PAUSE)
        {
            Value32 |= PHY_AN_AD_PAUSE_CAPABLE | PHY_AN_AD_ASYM_PAUSE;
        }
    }

    return Value32;
}


LM_STATUS
b44_LM_GetStats(PLM_DEVICE_BLOCK pDevice)
{
    return LM_STATUS_SUCCESS;
}

void
b44_LM_PollLink(LM_DEVICE_BLOCK *pDevice)
{
	LM_UINT32 status, aux;
	LM_UINT32 txcontrol;
	LM_UINT32 LocalAdv, RemoteAdv;

	b44_LM_ReadPhy(pDevice, 1, &status);
	b44_LM_ReadPhy(pDevice, 1, &status);

	b44_LM_ReadPhy(pDevice, 24, &aux);

	/* check for bad mdio read */
	if (status == 0xffff) {
		return;
	}

	/* update current speed and duplex */
	if (aux & AUX_SPEED)
		pDevice->LineSpeed = LM_LINE_SPEED_100MBPS;
	else
		pDevice->LineSpeed = LM_LINE_SPEED_10MBPS;
	if (aux & AUX_DUPLEX)
		pDevice->DuplexMode = LM_DUPLEX_MODE_FULL;
	else
		pDevice->DuplexMode = LM_DUPLEX_MODE_HALF;

	/* monitor link state */
	if ((pDevice->LinkStatus == LM_STATUS_LINK_DOWN) &&
		(status & STAT_LINK)) {

		/* keep emac txcontrol duplex bit consistent with current */
		/* phy duplex */
		txcontrol = REG_RD(pDevice, txcontrol);
		if ((pDevice->DuplexMode == LM_DUPLEX_MODE_FULL) &&
			!(txcontrol & EXC_FD)) {

			REG_OR(pDevice, txcontrol, EXC_FD);
		}
		else if ((pDevice->DuplexMode == LM_DUPLEX_MODE_HALF) &&
			(txcontrol & EXC_FD)) {

			REG_AND(pDevice, txcontrol, ~EXC_FD);
		}
		if (!pDevice->DisableAutoNeg && (pDevice->DuplexMode ==
			LM_DUPLEX_MODE_FULL)) {

			b44_LM_ReadPhy(pDevice, PHY_AN_AD_REG, &LocalAdv);
			b44_LM_ReadPhy(pDevice, PHY_LINK_PARTNER_ABILITY_REG,
				&RemoteAdv);
			b44_LM_SetFlowControl(pDevice, LocalAdv, RemoteAdv);
		}

		pDevice->LinkStatus = LM_STATUS_LINK_ACTIVE;
		b44_MM_IndicateStatus(pDevice, LM_STATUS_LINK_ACTIVE);
	}
	else if ((pDevice->LinkStatus == LM_STATUS_LINK_ACTIVE) &&
		!(status & STAT_LINK)) {

		pDevice->LinkStatus = LM_STATUS_LINK_DOWN;
		b44_MM_IndicateStatus(pDevice, LM_STATUS_LINK_DOWN);
	}


	/* check for remote fault error */
	if (status & STAT_REMFAULT) {
		printf("remote fault\n");
	}

	/* check for jabber error */
	if (status & STAT_JAB) {
		printf("jabber\n");
	}
}

/* reset and re-enable a core */
void
b44_LM_sb_core_reset(LM_DEVICE_BLOCK *pDevice)
{
	volatile LM_UINT32 dummy;

	/*
	 * Must do the disable sequence first to work for arbitrary current core state.
	 */
	b44_LM_sb_core_disable(pDevice);

	/*
	 * Now do the initialization sequence.
	 */

	/* set reset while enabling the clock and forcing them on throughout the core */
	REG_WR(pDevice, sbconfig.sbtmstatelow,
		(SBTML_FGC | SBTML_CLK | SBTML_RESET));

	dummy = REG_RD(pDevice, sbconfig.sbtmstatelow);
	b44_MM_Wait(1);

	/* PR3158 workaround - not fixed in any chip yet */
	if (REG_RD(pDevice, sbconfig.sbtmstatehigh) & SBTMH_SERR) {
		printf("SBTMH_SERR; clearing...\n");
		REG_WR(pDevice, sbconfig.sbtmstatehigh, 0);
		ASSERT(0);
	}
	if ((dummy = REG_RD(pDevice, sbconfig.sbimstate)) &
		(SBIM_IBE | SBIM_TO)) {

		REG_AND(pDevice, sbconfig.sbimstate, ~(SBIM_IBE | SBIM_TO));
		ASSERT(0);
	}

	/* clear reset and allow it to propagate throughout the core */
	REG_WR(pDevice, sbconfig.sbtmstatelow, (SBTML_FGC | SBTML_CLK));
	dummy = REG_RD(pDevice, sbconfig.sbtmstatelow);
	b44_MM_Wait(1);

	/* leave clock enabled */
	REG_WR(pDevice, sbconfig.sbtmstatelow, SBTML_CLK);
	dummy = REG_RD(pDevice, sbconfig.sbtmstatelow);
	b44_MM_Wait(1);
}

void
b44_LM_sb_core_disable(LM_DEVICE_BLOCK *pDevice)
{
	volatile LM_UINT32 dummy;

	/* must return if core is already in reset */
	if (REG_RD(pDevice, sbconfig.sbtmstatelow) & SBTML_RESET)
		return;
	
	/* set the reject bit */
	REG_WR(pDevice, sbconfig.sbtmstatelow, (SBTML_CLK | SBTML_REJ));

	/* spin until reject is set */
	while ((REG_RD(pDevice, sbconfig.sbtmstatelow) & SBTML_REJ) == 0)
		b44_MM_Wait(1);

	/* spin until sbtmstatehigh.busy is clear */
	while (REG_RD(pDevice, sbconfig.sbtmstatehigh) & SBTMH_BUSY)
		b44_MM_Wait(1);

	/* set reset and reject while enabling the clocks */
	REG_WR(pDevice, sbconfig.sbtmstatelow,
		(SBTML_FGC | SBTML_CLK | SBTML_REJ | SBTML_RESET));

	dummy = REG_RD(pDevice, sbconfig.sbtmstatelow);
	b44_MM_Wait(10);

	/* leave reset and reject asserted */
	REG_WR(pDevice, sbconfig.sbtmstatelow, (SBTML_REJ | SBTML_RESET));
	b44_MM_Wait(1);
}

/*
 * Configure the pci core for pci client (NIC) action
 * and return the pci core revision.
 */
LM_UINT32
b44_LM_sb_pci_setup(LM_DEVICE_BLOCK *pDevice, LM_UINT32 cores)
{
	LM_UINT32 bar0window;
	sbpciregs_t *pciregs;
	LM_UINT32 pcirev;

	pciregs = (sbpciregs_t *) pDevice->pMemView;

	/* save bar0window */
	b44_MM_ReadConfig32(pDevice, PCI_BAR0_WIN, &bar0window);
	/* point bar0 at pci core registers */
	b44_MM_WriteConfig32(pDevice, PCI_BAR0_WIN, b44_LM_getsbaddr(pDevice,
		SBID_REG_PCI, 0));

	ASSERT(b44_LM_sb_coreid(pDevice) == SB_PCI);

	pcirev = b44_LM_sb_corerev(pDevice);

	/* enable sb->pci interrupts */
	REG_OR(pDevice, sbconfig.sbintvec, cores);

	/* enable prefetch and bursts for sonics-to-pci translation 2 */
	REG_WR_OFFSET(pDevice, OFFSETOF(sbpciregs_t, sbtopci2),
		REG_RD_OFFSET(pDevice, OFFSETOF(sbpciregs_t, sbtopci2)) |
		(SBTOPCI_PREF|SBTOPCI_BURST));

	/* restore bar0window */
	b44_MM_WriteConfig32(pDevice, PCI_BAR0_WIN, bar0window);

	return (pcirev);
}

/*
 * Return the SB address corresponding to core <id> instance <coreunit>.
 * Provide a layer of indirection between SB address map elements
 * and the individual chip maps.
 */
LM_UINT32
b44_LM_getsbaddr(LM_DEVICE_BLOCK *pDevice, LM_UINT32 id, LM_UINT32 coreunit)
{
	struct sbmap *sbmap;
	int i;

	sbmap = pDevice->sbmap;
	ASSERT(sbmap);

	for (i = 0; i < SBID_MAX; i++)
		if ((id == sbmap[i].id) && (coreunit == sbmap[i].coreunit))
			return (sbmap[i].sbaddr);

	ASSERT(0);
	return (0xdeadbeef);
}

LM_UINT32
b44_LM_sb_base(LM_UINT32 admatch)
{
	LM_UINT32 base;
	LM_UINT32 type;

	type = admatch & SBAM_TYPE_MASK;
	ASSERT(type < 3);

	base = 0;

	if (type == 0) {
		base = admatch & SBAM_BASE0_MASK;
	} else if (type == 1) {
		ASSERT(admatch & SBAM_ADEN);
		ASSERT(!(admatch & SBAM_ADNEG));	/* neg not supported */
		base = admatch & SBAM_BASE1_MASK;
	} else if (type == 2) {
		ASSERT(admatch & SBAM_ADEN);
		ASSERT(!(admatch & SBAM_ADNEG));	/* neg not supported */
		base = admatch & SBAM_BASE2_MASK;
	}

	return (base);
}

LM_UINT32
b44_LM_sb_size(LM_UINT32 admatch)
{
	LM_UINT32 size;
	LM_UINT32 type;

	type = admatch & SBAM_TYPE_MASK;
	ASSERT(type < 3);

	size = 0;

	if (type == 0) {
		size = 1 << (((admatch & SBAM_ADINT0_MASK) >> SBAM_ADINT0_SHIFT) + 1);
	} else if (type == 1) {
		ASSERT(admatch & SBAM_ADEN);
		ASSERT(!(admatch & SBAM_ADNEG));	/* neg not supported */
		size = 1 << (((admatch & SBAM_ADINT1_MASK) >> SBAM_ADINT1_SHIFT) + 1);
	} else if (type == 2) {
		ASSERT(admatch & SBAM_ADEN);
		ASSERT(!(admatch & SBAM_ADNEG));	/* neg not supported */
		size = 1 << (((admatch & SBAM_ADINT2_MASK) >> SBAM_ADINT2_SHIFT) + 1);
	}

	return (size);
}

/* return the core instance number of this core */
LM_UINT32
b44_LM_sb_coreunit(LM_DEVICE_BLOCK *pDevice)
{
	struct sbmap *sbmap;
	LM_UINT32 base;
	int i;

	sbmap = pDevice->sbmap;
	ASSERT(sbmap);

	base = b44_LM_sb_base(REG_RD(pDevice, sbconfig.sbadmatch0));

	for (i = 0; i < SBID_MAX; i++)
		if (base == sbmap[i].sbaddr)
			return (sbmap[i].coreunit);

	ASSERT(0);
	return (0xdeadbeef);
}


LM_UINT32
b44_LM_sb_clock(LM_DEVICE_BLOCK *pDevice, LM_UINT32 extifva)
{
	ASSERT(0);	/* XXX TBD */
	return (0);
}

LM_UINT32
b44_LM_sb_coreid(LM_DEVICE_BLOCK *pDevice)
{
	return ((REG_RD(pDevice, sbconfig.sbidhigh) &
		SBIDH_CC_MASK) >> SBIDH_CC_SHIFT);
}

LM_UINT32
b44_LM_sb_corerev(LM_DEVICE_BLOCK *pDevice)
{
	return (REG_RD(pDevice, sbconfig.sbidhigh) & SBIDH_RC_MASK);
}

LM_UINT32
b44_LM_sb_iscoreup(LM_DEVICE_BLOCK *pDevice)
{
	return ((REG_RD(pDevice, sbconfig.sbtmstatelow) &
		(SBTML_RESET | SBTML_REJ | SBTML_CLK)) == SBTML_CLK);
}

LM_VOID
b44_LM_PowerDownPhy(LM_DEVICE_BLOCK *pDevice)
{
	REG_WR(pDevice, emaccontrol, EMC_EP);
}

#ifdef BCM_WOL

/* Program patterns on the chip */
static void
b44_LM_pmprog(LM_DEVICE_BLOCK *pDevice)
{
	LM_UINT32 wfl;
	int plen0, plen1, max, i, j;
	LM_UINT8 wol_pattern[BCMENET_PMPSIZE];
	LM_UINT8 wol_mask[BCMENET_PMMSIZE];

	/* program the chip with wakeup patterns, masks, and lengths */

	if (pDevice->WakeUpMode == LM_WAKE_UP_MODE_NONE) {
   		wfl = DISABLE_3210_PATMATCH;  
		REG_WR(pDevice, wakeuplength, wfl);
	}
	else if (pDevice->WakeUpMode == LM_WAKE_UP_MODE_MAGIC_PACKET) {
		/* allow multicast magic packet */
		REG_OR(pDevice, rxconfig, ERC_AM);

		if (pDevice->corerev >= 7) {
			LM_UINT32 addr;

			REG_WR(pDevice, wakeuplength, DISABLE_3210_PATMATCH);

			addr = (pDevice->NodeAddress[2] << 24) |
				(pDevice->NodeAddress[3] << 16) |
				(pDevice->NodeAddress[4] << 8) |
				pDevice->NodeAddress[5];
			REG_WR(pDevice, enetaddrlo, addr);

			addr = (pDevice->NodeAddress[0] << 8) |
			       pDevice->NodeAddress[1];
			REG_WR(pDevice, enetaddrhi, addr);

			REG_OR(pDevice, devcontrol, DC_MPM | DC_PM);
			return;
		}
		/* older chip */
		/* UDP magic packet pattern */
		memset(wol_pattern, 0, BCMENET_PMPSIZE);
		memset(wol_pattern + 42, 0xff, 6);	/* sync pattern */
		max = ETHERNET_ADDRESS_SIZE;
		for (i = 0; i < 14; ++i) {
			if (i == 13)
				max = 2;
			for (j = 0; j < max; ++j) {
				wol_pattern[42 + 6 +
				(i * ETHERNET_ADDRESS_SIZE) + j] =
				pDevice->NodeAddress[j];
			}
		}
		memset(wol_mask, 0, BCMENET_PMMSIZE);
		wol_mask[5] = 0xfc;
		memset(wol_mask + 6, 0xff, 10);
		plen0 = BCMENET_PMPSIZE - 1;

   		b44_LM_ftwrite(pDevice, (LM_UINT32 *)wol_pattern,
			BCMENET_PMPSIZE, BCMENET_PMPBASE);

   		b44_LM_ftwrite(pDevice, (LM_UINT32 *)wol_mask, BCMENET_PMMSIZE,
    			BCMENET_PMMBASE);

		/* raw ethernet II magic packet pattern */
		memset(wol_pattern, 0, BCMENET_PMPSIZE);
		memset(wol_pattern + 14, 0xff, 6);	/* sync pattern */
		max = ETHERNET_ADDRESS_SIZE;
		for (i = 0; i < 16; ++i) {
			for (j = 0; j < max; ++j) {
				wol_pattern[14 + 6 +
				(i * ETHERNET_ADDRESS_SIZE) + j] =
				pDevice->NodeAddress[j];
			}
		}
		memset(wol_mask, 0, BCMENET_PMMSIZE);
		wol_mask[2] = 0xf0;
		memset(wol_mask + 3, 0xff, 11);
		wol_mask[14] = 0xf;
		plen1 = 14 + 6 + 96 - 1;

   		b44_LM_ftwrite(pDevice, (LM_UINT32 *)wol_pattern,
			BCMENET_PMPSIZE, BCMENET_PMPBASE + BCMENET_PMPSIZE);

   		b44_LM_ftwrite(pDevice, (LM_UINT32 *)wol_mask, BCMENET_PMMSIZE,
    			BCMENET_PMMBASE + BCMENET_PMMSIZE);

   		/* set this pattern's length: one less than the real length */
   		wfl = plen0 | (plen1 << 8) | DISABLE_32_PATMATCH;

		REG_WR(pDevice, wakeuplength, wfl);

		/* enable chip wakeup pattern matching */
		REG_OR(pDevice, devcontrol, DC_PM);
	}

}

LM_VOID
b44_LM_pmset(LM_DEVICE_BLOCK *pDevice)
{
	LM_UINT16 Value16;

	b44_LM_Halt(pDevice);

	/* now turn on just enough of the chip to receive and match patterns */
	b44_LM_ResetAdapter(pDevice, FALSE);

	/* program patterns */
	b44_LM_pmprog(pDevice);

	/* enable sonics bus PME */
	REG_OR(pDevice, sbconfig.sbtmstatelow, SBTML_PE);

	b44_MM_ReadConfig16(pDevice, BCMENET_PMCSR, &Value16);
	b44_MM_WriteConfig16(pDevice, BCMENET_PMCSR,
		Value16 | ENABLE_PCICONFIG_PME);
}


static void
b44_LM_ftwrite(LM_DEVICE_BLOCK *pDevice, LM_UINT32 *b, LM_UINT32 nbytes,
	LM_UINT32 ftaddr)
{
	LM_UINT32 i;

	for (i = 0; i < nbytes; i += sizeof(LM_UINT32)) {
		REG_WR(pDevice, enetftaddr, ftaddr + i);
		REG_WR(pDevice, enetftdata, b[i / sizeof(LM_UINT32)]);
	}
}

#endif /* BCM_WOL */
