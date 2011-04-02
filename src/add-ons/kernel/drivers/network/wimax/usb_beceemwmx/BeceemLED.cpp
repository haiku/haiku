/*
 *	Beceem WiMax USB Driver.
 *	Copyright (c) 2010 Alexander von Gluck <kallisti5@unixzen.com>
 *	Distributed under the terms of the GNU General Public License.
 *
 *	Based on GPL code developed by: Beceem Communications Pvt. Ltd
 *
 *	Beceem devices use fairly complex GPIO registers to control the
 *	device LED(s).  As each implimentation uses the LEDs differently,
 *	each vendor provides a binary target configuration telling the
 *	driver what GPIO led register is used for what device state.
 */


#include "Settings.h"
#include "BeceemLED.h"


BeceemLED::BeceemLED()
{
	TRACE("Debug: Load LED handler\n");
}


status_t
BeceemLED::LEDInit(WIMAX_DEVICE* wmxdevice)
{
	pwmxdevice = wmxdevice;

	uint8_t			GPIO_Array[NUM_OF_LEDS+1];
	unsigned char	ucIndex			= 0;
	uint32_t		uiIndex			= 0;
	unsigned char*	puCFGData		= NULL;
	unsigned char	bData			= 0;

	memset(GPIO_Array, GPIO_DISABLE_VAL, NUM_OF_LEDS+1);

	TRACE("Debug: Raw GPIO information: 0x%x\n", pwmxdevice->gpioInfo);

	snooze(500000);

	if (pwmxdevice->deviceChipID == 0xbece0120
		|| pwmxdevice->deviceChipID == 0xbece0121)
	{
		/*Hardcode the values of GPIO numbers for ASIC board.*/
		GPIO_Array[RED_LED] = 2;
		GPIO_Array[BLUE_LED] = 3;
		GPIO_Array[YELLOW_LED] = GPIO_DISABLE_VAL;
		GPIO_Array[GREEN_LED] = 4;
	} else {
		// for all possible GPIO pins...
		for (ucIndex = 0; ucIndex < 32; ucIndex++)
		{
			switch(pwmxdevice->gpioInfo[ucIndex])
			{
				case RED_LED:
					TRACE("Debug: GPIO: %d found RED_LED!\n", ucIndex);
					GPIO_Array[RED_LED] = ucIndex;
					pwmxdevice->gpioBitMap |= (1<<ucIndex);
					break;
				case BLUE_LED:
					TRACE("Debug: GPIO: %d found BLUE_LED!\n", ucIndex);
					GPIO_Array[BLUE_LED] = ucIndex;
					pwmxdevice->gpioBitMap |= (1<<ucIndex);
					break;
				case YELLOW_LED:
					TRACE("Debug: GPIO: %d found YELLOW_LED!\n", ucIndex);
					GPIO_Array[YELLOW_LED] = ucIndex;
					pwmxdevice->gpioBitMap |= (1<<ucIndex);
					break;
				case GREEN_LED:
					TRACE("Debug: GPIO: %d found GREEN_LED!\n", ucIndex);
					GPIO_Array[GREEN_LED] = ucIndex;
					pwmxdevice->gpioBitMap |= (1<<ucIndex);
					break;
				default:
					// TRACE("Debug: GPIO: %d found NO_LED!\n", ucIndex);
					break;
			}
		}
	}

	TRACE("Debug: GPIO's bitmap correspond to LED :0x%X\n",
		pwmxdevice->gpioBitMap);

	// Load GPIO configuration data from vendor config
	puCFGData = (unsigned char *)&pwmxdevice->vendorcfg.HostDrvrConfig1;

	for (uiIndex = 0; uiIndex < NUM_OF_LEDS; uiIndex++)
	{
		bData = *puCFGData;

		// Check Bit 8 for polarity. If it is set, polarity is reverse
		if (bData & 0x80)
		{
			pwmxdevice->LEDMap.LEDState[uiIndex].BitPolarity = 0;
			// unset bit 8
			bData = bData & 0x7f;
		} else {
			// If polarity is not reverse, set to normal state.
			pwmxdevice->LEDMap.LEDState[uiIndex].BitPolarity = 1;
		}

		pwmxdevice->LEDMap.LEDState[uiIndex].LED_Type = ( bData );

		if (bData <= NUM_OF_LEDS)
			pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num = GPIO_Array[bData];
		else
			pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num = GPIO_DISABLE_VAL;

		puCFGData++;
		bData = *puCFGData;

		pwmxdevice->LEDMap.LEDState[uiIndex].LED_On_State = bData;

		puCFGData++;
		bData = *puCFGData;

		pwmxdevice->LEDMap.LEDState[uiIndex].LED_Blink_State = bData;

		puCFGData++;
	}

	GPIOReset();	// Set all GPIO pins to off

	for (uiIndex = 0; uiIndex<NUM_OF_LEDS; uiIndex++)
	{
		if (pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num != GPIO_DISABLE_VAL) {
			LEDOn(uiIndex);
			TRACE("Debug: LED[%d].LED_Type = %x\n",
				uiIndex, pwmxdevice->LEDMap.LEDState[uiIndex].LED_Type);
			TRACE("Debug: LED[%d].LED_On_State = %x\n",
				uiIndex, pwmxdevice->LEDMap.LEDState[uiIndex].LED_On_State);
			TRACE("Debug: LED[%d].LED_Blink_State = %x\n",
				uiIndex, pwmxdevice->LEDMap.LEDState[uiIndex].LED_Blink_State);
			TRACE("Debug: LED[%d].GPIO_Num = %i\n",
				uiIndex, pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num);
			TRACE("Debug: LED[%d].Polarity = %d\n",
				uiIndex, pwmxdevice->LEDMap.LEDState[uiIndex].BitPolarity);
			snooze(500000);
			LEDOff(uiIndex);
		}
	}


	// Spawn LED monitor / blink thread
	pwmxdevice->LEDThreadID		= 	spawn_kernel_thread(LEDThread,
										"usb_beceemwmx GPIO:LED",
										B_NORMAL_PRIORITY, this);

	if (pwmxdevice->LEDThreadID < 1) {
		TRACE("Error: Problem spawning LED Thread: %i\n", pwmxdevice->LEDThreadID);
		return B_ERROR;
	}

	resume_thread(pwmxdevice->LEDThreadID);
	TRACE("Debug: LED Thread spawned: %i\n", pwmxdevice->LEDThreadID);
	return B_OK;
}


status_t
BeceemLED::GPIOReset()
{
	int	index = 0;

	TRACE("Debug: GPIO reset called\n");

	for (index = 0; index < NUM_OF_LEDS; index++) {
		if (pwmxdevice->LEDMap.LEDState[index].GPIO_Num != GPIO_DISABLE_VAL)
			LEDOff(index);
	}

	return B_OK;
}


status_t
BeceemLED::LEDThreadTerminate()
{
	status_t thread_return_value;

	TRACE("Debug: Terminating LED thread PID %i\n", pwmxdevice->LEDThreadID);

	// Lets make sure driverHalt is true...  if it is false this will
	// wait forever.
	if (!pwmxdevice->driverHalt) {
		TRACE_ALWAYS("Wierd... driver is not halted.  Halting\n");
		pwmxdevice->driverHalt = true;
	}

	return wait_for_thread(pwmxdevice->LEDThreadID, &thread_return_value);
}


status_t
BeceemLED::LightsOut()
{
	unsigned int    uiIndex			= 0;

	for (uiIndex = 0; uiIndex < NUM_OF_LEDS; uiIndex++)
	{
		if (pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num != GPIO_DISABLE_VAL) {
			LEDOff(uiIndex);
		}
	}
	return B_OK;
}


status_t
BeceemLED::LEDOn(unsigned int index)
{
	status_t	result;
	uint32_t	gpio_towrite = 0;
	uint32_t	uiResetValue = 0;

	gpio_towrite = 1<<pwmxdevice->LEDMap.LEDState[index].GPIO_Num;

	if (pwmxdevice->LEDMap.LEDState[index].BitPolarity == 0)
		result = BizarroWriteRegister(GPIO_OUTPUT_SET_REG,
			sizeof(gpio_towrite), &gpio_towrite);
	else
		result = BizarroWriteRegister(GPIO_OUTPUT_CLR_REG,
			sizeof(gpio_towrite), &gpio_towrite);

	if (result != B_OK) {
		TRACE_ALWAYS("Error: Problem setting LED(%d) at GPIO 0x%x (bit %d)\n",
			index, gpio_towrite, pwmxdevice->LEDMap.LEDState[index].GPIO_Num);
	}

	uiResetValue = BizarroReadRegister(GPIO_MODE_REGISTER,
		sizeof(uiResetValue), &uiResetValue);
	uiResetValue |= gpio_towrite;
	BizarroWriteRegister(GPIO_MODE_REGISTER, sizeof(uiResetValue), &uiResetValue);

	return B_OK;
}


status_t
BeceemLED::LEDOff(unsigned int index)
{
	status_t	result;
	uint32_t	gpio_towrite = 0;
	uint32_t	uiResetValue = 0;

	gpio_towrite = 1<<pwmxdevice->LEDMap.LEDState[index].GPIO_Num;

	if (pwmxdevice->LEDMap.LEDState[index].BitPolarity == 0)
		result = BizarroWriteRegister(GPIO_OUTPUT_CLR_REG,
			sizeof(gpio_towrite), &gpio_towrite);
	else
		result = BizarroWriteRegister(GPIO_OUTPUT_SET_REG,
			sizeof(gpio_towrite), &gpio_towrite);

	if (result != B_OK) {
		TRACE_ALWAYS("Error: Problem setting LED(%d) at GPIO 0x%x (bit %d)\n",
			index, gpio_towrite, pwmxdevice->LEDMap.LEDState[index].GPIO_Num);
	}

	uiResetValue = BizarroReadRegister(GPIO_MODE_REGISTER,
		sizeof(uiResetValue), &uiResetValue);
	uiResetValue |= gpio_towrite;
	BizarroWriteRegister(GPIO_MODE_REGISTER,
		sizeof(uiResetValue), &uiResetValue);

	return B_OK;
}


status_t
BeceemLED::LEDThread(void *cookie)
{
	bool ledactive = false;
	BeceemLED *led = (BeceemLED *)cookie;

	// While the driver is active
	while (!led->pwmxdevice->driverHalt)
	{
		unsigned int  	uiIndex = 0;
		unsigned int  	uiLedIndex = 0;
		bool			blink = false;

		// LED state changes will be at least 500ms apart
		snooze(500000);

		// determine what the LED is doing in each state
		switch(led->pwmxdevice->driverState)
		{
			case STATE_FWPUSH:
				for (uiIndex = 0; uiIndex < NUM_OF_LEDS; uiIndex++)
				{
					if (led->pwmxdevice->LEDMap.LEDState[uiIndex].LED_Blink_State
						& STATE_FWPUSH)
					{
						if (led->pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num
							!= GPIO_DISABLE_VAL) {
							uiLedIndex = uiIndex;
						}
					}
				}
				blink = true;
				break;

			case STATE_NONET:
				for (uiIndex = 0; uiIndex < NUM_OF_LEDS; uiIndex++)
				{
					if (led->pwmxdevice->LEDMap.LEDState[uiIndex].LED_On_State
						& STATE_NONET)
					{
						if (led->pwmxdevice->LEDMap.LEDState[uiIndex].GPIO_Num
							!= GPIO_DISABLE_VAL) {
							uiLedIndex = uiIndex;
						}
					}
				}
				blink = false;
				break;

			default:
				blink = false;
				break;
		}

		TRACE("Debug: Detected to use LedIndex: 0x%x for driver state 0x%x\n",
			uiLedIndex, led->pwmxdevice->driverState);

		if (blink == true) {
			led->LEDOff(uiLedIndex);
			ledactive = false;
			snooze(500000);
			led->LEDOn(uiLedIndex);
			ledactive = true;
		} else {
			// else we just flip on the color it needs to be.
			led->LEDOn(uiLedIndex);
			ledactive = true;
			snooze(500000);
		}

	}

	return(B_OK);
}

