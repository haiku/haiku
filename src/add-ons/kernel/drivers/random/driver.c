/* Yarrow Random Number Generator (True Randomness Achieved in Software) *
 * Copyright (c) 1998-2000 by Yarrow Charnot, Identikey <mailto://ycharnot@identikey.com> 
 * All Lefts, Rights, Ups, Downs, Forwards, Backwards, Pasts and Futures Reserved *
 */

/* Made into a BeOS /dev/random and /dev/urandom by Daniel Berlin */
/* Adapted for OpenBeOS by david reid */

/* To compile on Sun: gcc -m32 -msupersparc -mcpu=v8 -g -O3 -finline-functions -Wa,-xarch=
   v8plusc -Winline -lrt */ 
/* To compile on Intel, MSVC with /G6 /Gr /O2 and with either /MT or /MD is recommended */ 
/* If you want to export any of the following functions, you know what to do */ 


#include <stdio.h> 
#include <memheap.h> 
#include <OS.h>
#include <string.h>
#include <debug.h>

#define thread_yield() snooze(10)
#define _rmtmp() 

#define __int8 char 
#define __int16 short 
#define __int32 long 
#define __int64 long long 
//#define __cdecl 
#define __fastcall 
#define __forceinline __inline__ 
#define ASM __asm__ 
#define _lrotr(x, n) ((((unsigned __int32)(x)) >> ((int) ((n) & 31))) | (((unsigned __int32)(x)) << ((int) ((32 - ((n) & 31)))))) 
#define _lrotl(x, n) ((((unsigned __int32)(x)) << ((int) ((n) & 31))) | (((unsigned __int32)(x)) >> ((int) ((32 - ((n) & 31)))))) 

#if defined (_M_IX86) || defined (__i386__) && defined (__GNUC__) 

static volatile  __inline__ unsigned long long clock_counter (void) 
{ 
	return system_time(); /* eventually real_time_clock_usecs() ? */
} 

#elif defined (sparc) || defined (__sparc) || defined (sun) || defined (__sun) && defined (__GNUC__) 
static __inline unsigned long long clock_counter (void) 
{ 
	register unsigned long x, y; 
	__asm__ __volatile__ ("rd %%tick, %0; clruw %0, %1; srlx %0, 32, %0" : "=r" (x), "=r" (y) : "0" (x), "1" (y)); 
	return ((unsigned long long) x << 32) | y; 
} 
#else 
extern unsigned __int64 clock_counter (void); /* one of the above or any other clock 
						 counter function compiled separately */ 
#endif

#define rotr32(x, n) _lrotr (x, n) 
#define rotl32(x, n) _lrotl (x, n) 

#define bswap32(x) ((rotl32 ((unsigned __int32)(x), 8) & 0x00ff00ff) | (rotr32 ((unsigned __int32)(x), 8) & 0xff00ff00)) 

#ifndef _OCTET_ 
#define _OCTET_ 
typedef union _OCTET 
{ 
	unsigned __int64 Q[1]; 
	unsigned __int32 D[2]; 
	unsigned __int16 W[4]; 
	unsigned __int8  B[8]; 
} OCTET; 
#endif

#define NK 257 /* internal state size */ 
#define NI 120 /* seed in increment */ 
#define NA 70 /* rand out increment A */ 
#define NB 139 /* rand out increment B */ 
typedef struct _ch_randgen 
{ 
	OCTET ira[NK]; /* numbers live here */ 
	OCTET *seedptr; /* next seed pointer */ 
	OCTET *rndptrA; /* randomizing pointer #1 */ 
	OCTET *rndptrB; /* randomizing pointer #2 */ 
	OCTET *rndptrX; /* main randout pointer */ 
	OCTET rndLeft; /* left rand accumulator */ 
	OCTET rndRite; /* rite rand accumulator */ 
} ch_randgen; 

extern void hash_block(const unsigned char *block, const unsigned int block_byte_size, unsigned char *md);


#define HASH_BITS 160 /* I use Tiger. Modify it to match your HASH */ 
#define HASH_BLOCK_BITS 512 /* I use Tiger. Modify it to match your HASH */ 
#define HASH_BYTES (HASH_BITS / 8) 
#define HASH_BLOCK_BYTES (HASH_BLOCK_BITS / 8) 
#define HASH_OCTETS (HASH_BITS / 64) 
#define HASH_BLOCK_OCTETS (HASH_BLOCK_BITS / 64) 
/* attach by Yarrow Charnot. attaches x to y. can be seen as about 2-3 rounds of RC6 encryption 
 */ 
__inline__ void __fastcall attach (OCTET *y, const OCTET *x, const unsigned __int32 anyA, const unsigned __int32 anyB, const unsigned __int32 oddC, const unsigned __int32 
		oddD) 
{ 
	register OCTET _x; 
	register OCTET _y; 
	_x.D[0] = x->D[0]; 
	_x.D[1] = x->D[1]; 
	_y.D[0] = y->D[0]; 
	_y.D[1] = y->D[1]; 
	_x.D[0] = rotl32 (((bswap32(_x.D[0]) | 1) * x->D[1]), 5); 
	_x.D[1] = rotl32 ((bswap32 (_x.D[1]) | 1) * x->D[0], 5); 
	_y.D[0] = (bswap32 (rotl32 (_y.D[0] ^ _x.D[0], _x.D[1])) + anyA) * oddC; 
	_y.D[1] = (bswap32 (rotl32 (_y.D[1] ^ _x.D[1], _x.D[0])) + anyB) * oddD; 
	y->D[1] = _y.D[0]; 
	y->D[0] = _y.D[1]; 
} 
/* detach by Yarrow Charnot. detaches x from y. can be seen as about 2-3 rounds of RC6 
   decryption */ 
__forceinline void __fastcall detach (OCTET *y, const OCTET *x, const unsigned __int32 
		sameA, const unsigned __int32 sameB, const unsigned __int32 invoddC, const unsigned __int32 
		invoddD) 
{ 
	register OCTET _x; 
	register OCTET _y; 
	_x.D[0] = x->D[0]; 
	_x.D[1] = x->D[1]; 
	_y.D[0] = y->D[1]; 
	_y.D[1] = y->D[0]; 
	_x.D[0] = rotl32 ((bswap32 (_x.D[0]) | 1) * x->D[1], 5); 
	_x.D[1] = rotl32 ((bswap32 (_x.D[1]) | 1) * x->D[0], 5); 
	_y.D[0] = rotr32 (bswap32 (_y.D[0] * invoddC - sameA), _x.D[1]) ^ _x.D[0]; 
	_y.D[1] = rotr32 (bswap32 (_y.D[1] * invoddD - sameB), _x.D[0]) ^ _x.D[1]; 
	y->D[0] = _y.D[0]; 
	y->D[1] = _y.D[1]; 
} 
/* QUICKLY seeds in a 64 bit number, modified so that a subsequent call really "stirs" in another 
   seed value (no bullshit XOR here!) */ 
__inline void chseed (ch_randgen *prandgen, const unsigned __int64 seed) 
{ 
	prandgen->seedptr += NI; 
	if (prandgen->seedptr >= (prandgen->ira + NK)) prandgen->seedptr -= NK; 
	attach (prandgen->seedptr, (OCTET *) &seed, 0x213D42F6U, 0x6552DAF9U, 
			0x2E496B7BU, 0x1749A255U); 
} 
/* The heart of Yarrow 2000 Chuma Random Number Generator: fast and reliable randomness 
   collection. */ 
/* thread yielding function is the most OPTIMAL source of randomness combined with a clock 
   counter. */ 
/* it doesn't have to switch to another thread, the call itself is random enough. test it yourself. */ 
/* this FASTEST way to collect minimal randomness on each step couldn't use the processor 
   any LESS. */ 
/* even functions based on just creation of threads and their destruction can not compare by 
   speed. */ 
/* temporary file creation is just a little extra thwart to bewilder the processor cache and pipes. */ /* if you make clock_counter return all 0's, still produces a stream indistinguishable from 
													  random. */ 
volatile void reseed (ch_randgen *prandgen, const unsigned int inittimes) 
{ 
	volatile unsigned int i, j; 
	OCTET x, y; 
	for (j = inittimes; j; j--) 
	{ 
		for (i = NK * inittimes; i; i--) 
		{ 
			thread_yield (); 
			y.Q[0] += clock_counter (); 
			attach (&x, &y, 0x52437EFFU, 0x026A4CEBU, 0xD9E66AC9U, 0x56E5A975U); 
			attach (&y, &x, 0xC70B8B41U, 0x9126B036U, 0x36CC6FDBU, 0x31D477F7U); 
			chseed (prandgen, y.Q[0]); 
		} 
	} 
} 
/* returns a 64 bit of Yarrow 2000 Chuma RNG random number */ 
volatile __inline unsigned __int64 chrand (ch_randgen *prandgen) 
{ 
	prandgen->rndptrX ++; 
	prandgen->rndptrA += NA; 
	prandgen->rndptrB += NB; 
	if (prandgen->rndptrX >= (prandgen->ira + NK)) 
	{ 
		prandgen->rndptrX -= NK; 
		reseed (prandgen, 1); 
	} 
	if (prandgen->rndptrA >= (prandgen->ira + NK)) prandgen->rndptrA -= NK; 
	if (prandgen->rndptrB >= (prandgen->ira + NK)) prandgen->rndptrB -= NK; 
	attach (&prandgen->rndLeft, prandgen->rndptrX, prandgen->rndptrA->D[0], prandgen
			->rndptrA->D[1], 0x49A3BC71UL, 0x60E285FDUL); 
	attach (&prandgen->rndRite, &prandgen->rndLeft, prandgen->rndptrB->D[0], prandgen
			->rndptrB->D[1], 0xC366A5FDUL, 0x20C763EFUL); 
	chseed (prandgen, prandgen->rndRite.Q[0]); 
	return prandgen->rndRite.Q[0] ^ prandgen->rndLeft.Q[0]; 
} 
/* returns a 32 bit random number */ 
volatile __inline unsigned __int32 chrand32 (ch_randgen *prandgen) 
{ 
	OCTET r = {{chrand (prandgen)}}; 
	return r.D[0] ^ r.D[1]; 
} 
/* returns an 8 bit random number */
volatile __inline unsigned __int8 chrand8 (ch_randgen *prandgen)
{
	OCTET r = {{chrand(prandgen)}};
	return r.B[0] ^ r.B[1] ^ r.B[2] ^ r.B[3] ^ r.B[4] ^ r.B[5] ^ r.B[6] ^ r.B[7] ;
}

/* generates a cryptographically secure random big number 0 <= x < 32^n */ 
/* automatically reseeds if necessary or if requested 1/16 of the internal state or more */ 
/* 
   __inline void bigrand (ch_randgen *prandgen, unsigned __int32 *x, unsigned __int32 n) 
   { 
   unsigned int i; 
   OCTET block[HASH_BLOCK_OCTETS]; 
   OCTET hash[HASH_OCTETS]; 
   OCTET *j; 
   if (n >= NK/8) reseed (prandgen, 1); 
   for (*x++ = n; (signed) n > 0; ) 
   { 
   for (i = 0; i < HASH_BLOCK_OCTETS; i++) block->Q[i] += chrand (prandgen) + hash
   ->Q[i % HASH_OCTETS]; 
   hash_block (block->B, HASH_BLOCK_BYTES, hash->B); 
   for (i = HASH_OCTETS, j = hash; i && ((signed) n > 0); i--, j++, x += 2, n -= 2) 
   { 
   attach ((OCTET *) &x, j, 0x0AEF7ED2U, 0x3F85C5C1U, 0xD3EFB373U, 
   0x13ECF0B9U); 
   } 
   } 
   } 
 */
/* Initializes Yarrow 2000 Chuma Random Number Generator */ 
/* reseeding about 8 times prior to the first use is recommended. */ 
/* more than 16 will probably be a bit too much as time increases by n^2 */ 
__inline ch_randgen *new_chrand (const unsigned int inittimes) 
{ 
	ch_randgen *prandgen; 
	prandgen = (ch_randgen *)kmalloc (sizeof (ch_randgen)); 
	prandgen->seedptr = prandgen->ira; 
	prandgen->rndptrX = prandgen->ira; 
	prandgen->rndptrA = prandgen->ira; 
	prandgen->rndptrB = prandgen->ira; 
	prandgen->rndLeft.Q[0] = 0x1A4B385C72D69E0FUL; 
	prandgen->rndRite.Q[0] = 0x9C805FE7361A42DBUL; 
	reseed (prandgen, inittimes); 
	prandgen->seedptr = prandgen->ira + chrand (prandgen) % NK; 
	prandgen->rndptrX = prandgen->ira + chrand (prandgen) % NK; 
	prandgen->rndptrA = prandgen->ira + chrand (prandgen) % NK; 
	prandgen->rndptrB = prandgen->ira + chrand (prandgen) % NK; 
	return prandgen; 
} 
/* Clean up after chuma */ 
__inline void kill_chrand (ch_randgen *randgen) 
{ 
	memset (randgen, 0, sizeof (ch_randgen)); 
	kfree (randgen); 
} 
/* ++++++++++
   driver.c
   A skeletal device driver
   +++++ */

//#include <drivers/KernelExport.h>
#include <drivers.h>
#include <Errors.h>
#include <string.h>
#include <stdlib.h>
static ch_randgen *rng;
static unsigned int randcount=0;
static sem_id rand_mutex;
#define	DRIVER_NAME    "random"
#define	DEVICE_NAME    "random"
#define DEVICE_NAME2   "urandom"
int32	api_version = B_CUR_DRIVER_API_VERSION;

/* ----------
   init_hardware - called once the first time the driver is loaded
   ----- */


status_t init_hardware (void)
{
	dprintf (DRIVER_NAME ": init_hardware()\n");
	return B_OK;
}


/* ----------
   init_driver - optional function - called every time the driver
   is loaded.
   ----- */


status_t init_driver (void)
{
	rng = new_chrand(8);
	rand_mutex = create_sem(1, "RNG semaphore");
	dprintf (DRIVER_NAME ": init_driver()\n");
	return B_OK;
}


/* ----------
   uninit_driver - optional function - called every time the driver
   is unloaded
   ----- */

void uninit_driver (void)
{
	kill_chrand(rng);
	delete_sem(rand_mutex);
	dprintf (DRIVER_NAME ": uninit_driver()\n");
}


/* ----------
   my_device_open - handle open() calls
   ----- */

static status_t my_device_open (const char *name, uint32 flags, void** cookie)
{
	dprintf (DRIVER_NAME ": open(\"%s\")\n", name);
	return B_OK;
}


/* ----------
   my_device_read - handle read() calls
   ----- */

static ssize_t my_device_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	int32* buffer = (int32 *)buf;
	unsigned char *buf8 = (unsigned char *)buf;
	int i,j;
	acquire_sem(rand_mutex);
	randcount += *num_bytes;
	
	/* Reseed if we have or are gonna use up > 1/16th the entropy around */
	if (randcount >= NK/8) {
		randcount = 0;
		reseed (rng, 1); 
	}

	/* Yes, i know this is not the way we should do it. What we really should do is
	 * take the md5 or sha1 hash of the state of the pool, and return that. Someday. */
	for(i = 0 ; i < (*num_bytes)/4 ; i++)
		buffer[i] = chrand32(rng);
	for(j = 0 ; j < (*num_bytes) % 4; j++)
		buf8[(i*4)+j] = chrand8(rng);
	release_sem(rand_mutex);

	return B_OK;
}


/* ----------
   my_device_write - handle write() calls
   ----- */

static ssize_t my_device_write (void* cookie, off_t position, const void* buf, size_t* num_bytes)
{
	return B_OK;
}


/* ----------
   my_device_control - handle ioctl calls
   ----- */

static status_t my_device_control (void* cookie, uint32 op, void* arg, size_t len)
{
	/*dprintf (DRIVER_NAME ": ioctl(%d)\n", op);*/
	return B_ERROR;
}


/* ----------
   my_device_close - handle close() calls
   ----- */


static status_t my_device_close (void* cookie)
{
	dprintf (DRIVER_NAME ": close()\n");
	return B_OK;
}


/* -----
   my_device_free - called after the last device is closed, and after
   all i/o is complete.
   ----- */

	
static status_t my_device_free (void* cookie)
{
	dprintf (DRIVER_NAME ": free()\n");
	return B_OK;
}


/* -----
   null-terminated array of device names supported by this driver
   ----- */

static const char *my_device_name[] = {
	DEVICE_NAME,
	DEVICE_NAME2,
	NULL
};

/* -----
   function pointers for the device hooks entry points
   ----- */

static device_hooks my_device_hooks = {
	my_device_open,             /* -> open entry point */
	my_device_close,            /* -> close entry point */
	my_device_free,             /* -> free cookie */
	my_device_control,          /* -> control entry point */
	my_device_read,             /* -> read entry point */
	my_device_write,            /* -> write entry point */
	NULL,
	NULL,
	NULL,
	NULL
};

/* ----------
   publish_devices - return a null-terminated array of devices
   supported by this driver.
   ----- */


const char **publish_devices()
{
	dprintf (DRIVER_NAME ": publish_devices()\n");
	return my_device_name;
}

/* ----------
   find_device - return ptr to device hooks structure for a
   given device name
   ----- */
device_hooks *find_device(const char* name)
{
	int	i;
	dprintf (DRIVER_NAME ": find_device(\"%s\")\n", name);
	for (i = 0; my_device_name[i] != NULL; i++)
		if (strcmp (name, my_device_name [i]) == 0)
			return &my_device_hooks;
	return NULL;
}

