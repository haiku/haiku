/*
 * Copyright 2012-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Alexander von Gluck IV, kallisti5@unixzen.com
 */
#ifndef _INTEL_POWER_H_
#define _INTEL_POWER_H_


#include <string.h>

#include "driver.h"


// Clocking configuration
#define INTEL6_GT_THREAD_STATUS_REG			0x13805c
#define INTEL6_GT_THREAD_STATUS_CORE_MASK	0x7
#define INTEL6_GT_THREAD_STATUS_CORE_MASK_HSW (0x7 | (0x07 << 16))
#define INTEL6_GT_PERF_STATUS				0x145948
#define INTEL6_RP_STATE_LIMITS				0x145994
#define INTEL6_RP_STATE_CAP					0x145998
#define INTEL6_RPNSWREQ						0xA008
#define  INTEL6_TURBO_DISABLE				(1<<31)
#define  INTEL6_FREQUENCY(x)				((x)<<25)
#define  INTEL6_OFFSET(x)					((x)<<19)
#define  INTEL6_AGGRESSIVE_TURBO			(0<<15)
#define INTEL6_RC_VIDEO_FREQ				0xA00C
#define INTEL6_RC_CONTROL					0xA090
#define  INTEL6_RC_CTL_RC6pp_ENABLE			(1<<16)
#define  INTEL6_RC_CTL_RC6p_ENABLE			(1<<17)
#define  INTEL6_RC_CTL_RC6_ENABLE			(1<<18)
#define  INTEL6_RC_CTL_RC1e_ENABLE			(1<<20)
#define  INTEL6_RC_CTL_RC7_ENABLE			(1<<22)
#define  INTEL6_RC_CTL_EI_MODE(x)			((x)<<27)
#define  INTEL6_RC_CTL_HW_ENABLE			(1<<31)
#define INTEL6_RP_DOWN_TIMEOUT				0xA010
#define INTEL6_RP_INTERRUPT_LIMITS			0xA014
#define INTEL6_RPSTAT1						0xA01C
#define  INTEL6_CAGF_SHIFT					8
#define  INTEL6_CAGF_MASK					(0x7f << INTEL6_CAGF_SHIFT)
#define INTEL6_RP_CONTROL					0xA024
#define  INTEL6_RP_MEDIA_TURBO				(1<<11)
#define  INTEL6_RP_MEDIA_MODE_MASK			(3<<9)
#define  INTEL6_RP_MEDIA_HW_TURBO_MODE		(3<<9)
#define  INTEL6_RP_MEDIA_HW_NORMAL_MODE		(2<<9)
#define  INTEL6_RP_MEDIA_HW_MODE			(1<<9)
#define  INTEL6_RP_MEDIA_SW_MODE			(0<<9)
#define  INTEL6_RP_MEDIA_IS_GFX				(1<<8)
#define  INTEL6_RP_ENABLE					(1<<7)
#define  INTEL6_RP_UP_IDLE_MIN				(0x1<<3)
#define  INTEL6_RP_UP_BUSY_AVG				(0x2<<3)
#define  INTEL6_RP_UP_BUSY_CONT				(0x4<<3)
#define  GEN7_RP_DOWN_IDLE_AVG				(0x2<<0)
#define  INTEL6_RP_DOWN_IDLE_CONT			(0x1<<0)
#define INTEL6_RP_UP_THRESHOLD				0xA02C
#define INTEL6_RP_DOWN_THRESHOLD			0xA030
#define INTEL6_RP_CUR_UP_EI					0xA050
#define  INTEL6_CURICONT_MASK				0xffffff
#define INTEL6_RP_CUR_UP					0xA054
#define  INTEL6_CURBSYTAVG_MASK				0xffffff
#define INTEL6_RP_PREV_UP					0xA058
#define INTEL6_RP_CUR_DOWN_EI				0xA05C
#define  INTEL6_CURIAVG_MASK				0xffffff
#define INTEL6_RP_CUR_DOWN					0xA060
#define INTEL6_RP_PREV_DOWN					0xA064
#define INTEL6_RP_UP_EI						0xA068
#define INTEL6_RP_DOWN_EI					0xA06C
#define INTEL6_RP_IDLE_HYSTERSIS			0xA070
#define INTEL6_RC_STATE						0xA094
#define INTEL6_RC1_WAKE_RATE_LIMIT			0xA098
#define INTEL6_RC6_WAKE_RATE_LIMIT			0xA09C
#define INTEL6_RC6pp_WAKE_RATE_LIMIT		0xA0A0
#define INTEL6_RC_EVALUATION_INTERVAL		0xA0A8
#define INTEL6_RC_IDLE_HYSTERSIS			0xA0AC
#define INTEL6_RC_SLEEP						0xA0B0
#define INTEL6_RC1e_THRESHOLD				0xA0B4
#define INTEL6_RC6_THRESHOLD				0xA0B8
#define INTEL6_RC6p_THRESHOLD				0xA0BC
#define INTEL6_RC6pp_THRESHOLD				0xA0C0
#define INTEL6_PMINTRMSK					0xA168
#define INTEL6_PMISR						0x44020
#define INTEL6_PMIMR						0x44024 /* rps_lock */
#define INTEL6_PMIIR						0x44028
#define INTEL6_PMIER						0x4402C
#define  INTEL6_PM_MBOX_EVENT				(1<<25)
#define  INTEL6_PM_THERMAL_EVENT			(1<<24)
#define  INTEL6_PM_RP_DOWN_TIMEOUT			(1<<6)
#define  INTEL6_PM_RP_UP_THRESHOLD			(1<<5)
#define  INTEL6_PM_RP_DOWN_THRESHOLD		(1<<4)
#define  INTEL6_PM_RP_UP_EI_EXPIRED			(1<<2)
#define  INTEL6_PM_RP_DOWN_EI_EXPIRED		(1<<1)
#define  INTEL6_PM_DEFERRED_EVENTS			(INTEL6_PM_RP_UP_THRESHOLD \
											| INTEL6_PM_RP_DOWN_THRESHOLD \
											| INTEL6_PM_RP_DOWN_TIMEOUT)
#define INTEL6_GT_GFX_RC6_LOCKED			0x138104
#define INTEL6_GT_GFX_RC6					0x138108
#define INTEL6_GT_GFX_RC6p					0x13810C
#define INTEL6_GT_GFX_RC6pp					0x138110
#define INTEL6_PCODE_MAILBOX				0x138124
#define  INTEL6_PCODE_READY					(1<<31)
#define  INTEL6_READ_OC_PARAMS				0xc
#define  INTEL6_PCODE_WRITE_MIN_FREQ_TABLE	0x8
#define  INTEL6_PCODE_READ_MIN_FREQ_TABLE	0x9
#define INTEL6_PCODE_DATA					0x138128
#define  INTEL6_PCODE_FREQ_IA_RATIO_SHIFT	8
#define INTEL6_GT_CORE_STATUS				0x138060
#define  INTEL6_CORE_CPD_STATE_MASK			(7<<4)
#define  INTEL6_RCn_MASK					7
#define  INTEL6_RC0							0
#define  INTEL6_RC3							2
#define  INTEL6_RC6							3
#define  INTEL6_RC7							4


status_t intel_en_gating(intel_info &info);
status_t intel_en_downclock(intel_info &info);


#endif /* _INTEL_POWER_H_ */