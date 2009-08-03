/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#define FUNCTION(x) .global x; .type x,@function; x

.text

/* uint32 get_sdr1(void);
 */
FUNCTION(get_sdr1):
	mfsdr1	%r3
	blr

/* void set_sdr1(uint32 value);
 *				 r3
 */
FUNCTION(set_sdr1):
	mtsdr1	%r3
	blr

/* uint32 get_sr(void *virtualAddress);
 *				 r3
 */
FUNCTION(get_sr):
	mfsrin	%r3, %r3
	blr

/* void set_sr(void *virtualAddress, uint32 value);
 *			   r3					 r4
 */
FUNCTION(set_sr):
	isync
	mtsrin	%r4, %r3
	isync
	blr

/* uint32 get_msr(void);
 */
FUNCTION(get_msr):
	mfmsr	%r3
	blr

/* uint32 set_msr(uint32 value);
 *				  r3
 */
FUNCTION(set_msr):
	mtmsr	%r3
	isync
	blr

/* uint32 get_pvr(void);
 */
FUNCTION(get_pvr):
	mfpvr	%r3
	blr


#define get_ibat(num) \
	mfibatu	%r4, num; \
	stw		%r4, 0(%r3); \
	mfibatl	%r4, num; \
	stw		%r4, 4(%r3); \

#define set_ibat(num); \
	lwz		%r4, 0(%r3); \
	mtibatu num, %r4; \
	lwz		%r4, 4(%r3); \
	mtibatl	num, %r4;

/* void get_ibat0-3(block_address_translation *bat);
 *					r3
 */
FUNCTION(get_ibat0):
	get_ibat(0)
	blr
FUNCTION(get_ibat1):
	get_ibat(1)
	blr
FUNCTION(get_ibat2):
	get_ibat(2)
	blr
FUNCTION(get_ibat3):
	get_ibat(3)
	blr

/* void set_ibat0-3(block_address_translation *bat);
 *					r3
 */
FUNCTION(set_ibat0):
	set_ibat(0)
	blr
FUNCTION(set_ibat1):
	set_ibat(1)
	blr
FUNCTION(set_ibat2):
	set_ibat(2)
	blr
FUNCTION(set_ibat3):
	set_ibat(3)
	blr

/* void reset_ibats(void)
 */
FUNCTION(reset_ibats):
	li		%r3, 0
	mtibatu	0, %r3
	mtibatl 0, %r3
	mtibatu	1, %r3
	mtibatl 1, %r3
	mtibatu	2, %r3
	mtibatl 2, %r3
	mtibatu	3, %r3
	mtibatl 3, %r3
	blr

#define get_dbat(num) \
	mfdbatu	%r4, num; \
	stw		%r4, 0(%r3); \
	mfdbatl	%r4, num; \
	stw		%r4, 4(%r3);

#define set_dbat(num) \
	lwz		%r4, 0(%r3); \
	mtdbatu num, %r4; \
	lwz		%r4, 4(%r3); \
	mtdbatl	num, %r4;

/* void get_dbat0-3(block_address_translation *bat);
 *					r3
 */
FUNCTION(get_dbat0):
	get_dbat(0)
	blr
FUNCTION(get_dbat1):
	get_dbat(1)
	blr
FUNCTION(get_dbat2):
	get_dbat(2)
	blr
FUNCTION(get_dbat3):
	get_dbat(3)
	blr

/* void set_dbat0-3(block_address_translation *bat);
 *					r3
 */
FUNCTION(set_dbat0):
	set_dbat(0)
	blr
FUNCTION(set_dbat1):
	set_dbat(1)
	blr
FUNCTION(set_dbat2):
	set_dbat(2)
	blr
FUNCTION(set_dbat3):
	set_dbat(3)
	blr

/* void reset_dbats(void)
 */
FUNCTION(reset_dbats):
	li		%r3, 0
	mtdbatu	0, %r3
	mtdbatl 0, %r3
	mtdbatu	1, %r3
	mtdbatl 1, %r3
	mtdbatu	2, %r3
	mtdbatl 2, %r3
	mtdbatu	3, %r3
	mtdbatl 3, %r3
	blr

/* void __eieio(void)
 */
FUNCTION(__eieio):
	eieio
	blr
