/*
 * Copyright 2012, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PLATFORM_DEBUG_H_
#define _PLATFORM_DEBUG_H_

#define DEBUG_DELAY_SHORT		200
#define DEBUG_DELAY_MEDIUM		500
#define DEBUG_DELAY_LONG		1000

void debug_delay(int time);
void debug_set_led(bool on);

void debug_toggle_led(int count, int delay = DEBUG_DELAY_MEDIUM);
	// Toggles the led count times with the delay for both on and off time.

void debug_blink_number(int number);
	// Implements a blink pattern to output arbitrary numbers:
	//
	//	1.	Led turns off and stays off for a long delay.
	//	2.	Led blinks x times with a short delay, where x is the current
	//		decimal place + 1 (so 0 values are easier to see).
	//	3.	Led stays off for a long delay to indicate the move to the next
	//		decimal place and step 2. is repeated until all the rest of the
	//		number is 0.
	//	4.	The led turns on and stays on with a long delay to indicate finish.
	//	5.	Led state is reset to the original value.
	//
	// The lowest decimal is output first, so the number will be reversed. As an
	// example the number 205 would be blinked as follows:
	//		long off (start indicator), 6 blinks (indicating 5),
	//		long off (moving to next decimal), 1 blink (indicating 0)
	//		long off (moving to next decimal), 3 blinks (indicating 2)
	//		long on (finish indicator)

void debug_halt();
	// Stalls execution in an endless loop.

void debug_assert(bool condition);
	// Flashes the led 20 times rapidly and stalls execution if the condition
	// isn't met.

#endif // _PLATFORM_DEBUG_H_

