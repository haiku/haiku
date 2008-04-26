/* blowfish.c */

#include "BlowFish.h"
#include "BlowFishTable.h"

#define N               16
#define noErr            0
#define DATAERROR         -1
#define KEYBYTES         8

unsigned long F(blf_ctx *bc, unsigned long x)
{
   unsigned long a;
   unsigned long b;
   unsigned long c;
   unsigned long d;
   unsigned long y;

   d = x & 0x00FF;
   x >>= 8;
   c = x & 0x00FF;
   x >>= 8;
   b = x & 0x00FF;
   x >>= 8;
   a = x & 0x00FF;
   y = bc->S[0][a] + bc->S[1][b];
   y = y ^ bc->S[2][c];
   y = y + bc->S[3][d];

   return y;
}

void Blowfish_encipher(blf_ctx *bc, unsigned long *xl, unsigned long *xr)
{
   unsigned long  Xl;
   unsigned long  Xr;
   unsigned long  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = 0; i < N; ++i)
   {
      Xl = Xl ^ bc->P[i];
      Xr = F(bc, Xl) ^ Xr;

      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ bc->P[N];
   Xl = Xl ^ bc->P[N + 1];

   *xl = Xl;
   *xr = Xr;
}

void Blowfish_decipher(blf_ctx *bc, unsigned long *xl, unsigned long *xr)
{
   unsigned long  Xl;
   unsigned long  Xr;
   unsigned long  temp;
   short          i;

   Xl = *xl;
   Xr = *xr;

   for (i = N + 1; i > 1; --i)
   {
      Xl = Xl ^ bc->P[i];
      Xr = F(bc, Xl) ^ Xr;

      /* Exchange Xl and Xr */
      temp = Xl;
      Xl = Xr;
      Xr = temp;
   }

   /* Exchange Xl and Xr */
   temp = Xl;
   Xl = Xr;
   Xr = temp;

   Xr = Xr ^ bc->P[1];
   Xl = Xl ^ bc->P[0];

   *xl = Xl;
   *xr = Xr;
}

short InitializeBlowfish(blf_ctx *bc, unsigned char key[], int keybytes)
{
	 short          i;
	 short          j;
	 short          k;
	 unsigned long  data;
	 unsigned long  datal;
	 unsigned long  datar;

	/* initialise p & s-boxes without file read */
	for (i = 0; i < N+2; i++)
	{
		bc->P[i] = bfp[i];
	}
	for (i = 0; i < 256; i++)
	{
		bc->S[0][i] = ks0[i];
		bc->S[1][i] = ks1[i];
		bc->S[2][i] = ks2[i];
		bc->S[3][i] = ks3[i];
	}

	j = 0;
	for (i = 0; i < N + 2; ++i)
	{
		data = 0x00000000;
		for (k = 0; k < 4; ++k)
		{
			data = (data << 8) | key[j];
			j = j + 1;
			if (j >= keybytes)
			{
	  			j = 0;
			}
		}
		bc->P[i] = bc->P[i] ^ data;
	}

	datal = 0x00000000;
	datar = 0x00000000;

	for (i = 0; i < N + 2; i += 2)
	{
		Blowfish_encipher(bc, &datal, &datar);

		bc->P[i] = datal;
		bc->P[i + 1] = datar;
	}

	for (i = 0; i < 4; ++i)
	{
		for (j = 0; j < 256; j += 2)
		{

			Blowfish_encipher(bc, &datal, &datar);

			bc->S[i][j] = datal;
			bc->S[i][j + 1] = datar;
		}
	}
	return 0;
}

void blf_key (blf_ctx *c, unsigned char *k, int len)
{
	InitializeBlowfish(c, k, len);
}

void blf_enc(blf_ctx *c, unsigned long *data, int blocks)
{
	unsigned long *d;
	int i;

	d = data;
	for (i = 0; i < blocks; i++)
	{
		Blowfish_encipher(c, d, d+1);
		d += 2;
	}
}

void blf_dec(blf_ctx *c, unsigned long *data, int blocks)
{
	unsigned long *d;
	int i;

	d = data;
	for (i = 0; i < blocks; i++)
	{
		Blowfish_decipher(c, d, d+1);
		d += 2;
	}
}
