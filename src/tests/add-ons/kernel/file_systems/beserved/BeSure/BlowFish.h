/* blowfish.h */

#ifdef _BUILDING_BESURE_
#define _IMPEXP_BESURE		__declspec(dllexport)
#else
#define _IMPEXP_BESURE		__declspec(dllimport)
#endif

#define UWORD32 unsigned long
#define UBYTE08 unsigned char

#define MAXKEYBYTES 56          /* 448 bits */

typedef struct
{
	unsigned long S[4][256], P[18];
} blf_ctx;

unsigned long F(blf_ctx *, unsigned long x);
void Blowfish_encipher(blf_ctx *, unsigned long *xl, unsigned long *xr);
void Blowfish_decipher(blf_ctx *, unsigned long *xl, unsigned long *xr);
short InitializeBlowfish(blf_ctx *, unsigned char key[], int keybytes);

#ifdef __cplusplus
extern "C" 
{
#endif

_IMPEXP_BESURE void blf_enc(blf_ctx *c, unsigned long *data, int blocks);
_IMPEXP_BESURE void blf_dec(blf_ctx *c, unsigned long *data, int blocks);
_IMPEXP_BESURE void blf_key(blf_ctx *c, unsigned char *key, int len);

#ifdef __cplusplus
}
#endif