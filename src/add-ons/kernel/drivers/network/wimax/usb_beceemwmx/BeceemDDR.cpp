/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *
 *	Description: Wrangle Beceem volatile DDR memory.
 */


#include "BeceemDDR.h"
#include "Settings.h"


BeceemDDR::BeceemDDR()
{
	TRACE("Debug: Load DDR handler\n");
}


status_t
BeceemDDR::DDRInit(WIMAX_DEVICE* swmxdevice)
{
	fWmxDevice = swmxdevice;
	PDDR_SETTING psDDRSetting = NULL;

	unsigned int chipID = fWmxDevice->deviceChipID;

	unsigned long registerCount = 0;
	unsigned long value = 0;
	unsigned int uiResetValue = 0;
	unsigned int uiClockSetting = 0;
	int retval = B_OK;

	// Grab the Config6 metric from the vendor config and convert endianness
	unsigned int vendorConfig6raw = fWmxDevice->vendorcfg.HostDrvrConfig6;
	vendorConfig6raw &= ~(htonl(1 << 15));
	unsigned int vendorConfig6 = ntohl(vendorConfig6raw);

	// Read our vendor provided Config6 metric and populate memory settings
	unsigned int vendorDDRSetting = (ntohl(vendorConfig6raw) >> 8) & 0x0F;
	bool vendorPmuMode = (vendorConfig6 >> 24) & 0x03;
	bool vendorMipsConfig = (vendorConfig6 >> 20) & 0x01;
	bool vendorPLLConfig = (vendorConfig6 >> 19) & 0x01;

	switch (chipID) {
		case 0xbece3200:
			switch (vendorDDRSetting) {
				case DDR_80_MHZ:
					psDDRSetting = asT3LP_DDRSetting80MHz;
					registerCount = sizeof(asT3LP_DDRSetting80MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_100_MHZ:
					psDDRSetting = asT3LP_DDRSetting100MHz;
					registerCount = sizeof(asT3LP_DDRSetting100MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_133_MHZ:
					psDDRSetting = asT3LP_DDRSetting133MHz;
					registerCount = sizeof(asT3LP_DDRSetting133MHz)
						/ sizeof(DDR_SETTING);
					if (vendorMipsConfig == MIPS_200_MHZ)
						uiClockSetting = 0x03F13652;
					else
						uiClockSetting = 0x03F1365B;
					break;

				default:
					return B_BAD_VALUE;
			}
			break;

		case T3LPB:
		case BCS220_2:
		case BCS220_2BC:
		case BCS250_BC:
		case BCS220_3 :
			// We need to check current value and additionally set bit 2 and
			// bit 6 to 1 for BBIC 2mA drive
			if ((chipID != BCS220_2)
				&& (chipID != BCS220_2BC)
				&& (chipID != BCS220_3)) {
				retval = BizarroReadRegister((unsigned int)0x0f000830,
					sizeof(uiResetValue), &uiResetValue);

				if (retval < 0) {
					TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
						__FUNCTION__, __LINE__);
					return retval;
				}
				uiResetValue |= 0x44;
				retval = BizarroWriteRegister((unsigned int)0x0f000830,
					sizeof(uiResetValue), &uiResetValue);
				if (retval < 0) {
					TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
						__FUNCTION__, __LINE__);
					return retval;
				}
			}

			switch(vendorDDRSetting) {
				case DDR_80_MHZ:
					TRACE("Debug: DDR 80MHz\n");
					psDDRSetting = asT3LPB_DDRSetting80MHz;
					registerCount = sizeof(asT3B_DDRSetting80MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_100_MHZ:
					TRACE("Debug: DDR 100MHz\n");
					psDDRSetting = asT3LPB_DDRSetting100MHz;
					registerCount = sizeof(asT3B_DDRSetting100MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_133_MHZ:
					TRACE("Debug: DDR 133MHz\n");
					psDDRSetting = asT3LPB_DDRSetting133MHz;
					registerCount = sizeof(asT3B_DDRSetting133MHz)
						/ sizeof(DDR_SETTING);

					if (vendorMipsConfig == MIPS_200_MHZ)
						uiClockSetting = 0x03F13652;
					else
						uiClockSetting = 0x03F1365B;
					break;

				case DDR_160_MHZ:
					TRACE("Debug: DDR 160MHz\n");
					psDDRSetting = asT3LPB_DDRSetting160MHz;
					registerCount = sizeof(asT3LPB_DDRSetting160MHz)
						/ sizeof(DDR_SETTING);

					if (vendorMipsConfig == MIPS_200_MHZ) {
						TRACE("Debug: MIPS 200Mhz\n");
						uiClockSetting = 0x03F137D2;
					} else {
						uiClockSetting = 0x03F137DB;
					}
			}
			break;

		case 0xbece0110:
		case 0xbece0120:
		case 0xbece0121:
		case 0xbece0130:
		case 0xbece0300:
			switch (vendorDDRSetting) {
				case DDR_80_MHZ:
					psDDRSetting = asT3_DDRSetting80MHz;
					registerCount = sizeof(asT3_DDRSetting80MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_100_MHZ:
					psDDRSetting = asT3_DDRSetting100MHz;
					registerCount = sizeof(asT3_DDRSetting100MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_133_MHZ:
					psDDRSetting = asT3_DDRSetting133MHz;
					registerCount = sizeof(asT3_DDRSetting133MHz)
						/ sizeof(DDR_SETTING);
					break;

				default:
					return B_BAD_VALUE;
			}
			break;
		case 0xbece0310:
		{
			switch (vendorDDRSetting) {
				case DDR_80_MHZ:
					psDDRSetting = asT3B_DDRSetting80MHz;
					registerCount = sizeof(asT3B_DDRSetting80MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_100_MHZ:
					psDDRSetting = asT3B_DDRSetting100MHz;
					registerCount = sizeof(asT3B_DDRSetting100MHz)
						/ sizeof(DDR_SETTING);
					break;

				case DDR_133_MHZ:
					if (vendorPLLConfig == PLL_266_MHZ) {
						// 266Mhz PLL selected.
						memcpy(asT3B_DDRSetting133MHz, asDPLL_266MHZ,
							sizeof(asDPLL_266MHZ));
						psDDRSetting = asT3B_DDRSetting133MHz;
						registerCount = sizeof(asT3B_DDRSetting133MHz)
							/ sizeof(DDR_SETTING);

					} else {
						psDDRSetting = asT3B_DDRSetting133MHz;
						registerCount = sizeof(asT3B_DDRSetting133MHz)
							/ sizeof(DDR_SETTING);

						if (vendorMipsConfig == MIPS_200_MHZ)
							uiClockSetting = 0x07F13652;
						else
							uiClockSetting = 0x07F1365B;
					}
					break;

				default:
					return B_BAD_VALUE;
			}
			break;
		}

		default:
			return B_BAD_VALUE;
	}

	value = 0;
	TRACE("Debug: Register count is %lu\n", registerCount);
	while (registerCount && !retval) {
		if (uiClockSetting && psDDRSetting->ulRegAddress == MIPS_CLOCK_REG)
			value = uiClockSetting;
		else
			value = psDDRSetting->ulRegValue;

		retval = BizarroWriteRegister(psDDRSetting->ulRegAddress,
			sizeof(value), (unsigned int*)&value);

		if (B_OK != retval) {
			TRACE_ALWAYS(
				"%s:%d BizarroWriteRegister failed at 0x%x on Register #%d.\n",
				__FUNCTION__, __LINE__, psDDRSetting->ulRegAddress,
				registerCount);
			break;
		}

		registerCount--;
		psDDRSetting++;
	}

	if (chipID >= 0xbece3300) {
		snooze(3);
		if ((chipID != BCS220_2)
			&& (chipID != BCS220_2BC)
			&& (chipID != BCS220_3)) {
			/* drive MDDR to half in case of UMA-B:	*/
			uiResetValue = 0x01010001;
			retval = BizarroWriteRegister((unsigned int)0x0F007018,
				sizeof(uiResetValue), &uiResetValue);

			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x00040020;
			retval = BizarroWriteRegister((unsigned int)0x0F007094,
				sizeof(uiResetValue), &uiResetValue);

			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x01020101;
			retval = BizarroWriteRegister((unsigned int)0x0F00701c,
				sizeof(uiResetValue), &uiResetValue);

			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x01010000;
			retval = BizarroWriteRegister((unsigned int)0x0F007018,
				sizeof(uiResetValue), &uiResetValue);

			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
		}
		snooze(3);

		/* DC/DC standby change...
		 * This is to be done only for Hybrid PMU mode.
		 * with the current h/w there is no way to detect this.
		 * and since we dont have internal PMU lets do it under
		 * UMA-B chip id. we will change this when we will have an
		 * internal PMU.
	     */
		if (vendorPmuMode == HYBRID_MODE_7C) {
			TRACE("Debug: Hybrid Power Mode 7C\n");
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);

			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x1322a8;
			retval = BizarroWriteRegister((unsigned int)0x0f000d1c,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x132296;
			retval = BizarroWriteRegister((unsigned int)0x0f000d14,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
		} else if (vendorPmuMode == HYBRID_MODE_6) {

			TRACE("Debug: Hybrid Power Mode 6\n");
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x6003229a;
			retval = BizarroWriteRegister((unsigned int)0x0f000d14,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x1322a8;
			retval = BizarroWriteRegister((unsigned int)0x0f000d1c,
				sizeof(uiResetValue), &uiResetValue);
			if (retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
		}

	}
	return retval;
}

