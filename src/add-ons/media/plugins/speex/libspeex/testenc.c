#include "speex.h"
#include <stdio.h>
#include <stdlib.h>
#include "speex_callbacks.h"

#define FRAME_SIZE 160
#include <math.h>
int main(int argc, char **argv)
{
   char *inFile, *outFile, *bitsFile;
   FILE *fin, *fout, *fbits=NULL;
   short in[FRAME_SIZE];
   float input[FRAME_SIZE], bak[FRAME_SIZE], bak2[FRAME_SIZE];
   char cbits[200];
   int nbBits;
   int i;
   void *st;
   void *dec;
   SpeexBits bits;
   int tmp;
   int bitCount=0;
   SpeexCallback callback;

   for (i=0;i<FRAME_SIZE;i++)
      bak2[i]=0;
   st = speex_encoder_init(&speex_nb_mode);
   dec = speex_decoder_init(&speex_nb_mode);

   callback.callback_id = SPEEX_INBAND_CHAR;
   callback.func = speex_std_char_handler;
   callback.data = stderr;
   speex_decoder_ctl(dec, SPEEX_SET_HANDLER, &callback);

   callback.callback_id = SPEEX_INBAND_MODE_REQUEST;
   callback.func = speex_std_mode_request_handler;
   callback.data = st;
   speex_decoder_ctl(dec, SPEEX_SET_HANDLER, &callback);

   tmp=0;
   speex_decoder_ctl(dec, SPEEX_SET_ENH, &tmp);
   tmp=0;
   speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);
   tmp=8;
   speex_encoder_ctl(st, SPEEX_SET_QUALITY, &tmp);
   tmp=1;
   speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &tmp);

   speex_mode_query(&speex_nb_mode, SPEEX_MODE_FRAME_SIZE, &tmp);
   fprintf (stderr, "frame size: %d\n", tmp);

   if (argc != 4 && argc != 3)
   {
      fprintf (stderr, "Usage: encode [in file] [out file] [bits file]\nargc = %d", argc);
      exit(1);
   }
   inFile = argv[1];
   fin = fopen(inFile, "r");
   outFile = argv[2];
   fout = fopen(outFile, "w");
   if (argc==4)
   {
      bitsFile = argv[3];
      fbits = fopen(bitsFile, "w");
   }
   speex_bits_init(&bits);
   while (!feof(fin))
   {
      fread(in, sizeof(short), FRAME_SIZE, fin);
      if (feof(fin))
         break;
      for (i=0;i<FRAME_SIZE;i++)
         bak[i]=input[i]=in[i];
      speex_bits_reset(&bits);
      /*
      speex_bits_pack(&bits, 14, 5);
      speex_bits_pack(&bits, SPEEX_INBAND_CHAR, 4);
      speex_bits_pack(&bits, 'A', 8);
      
      speex_bits_pack(&bits, 14, 5);
      speex_bits_pack(&bits, SPEEX_INBAND_MODE_REQUEST, 4);
      speex_bits_pack(&bits, 7, 4);

      speex_bits_pack(&bits, 15, 5);
      speex_bits_pack(&bits, 2, 4);
      speex_bits_pack(&bits, 0, 16);
      */
      speex_encode(st, input, &bits);
      nbBits = speex_bits_write(&bits, cbits, 200);
      bitCount+=bits.nbBits;
      printf ("Encoding frame in %d bits\n", nbBits*8);
      if (argc==4)
         fwrite(cbits, 1, nbBits, fbits);
      {
         float enoise=0, esig=0, snr;
         for (i=0;i<FRAME_SIZE;i++)
         {
            enoise+=(bak2[i]-input[i])*(bak2[i]-input[i]);
            esig += bak2[i]*bak2[i];
         }
         snr = 10*log10((esig+1)/(enoise+1));
         printf ("real SNR = %f\n", snr);
      }
      speex_bits_rewind(&bits);
      
      speex_decode(dec, &bits, input);
      
      /* Save the bits here */
      for (i=0;i<FRAME_SIZE;i++)
      {
         if (input[i]>32000)
            input[i]=32000;
         else if (input[i]<-32000)
            input[i]=-32000;
      }
      speex_bits_reset(&bits);
      for (i=0;i<FRAME_SIZE;i++)
         in[i]=(short)input[i];
      for (i=0;i<FRAME_SIZE;i++)
         bak2[i]=bak[i];
      fwrite(in, sizeof(short), FRAME_SIZE, fout);
   }
   fprintf (stderr, "Total encoded size: %d bits\n", bitCount);
   
   speex_encoder_destroy(st);
   speex_decoder_destroy(dec);
   return 1;
}
