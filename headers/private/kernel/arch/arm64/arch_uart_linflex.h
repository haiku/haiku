/*
 * Copyright 2022 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Ruiz Dorantes, oliver.ruiz.dorantes@gmail.com
 */
#ifndef __DEV_UART_LINFLEX_H
#define __DEV_UART_LINFLEX_H


#include <sys/types.h>
#include <ByteOrder.h>
#include <SupportDefs.h>

#include <arch/generic/debug_uart.h>


#define UART_KIND_LINFLEX "linflex"

namespace LINFlexRegisters {
	typedef union { /* LINFLEX LIN Control 1 (Base+0x0000) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 CCD:1;
			vuint32 CFD:1;
			vuint32 LASE:1;
			vuint32 AWUM:1;
			vuint32 MBL:4;
			vuint32 BF:1;
			vuint32 SFTM:1;
			vuint32 LBKM:1;
			vuint32 MME:1;
			vuint32 SBDT:1;
			vuint32 RBLM:1;
			vuint32 SLEEP:1;
			vuint32 INIT:1;
	#endif
	#if __BYTE_ORDER == __LITTLE_ENDIAN
			vuint32 INIT:1;
			vuint32 SLEEP:1;
			vuint32 RBLM:1;
			vuint32 SBDT:1;
			vuint32 MME:1;
			vuint32 LBKM:1;
			vuint32 SFTM:1;
			vuint32 BF:1;
			vuint32 MBL:4;
			vuint32 AWUM:1;
			vuint32 LASE:1;
			vuint32 CFD:1;
			vuint32 CCD:1;
			vuint32 padding:16;
	#endif
		} B;
	} LINCR1_register;


	typedef union { /* LINFLEX LIN Interrupt Enable (Base+0x0004) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 SZIE:1;
			vuint32 OCIE:1;
			vuint32 BEIE:1;
			vuint32 CEIE:1;
			vuint32 HEIE:1;
			vuint32 padding1:2;
			vuint32 FEIE:1;
			vuint32 BOIE:1;
			vuint32 LSIE:1;
			vuint32 WUIE:1;
			vuint32 DBFIE:1;
			vuint32 DBEIE:1;
			vuint32 DRIE:1;
			vuint32 DTIE:1;
			vuint32 HRIE:1;
	#endif
		} B;
	} LINIER_register;


	typedef	union { /* LINFLEX LIN Status (Base+0x0008) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 LINS:4;
			vuint32 padding1:2;
			vuint32 RMB:1;
			vuint32 padding2:1;
			vuint32 RBSY:1;
			vuint32 RPS:1;
			vuint32 WUF:1;
			vuint32 DBFF:1;
			vuint32 DBEF:1;
			vuint32 DRF:1;
			vuint32 DTF:1;
			vuint32 HRF:1;
	#endif
		} B;
	} LINSR_register;


	typedef union { /* LINFLEX LIN Error Status (Base+0x000C) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 SZF:1;
			vuint32 OCF:1;
			vuint32 BEF:1;
			vuint32 CEF:1;
			vuint32 SFEF:1;
			vuint32 BDEF:1;
			vuint32 IDPEF:1;
			vuint32 FEF:1;
			vuint32 BOF:1;
			vuint32 padding1:6;
			vuint32 NF:1;
	#endif
		} B;
	} LINESR_register;


	typedef union { /* LINFLEX UART Mode Control (Base+0x0010) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 padding1:1;
			vuint32 TDFL:2;
			vuint32 padding2:1;
			vuint32 RDFL:2;
			vuint32 RFBM:1;
			vuint32 TFBM:1;
			vuint32 WL1:1;
			vuint32 PC1:1;
			vuint32 RXEN:1;
			vuint32 TXEN:1;
			vuint32 PC0:1;
			vuint32 PCE:1;
			vuint32 WL:1;
			vuint32 UART:1;
	#endif
	#if __BYTE_ORDER == __LITTLE_ENDIAN
			vuint32 UART:1;
			vuint32 WL:1;
			vuint32 PCE:1;
			vuint32 PC0:1;
			vuint32 TXEN:1;
			vuint32 RXEN:1;
			vuint32 PC1:1;
			vuint32 WL1:1;
			vuint32 TFBM:1;
			vuint32 RFBM:1;
			vuint32 RDFL:2;
			vuint32 padding2:1;
			vuint32 TDFL:2;
			vuint32 padding1:1;
			vuint32 padding:16;
	#endif
		} B;
	} UARTCR_register;


	typedef union { /* LINFLEX UART Mode Status (Base+0x0014) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 SZF:1;
			vuint32 OCF:1;
			vuint32 PE:4; /*Can check all 4 RX'd bytes at once with array*/
			vuint32 RMB:1;
			vuint32 FEF:1;
			vuint32 BOF:1;
			vuint32 RPS:1;
			vuint32 WUF:1;
			vuint32 padding1:2;
			vuint32 DRF:1;
			vuint32 DTF:1;
			vuint32 NF:1;
	#endif
	#if __BYTE_ORDER == __LITTLE_ENDIAN
			vuint32 NF:1;
			vuint32 DTF:1;
			vuint32 DRF:1;
			vuint32 padding1:2;
			vuint32 WUF:1;
			vuint32 RPS:1;
			vuint32 BOF:1;
			vuint32 FEF:1;
			vuint32 RMB:1;
			vuint32 PE:4; /*Can check all 4 RX'd bytes at once with array*/
			vuint32 OCF:1;
			vuint32 SZF:1;
			vuint32 padding:16;
	#endif
		} B;
	} UARTSR_register;


	typedef union { /* LINFLEX TimeOut Control Status ((Base+0x0018)*/
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 padding1:5;
			vuint32 LTOM:1;
			vuint32 IOT:1;
			vuint32 TOCE:1;
			vuint32 CNT:8;
	#endif
		} B;
	} LINTCSR_register;


	typedef union { /* LINFLEX LIN Output Compare (Base+0x001C) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 OC2:8;
			vuint32 OC1:8;
	#endif
		} B;
	} LINOCR_register;


	typedef union { /* LINFLEX LIN Timeout Control (Base+0x0020) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:20;
			vuint32 RTO:4;
			vuint32 padding:1;
			vuint32 HTO:7;
	#endif
		} B;
	} LINTOCR_register;


	typedef union { /* LINFLEX LIN Fractional Baud Rate (+0x0024) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:28;
			vuint32 DIV_F:4;
	#endif
		} B;
	} LINFBRR_register;


	typedef union { /* LINFLEX LIN Integer Baud Rate (Base+0x0028) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:19;
			vuint32 DIV_M:13;
	#endif
		} B;
	} LINIBRR_register;


	typedef union { /* LINFLEX LIN Checksum Field (Base+0x002C) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:24;
			vuint32 CF:8;
	#endif
		} B;
	} LINCFR_register;


	typedef union { /* LINFLEX LIN Control 2 (Base+0x0030) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:17;
			vuint32 IOBE:1;
			vuint32 IOPE:1;
			vuint32 WURQ:1;
			vuint32 DDRQ:1;
			vuint32 DTRQ:1;
			vuint32 ABRQ:1;
			vuint32 HTRQ:1;
			vuint32:8;
	#endif
		} B;
	} LINCR2_register;


	typedef union { /* LINFLEX Buffer Identifier (Base+0x0034) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 DFL:6;
			vuint32 DIR:1;
			vuint32 CCS:1;
			vuint32 padding1:2;
			vuint32 ID:6;
	#endif
		} B;
	} BIDR_register;


	typedef union { /* LINFLEX Buffer Data LSB (Base+0x0038) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 DATA3:8;
			vuint32 DATA2:8;
			vuint32 DATA1:8;
			vuint32 DATA0:8;
	#endif
		} B;
	} BDRL_register;


	typedef union { /* LINFLEX Buffer Data MSB (Base+0x003C */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 DATA7:8;
			vuint32 DATA6:8;
			vuint32 DATA5:8;
			vuint32 DATA4:8;
	#endif
		} B;
	} BDRM_register;


	typedef union { /* LINFLEX Identifier Filter Enable (+0x0040) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:24;
			vuint32 FACT:8;
	#endif
		} B;
	} IFER_register;


	typedef union { /* LINFLEX Identifier Filter Match Index (+0x0044)*/
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:28;
			vuint32 IFMI:4;
	#endif
		} B;
	} IFMI_register;


	typedef union { /* LINFLEX Identifier Filter Mode (Base+0x0048) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:28;
			vuint32 IFM:4;
	#endif
		} B;
	} IFMR_register;


	typedef union { /* LINFLEX Identifier Filter Control 0..15 (+0x004C-0x0088)*/
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 padding1:3;
			vuint32 DFL:3;
			vuint32 DIR:1;
			vuint32 CCS:1;
			vuint32 padding2:2;
			vuint32 ID:6;
	#endif
		} B;
	} IFCR_register;


	typedef union { /* LINFLEX Global Counter (+0x008C) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:26;
			vuint32 TDFBM:1;
			vuint32 RDFBM:1;
			vuint32 TDLIS:1;
			vuint32 RDLIS:1;
			vuint32 STOP:1;
			vuint32 SR:1;
	#endif
		} B;
	} GCR_register;


	typedef union { /* LINFLEX UART preset timeout (+0x0090) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:20;
			vuint32 PTO:12;
	#endif
		} B;
	} UARTPTO_register;


	typedef union { /* LINFLEX UART current timeout (+0x0094) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:20;
			vuint32 CTO:12;
	#endif
		} B;
	} UARTCTO_register;


	typedef union { /* LINFLEX DMA Tx Enable (+0x0098) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 DTE15:1;
			vuint32 DTE14:1;
			vuint32 DTE13:1;
			vuint32 DTE12:1;
			vuint32 DTE11:1;
			vuint32 DTE10:1;
			vuint32 DTE9:1;
			vuint32 DTE8:1;
			vuint32 DTE7:1;
			vuint32 DTE6:1;
			vuint32 DTE5:1;
			vuint32 DTE4:1;
			vuint32 DTE3:1;
			vuint32 DTE2:1;
			vuint32 DTE1:1;
			vuint32 DTE0:1;
	#endif
		} B;
	} DMATXE_register;


	typedef union { /* LINFLEX DMA RX Enable (+0x009C) */
		vuint32 R;
		struct {
	#if __BYTE_ORDER == __BIG_ENDIAN
			vuint32 padding:16;
			vuint32 DRE15:1;
			vuint32 DRE14:1;
			vuint32 DRE13:1;
			vuint32 DRE12:1;
			vuint32 DRE11:1;
			vuint32 DRE10:1;
			vuint32 DRE9:1;
			vuint32 DRE8:1;
			vuint32 DRE7:1;
			vuint32 DRE6:1;
			vuint32 DRE5:1;
			vuint32 DRE4:1;
			vuint32 DRE3:1;
			vuint32 DRE2:1;
			vuint32 DRE1:1;
			vuint32 DRE0:1;
	#endif
		} B;
	} DMARXE_register;

	// Helper to deal with w1c (Write 1 to clear) register fields
	template<typename REG>
	REG BitfieldRegister() {
		REG reg;
		reg.R = 0;
		return reg;
	}

	typedef struct {
		LINCR1_register LINCR1;     /* LINFLEX LIN Control 1 (Base+0x0000) */
		LINIER_register LINIER;     /* LINFLEX LIN Interrupt Enable (Base+0x0004) */
		LINSR_register LINSR;       /* LINFLEX LIN Status (Base+0x0008) */
		LINESR_register LINESR;     /* LINFLEX LIN Error Status (Base+0x000C) */
		UARTCR_register UARTCR;     /* LINFLEX UART Mode Control (Base+0x0010) */
		UARTSR_register UARTSR;     /* LINFLEX UART Mode Status (Base+0x0014) */
		LINTCSR_register LINTCSR;   /* LINFLEX TimeOut Control Status ((Base+0x0018)*/
		LINOCR_register LINOCR;     /* LINFLEX LIN Output Compare (Base+0x001C) */
		LINTOCR_register LINTOCR;   /* LINFLEX LIN Timeout Control (Base+0x0020) */
		LINFBRR_register LINFBRR;   /* LINFLEX LIN Fractional Baud Rate (+0x0024) */
		LINIBRR_register LINIBRR;   /* LINFLEX LIN Integer Baud Rate (Base+0x0028) */
		LINCFR_register LINCFR;     /* LINFLEX LIN Checksum Field (Base+0x002C) */
		LINCR2_register LINCR2;     /* LINFLEX LIN Control 2 (Base+0x0030) */
		BIDR_register BIDR;        /* LINFLEX Buffer Identifier (Base+0x0034) */
		BDRL_register BDRL;        /* LINFLEX Buffer Data LSB (Base+0x0038) */
		BDRM_register BDRM;        /* LINFLEX Buffer Data MSB (Base+0x003C */
			IFER_register IFER;    /* LINFLEX Identifier Filter Enable (+0x0040) */
			IFMI_register IFMI;    /* LINFLEX Identifier Filter Match Index (+0x0044)*/
			IFMR_register IFMR;    /* LINFLEX Identifier Filter Mode (Base+0x0048) */
			IFCR_register IFCR[16]; /* LINFLEX Identifier Filter Control 0..15 (+0x004C-0x0088)*/
		GCR_register GCR;          /* LINFLEX Global Counter (+0x008C) */
		UARTPTO_register UARTPTO;  /* LINFLEX UART preset timeout (+0x0090) */
		UARTCTO_register UARTCTO;  /* LINFLEX UART current timeout (+0x0094) */
		DMATXE_register DMATXE;    /* LINFLEX DMA Tx Enable (+0x0098) */
		DMARXE_register DMARXE;    /* LINFLEX DMA RX Enable (+0x009C) */
	} LINFlex;
}


class ArchUARTlinflex : public DebugUART {

public:
							ArchUARTlinflex(addr_t base, int64 clock);
							~ArchUARTlinflex();

			void			InitEarly();
			void			InitPort(uint32 baud);

			void			Enable();
			void			Disable();

			int				PutChar(char c);
			int				GetChar(bool wait);

			void			FlushTx();
			void			FlushRx();

private:
			template<typename TO, typename TA>
			void			Out(TA* reg, TO value) {
				*(volatile TO*)(reg) = value;
			}

			template<typename TI, typename TA>
			TI				In(TA* reg) {
				return *(volatile TI*)(reg);
			}

	virtual	void							Barrier();
			LINFlexRegisters::LINFlex* 		LinflexCell() {
				return reinterpret_cast<LINFlexRegisters::LINFlex*>(Base());
			}
};


ArchUARTlinflex *arch_get_uart_linflex(addr_t base, int64 clock);


#endif
