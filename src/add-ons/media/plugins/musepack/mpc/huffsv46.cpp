#include <stdio.h>
#include "huffsv46.h"
#include "requant.h"

void
MPC_decoder::Huffman_SV6_Decoder ( void )
{
    Huffman_SV6_Encoder ();
    Resort_HuffTables( 16, Region_A      , 0);
    Resort_HuffTables(  8, Region_B      , 0);
    Resort_HuffTables(  4, Region_C      , 0);
    Resort_HuffTables(  8, SCFI_Bundle   , 0);
    Resort_HuffTables( 13, DSCF_Entropie , 6);
    Resort_HuffTables(  3, Entropie_1    , Dc[1]);
    Resort_HuffTables(  5, Entropie_2    , Dc[2]);
    Resort_HuffTables(  7, Entropie_3    , Dc[3]);
    Resort_HuffTables(  9, Entropie_4    , Dc[4]);
    Resort_HuffTables( 15, Entropie_5    , Dc[5]);
    Resort_HuffTables( 31, Entropie_6    , Dc[6]);
    Resort_HuffTables( 63, Entropie_7    , Dc[7]);
}

void MPC_decoder::Huffman_SV6_Encoder ( void )
{
    // SCFI-bundle
    SCFI_Bundle[7].Code=  1; SCFI_Bundle[7].Length= 1;
    SCFI_Bundle[3].Code=  1; SCFI_Bundle[3].Length= 2;
    SCFI_Bundle[5].Code=  0; SCFI_Bundle[5].Length= 3;
    SCFI_Bundle[1].Code=  7; SCFI_Bundle[1].Length= 5;
    SCFI_Bundle[2].Code=  6; SCFI_Bundle[2].Length= 5;
    SCFI_Bundle[4].Code=  4; SCFI_Bundle[4].Length= 5;
    SCFI_Bundle[0].Code= 11; SCFI_Bundle[0].Length= 6;
    SCFI_Bundle[6].Code= 10; SCFI_Bundle[6].Length= 6;

    // region A (subbands  0..10)
    Region_A[ 1].Code=    1; Region_A[ 1].Length=  1;
    Region_A[ 2].Code=    0; Region_A[ 2].Length=  2;
    Region_A[ 0].Code=    2; Region_A[ 0].Length=  3;
    Region_A[ 3].Code=   15; Region_A[ 3].Length=  5;
    Region_A[ 5].Code=   13; Region_A[ 5].Length=  5;
    Region_A[ 6].Code=   12; Region_A[ 6].Length=  5;
    Region_A[ 4].Code=   29; Region_A[ 4].Length=  6;
    Region_A[ 7].Code=   57; Region_A[ 7].Length=  7;
    Region_A[ 8].Code=  113; Region_A[ 8].Length=  8;
    Region_A[ 9].Code=  225; Region_A[ 9].Length=  9;
    Region_A[10].Code=  449; Region_A[10].Length= 10;
    Region_A[11].Code=  897; Region_A[11].Length= 11;
    Region_A[12].Code= 1793; Region_A[12].Length= 12;
    Region_A[13].Code= 3585; Region_A[13].Length= 13;
    Region_A[14].Code= 7169; Region_A[14].Length= 14;
    Region_A[15].Code= 7168; Region_A[15].Length= 14;

    // region B (subbands 11..22)
    Region_B[1].Code= 1; Region_B[1].Length= 1;
    Region_B[0].Code= 1; Region_B[0].Length= 2;
    Region_B[2].Code= 1; Region_B[2].Length= 3;
    Region_B[3].Code= 1; Region_B[3].Length= 4;
    Region_B[4].Code= 1; Region_B[4].Length= 5;
    Region_B[5].Code= 1; Region_B[5].Length= 6;
    Region_B[6].Code= 1; Region_B[6].Length= 7;
    Region_B[7].Code= 0; Region_B[7].Length= 7;

    // region C (subbands 23..31)
    Region_C[0].Code= 1; Region_C[0].Length= 1;
    Region_C[1].Code= 1; Region_C[1].Length= 2;
    Region_C[2].Code= 1; Region_C[2].Length= 3;
    Region_C[3].Code= 0; Region_C[3].Length= 3;

    // DSCF
    DSCF_Entropie[ 6].Code=  0; DSCF_Entropie[ 6].Length= 2;
    DSCF_Entropie[ 7].Code=  7; DSCF_Entropie[ 7].Length= 3;
    DSCF_Entropie[ 5].Code=  4; DSCF_Entropie[ 5].Length= 3;
    DSCF_Entropie[ 8].Code=  3; DSCF_Entropie[ 8].Length= 3;
    DSCF_Entropie[ 9].Code= 13; DSCF_Entropie[ 9].Length= 4;
    DSCF_Entropie[ 4].Code= 11; DSCF_Entropie[ 4].Length= 4;
    DSCF_Entropie[10].Code= 10; DSCF_Entropie[10].Length= 4;
    DSCF_Entropie[ 2].Code=  4; DSCF_Entropie[ 2].Length= 4;
    DSCF_Entropie[11].Code= 25; DSCF_Entropie[11].Length= 5;
    DSCF_Entropie[ 3].Code= 24; DSCF_Entropie[ 3].Length= 5;
    DSCF_Entropie[ 1].Code= 11; DSCF_Entropie[ 1].Length= 5;
    DSCF_Entropie[12].Code= 21; DSCF_Entropie[12].Length= 6;
    DSCF_Entropie[ 0].Code= 20; DSCF_Entropie[ 0].Length= 6;

    // first quantizer
    Entropie_1[1].Code= 1; Entropie_1[1].Length= 1;
    Entropie_1[0].Code= 1; Entropie_1[0].Length= 2;
    Entropie_1[2].Code= 0; Entropie_1[2].Length= 2;

    // second quantizer
    Entropie_2[2].Code=  3; Entropie_2[2].Length= 2;
    Entropie_2[3].Code=  1; Entropie_2[3].Length= 2;
    Entropie_2[1].Code=  0; Entropie_2[1].Length= 2;
    Entropie_2[4].Code=  5; Entropie_2[4].Length= 3;
    Entropie_2[0].Code=  4; Entropie_2[0].Length= 3;

    // third quantizer
    Entropie_3[3].Code=  3; Entropie_3[3].Length= 2;
    Entropie_3[2].Code=  1; Entropie_3[2].Length= 2;
    Entropie_3[4].Code=  0; Entropie_3[4].Length= 2;
    Entropie_3[1].Code=  5; Entropie_3[1].Length= 3;
    Entropie_3[5].Code=  9; Entropie_3[5].Length= 4;
    Entropie_3[0].Code= 17; Entropie_3[0].Length= 5;
    Entropie_3[6].Code= 16; Entropie_3[6].Length= 5;

    // forth quantizer
    Entropie_4[4].Code=  0; Entropie_4[4].Length= 2;
    Entropie_4[5].Code=  6; Entropie_4[5].Length= 3;
    Entropie_4[3].Code=  5; Entropie_4[3].Length= 3;
    Entropie_4[6].Code=  4; Entropie_4[6].Length= 3;
    Entropie_4[2].Code=  3; Entropie_4[2].Length= 3;
    Entropie_4[7].Code= 15; Entropie_4[7].Length= 4;
    Entropie_4[1].Code= 14; Entropie_4[1].Length= 4;
    Entropie_4[0].Code=  5; Entropie_4[0].Length= 4;
    Entropie_4[8].Code=  4; Entropie_4[8].Length= 4;

    // fifth quantizer
    Entropie_5[7 ].Code=  4; Entropie_5[7 ].Length= 3;
    Entropie_5[8 ].Code=  3; Entropie_5[8 ].Length= 3;
    Entropie_5[6 ].Code=  2; Entropie_5[6 ].Length= 3;
    Entropie_5[9 ].Code=  0; Entropie_5[9 ].Length= 3;
    Entropie_5[5 ].Code= 15; Entropie_5[5 ].Length= 4;
    Entropie_5[4 ].Code= 13; Entropie_5[4 ].Length= 4;
    Entropie_5[10].Code= 12; Entropie_5[10].Length= 4;
    Entropie_5[11].Code= 10; Entropie_5[11].Length= 4;
    Entropie_5[3 ].Code=  3; Entropie_5[3 ].Length= 4;
    Entropie_5[12].Code=  2; Entropie_5[12].Length= 4;
    Entropie_5[2 ].Code= 29; Entropie_5[2 ].Length= 5;
    Entropie_5[1 ].Code= 23; Entropie_5[1 ].Length= 5;
    Entropie_5[13].Code= 22; Entropie_5[13].Length= 5;
    Entropie_5[0 ].Code= 57; Entropie_5[0 ].Length= 6;
    Entropie_5[14].Code= 56; Entropie_5[14].Length= 6;

    // sixth quantizer
    Entropie_6[15].Code=  9; Entropie_6[15].Length= 4;
    Entropie_6[16].Code=  8; Entropie_6[16].Length= 4;
    Entropie_6[14].Code=  7; Entropie_6[14].Length= 4;
    Entropie_6[18].Code=  6; Entropie_6[18].Length= 4;
    Entropie_6[17].Code=  5; Entropie_6[17].Length= 4;
    Entropie_6[12].Code=  3; Entropie_6[12].Length= 4;
    Entropie_6[13].Code=  2; Entropie_6[13].Length= 4;
    Entropie_6[19].Code=  0; Entropie_6[19].Length= 4;
    Entropie_6[11].Code= 31; Entropie_6[11].Length= 5;
    Entropie_6[20].Code= 30; Entropie_6[20].Length= 5;
    Entropie_6[10].Code= 29; Entropie_6[10].Length= 5;
    Entropie_6[9 ].Code= 27; Entropie_6[9 ].Length= 5;
    Entropie_6[21].Code= 26; Entropie_6[21].Length= 5;
    Entropie_6[22].Code= 25; Entropie_6[22].Length= 5;
    Entropie_6[8 ].Code= 24; Entropie_6[8 ].Length= 5;
    Entropie_6[7 ].Code= 23; Entropie_6[7 ].Length= 5;
    Entropie_6[23].Code= 21; Entropie_6[23].Length= 5;
    Entropie_6[6 ].Code=  9; Entropie_6[6 ].Length= 5;
    Entropie_6[24].Code=  3; Entropie_6[24].Length= 5;
    Entropie_6[25].Code= 57; Entropie_6[25].Length= 6;
    Entropie_6[5 ].Code= 56; Entropie_6[5 ].Length= 6;
    Entropie_6[4 ].Code= 45; Entropie_6[4 ].Length= 6;
    Entropie_6[26].Code= 41; Entropie_6[26].Length= 6;
    Entropie_6[2 ].Code= 40; Entropie_6[2 ].Length= 6;
    Entropie_6[27].Code= 17; Entropie_6[27].Length= 6;
    Entropie_6[28].Code= 16; Entropie_6[28].Length= 6;
    Entropie_6[3 ].Code=  5; Entropie_6[3 ].Length= 6;
    Entropie_6[29].Code= 89; Entropie_6[29].Length= 7;
    Entropie_6[1 ].Code= 88; Entropie_6[1 ].Length= 7;
    Entropie_6[30].Code=  9; Entropie_6[30].Length= 7;
    Entropie_6[0 ].Code=  8; Entropie_6[0 ].Length= 7;

    // seventh quantizer
    Entropie_7[25].Code=   0; Entropie_7[25].Length= 5;
    Entropie_7[37].Code=   1; Entropie_7[37].Length= 5;
    Entropie_7[62].Code=  16; Entropie_7[62].Length= 8;
    Entropie_7[ 0].Code=  17; Entropie_7[ 0].Length= 8;
    Entropie_7[ 3].Code=   9; Entropie_7[ 3].Length= 7;
    Entropie_7[ 5].Code=  10; Entropie_7[ 5].Length= 7;
    Entropie_7[ 6].Code=  11; Entropie_7[ 6].Length= 7;
    Entropie_7[38].Code=   3; Entropie_7[38].Length= 5;
    Entropie_7[35].Code=   4; Entropie_7[35].Length= 5;
    Entropie_7[33].Code=   5; Entropie_7[33].Length= 5;
    Entropie_7[24].Code=   6; Entropie_7[24].Length= 5;
    Entropie_7[27].Code=   7; Entropie_7[27].Length= 5;
    Entropie_7[26].Code=   8; Entropie_7[26].Length= 5;
    Entropie_7[12].Code=  18; Entropie_7[12].Length= 6;
    Entropie_7[50].Code=  19; Entropie_7[50].Length= 6;
    Entropie_7[29].Code=  10; Entropie_7[29].Length= 5;
    Entropie_7[31].Code=  11; Entropie_7[31].Length= 5;
    Entropie_7[36].Code=  12; Entropie_7[36].Length= 5;
    Entropie_7[34].Code=  13; Entropie_7[34].Length= 5;
    Entropie_7[28].Code=  14; Entropie_7[28].Length= 5;
    Entropie_7[49].Code=  30; Entropie_7[49].Length= 6;
    Entropie_7[56].Code=  62; Entropie_7[56].Length= 7;
    Entropie_7[ 7].Code=  63; Entropie_7[ 7].Length= 7;
    Entropie_7[32].Code=  16; Entropie_7[32].Length= 5;
    Entropie_7[30].Code=  17; Entropie_7[30].Length= 5;
    Entropie_7[13].Code=  36; Entropie_7[13].Length= 6;
    Entropie_7[55].Code=  74; Entropie_7[55].Length= 7;
    Entropie_7[61].Code= 150; Entropie_7[61].Length= 8;
    Entropie_7[ 1].Code= 151; Entropie_7[ 1].Length= 8;
    Entropie_7[14].Code=  38; Entropie_7[14].Length= 6;
    Entropie_7[48].Code=  39; Entropie_7[48].Length= 6;
    Entropie_7[ 4].Code=  80; Entropie_7[ 4].Length= 7;
    Entropie_7[58].Code=  81; Entropie_7[58].Length= 7;
    Entropie_7[47].Code=  41; Entropie_7[47].Length= 6;
    Entropie_7[15].Code=  42; Entropie_7[15].Length= 6;
    Entropie_7[16].Code=  43; Entropie_7[16].Length= 6;
    Entropie_7[54].Code=  88; Entropie_7[54].Length= 7;
    Entropie_7[ 8].Code=  89; Entropie_7[ 8].Length= 7;
    Entropie_7[17].Code=  45; Entropie_7[17].Length= 6;
    Entropie_7[46].Code=  46; Entropie_7[46].Length= 6;
    Entropie_7[45].Code=  47; Entropie_7[45].Length= 6;
    Entropie_7[53].Code=  96; Entropie_7[53].Length= 7;
    Entropie_7[ 9].Code=  97; Entropie_7[ 9].Length= 7;
    Entropie_7[43].Code=  49; Entropie_7[43].Length= 6;
    Entropie_7[19].Code=  50; Entropie_7[19].Length= 6;
    Entropie_7[18].Code=  51; Entropie_7[18].Length= 6;
    Entropie_7[44].Code=  52; Entropie_7[44].Length= 6;
    Entropie_7[ 2].Code= 212; Entropie_7[ 2].Length= 8;
    Entropie_7[60].Code= 213; Entropie_7[60].Length= 8;
    Entropie_7[10].Code= 107; Entropie_7[10].Length= 7;
    Entropie_7[42].Code=  54; Entropie_7[42].Length= 6;
    Entropie_7[41].Code=  55; Entropie_7[41].Length= 6;
    Entropie_7[20].Code=  56; Entropie_7[20].Length= 6;
    Entropie_7[21].Code=  57; Entropie_7[21].Length= 6;
    Entropie_7[52].Code= 116; Entropie_7[52].Length= 7;
    Entropie_7[51].Code= 117; Entropie_7[51].Length= 7;
    Entropie_7[40].Code=  59; Entropie_7[40].Length= 6;
    Entropie_7[22].Code=  60; Entropie_7[22].Length= 6;
    Entropie_7[23].Code=  61; Entropie_7[23].Length= 6;
    Entropie_7[39].Code=  62; Entropie_7[39].Length= 6;
    Entropie_7[11].Code= 126; Entropie_7[11].Length= 7;
    Entropie_7[57].Code= 254; Entropie_7[57].Length= 8;
    Entropie_7[59].Code= 255; Entropie_7[59].Length= 8;
}
