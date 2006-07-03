/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef _SYM53C8XX_H
#define _SYM53C8XX_H

/*======================================================================*/
/* operating registers definitions */

#define sym_scntl0	0x00	/* scsi control 0 */
#define sym_scntl1	0x01	/* scsi control 1 */
#define sym_scntl2	0x02	/* scsi control 2 */
#define sym_scntl3	0x03	/* scsi control 3 */
#define sym_scid	0x04	/* scsi chip ID */
#define sym_sxfer	0x05	/* scsi transfer */
#define sym_sdid	0x06	/* scsi destination ID */
#define sym_gpreg	0x07	/* general purpose bits */
#define sym_sfbr	0x08	/* scsi first byte received */
#define sym_socl	0x09	/* scsi output control latch */
#define sym_ssid	0x0a	/* scsi selector ID */
#define sym_sbcl	0x0b	/* scsi bus control lines */
#define sym_dstat	0x0c	/* dma status */
#define sym_sstat0	0x0d	/* scsi status 0 */
#define sym_sstat1	0x0e	/* scsi status 1 */
#define sym_sstat2	0x0f	/* scsi status 2 */
#define	sym_dsa		0x10	/* data structure address [4 bytes] */
#define	sym_istat	0x14	/* interrupt status */
#define sym_ctest0	0x18	/* chip test 0 */
#define sym_ctest1	0x19	/* chip test 1 */
#define sym_ctest2	0x1a	/* chip test 2 */
#define sym_ctest3	0x1b	/* chip test 3 */
#define sym_temp	0x1c	/* temporary stack [4 bytes] */
#define sym_dfifo	0x20	/* dma fifo */
#define sym_ctest4	0x21	/* chip test 4 */
#define sym_ctest5	0x22	/* chip test 5 */
#define sym_ctest6	0x23	/* chip test 6 */
#define sym_dbc		0x24	/* dma byte counter [3 bytes] */
#define sym_dcmd	0x27	/* dma command */
#define sym_dnad	0x28	/* dma next address for data [4 bytes] */
#define sym_dsp		0x2c	/* dma scripts pointer [4 bytes] */
#define sym_dsps	0x30	/* dma scripts pointer save [4 bytes] */
#define sym_scratcha 0x34	/* general purpose scratch pad a [4 bytes] */
#define sym_dmode	0x38	/* dma mode */
#define sym_dien	0x39	/* dma interrupt enable */
#define sym_dwt		0x3a	/* dma watchdog timer */
#define sym_dcntl	0x3b	/* dma control */
#define sym_adder	0x3c	/* sum output of internal adder [4 bytes] */
#define sym_sien0	0x40	/* scsi interrupt enable 0 */
#define sym_sien1	0x41	/* scsi interrupt enable 1 */
#define sym_sist0	0x42	/* scsi interrupt status 0 */
#define sym_sist1	0x43	/* scsi interrupt status 1 */
#define sym_slpar	0x44	/* scsi longitudinal parity */
#define sym_macntl	0x46	/* memory access control */
#define sym_gpcntl	0x47	/* general purpose control */
#define sym_stime0	0x48	/* scsi timer 0 */
#define sym_stime1	0x49	/* scsi timer 1 */
#define sym_respid	0x4a	/* responce ID */
#define sym_stest0	0x4c	/* scsi test 0 */
#define sym_stest1	0x4d	/* scsi test 1 */
#define sym_stest2	0x4e	/* scsi test 2 */
#define sym_stest3	0x4f	/* scsi test 3 */
#define sym_sidl	0x50	/* scsi input data latch */
#define sym_stest4  0x52    /* new with 895/896 */
#define sym_sodl	0x54	/* scsi output data latch */
#define sym_sbdl	0x58	/* scsi bus data lines */
#define sym_scratchb 0x5c	/* general purpose scratch pad b [4 bytes] */


/*----------------------------------------------------------------------*/
/* operating registers bit definitions */

/* 0x00 - scntl0 (scsi control 0) bit definitions */

#define sym_scntl0_trg	0x01	/* target mode [1 = target, 0 = initiator */
#define sym_scntl0_aap	0x02	/* assert SATN on parity */
#define sym_scntl0_epc	0x08	/* enable parity checking */
#define sym_scntl0_watn	0x10	/* select with SATN on a start sequence */
#define sym_scntl0_start 0x20	/* start sequence */
#define sym_scntl0_arb0	0x40	/* arbitration mode bit 0 */
#define sym_scntl0_arb1	0x80	/* arbitration mode bit 1 */


/* 0x01 - scntl1 (scsi control 1) bit definitions */

#define sym_scntl1_sst	0x01	/* start scsi transfer */
#define sym_scntl1_iarb	0x02	/* immediate arbitration */
#define sym_scntl1_aesp	0x04	/* assert even scsi parity (force bad parity) */
#define sym_scntl1_rst	0x08	/* assert scsi RST signal */
#define sym_scntl1_con	0x10	/* connected */
#define sym_scntl1_dhp	0x20	/* disable halt on parity error or ATN */
#define sym_scntl1_adb	0x40	/* assert scsi data bus */
#define sym_scntl1_exc	0x80	/* extra clock cycle of data setup */


/* 0x02 - scntl2 (scsi control 2) bit definitions */

#define sym_scntl2_sdu	0x80	/* scsi disconnect unexpected */


/* 0x03 - scntl3 (scsi control 3) bit definitions */

#define sym_scntl3_ccf	0x07	/* clock conversion factor [mask] */
#define sym_scntl3_scf	0x70	/* synchronous clock conversion factor [mask] */


/* 0x04 - scid (scsi chip id) bit definitions */

#define sym_scid_enc	0x07	/* encoded scsi id [mask] */
#define sym_scid_sre	0x20	/* enable responce to selection */
#define sym_scid_rre	0x40	/* enable responce to reselection */


/* 0x05 - sxfer (scsi transfer) bit definitions */

#define sym_sxfer_mo	0x0f	/* max scsi synchronous offset [mask] */
#define sym_sxfer_tp	0xe0	/* scsi synchronous transfer period [mask] */


/* 0x06 - sdid (scsi destination id) bit definitions */

#define sym_sdid_enc	0x07	/* encoded destination scsi id [mask] */


/* 0x07 - gpreg (general purpose) bit definitions */

#define sym_gpreg_gpio0	0x01	/* general purpose */
#define sym_gpreg_gpio1	0x02	/* general purpose */


/* 0x09 - socl (scsi output control latch) bit definitions */

#define sym_socl_io		0x01	/* assert i/o signal */
#define sym_socl_cd		0x02	/* assert c/d signal */
#define sym_socl_msg	0x04	/* assert msg signal */
#define sym_socl_atn	0x08	/* assert atn signal */
#define sym_socl_sel	0x10	/* assert sel signal */
#define sym_socl_bsy	0x20	/* assert bsy signal */
#define sym_socl_ack	0x40	/* assert ack signal */
#define sym_socl_req	0x80	/* assert req signal */


/* 0x0a - ssid (scsi selector id) bit definitions */

#define sym_ssid_encid	0x07	/* encoded destination scsi id [mask] */
#define sym_ssid_val	0x80	/* scsi valid bit */


/* 0x0b - sbcl (scsi bus control lines) bit definitions */

#define sym_sbcl_io		0x01	/* i/o status */
#define sym_sbcl_cd		0x02	/* c/d status */
#define sym_sbcl_msg	0x04	/* msg status */
#define sym_sbcl_atn	0x08	/* atn status */
#define sym_sbcl_sel	0x10	/* sel status */
#define sym_sbcl_bsy	0x20	/* bsy status */
#define sym_sbcl_ack	0x40	/* ack status */
#define sym_sbcl_req	0x80	/* req status */


/* 0x0c - dstat (dma status) bit definitions */

#define sym_dstat_iid	0x01	/* illegal instruction detected */
#define sym_dstat_sir	0x04	/* scripts interrupt instruction received */
#define sym_dstat_ssi	0x08	/* single step interrupt */
#define sym_dstat_abrt	0x10	/* aborted */
#define sym_dstat_bf	0x20	/* bus fault */
#define sym_dstat_mdpe	0x40	/* master data parity error */
#define sym_dstat_dfe	0x80	/* dma fifo empty */


/* 0x0d - sstat0 (scsi status 0) bit definitions */

#define sym_sstat0_sdp	0x01	/* scsi parity signal */
#define sym_sstat0_rst	0x02	/* scsi reset signal */
#define sym_sstat0_woa	0x04	/* won arbitration */
#define sym_sstat0_loa	0x08	/* lost arbitration */
#define sym_sstat0_aip	0x10	/* arbitration in progress */
#define sym_sstat0_olf	0x20	/* scsi output data latch full */
#define sym_sstat0_orf	0x40	/* scsi output data register full */
#define sym_sstat0_ilf	0x80	/* scsi input data latch full */


/* 0x0e - sstat1 (scsi status 1) bit definitions */

#define sym_sstat1_io	0x01	/* scsi i/o signal */
#define sym_sstat1_cd	0x02	/* scsi c/d signal */
#define sym_sstat1_msg	0x04	/* scsi msg signal */
#define sym_sstat1_sdpl	0x08	/* latched scsi parity */
#define sym_sstat1_fifo	0xf0	/* mask for fifo flags [mask] */


/* 0x0f - sstat2 (scsi status 2) bit definitions */

#define sym_sstat2_ldsc	0x02	/* last disconnect */


/* 0x14 - istat (interrupt status) bit definitions */

#define sym_istat_dip	0x01	/* dma interrupt pending */
#define sym_istat_sip	0x02	/* scsi interrupt pending */
#define sym_istat_intf	0x04	/* interrupt on the fly */
#define sym_istat_con	0x08	/* connected */
#define sym_istat_sem	0x10	/* semaphore */
#define sym_istat_sigp	0x20	/* signal process */
#define sym_istat_srst	0x40	/* software reset */
#define sym_istat_abrt	0x80	/* abort operation */


/* 0x19 - ctest1 (chip test 1) bit definitions */

#define sym_ctest1_ffl	0x0f	/* byte empty in dma fifo [mask] */
#define sym_ctest1_fmt	0xf0	/* byte full in dma fifo [mask] */


/* 0x1a - ctest2 (chip test 2) bit definitions */

#define sym_ctest2_dack	0x01	/* data acknowledge status */
#define sym_ctest2_dreq	0x02	/* data request status */
#define sym_ctest2_teop	0x04	/* scsi true end of process */
#define sym_ctest2_cm	0x10	/* configured as memory */
#define sym_ctest2_cio	0x20	/* configured as i/o */
#define sym_ctest2_sigp	0x40	/* signal process */
#define sym_ctest2_ddir	0x80	/* data transfer direction */


/* 0x1b - ctest3 (chip test 3) bit definitions */

#define sym_ctest3_fm	0x02	/* fetch pin mode */
#define sym_ctest3_clf	0x04	/* clear dma fifo */
#define sym_ctest3_flf	0x08	/* flush dma fifo */
#define sym_ctest3_v	0xf0	/* chip revision level [mask] */


/* 0x20 - dfifo (dma fifo) bit definitions */

#define sym_dfifo_bo	0x7f	/* byte offset counter */


/* 0x21 - ctest4 (chip test 4) bit definitions */

#define sym_ctest4_fbl	0x07	/* fifo byte control [mask] */
#define sym_ctest4_mpee	0x08	/* master parity error enable */
#define sym_ctest4_srtm	0x10	/* shadow register test mode */
#define sym_ctest4_zsd	0x20	/* scsi data high impedance */
#define sym_ctest4_zmod	0x40	/* high impedance mode */
#define sym_ctest4_bdis	0x80	/* burst disable */

/* 0x22 - ctest5 (chip test 5) bit definitions */

#define sym_ctest5_ddir	0x08	/* dma direction */
#define sym_ctest5_masr	0x10	/* master control for set or reset pulses */
#define sym_ctest5_bbck	0x40	/* clock byte counter */
#define sym_ctest5_adck	0x80	/* clock address incrementor */


/* 0x38 - dmode (dma mode) bit definitions */

#define sym_dmode_man	0x01	/* manual start mode */
#define sym_dmode_erl	0x08	/* enable read line */
#define sym_dmode_diom	0x10	/* destination i/o memory enable */
#define sym_dmode_siom	0x20	/* source i/o memory enable */
#define sym_dmode_bl	0xc0	/* burst length */


/* 0x39 - dien (dma interrupt enable) bit definitions */

#define sym_dien_iid	0x01	/* illegal instruction detected */
#define sym_dien_sir	0x04	/* scripts interrupt instruction received */
#define sym_dien_ssi	0x08	/* single step instruction */
#define sym_dien_abrt	0x10	/* aborted */
#define sym_dien_bf		0x20	/* bus fault */
#define sym_dien_mdpe	0x40	/* master data parity error */


/* 0x3b - dcntl (dma control) bit definitions */

#define sym_dcntl_com	0x01	/* 53c700 compatibility */
#define sym_dcntl_std	0x04	/* start dma operation */
#define sym_dcntl_irqm	0x08	/* irq mode */
#define sym_dcntl_ssm	0x10	/* single step mode */


/* 0x40 - sien0 (scsi interrupt enable 0) bit definitions */

#define sym_sien0_par	0x01	/* scsi parity error */
#define sym_sien0_rst	0x02	/* scsi reset condition */
#define sym_sien0_udc	0x04	/* unexpected disconnect */
#define sym_sien0_sge	0x08	/* scsi gross error */
#define sym_sien0_rsl	0x10	/* reselected */
#define sym_sien0_sel	0x20	/* selected */
#define sym_sien0_cmp	0x40	/* function complete */
#define sym_sien0_ma	0x80	/* scsi phase mismatch or scsi atn condition */


/* 0x41 - sien1 (scsi interrupt enable 1) bit definitions */

#define sym_sien1_hth	0x01	/* handshake to handshake timer expired */
#define sym_sien1_gen	0x02	/* general purpose timer expired */
#define sym_sien1_sto	0x04	/* selection or reselection timeout */
#define sym_sien1_sbmc  0x10    /* SCSI Bus Mode Change */

/* 0x42 - sist0 (scsi interrupt status 0) bit definitions */

#define sym_sist0_par	0x01	/* parity error */
#define sym_sist0_rst	0x02	/* scsi rst/received */
#define sym_sist0_udc	0x04	/* unexpected disconnect */
#define sym_sist0_sge	0x08	/* scsi gross error */
#define sym_sist0_rsl	0x10	/* reselected */
#define sym_sist0_sel	0x20	/* selected */
#define sym_sist0_cmp	0x40	/* function complete */
#define sym_sist0_ma	0x80	/* scsi phase mismatch or scsi atn condition */


/* 0x43 - sist1 (scsi interrupt status 1) bit definitions */

#define sym_sist1_hth	0x01	/* handshake to handshake timer expired */
#define sym_sist1_gen	0x02	/* general purpose timer expired */
#define sym_sist1_sto	0x04	/* selection or reselection timeout */
#define sym_sist1_sbmc  0x10    /* SCSI Bus Mode Change */

/* 0x46 - macntl (memory access control) bit definitions */

#define sym_macntl_scpts 0x01	/* scripts */
#define sym_macntl_pscpt 0x02	/* pointer scripts */
#define sym_macntl_drd	0x04	/* data read */
#define sym_macntl_dwr	0x08	/* data write */
#define sym_macntl_typ	0xf0	/* chip type [mask] */


/* 0x47 - gpcntl (general purpose pin control) bit definitions */

#define sym_gpcntl_gpio0 0x01	/* gpio 0 */
#define sym_gpcntl_gpio1 0x02	/* gpio 1 */
#define sym_gpcntl_fe	0x40	/* fetch enable */
#define sym_gpcntl_me	0x80	/* master enable */


/* 0x48 - stime0 (scsi timer 0) bit definitions */

#define sym_stime0_sel	0x07	/* selection time-out [mask] */
#define sym_stime0_hth	0xf8	/* handshake to handshake timer period [mask] */


/* 0x49 - stime1 (scsi timer 1) bit definitions */

#define sym_stime1_gen	0x0f	/* general purpose timer period [mask] */


/* 0x4c - stest0 (scsi test 0) bit definitions */

#define sym_stest0_som	0x01	/* scsi synchronous offset maximum */
#define sym_stest0_soz	0x02	/* scsi synchronous offset zero */
#define sym_stest0_art	0x04	/* arbitration priority encoder test */
#define sym_stest0_slt	0x08	/* selection response logic test */


/* 0x4d - stest1 (scsi test 1) bit definitions */

#define sym_stest1_sclk	0x80	/* sclk */


/* 0x4e - stest2 (scsi test 2) bit definitions */

#define sym_stest2_low	0x01	/* scsi low level mode */
#define sym_stest2_ext	0x02	/* extend sreq/sack filtering */
#define sym_stest2_szm	0x08	/* scsi high-impedance mode */
#define sym_stest2_slb	0x10	/* scsi loopback mode */
#define sym_stest2_rof	0x40	/* reset scsi offset */
#define sym_stest2_sce	0x80	/* scsi control enable */


/* 0x4f - stest3 (scsi test 3) bit definitions */

#define sym_stest3_stw	0x01	/* scsi fifo test write */
#define sym_stest3_csf	0x02	/* clear scsi fifo */
#define sym_stest3_ttm	0x04	/* timer test mode */
#define sym_stest3_dsi	0x10	/* disable single initiator response */
#define sym_stest3_hsc	0x20	/* halt scsi clock */
#define sym_stest3_str	0x40	/* scsi fifo test read */
#define sym_stest3_te	0x80	/* tolerant enable */

#define sym_stest4_hvd  0x40    /* high voltage diff */
#define sym_stest4_se   0x80    /* single ended */
#define sym_stest4_lvd  0xc0    /* low voltage diff */
#define sym_stest4_lock 0x20    /* clock quadrupler locker */

#endif
