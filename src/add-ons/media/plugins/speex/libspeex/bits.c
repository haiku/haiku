/* Copyright (C) 2002 Jean-Marc Valin 
   File: speex_bits.c

   Handles bit packing/unpacking

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include "speex_bits.h"
#include "misc.h"

void speex_bits_init(SpeexBits *bits)
{
   int i;
   bits->bytes = (char*)speex_alloc(MAX_BYTES_PER_FRAME);
   bits->buf_size = MAX_BYTES_PER_FRAME;

   for (i=0;i<bits->buf_size;i++)
      bits->bytes[i]=0;
   bits->nbBits=0;
   bits->bytePtr=0;
   bits->bitPtr=0;
   bits->owner=1;
   bits->overflow=0;
}

void speex_bits_init_buffer(SpeexBits *bits, void *buff, int buf_size)
{
   int i;
   bits->bytes = (char*)buff;
   bits->buf_size = buf_size;

   for (i=0;i<buf_size;i++)
      bits->bytes[i]=0;
   bits->nbBits=0;
   bits->bytePtr=0;
   bits->bitPtr=0;
   bits->owner=0;
   bits->overflow=0;
}

void speex_bits_destroy(SpeexBits *bits)
{
   if (bits->owner)
      speex_free(bits->bytes);
   /* Will do something once the allocation is dynamic */
}

void speex_bits_reset(SpeexBits *bits)
{
   int i;
   for (i=0;i<bits->buf_size;i++)
      bits->bytes[i]=0;
   bits->nbBits=0;
   bits->bytePtr=0;
   bits->bitPtr=0;
   bits->overflow=0;
}

void speex_bits_rewind(SpeexBits *bits)
{
   bits->bytePtr=0;
   bits->bitPtr=0;
   bits->overflow=0;
}

void speex_bits_read_from(SpeexBits *bits, char *bytes, int len)
{
   int i;
   if (len > bits->buf_size)
   {
      speex_warning_int("Packet is larger than allocated buffer: ", len);
      if (bits->owner)
      {
         char *tmp = (char*)speex_realloc(bits->bytes, len);
         if (tmp)
         {
            bits->buf_size=len;
            bits->bytes=tmp;
         } else {
            len=bits->buf_size;
            speex_warning("Could not resize input buffer: truncating input");
         }
      } else {
         speex_warning("Do not own input buffer: truncating input");
         len=bits->buf_size;
      }
   }
   for (i=0;i<len;i++)
      bits->bytes[i]=bytes[i];
   bits->nbBits=len<<3;
   bits->bytePtr=0;
   bits->bitPtr=0;
   bits->overflow=0;
}

static void speex_bits_flush(SpeexBits *bits)
{
   int i;
   if (bits->bytePtr>0)
   {
      for (i=bits->bytePtr;i<((bits->nbBits+7)>>3);i++)
         bits->bytes[i-bits->bytePtr]=bits->bytes[i];
   }
   bits->nbBits -= bits->bytePtr<<3;
   bits->bytePtr=0;
}

void speex_bits_read_whole_bytes(SpeexBits *bits, char *bytes, int len)
{
   int i,pos;

   if ((bits->nbBits>>3)+len+1 > bits->buf_size)
   {
      speex_warning_int("Packet is larger than allocated buffer: ", len);
      if (bits->owner)
      {
         char *tmp = (char*)speex_realloc(bits->bytes, (bits->nbBits>>3)+len+1);
         if (tmp)
         {
            bits->buf_size=(bits->nbBits>>3)+len+1;
            bits->bytes=tmp;
         } else {
            len=bits->buf_size-(bits->nbBits>>3)-1;
            speex_warning("Could not resize input buffer: truncating input");
         }
      } else {
         speex_warning("Do not own input buffer: truncating input");
         len=bits->buf_size;
      }
   }

   speex_bits_flush(bits);
   pos=bits->nbBits>>3;
   for (i=0;i<len;i++)
      bits->bytes[pos+i]=bytes[i];
   bits->nbBits+=len<<3;
}

int speex_bits_write(SpeexBits *bits, char *bytes, int max_len)
{
   int i;
   int bytePtr, bitPtr, nbBits;

   /* Insert terminator, but save the data so we can put it back after */
   bitPtr=bits->bitPtr;
   bytePtr=bits->bytePtr;
   nbBits=bits->nbBits;
   speex_bits_insert_terminator(bits);
   bits->bitPtr=bitPtr;
   bits->bytePtr=bytePtr;
   bits->nbBits=nbBits;

   if (max_len > ((bits->nbBits+7)>>3))
      max_len = ((bits->nbBits+7)>>3);
   for (i=0;i<max_len;i++)
      bytes[i]=bits->bytes[i];
   return max_len;
}

int speex_bits_write_whole_bytes(SpeexBits *bits, char *bytes, int max_len)
{
   int i;
   if (max_len > ((bits->nbBits)>>3))
      max_len = ((bits->nbBits)>>3);
   for (i=0;i<max_len;i++)
      bytes[i]=bits->bytes[i];
   
   if (bits->bitPtr>0)
      bits->bytes[0]=bits->bytes[max_len];
   else
      bits->bytes[0]=0;
   for (i=1;i<((bits->nbBits)>>3)+1;i++)
      bits->bytes[i]=0;
   bits->bytePtr=0;
   bits->nbBits &= 7;
   return max_len;
}


void speex_bits_pack(SpeexBits *bits, int data, int nbBits)
{
   int i;
   unsigned int d=data;

   if (bits->bytePtr+((nbBits+bits->bitPtr)>>3) >= bits->buf_size)
   {
      speex_warning("Buffer too small to pack bits");
      if (bits->owner)
      {
         char *tmp = (char*)speex_realloc(bits->bytes, ((bits->buf_size+5)*3)>>1);
         if (tmp)
         {
            for (i=bits->buf_size;i<(((bits->buf_size+5)*3)>>1);i++)
               tmp[i]=0;
            bits->buf_size=((bits->buf_size+5)*3)>>1;
            bits->bytes=tmp;
         } else {
            speex_warning("Could not resize input buffer: not packing");
            return;
         }
      } else {
         speex_warning("Do not own input buffer: not packing");
         return;
      }
   }

   while(nbBits)
   {
      int bit;
      bit = (d>>(nbBits-1))&1;
      bits->bytes[bits->bytePtr] |= bit<<(7-bits->bitPtr);
      bits->bitPtr++;

      if (bits->bitPtr==8)
      {
         bits->bitPtr=0;
         bits->bytePtr++;
      }
      bits->nbBits++;
      nbBits--;
   }
}

int speex_bits_unpack_signed(SpeexBits *bits, int nbBits)
{
   unsigned int d=speex_bits_unpack_unsigned(bits,nbBits);
   /* If number is negative */
   if (d>>(nbBits-1))
   {
      d |= (-1)<<nbBits;
   }
   return d;
}

unsigned int speex_bits_unpack_unsigned(SpeexBits *bits, int nbBits)
{
   unsigned int d=0;
   if ((bits->bytePtr<<3)+bits->bitPtr+nbBits>bits->nbBits)
      bits->overflow=1;
   if (bits->overflow)
      return 0;
   while(nbBits)
   {
      d<<=1;
      d |= (bits->bytes[bits->bytePtr]>>(7-bits->bitPtr))&1;
      bits->bitPtr++;
      if (bits->bitPtr==8)
      {
         bits->bitPtr=0;
         bits->bytePtr++;
      }
      nbBits--;
   }
   return d;
}

unsigned int speex_bits_peek_unsigned(SpeexBits *bits, int nbBits)
{
   unsigned int d=0;
   int bitPtr, bytePtr;
   char *bytes;

   if ((bits->bytePtr<<3)+bits->bitPtr+nbBits>bits->nbBits)
     bits->overflow=1;
   if (bits->overflow)
      return 0;

   bitPtr=bits->bitPtr;
   bytePtr=bits->bytePtr;
   bytes = bits->bytes;
   while(nbBits)
   {
      d<<=1;
      d |= (bytes[bytePtr]>>(7-bitPtr))&1;
      bitPtr++;
      if (bitPtr==8)
      {
         bitPtr=0;
         bytePtr++;
      }
      nbBits--;
   }
   return d;
}

int speex_bits_peek(SpeexBits *bits)
{
   if ((bits->bytePtr<<3)+bits->bitPtr+1>bits->nbBits)
      bits->overflow=1;
   if (bits->overflow)
      return 0;
   return (bits->bytes[bits->bytePtr]>>(7-bits->bitPtr))&1;
}

void speex_bits_advance(SpeexBits *bits, int n)
{
   int nbytes, nbits;

   if ((bits->bytePtr<<3)+bits->bitPtr+n>bits->nbBits)
      bits->overflow=1;
   if (bits->overflow)
      return;

   nbytes = n >> 3;
   nbits = n & 7;
   
   bits->bytePtr += nbytes;
   bits->bitPtr += nbits;
   
   if (bits->bitPtr>7)
   {
      bits->bitPtr-=8;
      bits->bytePtr++;
   }
}

int speex_bits_remaining(SpeexBits *bits)
{
   if (bits->overflow)
      return -1;
   else
      return bits->nbBits-((bits->bytePtr<<3)+bits->bitPtr);
}

int speex_bits_nbytes(SpeexBits *bits)
{
   return ((bits->nbBits+7)>>3);
}

void speex_bits_insert_terminator(SpeexBits *bits)
{
   if (bits->bitPtr<7)
      speex_bits_pack(bits, 0, 1);
   while (bits->bitPtr<7)
      speex_bits_pack(bits, 1, 1);
}
