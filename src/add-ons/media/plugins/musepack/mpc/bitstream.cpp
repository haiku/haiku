#include <stdlib.h>
#include "bitstream.h"

/* C O N S T A N T S */
const unsigned int  mask [33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
    0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
    0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
    0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
    0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
    0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
    0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
    0xFFFFFFFF
};

/* F U N C T I O N S */

// resets bitstream decoding
void
MPC_decoder::Reset_BitstreamDecode ( void )
{
    dword     = 0;
    pos       = 0;
    Zaehler   = 0;
    WordsRead = 0;
}

// reports the number of read bits
unsigned int
MPC_decoder::BitsRead ( void )
{
    return 32*WordsRead + pos;
}

// read desired number of bits out of the bitstream
unsigned int
MPC_decoder::Bitstream_read ( const unsigned int bits )
{
    unsigned int out = dword;

    pos += bits;

    if (pos<32)
    {
        out >>= (32-pos);
    }
    else
    {
        dword = Speicher[Zaehler=(Zaehler+1)&MEMMASK];
        pos -= 32;
        if (pos)
        {
            out <<= pos;
            out |= dword >> (32-pos);
        }
        ++WordsRead;
    }

    return out & mask[bits];
}

// decode huffman
int
MPC_decoder::Huffman_Decode ( const HuffmanTyp* Table )
{
    // load preview and decode
    unsigned int code  = dword << pos;
    if (pos>18)  code |= Speicher[(Zaehler+1)&MEMMASK] >> (32-pos);
    while (code < Table->Code) Table++;

    // set the new position within bitstream without performing a dummy-read
    if ((pos += Table->Length)>=32)
    {
        pos -= 32;
        dword = Speicher[Zaehler=(Zaehler+1)&MEMMASK];
        ++WordsRead;
    }

    return Table->Value;
}

// faster huffman through previewing less bits
int
MPC_decoder::Huffman_Decode_fast ( const HuffmanTyp* Table )
{
    // load preview and decode
    unsigned int code  = dword << pos;
    if (pos>22)  code |= Speicher[(Zaehler+1)&MEMMASK] >> (32-pos);
    while (code < Table->Code) Table++;

    // set the new position within bitstream without performing a dummy-read
    if ((pos += Table->Length)>=32)
    {
        pos -= 32;
        dword = Speicher[Zaehler=(Zaehler+1)&MEMMASK];
        ++WordsRead;
    }

    return Table->Value;
}

// even faster huffman through previewing even less bits
int
MPC_decoder::Huffman_Decode_faster ( const HuffmanTyp* Table )
{
    // load preview and decode
    unsigned int code  = dword << pos;
    if (pos>27)  code |= Speicher[(Zaehler+1)&MEMMASK] >> (32-pos);
    while (code < Table->Code) Table++;

    // set the new position within bitstream without performing a dummy-read
    if ((pos += Table->Length)>=32)
    {
        pos -= 32;
        dword = Speicher[Zaehler=(Zaehler+1)&MEMMASK];
        ++WordsRead;
    }

    return Table->Value;
}

// decode SCFI-bundle (sv4,5,6)
void
MPC_decoder::SCFI_Bundle_read ( const HuffmanTyp* Table, int* SCFI, int* DSCF )
{
    // load preview and decode
    unsigned int code  = dword << pos;
    if (pos>26)  code |= Speicher[(Zaehler+1)&MEMMASK] >> (32-pos);
    while (code < Table->Code) Table++;

    // set the new position within bitstream without performing a dummy-read
    if ((pos += Table->Length)>=32)
    {
        pos -= 32;
        dword = Speicher[Zaehler=(Zaehler+1)&MEMMASK];
        ++WordsRead;
    }

    *SCFI = Table->Value >> 1;
    *DSCF = Table->Value &  1;
}

static int
cmpfn ( const void* p1, const void* p2 )
{
    if ( ((const MPC_decoder::HuffmanTyp*) p1)->Code < ((const MPC_decoder::HuffmanTyp*) p2)->Code ) return +1;
    if ( ((const MPC_decoder::HuffmanTyp*) p1)->Code > ((const MPC_decoder::HuffmanTyp*) p2)->Code ) return -1;
    return 0;
}

// sort huffman-tables by codeword
// offset resulting value
void
MPC_decoder::Resort_HuffTables ( const unsigned int elements, HuffmanTyp* Table, const int offset )
{
    unsigned int  i;

    for ( i = 0; i < elements; i++ ) {
        Table[i].Code <<= 32 - Table[i].Length;
        Table[i].Value  =  i - offset;
    }
    qsort ( Table, elements, sizeof(*Table), cmpfn );
}

/* end of bitstream.c */
