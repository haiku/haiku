/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *	
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *	
 *	Description: Wrangle Beceem volatile DDR memory.
 */


#include "Settings.h"
#include "BeceemDDR.h"


BeceemDDR::BeceemDDR()
{
  TRACE("Debug: Load DDR handler\n");
}


status_t
BeceemDDR::DDRInit(WIMAX_DEVICE* swmxdevice)
{
	pwmxdevice = swmxdevice;
	PDDR_SETTING psDDRSetting=NULL;

	unsigned int HostDrvrConfig6 = pwmxdevice->vendorcfg.HostDrvrConfig6;
	unsigned int uiHostDrvrCfg6 = 0;
	unsigned int ChipID = pwmxdevice->deviceChipID;

	unsigned long RegCount=0;
	unsigned long value = 0;
	unsigned int uiResetValue = 0;
	unsigned int uiClockSetting = 0;
	int retval = B_OK;

	unsigned int	DDRSetting = 0;
	bool			PmuMode = 0;
	bool			MipsConfig = 0;
	bool			PLLConfig = 0;

	HostDrvrConfig6 &= ~(htonl(1 << 15));
	uiHostDrvrCfg6 = ntohl(HostDrvrConfig6);

	DDRSetting =	(ntohl(HostDrvrConfig6) >>8)&0x0F ;
	PmuMode =		(uiHostDrvrCfg6>>24)&0x03;
	MipsConfig =	(uiHostDrvrCfg6>>20)&0x01;
	PLLConfig = 	(uiHostDrvrCfg6>>19)&0x01;

    switch (ChipID)
	{
	case 0xbece3200:
	    switch (DDRSetting)
	    {
	        case DDR_80_MHZ:
				psDDRSetting=asT3LP_DDRSetting80MHz;
			    RegCount=(sizeof(asT3LP_DDRSetting80MHz)/
			  	sizeof(DDR_SETTING));
			    break;
		    case DDR_100_MHZ:
				psDDRSetting=asT3LP_DDRSetting100MHz;
			    RegCount=(sizeof(asT3LP_DDRSetting100MHz)/
			  	sizeof(DDR_SETTING));
			    break;
		    case DDR_133_MHZ:
				psDDRSetting=asT3LP_DDRSetting133MHz;
			    RegCount=(sizeof(asT3LP_DDRSetting133MHz)/
		 	  		sizeof(DDR_SETTING));
				if(MipsConfig == MIPS_200_MHZ)
				{
					uiClockSetting = 0x03F13652;
				}
				else
				{
					uiClockSetting = 0x03F1365B;
				}
				break;
		    default:
			    return -EINVAL;
        }

		break;
	case T3LPB:
	case BCS220_2:
	case BCS220_2BC:
	case BCS250_BC:
	case BCS220_3 :
		// We need to check current value and additionally set bit 2 and
		// bit 6 to 1 for BBIC 2mA drive
		if( (ChipID != BCS220_2) &&
			(ChipID != BCS220_2BC) &&
			(ChipID != BCS220_3) )
		{
				retval= BizarroReadRegister((unsigned int)0x0f000830,
					sizeof(uiResetValue), &uiResetValue);

				if(retval < 0) {
					TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
						__FUNCTION__, __LINE__);
					return retval;
				}
				uiResetValue |= 0x44;
				retval = BizarroWriteRegister((unsigned int)0x0f000830,
					sizeof(uiResetValue), &uiResetValue);
				if(retval < 0) {
					TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
						__FUNCTION__, __LINE__);
					return retval;
				}
		}
		switch(DDRSetting)
		{
			case DDR_80_MHZ:
				TRACE("Debug: DDR 80MHz\n");
				psDDRSetting = asT3LPB_DDRSetting80MHz;
		        RegCount=(sizeof(asT3B_DDRSetting80MHz)/
		                  sizeof(DDR_SETTING));
			break;
            case DDR_100_MHZ:
				TRACE("Debug: DDR 100MHz\n");
				psDDRSetting=asT3LPB_DDRSetting100MHz;
		        RegCount=(sizeof(asT3B_DDRSetting100MHz)/
		                 sizeof(DDR_SETTING));
			break;
            case DDR_133_MHZ:
				TRACE("Debug: DDR 133MHz\n");
				psDDRSetting = asT3LPB_DDRSetting133MHz;
				RegCount=(sizeof(asT3B_DDRSetting133MHz)/
						 sizeof(DDR_SETTING));

				if(MipsConfig == MIPS_200_MHZ)
				{
					uiClockSetting = 0x03F13652;
				}
				else
				{
					uiClockSetting = 0x03F1365B;
				}
			break;

			case DDR_160_MHZ:
				TRACE("Debug: DDR 160MHz\n");
				psDDRSetting = asT3LPB_DDRSetting160MHz;
				RegCount = sizeof(asT3LPB_DDRSetting160MHz)/sizeof(DDR_SETTING);

				if(MipsConfig == MIPS_200_MHZ)
				{
					TRACE("Debug: MIPS 200Mhz\n");
					uiClockSetting = 0x03F137D2;
				}
				else
				{
					uiClockSetting = 0x03F137DB;
				}
			}
			break;

	case 0xbece0110:
	case 0xbece0120:
	case 0xbece0121:
	case 0xbece0130:
	case 0xbece0300:
	    switch (DDRSetting)
	    {
	        case DDR_80_MHZ:
				psDDRSetting = asT3_DDRSetting80MHz;
			    RegCount = (sizeof(asT3_DDRSetting80MHz)/
			  	sizeof(DDR_SETTING));
			    break;
		    case DDR_100_MHZ:
				psDDRSetting = asT3_DDRSetting100MHz;
			    RegCount = (sizeof(asT3_DDRSetting100MHz)/
			  	sizeof(DDR_SETTING));
			    break;
		    case DDR_133_MHZ:
				psDDRSetting = asT3_DDRSetting133MHz;
			    RegCount = (sizeof(asT3_DDRSetting133MHz)/
		 	  	sizeof(DDR_SETTING));
				break;
		    default:
			    return -EINVAL;
        }
	case 0xbece0310:
	{
	    switch (DDRSetting)
	    {
	        case DDR_80_MHZ:
				psDDRSetting = asT3B_DDRSetting80MHz;
		        RegCount=(sizeof(asT3B_DDRSetting80MHz)/
		                  sizeof(DDR_SETTING));
		    break;
            case DDR_100_MHZ:
				psDDRSetting=asT3B_DDRSetting100MHz;
		        RegCount=(sizeof(asT3B_DDRSetting100MHz)/
		                 sizeof(DDR_SETTING));
			break;
            case DDR_133_MHZ:

				if(PLLConfig == PLL_266_MHZ)//266Mhz PLL selected.
				{
					memcpy(asT3B_DDRSetting133MHz, asDPLL_266MHZ,
									 sizeof(asDPLL_266MHZ));
					psDDRSetting = asT3B_DDRSetting133MHz;
					RegCount=(sizeof(asT3B_DDRSetting133MHz)/
									sizeof(DDR_SETTING));
				}
				else
				{
					psDDRSetting = asT3B_DDRSetting133MHz;
					RegCount=(sizeof(asT3B_DDRSetting133MHz)/
									sizeof(DDR_SETTING));
					if(MipsConfig == MIPS_200_MHZ)
					{
						uiClockSetting = 0x07F13652;
					}
					else
					{
						uiClockSetting = 0x07F1365B;
					}
				}
				break;
		    default:
			    return -EINVAL;
		}
		break;

	}
	default:
		return -EINVAL;
	}

	value=0;
	TRACE("Debug: Register count is %lu\n", RegCount);
	while(RegCount && !retval)
	{
		if(uiClockSetting && psDDRSetting->ulRegAddress == MIPS_CLOCK_REG)
		{
			value = uiClockSetting;
		}
		else
		{
			value = psDDRSetting->ulRegValue;
		}
		retval = BizarroWriteRegister(psDDRSetting->ulRegAddress,
			sizeof(value), (unsigned int*)&value);

		if(B_OK != retval) {
			TRACE_ALWAYS(
				"%s:%d BizarroWriteRegister failed at 0x%x on Register #%d.\n",
				 __FUNCTION__, __LINE__, psDDRSetting->ulRegAddress, RegCount);
			break;
		}

		RegCount--;
		psDDRSetting++;
	}

	if(ChipID >= 0xbece3300  )
	{
		snooze(3);
		if( (ChipID != BCS220_2)&&
			(ChipID != BCS220_2BC)&&
			(ChipID != BCS220_3))
		{
			/* drive MDDR to half in case of UMA-B:	*/
			uiResetValue = 0x01010001;
			retval = BizarroWriteRegister((unsigned int)0x0F007018,
				sizeof(uiResetValue), &uiResetValue);

			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x00040020;
			retval = BizarroWriteRegister((unsigned int)0x0F007094,
				sizeof(uiResetValue), &uiResetValue);

			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x01020101;
			retval = BizarroWriteRegister((unsigned int)0x0F00701c,
				sizeof(uiResetValue), &uiResetValue);

			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x01010000;
			retval = BizarroWriteRegister((unsigned int)0x0F007018,
				sizeof(uiResetValue), &uiResetValue);

			if(retval < 0) {
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
		if(PmuMode == HYBRID_MODE_7C)
		{
			TRACE("Debug: Hybrid Power Mode 7C\n");
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);

			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x1322a8;
			retval = BizarroWriteRegister((unsigned int)0x0f000d1c,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x132296;
			retval = BizarroWriteRegister((unsigned int)0x0f000d14,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
		}
		else if(PmuMode == HYBRID_MODE_6 )
		{
			TRACE("Debug: Hybrid Power Mode 6\n");
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x6003229a;
			retval = BizarroWriteRegister((unsigned int)0x0f000d14,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			retval = BizarroReadRegister((unsigned int)0x0f000c00,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroReadRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
			uiResetValue = 0x1322a8;
			retval = BizarroWriteRegister((unsigned int)0x0f000d1c,
				sizeof(uiResetValue), &uiResetValue);
			if(retval < 0) {
				TRACE_ALWAYS("%s:%d BizarroWriteRegister failed\n",
					__FUNCTION__, __LINE__);
				return retval;
			}
		}

	}
	return retval;
}

