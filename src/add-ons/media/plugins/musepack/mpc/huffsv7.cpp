#include "huffsv7.h"
#include "requant.h"

void
MPC_decoder::Huffman_SV7_Decoder ( void )
{
    Huffman_SV7_Encoder ();
    Resort_HuffTables(10, &HuffHdr[0]   , 5);
    Resort_HuffTables( 4, &HuffSCFI[0]  , 0);
    Resort_HuffTables(16, &HuffDSCF[0]  , 7);
    Resort_HuffTables(27, &HuffQ1[0][0] , 0);
    Resort_HuffTables(27, &HuffQ1[1][0] , 0);
    Resort_HuffTables(25, &HuffQ2[0][0] , 0);
    Resort_HuffTables(25, &HuffQ2[1][0] , 0);
    Resort_HuffTables( 7, &HuffQ3[0][0] , Dc[3]);
    Resort_HuffTables( 7, &HuffQ3[1][0] , Dc[3]);
    Resort_HuffTables( 9, &HuffQ4[0][0] , Dc[4]);
    Resort_HuffTables( 9, &HuffQ4[1][0] , Dc[4]);
    Resort_HuffTables(15, &HuffQ5[0][0] , Dc[5]);
    Resort_HuffTables(15, &HuffQ5[1][0] , Dc[5]);
    Resort_HuffTables(31, &HuffQ6[0][0] , Dc[6]);
    Resort_HuffTables(31, &HuffQ6[1][0] , Dc[6]);
    Resort_HuffTables(63, &HuffQ7[0][0] , Dc[7]);
    Resort_HuffTables(63, &HuffQ7[1][0] , Dc[7]);
}

void
MPC_decoder::Huffman_SV7_Encoder ( void )
{
    /***************************** SCFI *******************************/
    HuffSCFI[0].Code = 2; HuffSCFI[0].Length = 3;
    HuffSCFI[1].Code = 1; HuffSCFI[1].Length = 1;
    HuffSCFI[2].Code = 3; HuffSCFI[2].Length = 3;
    HuffSCFI[3].Code = 0; HuffSCFI[3].Length = 2;

    /***************************** DSCF *******************************/
    HuffDSCF[ 0].Code = 32; HuffDSCF[ 0].Length = 6;
    HuffDSCF[ 1].Code =  4; HuffDSCF[ 1].Length = 5;
    HuffDSCF[ 2].Code = 17; HuffDSCF[ 2].Length = 5;
    HuffDSCF[ 3].Code = 30; HuffDSCF[ 3].Length = 5;
    HuffDSCF[ 4].Code = 13; HuffDSCF[ 4].Length = 4;
    HuffDSCF[ 5].Code =  0; HuffDSCF[ 5].Length = 3;
    HuffDSCF[ 6].Code =  3; HuffDSCF[ 6].Length = 3;
    HuffDSCF[ 7].Code =  9; HuffDSCF[ 7].Length = 4;
    HuffDSCF[ 8].Code =  5; HuffDSCF[ 8].Length = 3;
    HuffDSCF[ 9].Code =  2; HuffDSCF[ 9].Length = 3;
    HuffDSCF[10].Code = 14; HuffDSCF[10].Length = 4;
    HuffDSCF[11].Code =  3; HuffDSCF[11].Length = 4;
    HuffDSCF[12].Code = 31; HuffDSCF[12].Length = 5;
    HuffDSCF[13].Code =  5; HuffDSCF[13].Length = 5;
    HuffDSCF[14].Code = 33; HuffDSCF[14].Length = 6;
    HuffDSCF[15].Code = 12; HuffDSCF[15].Length = 4;

    /************************* frame-header ***************************/
    /***************** differential quantizer indizes *****************/
    HuffHdr[0].Code =  92; HuffHdr[0].Length = 8;
    HuffHdr[1].Code =  47; HuffHdr[1].Length = 7;
    HuffHdr[2].Code =  10; HuffHdr[2].Length = 5;
    HuffHdr[3].Code =   4; HuffHdr[3].Length = 4;
    HuffHdr[4].Code =   0; HuffHdr[4].Length = 2;
    HuffHdr[5].Code =   1; HuffHdr[5].Length = 1;
    HuffHdr[6].Code =   3; HuffHdr[6].Length = 3;
    HuffHdr[7].Code =  22; HuffHdr[7].Length = 6;
    HuffHdr[8].Code = 187; HuffHdr[8].Length = 9;
    HuffHdr[9].Code = 186; HuffHdr[9].Length = 9;

    /********************** 3-step quantizer **************************/
    /********************* 3 bundled samples **************************/
    //less shaped, book 0
    HuffQ1[0][ 0].Code = 54; HuffQ1[0][ 0].Length = 6;
    HuffQ1[0][ 1].Code =  9; HuffQ1[0][ 1].Length = 5;
    HuffQ1[0][ 2].Code = 32; HuffQ1[0][ 2].Length = 6;
    HuffQ1[0][ 3].Code =  5; HuffQ1[0][ 3].Length = 5;
    HuffQ1[0][ 4].Code = 10; HuffQ1[0][ 4].Length = 4;
    HuffQ1[0][ 5].Code =  7; HuffQ1[0][ 5].Length = 5;
    HuffQ1[0][ 6].Code = 52; HuffQ1[0][ 6].Length = 6;
    HuffQ1[0][ 7].Code =  0; HuffQ1[0][ 7].Length = 5;
    HuffQ1[0][ 8].Code = 35; HuffQ1[0][ 8].Length = 6;
    HuffQ1[0][ 9].Code = 10; HuffQ1[0][ 9].Length = 5;
    HuffQ1[0][10].Code =  6; HuffQ1[0][10].Length = 4;
    HuffQ1[0][11].Code =  4; HuffQ1[0][11].Length = 5;
    HuffQ1[0][12].Code = 11; HuffQ1[0][12].Length = 4;
    HuffQ1[0][13].Code =  7; HuffQ1[0][13].Length = 3;
    HuffQ1[0][14].Code = 12; HuffQ1[0][14].Length = 4;
    HuffQ1[0][15].Code =  3; HuffQ1[0][15].Length = 5;
    HuffQ1[0][16].Code =  7; HuffQ1[0][16].Length = 4;
    HuffQ1[0][17].Code = 11; HuffQ1[0][17].Length = 5;
    HuffQ1[0][18].Code = 34; HuffQ1[0][18].Length = 6;
    HuffQ1[0][19].Code =  1; HuffQ1[0][19].Length = 5;
    HuffQ1[0][20].Code = 53; HuffQ1[0][20].Length = 6;
    HuffQ1[0][21].Code =  6; HuffQ1[0][21].Length = 5;
    HuffQ1[0][22].Code =  9; HuffQ1[0][22].Length = 4;
    HuffQ1[0][23].Code =  2; HuffQ1[0][23].Length = 5;
    HuffQ1[0][24].Code = 33; HuffQ1[0][24].Length = 6;
    HuffQ1[0][25].Code =  8; HuffQ1[0][25].Length = 5;
    HuffQ1[0][26].Code = 55; HuffQ1[0][26].Length = 6;

    //more shaped, book 1
    HuffQ1[1][ 0].Code = 103; HuffQ1[1][ 0].Length = 8;
    HuffQ1[1][ 1].Code =  62; HuffQ1[1][ 1].Length = 7;
    HuffQ1[1][ 2].Code = 225; HuffQ1[1][ 2].Length = 9;
    HuffQ1[1][ 3].Code =  55; HuffQ1[1][ 3].Length = 7;
    HuffQ1[1][ 4].Code =   3; HuffQ1[1][ 4].Length = 4;
    HuffQ1[1][ 5].Code =  52; HuffQ1[1][ 5].Length = 7;
    HuffQ1[1][ 6].Code = 101; HuffQ1[1][ 6].Length = 8;
    HuffQ1[1][ 7].Code =  60; HuffQ1[1][ 7].Length = 7;
    HuffQ1[1][ 8].Code = 227; HuffQ1[1][ 8].Length = 9;
    HuffQ1[1][ 9].Code =  24; HuffQ1[1][ 9].Length = 6;
    HuffQ1[1][10].Code =   0; HuffQ1[1][10].Length = 4;
    HuffQ1[1][11].Code =  61; HuffQ1[1][11].Length = 7;
    HuffQ1[1][12].Code =   4; HuffQ1[1][12].Length = 4;
    HuffQ1[1][13].Code =   1; HuffQ1[1][13].Length = 1;
    HuffQ1[1][14].Code =   5; HuffQ1[1][14].Length = 4;
    HuffQ1[1][15].Code =  63; HuffQ1[1][15].Length = 7;
    HuffQ1[1][16].Code =   1; HuffQ1[1][16].Length = 4;
    HuffQ1[1][17].Code =  59; HuffQ1[1][17].Length = 7;
    HuffQ1[1][18].Code = 226; HuffQ1[1][18].Length = 9;
    HuffQ1[1][19].Code =  57; HuffQ1[1][19].Length = 7;
    HuffQ1[1][20].Code = 100; HuffQ1[1][20].Length = 8;
    HuffQ1[1][21].Code =  53; HuffQ1[1][21].Length = 7;
    HuffQ1[1][22].Code =   2; HuffQ1[1][22].Length = 4;
    HuffQ1[1][23].Code =  54; HuffQ1[1][23].Length = 7;
    HuffQ1[1][24].Code = 224; HuffQ1[1][24].Length = 9;
    HuffQ1[1][25].Code =  58; HuffQ1[1][25].Length = 7;
    HuffQ1[1][26].Code = 102; HuffQ1[1][26].Length = 8;

    /********************** 5-step quantizer **************************/
    /********************* 2 bundled samples **************************/
    //less shaped, book 0
    HuffQ2[0][ 0].Code =  89; HuffQ2[0][ 0].Length = 7;
    HuffQ2[0][ 1].Code =  47; HuffQ2[0][ 1].Length = 6;
    HuffQ2[0][ 2].Code =  15; HuffQ2[0][ 2].Length = 5;
    HuffQ2[0][ 3].Code =   0; HuffQ2[0][ 3].Length = 5;
    HuffQ2[0][ 4].Code =  91; HuffQ2[0][ 4].Length = 7;
    HuffQ2[0][ 5].Code =   4; HuffQ2[0][ 5].Length = 5;
    HuffQ2[0][ 6].Code =   6; HuffQ2[0][ 6].Length = 4;
    HuffQ2[0][ 7].Code =  13; HuffQ2[0][ 7].Length = 4;
    HuffQ2[0][ 8].Code =   4; HuffQ2[0][ 8].Length = 4;
    HuffQ2[0][ 9].Code =   5; HuffQ2[0][ 9].Length = 5;
    HuffQ2[0][10].Code =  20; HuffQ2[0][10].Length = 5;
    HuffQ2[0][11].Code =  12; HuffQ2[0][11].Length = 4;
    HuffQ2[0][12].Code =   4; HuffQ2[0][12].Length = 3;
    HuffQ2[0][13].Code =  15; HuffQ2[0][13].Length = 4;
    HuffQ2[0][14].Code =  14; HuffQ2[0][14].Length = 5;
    HuffQ2[0][15].Code =   3; HuffQ2[0][15].Length = 5;
    HuffQ2[0][16].Code =   3; HuffQ2[0][16].Length = 4;
    HuffQ2[0][17].Code =  14; HuffQ2[0][17].Length = 4;
    HuffQ2[0][18].Code =   5; HuffQ2[0][18].Length = 4;
    HuffQ2[0][19].Code =   1; HuffQ2[0][19].Length = 5;
    HuffQ2[0][20].Code =  90; HuffQ2[0][20].Length = 7;
    HuffQ2[0][21].Code =   2; HuffQ2[0][21].Length = 5;
    HuffQ2[0][22].Code =  21; HuffQ2[0][22].Length = 5;
    HuffQ2[0][23].Code =  46; HuffQ2[0][23].Length = 6;
    HuffQ2[0][24].Code =  88; HuffQ2[0][24].Length = 7;

    //more shaped, book 1
    HuffQ2[1][ 0].Code =  921; HuffQ2[1][ 0].Length = 10;
    HuffQ2[1][ 1].Code =  113; HuffQ2[1][ 1].Length =  7;
    HuffQ2[1][ 2].Code =   51; HuffQ2[1][ 2].Length =  6;
    HuffQ2[1][ 3].Code =  231; HuffQ2[1][ 3].Length =  8;
    HuffQ2[1][ 4].Code =  922; HuffQ2[1][ 4].Length = 10;
    HuffQ2[1][ 5].Code =  104; HuffQ2[1][ 5].Length =  7;
    HuffQ2[1][ 6].Code =   30; HuffQ2[1][ 6].Length =  5;
    HuffQ2[1][ 7].Code =    0; HuffQ2[1][ 7].Length =  3;
    HuffQ2[1][ 8].Code =   29; HuffQ2[1][ 8].Length =  5;
    HuffQ2[1][ 9].Code =  105; HuffQ2[1][ 9].Length =  7;
    HuffQ2[1][10].Code =   50; HuffQ2[1][10].Length =  6;
    HuffQ2[1][11].Code =    1; HuffQ2[1][11].Length =  3;
    HuffQ2[1][12].Code =    2; HuffQ2[1][12].Length =  2;
    HuffQ2[1][13].Code =    3; HuffQ2[1][13].Length =  3;
    HuffQ2[1][14].Code =   49; HuffQ2[1][14].Length =  6;
    HuffQ2[1][15].Code =  107; HuffQ2[1][15].Length =  7;
    HuffQ2[1][16].Code =   27; HuffQ2[1][16].Length =  5;
    HuffQ2[1][17].Code =    2; HuffQ2[1][17].Length =  3;
    HuffQ2[1][18].Code =   31; HuffQ2[1][18].Length =  5;
    HuffQ2[1][19].Code =  112; HuffQ2[1][19].Length =  7;
    HuffQ2[1][20].Code =  920; HuffQ2[1][20].Length = 10;
    HuffQ2[1][21].Code =  106; HuffQ2[1][21].Length =  7;
    HuffQ2[1][22].Code =   48; HuffQ2[1][22].Length =  6;
    HuffQ2[1][23].Code =  114; HuffQ2[1][23].Length =  7;
    HuffQ2[1][24].Code =  923; HuffQ2[1][24].Length = 10;

    /********************** 7-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    HuffQ3[0][0].Code = 12; HuffQ3[0][0].Length = 4;
    HuffQ3[0][1].Code =  4; HuffQ3[0][1].Length = 3;
    HuffQ3[0][2].Code =  0; HuffQ3[0][2].Length = 2;
    HuffQ3[0][3].Code =  1; HuffQ3[0][3].Length = 2;
    HuffQ3[0][4].Code =  7; HuffQ3[0][4].Length = 3;
    HuffQ3[0][5].Code =  5; HuffQ3[0][5].Length = 3;
    HuffQ3[0][6].Code = 13; HuffQ3[0][6].Length = 4;

    //more shaped, book 1
    HuffQ3[1][0].Code = 4; HuffQ3[1][0].Length = 5;
    HuffQ3[1][1].Code = 3; HuffQ3[1][1].Length = 4;
    HuffQ3[1][2].Code = 2; HuffQ3[1][2].Length = 2;
    HuffQ3[1][3].Code = 3; HuffQ3[1][3].Length = 2;
    HuffQ3[1][4].Code = 1; HuffQ3[1][4].Length = 2;
    HuffQ3[1][5].Code = 0; HuffQ3[1][5].Length = 3;
    HuffQ3[1][6].Code = 5; HuffQ3[1][6].Length = 5;

    /********************** 9-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    HuffQ4[0][0].Code = 5; HuffQ4[0][0].Length = 4;
    HuffQ4[0][1].Code = 0; HuffQ4[0][1].Length = 3;
    HuffQ4[0][2].Code = 4; HuffQ4[0][2].Length = 3;
    HuffQ4[0][3].Code = 6; HuffQ4[0][3].Length = 3;
    HuffQ4[0][4].Code = 7; HuffQ4[0][4].Length = 3;
    HuffQ4[0][5].Code = 5; HuffQ4[0][5].Length = 3;
    HuffQ4[0][6].Code = 3; HuffQ4[0][6].Length = 3;
    HuffQ4[0][7].Code = 1; HuffQ4[0][7].Length = 3;
    HuffQ4[0][8].Code = 4; HuffQ4[0][8].Length = 4;

    //more shaped, book 1
    HuffQ4[1][0].Code =  9; HuffQ4[1][0].Length = 5;
    HuffQ4[1][1].Code = 12; HuffQ4[1][1].Length = 4;
    HuffQ4[1][2].Code =  3; HuffQ4[1][2].Length = 3;
    HuffQ4[1][3].Code =  0; HuffQ4[1][3].Length = 2;
    HuffQ4[1][4].Code =  2; HuffQ4[1][4].Length = 2;
    HuffQ4[1][5].Code =  7; HuffQ4[1][5].Length = 3;
    HuffQ4[1][6].Code = 13; HuffQ4[1][6].Length = 4;
    HuffQ4[1][7].Code =  5; HuffQ4[1][7].Length = 4;
    HuffQ4[1][8].Code =  8; HuffQ4[1][8].Length = 5;

    /********************* 15-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    HuffQ5[0][ 0].Code = 57; HuffQ5[0][ 0].Length = 6;
    HuffQ5[0][ 1].Code = 23; HuffQ5[0][ 1].Length = 5;
    HuffQ5[0][ 2].Code =  8; HuffQ5[0][ 2].Length = 4;
    HuffQ5[0][ 3].Code = 10; HuffQ5[0][ 3].Length = 4;
    HuffQ5[0][ 4].Code = 13; HuffQ5[0][ 4].Length = 4;
    HuffQ5[0][ 5].Code =  0; HuffQ5[0][ 5].Length = 3;
    HuffQ5[0][ 6].Code =  2; HuffQ5[0][ 6].Length = 3;
    HuffQ5[0][ 7].Code =  3; HuffQ5[0][ 7].Length = 3;
    HuffQ5[0][ 8].Code =  1; HuffQ5[0][ 8].Length = 3;
    HuffQ5[0][ 9].Code = 15; HuffQ5[0][ 9].Length = 4;
    HuffQ5[0][10].Code = 12; HuffQ5[0][10].Length = 4;
    HuffQ5[0][11].Code =  9; HuffQ5[0][11].Length = 4;
    HuffQ5[0][12].Code = 29; HuffQ5[0][12].Length = 5;
    HuffQ5[0][13].Code = 22; HuffQ5[0][13].Length = 5;
    HuffQ5[0][14].Code = 56; HuffQ5[0][14].Length = 6;

    //more shaped, book 1
    HuffQ5[1][ 0].Code = 229; HuffQ5[1][ 0].Length = 8;
    HuffQ5[1][ 1].Code =  56; HuffQ5[1][ 1].Length = 6;
    HuffQ5[1][ 2].Code =   7; HuffQ5[1][ 2].Length = 5;
    HuffQ5[1][ 3].Code =   2; HuffQ5[1][ 3].Length = 4;
    HuffQ5[1][ 4].Code =   0; HuffQ5[1][ 4].Length = 3;
    HuffQ5[1][ 5].Code =   3; HuffQ5[1][ 5].Length = 3;
    HuffQ5[1][ 6].Code =   5; HuffQ5[1][ 6].Length = 3;
    HuffQ5[1][ 7].Code =   6; HuffQ5[1][ 7].Length = 3;
    HuffQ5[1][ 8].Code =   4; HuffQ5[1][ 8].Length = 3;
    HuffQ5[1][ 9].Code =   2; HuffQ5[1][ 9].Length = 3;
    HuffQ5[1][10].Code =  15; HuffQ5[1][10].Length = 4;
    HuffQ5[1][11].Code =  29; HuffQ5[1][11].Length = 5;
    HuffQ5[1][12].Code =   6; HuffQ5[1][12].Length = 5;
    HuffQ5[1][13].Code = 115; HuffQ5[1][13].Length = 7;
    HuffQ5[1][14].Code = 228; HuffQ5[1][14].Length = 8;

    /********************* 31-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    HuffQ6[0][ 0].Code =  65; HuffQ6[0][ 0].Length = 7;
    HuffQ6[0][ 1].Code =   6; HuffQ6[0][ 1].Length = 6;
    HuffQ6[0][ 2].Code =  44; HuffQ6[0][ 2].Length = 6;
    HuffQ6[0][ 3].Code =  45; HuffQ6[0][ 3].Length = 6;
    HuffQ6[0][ 4].Code =  59; HuffQ6[0][ 4].Length = 6;
    HuffQ6[0][ 5].Code =  13; HuffQ6[0][ 5].Length = 5;
    HuffQ6[0][ 6].Code =  17; HuffQ6[0][ 6].Length = 5;
    HuffQ6[0][ 7].Code =  19; HuffQ6[0][ 7].Length = 5;
    HuffQ6[0][ 8].Code =  23; HuffQ6[0][ 8].Length = 5;
    HuffQ6[0][ 9].Code =  21; HuffQ6[0][ 9].Length = 5;
    HuffQ6[0][10].Code =  26; HuffQ6[0][10].Length = 5;
    HuffQ6[0][11].Code =  30; HuffQ6[0][11].Length = 5;
    HuffQ6[0][12].Code =   0; HuffQ6[0][12].Length = 4;
    HuffQ6[0][13].Code =   2; HuffQ6[0][13].Length = 4;
    HuffQ6[0][14].Code =   5; HuffQ6[0][14].Length = 4;
    HuffQ6[0][15].Code =   7; HuffQ6[0][15].Length = 4;
    HuffQ6[0][16].Code =   3; HuffQ6[0][16].Length = 4;
    HuffQ6[0][17].Code =   4; HuffQ6[0][17].Length = 4;
    HuffQ6[0][18].Code =  31; HuffQ6[0][18].Length = 5;
    HuffQ6[0][19].Code =  28; HuffQ6[0][19].Length = 5;
    HuffQ6[0][20].Code =  25; HuffQ6[0][20].Length = 5;
    HuffQ6[0][21].Code =  27; HuffQ6[0][21].Length = 5;
    HuffQ6[0][22].Code =  24; HuffQ6[0][22].Length = 5;
    HuffQ6[0][23].Code =  20; HuffQ6[0][23].Length = 5;
    HuffQ6[0][24].Code =  18; HuffQ6[0][24].Length = 5;
    HuffQ6[0][25].Code =  12; HuffQ6[0][25].Length = 5;
    HuffQ6[0][26].Code =   2; HuffQ6[0][26].Length = 5;
    HuffQ6[0][27].Code =  58; HuffQ6[0][27].Length = 6;
    HuffQ6[0][28].Code =  33; HuffQ6[0][28].Length = 6;
    HuffQ6[0][29].Code =   7; HuffQ6[0][29].Length = 6;
    HuffQ6[0][30].Code =  64; HuffQ6[0][30].Length = 7;

    //more shaped, book 1
    HuffQ6[1][ 0].Code = 6472; HuffQ6[1][ 0].Length = 13;
    HuffQ6[1][ 1].Code = 6474; HuffQ6[1][ 1].Length = 13;
    HuffQ6[1][ 2].Code =  808; HuffQ6[1][ 2].Length = 10;
    HuffQ6[1][ 3].Code =  405; HuffQ6[1][ 3].Length =  9;
    HuffQ6[1][ 4].Code =  203; HuffQ6[1][ 4].Length =  8;
    HuffQ6[1][ 5].Code =  102; HuffQ6[1][ 5].Length =  7;
    HuffQ6[1][ 6].Code =   49; HuffQ6[1][ 6].Length =  6;
    HuffQ6[1][ 7].Code =    9; HuffQ6[1][ 7].Length =  5;
    HuffQ6[1][ 8].Code =   15; HuffQ6[1][ 8].Length =  5;
    HuffQ6[1][ 9].Code =   31; HuffQ6[1][ 9].Length =  5;
    HuffQ6[1][10].Code =    2; HuffQ6[1][10].Length =  4;
    HuffQ6[1][11].Code =    6; HuffQ6[1][11].Length =  4;
    HuffQ6[1][12].Code =    8; HuffQ6[1][12].Length =  4;
    HuffQ6[1][13].Code =   11; HuffQ6[1][13].Length =  4;
    HuffQ6[1][14].Code =   13; HuffQ6[1][14].Length =  4;
    HuffQ6[1][15].Code =    0; HuffQ6[1][15].Length =  3;
    HuffQ6[1][16].Code =   14; HuffQ6[1][16].Length =  4;
    HuffQ6[1][17].Code =   10; HuffQ6[1][17].Length =  4;
    HuffQ6[1][18].Code =    9; HuffQ6[1][18].Length =  4;
    HuffQ6[1][19].Code =    5; HuffQ6[1][19].Length =  4;
    HuffQ6[1][20].Code =    3; HuffQ6[1][20].Length =  4;
    HuffQ6[1][21].Code =   30; HuffQ6[1][21].Length =  5;
    HuffQ6[1][22].Code =   14; HuffQ6[1][22].Length =  5;
    HuffQ6[1][23].Code =    8; HuffQ6[1][23].Length =  5;
    HuffQ6[1][24].Code =   48; HuffQ6[1][24].Length =  6;
    HuffQ6[1][25].Code =  103; HuffQ6[1][25].Length =  7;
    HuffQ6[1][26].Code =  201; HuffQ6[1][26].Length =  8;
    HuffQ6[1][27].Code =  200; HuffQ6[1][27].Length =  8;
    HuffQ6[1][28].Code = 1619; HuffQ6[1][28].Length = 11;
    HuffQ6[1][29].Code = 6473; HuffQ6[1][29].Length = 13;
    HuffQ6[1][30].Code = 6475; HuffQ6[1][30].Length = 13;

    /********************* 63-step quantizer **************************/
    /*********************** single samples ***************************/
    //less shaped, book 0
    HuffQ7[0][ 0].Code = 103; HuffQ7[0][ 0].Length = 8;    /* 0.003338  -          01100111  */
    HuffQ7[0][ 1].Code = 153; HuffQ7[0][ 1].Length = 8;    /* 0.003766  -          10011001  */
    HuffQ7[0][ 2].Code = 181; HuffQ7[0][ 2].Length = 8;    /* 0.004715  -          10110101  */
    HuffQ7[0][ 3].Code = 233; HuffQ7[0][ 3].Length = 8;    /* 0.005528  -          11101001  */
    HuffQ7[0][ 4].Code =  64; HuffQ7[0][ 4].Length = 7;    /* 0.006677  -           1000000  */
    HuffQ7[0][ 5].Code =  65; HuffQ7[0][ 5].Length = 7;    /* 0.007041  -           1000001  */
    HuffQ7[0][ 6].Code =  77; HuffQ7[0][ 6].Length = 7;    /* 0.007733  -           1001101  */
    HuffQ7[0][ 7].Code =  81; HuffQ7[0][ 7].Length = 7;    /* 0.008296  -           1010001  */
    HuffQ7[0][ 8].Code =  91; HuffQ7[0][ 8].Length = 7;    /* 0.009295  -           1011011  */
    HuffQ7[0][ 9].Code = 113; HuffQ7[0][ 9].Length = 7;    /* 0.010814  -           1110001  */
    HuffQ7[0][10].Code = 112; HuffQ7[0][10].Length = 7;    /* 0.010807  -           1110000  */
    HuffQ7[0][11].Code =  24; HuffQ7[0][11].Length = 6;    /* 0.012748  -            011000  */
    HuffQ7[0][12].Code =  29; HuffQ7[0][12].Length = 6;    /* 0.013390  -            011101  */
    HuffQ7[0][13].Code =  35; HuffQ7[0][13].Length = 6;    /* 0.014224  -            100011  */
    HuffQ7[0][14].Code =  37; HuffQ7[0][14].Length = 6;    /* 0.015201  -            100101  */
    HuffQ7[0][15].Code =  41; HuffQ7[0][15].Length = 6;    /* 0.016642  -            101001  */
    HuffQ7[0][16].Code =  44; HuffQ7[0][16].Length = 6;    /* 0.017292  -            101100  */
    HuffQ7[0][17].Code =  46; HuffQ7[0][17].Length = 6;    /* 0.018647  -            101110  */
    HuffQ7[0][18].Code =  51; HuffQ7[0][18].Length = 6;    /* 0.020473  -            110011  */
    HuffQ7[0][19].Code =  49; HuffQ7[0][19].Length = 6;    /* 0.020152  -            110001  */
    HuffQ7[0][20].Code =  54; HuffQ7[0][20].Length = 6;    /* 0.021315  -            110110  */
    HuffQ7[0][21].Code =  55; HuffQ7[0][21].Length = 6;    /* 0.021358  -            110111  */
    HuffQ7[0][22].Code =  57; HuffQ7[0][22].Length = 6;    /* 0.021700  -            111001  */
    HuffQ7[0][23].Code =  60; HuffQ7[0][23].Length = 6;    /* 0.022449  -            111100  */
    HuffQ7[0][24].Code =   0; HuffQ7[0][24].Length = 5;    /* 0.023063  -             00000  */
    HuffQ7[0][25].Code =   2; HuffQ7[0][25].Length = 5;    /* 0.023854  -             00010  */
    HuffQ7[0][26].Code =  10; HuffQ7[0][26].Length = 5;    /* 0.025481  -             01010  */
    HuffQ7[0][27].Code =   5; HuffQ7[0][27].Length = 5;    /* 0.024867  -             00101  */
    HuffQ7[0][28].Code =   9; HuffQ7[0][28].Length = 5;    /* 0.025352  -             01001  */
    HuffQ7[0][29].Code =   6; HuffQ7[0][29].Length = 5;    /* 0.025074  -             00110  */
    HuffQ7[0][30].Code =  13; HuffQ7[0][30].Length = 5;    /* 0.025745  -             01101  */
    HuffQ7[0][31].Code =   7; HuffQ7[0][31].Length = 5;    /* 0.025195  -             00111  */
    HuffQ7[0][32].Code =  11; HuffQ7[0][32].Length = 5;    /* 0.025502  -             01011  */
    HuffQ7[0][33].Code =  15; HuffQ7[0][33].Length = 5;    /* 0.026251  -             01111  */
    HuffQ7[0][34].Code =   8; HuffQ7[0][34].Length = 5;    /* 0.025260  -             01000  */
    HuffQ7[0][35].Code =   4; HuffQ7[0][35].Length = 5;    /* 0.024418  -             00100  */
    HuffQ7[0][36].Code =   3; HuffQ7[0][36].Length = 5;    /* 0.023983  -             00011  */
    HuffQ7[0][37].Code =   1; HuffQ7[0][37].Length = 5;    /* 0.023697  -             00001  */
    HuffQ7[0][38].Code =  63; HuffQ7[0][38].Length = 6;    /* 0.023041  -            111111  */
    HuffQ7[0][39].Code =  62; HuffQ7[0][39].Length = 6;    /* 0.022656  -            111110  */
    HuffQ7[0][40].Code =  61; HuffQ7[0][40].Length = 6;    /* 0.022549  -            111101  */
    HuffQ7[0][41].Code =  53; HuffQ7[0][41].Length = 6;    /* 0.021151  -            110101  */
    HuffQ7[0][42].Code =  59; HuffQ7[0][42].Length = 6;    /* 0.022042  -            111011  */
    HuffQ7[0][43].Code =  52; HuffQ7[0][43].Length = 6;    /* 0.020837  -            110100  */
    HuffQ7[0][44].Code =  48; HuffQ7[0][44].Length = 6;    /* 0.019446  -            110000  */
    HuffQ7[0][45].Code =  47; HuffQ7[0][45].Length = 6;    /* 0.019189  -            101111  */
    HuffQ7[0][46].Code =  43; HuffQ7[0][46].Length = 6;    /* 0.017177  -            101011  */
    HuffQ7[0][47].Code =  42; HuffQ7[0][47].Length = 6;    /* 0.017035  -            101010  */
    HuffQ7[0][48].Code =  39; HuffQ7[0][48].Length = 6;    /* 0.015287  -            100111  */
    HuffQ7[0][49].Code =  36; HuffQ7[0][49].Length = 6;    /* 0.014559  -            100100  */
    HuffQ7[0][50].Code =  33; HuffQ7[0][50].Length = 6;    /* 0.014117  -            100001  */
    HuffQ7[0][51].Code =  28; HuffQ7[0][51].Length = 6;    /* 0.012776  -            011100  */
    HuffQ7[0][52].Code = 117; HuffQ7[0][52].Length = 7;    /* 0.011107  -           1110101  */
    HuffQ7[0][53].Code = 101; HuffQ7[0][53].Length = 7;    /* 0.010636  -           1100101  */
    HuffQ7[0][54].Code = 100; HuffQ7[0][54].Length = 7;    /* 0.009751  -           1100100  */
    HuffQ7[0][55].Code =  80; HuffQ7[0][55].Length = 7;    /* 0.008132  -           1010000  */
    HuffQ7[0][56].Code =  69; HuffQ7[0][56].Length = 7;    /* 0.007091  -           1000101  */
    HuffQ7[0][57].Code =  68; HuffQ7[0][57].Length = 7;    /* 0.007084  -           1000100  */
    HuffQ7[0][58].Code =  50; HuffQ7[0][58].Length = 7;    /* 0.006277  -           0110010  */
    HuffQ7[0][59].Code = 232; HuffQ7[0][59].Length = 8;    /* 0.005386  -          11101000  */
    HuffQ7[0][60].Code = 180; HuffQ7[0][60].Length = 8;    /* 0.004408  -          10110100  */
    HuffQ7[0][61].Code = 152; HuffQ7[0][61].Length = 8;    /* 0.003759  -          10011000  */
    HuffQ7[0][62].Code = 102; HuffQ7[0][62].Length = 8;    /* 0.003160  -          01100110  */

    //more shaped, book 1
    HuffQ7[1][ 0].Code = 14244; HuffQ7[1][ 0].Length = 14;    /* 0.000059  -        11011110100100  */
    HuffQ7[1][ 1].Code = 14253; HuffQ7[1][ 1].Length = 14;    /* 0.000098  -        11011110101101  */
    HuffQ7[1][ 2].Code = 14246; HuffQ7[1][ 2].Length = 14;    /* 0.000078  -        11011110100110  */
    HuffQ7[1][ 3].Code = 14254; HuffQ7[1][ 3].Length = 14;    /* 0.000111  -        11011110101110  */
    HuffQ7[1][ 4].Code =  3562; HuffQ7[1][ 4].Length = 12;    /* 0.000320  -          110111101010  */
    HuffQ7[1][ 5].Code =   752; HuffQ7[1][ 5].Length = 10;    /* 0.000920  -            1011110000  */
    HuffQ7[1][ 6].Code =   753; HuffQ7[1][ 6].Length = 10;    /* 0.001057  -            1011110001  */
    HuffQ7[1][ 7].Code =   160; HuffQ7[1][ 7].Length =  9;    /* 0.001403  -             010100000  */
    HuffQ7[1][ 8].Code =   162; HuffQ7[1][ 8].Length =  9;    /* 0.001579  -             010100010  */
    HuffQ7[1][ 9].Code =   444; HuffQ7[1][ 9].Length =  9;    /* 0.002486  -             110111100  */
    HuffQ7[1][10].Code =   122; HuffQ7[1][10].Length =  8;    /* 0.003772  -              01111010  */
    HuffQ7[1][11].Code =   223; HuffQ7[1][11].Length =  8;    /* 0.005710  -              11011111  */
    HuffQ7[1][12].Code =    60; HuffQ7[1][12].Length =  7;    /* 0.006858  -               0111100  */
    HuffQ7[1][13].Code =    73; HuffQ7[1][13].Length =  7;    /* 0.008033  -               1001001  */
    HuffQ7[1][14].Code =   110; HuffQ7[1][14].Length =  7;    /* 0.009827  -               1101110  */
    HuffQ7[1][15].Code =    14; HuffQ7[1][15].Length =  6;    /* 0.012601  -                001110  */
    HuffQ7[1][16].Code =    24; HuffQ7[1][16].Length =  6;    /* 0.013194  -                011000  */
    HuffQ7[1][17].Code =    25; HuffQ7[1][17].Length =  6;    /* 0.013938  -                011001  */
    HuffQ7[1][18].Code =    34; HuffQ7[1][18].Length =  6;    /* 0.015693  -                100010  */
    HuffQ7[1][19].Code =    37; HuffQ7[1][19].Length =  6;    /* 0.017846  -                100101  */
    HuffQ7[1][20].Code =    54; HuffQ7[1][20].Length =  6;    /* 0.020078  -                110110  */
    HuffQ7[1][21].Code =     3; HuffQ7[1][21].Length =  5;    /* 0.022975  -                 00011  */
    HuffQ7[1][22].Code =     9; HuffQ7[1][22].Length =  5;    /* 0.025631  -                 01001  */
    HuffQ7[1][23].Code =    11; HuffQ7[1][23].Length =  5;    /* 0.027021  -                 01011  */
    HuffQ7[1][24].Code =    16; HuffQ7[1][24].Length =  5;    /* 0.031465  -                 10000  */
    HuffQ7[1][25].Code =    19; HuffQ7[1][25].Length =  5;    /* 0.034244  -                 10011  */
    HuffQ7[1][26].Code =    21; HuffQ7[1][26].Length =  5;    /* 0.035921  -                 10101  */
    HuffQ7[1][27].Code =    24; HuffQ7[1][27].Length =  5;    /* 0.037938  -                 11000  */
    HuffQ7[1][28].Code =    26; HuffQ7[1][28].Length =  5;    /* 0.039595  -                 11010  */
    HuffQ7[1][29].Code =    29; HuffQ7[1][29].Length =  5;    /* 0.041546  -                 11101  */
    HuffQ7[1][30].Code =    31; HuffQ7[1][30].Length =  5;    /* 0.042623  -                 11111  */
    HuffQ7[1][31].Code =     2; HuffQ7[1][31].Length =  4;    /* 0.045180  -                  0010  */
    HuffQ7[1][32].Code =     0; HuffQ7[1][32].Length =  4;    /* 0.043151  -                  0000  */
    HuffQ7[1][33].Code =    30; HuffQ7[1][33].Length =  5;    /* 0.042538  -                 11110  */
    HuffQ7[1][34].Code =    28; HuffQ7[1][34].Length =  5;    /* 0.041422  -                 11100  */
    HuffQ7[1][35].Code =    25; HuffQ7[1][35].Length =  5;    /* 0.039145  -                 11001  */
    HuffQ7[1][36].Code =    22; HuffQ7[1][36].Length =  5;    /* 0.036691  -                 10110  */
    HuffQ7[1][37].Code =    20; HuffQ7[1][37].Length =  5;    /* 0.034955  -                 10100  */
    HuffQ7[1][38].Code =    14; HuffQ7[1][38].Length =  5;    /* 0.029155  -                 01110  */
    HuffQ7[1][39].Code =    13; HuffQ7[1][39].Length =  5;    /* 0.027921  -                 01101  */
    HuffQ7[1][40].Code =     8; HuffQ7[1][40].Length =  5;    /* 0.025553  -                 01000  */
    HuffQ7[1][41].Code =     6; HuffQ7[1][41].Length =  5;    /* 0.023093  -                 00110  */
    HuffQ7[1][42].Code =     2; HuffQ7[1][42].Length =  5;    /* 0.021200  -                 00010  */
    HuffQ7[1][43].Code =    46; HuffQ7[1][43].Length =  6;    /* 0.018134  -                101110  */
    HuffQ7[1][44].Code =    35; HuffQ7[1][44].Length =  6;    /* 0.015824  -                100011  */
    HuffQ7[1][45].Code =    31; HuffQ7[1][45].Length =  6;    /* 0.014701  -                011111  */
    HuffQ7[1][46].Code =    21; HuffQ7[1][46].Length =  6;    /* 0.013187  -                010101  */
    HuffQ7[1][47].Code =    15; HuffQ7[1][47].Length =  6;    /* 0.012776  -                001111  */
    HuffQ7[1][48].Code =    95; HuffQ7[1][48].Length =  7;    /* 0.009664  -               1011111  */
    HuffQ7[1][49].Code =    72; HuffQ7[1][49].Length =  7;    /* 0.007922  -               1001000  */
    HuffQ7[1][50].Code =    41; HuffQ7[1][50].Length =  7;    /* 0.006838  -               0101001  */
    HuffQ7[1][51].Code =   189; HuffQ7[1][51].Length =  8;    /* 0.005024  -              10111101  */
    HuffQ7[1][52].Code =   123; HuffQ7[1][52].Length =  8;    /* 0.003830  -              01111011  */
    HuffQ7[1][53].Code =   377; HuffQ7[1][53].Length =  9;    /* 0.002232  -             101111001  */
    HuffQ7[1][54].Code =   161; HuffQ7[1][54].Length =  9;    /* 0.001566  -             010100001  */
    HuffQ7[1][55].Code =   891; HuffQ7[1][55].Length = 10;    /* 0.001383  -            1101111011  */
    HuffQ7[1][56].Code =   327; HuffQ7[1][56].Length = 10;    /* 0.000900  -            0101000111  */
    HuffQ7[1][57].Code =   326; HuffQ7[1][57].Length = 10;    /* 0.000790  -            0101000110  */
    HuffQ7[1][58].Code =  3560; HuffQ7[1][58].Length = 12;    /* 0.000254  -          110111101000  */
    HuffQ7[1][59].Code = 14255; HuffQ7[1][59].Length = 14;    /* 0.000117  -        11011110101111  */
    HuffQ7[1][60].Code = 14247; HuffQ7[1][60].Length = 14;    /* 0.000085  -        11011110100111  */
    HuffQ7[1][61].Code = 14252; HuffQ7[1][61].Length = 14;    /* 0.000085  -        11011110101100  */
    HuffQ7[1][62].Code = 14245; HuffQ7[1][62].Length = 14;    /* 0.000065  -        11011110100101  */
}
