/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

#warning MIPSEL: fixme

.text

/* int atomic_add(int *value, int increment)
 * (r3)           r3          r4
 */
FUNCTION(atomic_add):
lost1:		jr	$ra

/* int atomic_and(int *value, int andValue)
 * (r3)           r3          r4
 */
FUNCTION(atomic_and):
lost2:		jr	$ra

/* int atomic_or(int *value, int orValue)
 * (r3)          r3          r4
 */
FUNCTION(atomic_or):
lost3:		jr	$ra

/* int atomic_set(int *value, int setTo)
 * (r3)           r3          r4
 */
FUNCTION(atomic_set):
lost4:		jr	$ra

/* int atomic_test_and_set(int *value, int setTo, int testValue)
 * (r3)                    r3          r4         r5
 */
FUNCTION(atomic_test_and_set):
lost5:		jr	$ra

/* int atomic_get(int *value)
 * (r3)           r3
 */
FUNCTION(atomic_get):
lost6:		jr	$ra
