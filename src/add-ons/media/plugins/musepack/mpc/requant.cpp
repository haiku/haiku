#include <math.h>
#include "requant.h"

/* C O N S T A N T S */
// bits per sample for chosen quantizer
const unsigned int  Res_bit [18] = {
    0,  0,  0,  0,  0,  0,  0,  0,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16
};

// coefficients for requantization
// 65536/step bzw. 65536/(2*D+1)
const float  __Cc [1 + 18] = {
      111.285962475327f,                                        // 32768/2/255*sqrt(3)
    65536.000000000000f, 21845.333333333332f, 13107.200000000001f, 9362.285714285713f,
     7281.777777777777f,  4369.066666666666f,  2114.064516129032f, 1040.253968253968f,
      516.031496062992f,   257.003921568627f,   128.250489236790f,   64.062561094819f,
       32.015632633121f,    16.003907203907f,     8.000976681723f,    4.000244155527f,
        2.000061037018f,     1.000015259021f
};

// offset for requantization
// 2*D+1 = steps of quantizer
const int  __Dc [1 + 18] = {
      2,
      0,     1,     2,     3,     4,     7,    15,    31,    63,
    127,   255,   511,  1023,  2047,  4095,  8191, 16383, 32767
};

/* F U N C T I O N S */
void
MPC_decoder::ScaleOutput ( double factor )
{
    int     n;
    double  f1 = factor;
    double  f2 = factor;

    // handles +1.58...-98.41 dB, where's scf[n] / scf[n-1] = 1.20050805774840750476
    for ( n = 0; n <= 128; n++ ) {
        SCF [(unsigned char)(1+n)] = (float) f1;
        SCF [(unsigned char)(1-n)] = (float) f2;
        f1 *=   0.83298066476582673961;
        f2 *= 1/0.83298066476582673961;
    }
}

void
MPC_decoder::Quantisierungsmodes ( void )            // conversion: index -> quantizer (bitstream reading)
{                                       // conversion: quantizer -> index (bitstream writing)
    int  Band = 0;
    int  i;

    do {
        Q_bit [Band] = 4;
        for ( i = 0; i < 16-1; i++ )
            Q_res [Band] [i] = i;
        Q_res [Band][i] = 17;
        Band++;
    } while ( Band < 11 );

    do {
        Q_bit [Band] = 3;
        for ( i = 0; i < 8-1; i++ )
            Q_res [Band] [i] = i;
        Q_res [Band] [i] = 17;
        Band++;
    } while ( Band < 23 );

    do {
        Q_bit [Band] = 2;
        for ( i = 0; i < 4-1; i++ )
            Q_res [Band] [i] = i;
        Q_res [Band] [i] = 17;
        Band++;
    } while ( Band < 32 );
}

void
MPC_decoder::initialisiere_Quantisierungstabellen ( void )
{
    Quantisierungsmodes ();
    ScaleOutput ( 1.0 );
}
