
// Klemm settings
#define OTHER_SEEK

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <malloc.h>
#include "minimax.h"
#include "mpc_dec.h"
#include "requant.h"
#include "huffsv46.h"
#include "huffsv7.h"
#include "synth_filter.h"
#include "bitstream.h"
#include "in_mpc.h"

#ifndef M_PI
#	define M_PI             3.141592653589793238462643383276
#endif

//#include "..\cnv_mpc.h"

void
MPC_decoder::RESET_Synthesis ( void )
{
    Reset_V ();
}

void
MPC_decoder::RESET_Y ( void )
{
    memset ( Y_L, 0, sizeof Y_L );
    memset ( Y_R, 0, sizeof Y_R );
}

void
MPC_decoder::RESET_Globals ( void )
{
    Reset_BitstreamDecode ();

    DecodedFrames  = 0;
    LastFrame      = 0;
    LastBitsRead   = 0;
    StreamVersion  = 0;
    MS_used        = 0;
    clips          = 0;
    SectionBitrate = 0;
    NumberOfConsecutiveBrokenFrames = 0;

    memset ( Y_L             , 0, sizeof Y_L              );
    memset ( Y_R             , 0, sizeof Y_R              );
    memset ( SCF_Index_L     , 0, sizeof SCF_Index_L      );
    memset ( SCF_Index_R     , 0, sizeof SCF_Index_R      );
    memset ( Res_L           , 0, sizeof Res_L            );
    memset ( Res_R           , 0, sizeof Res_R            );
    memset ( SCFI_L          , 0, sizeof SCFI_L           );
    memset ( SCFI_R          , 0, sizeof SCFI_R           );
    memset ( DSCF_Flag_L     , 0, sizeof DSCF_Flag_L      );
    memset ( DSCF_Flag_R     , 0, sizeof DSCF_Flag_R      );
    memset ( DSCF_Reference_L, 0, sizeof DSCF_Reference_L );
    memset ( DSCF_Reference_R, 0, sizeof DSCF_Reference_R );
    memset ( Q               , 0, sizeof Q                );
    memset ( MS_Flag         , 0, sizeof MS_Flag          );
}

void
MPC_decoder::perform_EQ ( void )
{
#if 0
    /*static*/ float  SAVE_L     [DELAY] [32];                      // buffer for ...
    /*static*/ float  SAVE_R     [DELAY] [32];                      // ... upper subbands
    /*static*/ float  FirSave_L  [FIR_BANDS] [EQ_TAP];              // buffer for ...
    /*static*/ float  FirSave_R  [FIR_BANDS] [EQ_TAP];              // ... lowest subbands
#endif
    float         lowestSB_L [FIR_BANDS] [36];
    float         lowestSB_R [FIR_BANDS] [36];
    float         SWAP       [DELAY] [32];
    int           n;
    int           k;
    int           i;
    float         tmp;

    for ( i = 0; i < FIR_BANDS; i++ )                           // L: delay subbands (synchronize FIR-filtered and gained subbands-samples)
        for ( k = 0; k < 36; k++ )
            lowestSB_L [i] [k] = Y_L [k] [i];

    memcpy  ( SWAP,       SAVE_L,       DELAY        * 32 * sizeof(float) );
    memcpy  ( SAVE_L,     Y_L+36-DELAY, DELAY        * 32 * sizeof(float) );
    memmove ( Y_L+DELAY,  Y_L,          (36 - DELAY) * 32 * sizeof(float) );
    memcpy  ( Y_L,        SWAP,         DELAY        * 32 * sizeof(float) );

    for ( i = 0; i < FIR_BANDS; i++ )                           // R: delay subbands (synchronize FIR-filtered and gained subbands-samples)
        for ( k = 0; k < 36; k++ )
            lowestSB_R [i] [k] = Y_R [k] [i];

    memcpy  ( SWAP,       SAVE_R,       DELAY        * 32 * sizeof(float) );
    memcpy  ( SAVE_R,     Y_R+36-DELAY, DELAY        * 32 * sizeof(float) );
    memmove ( Y_R+DELAY,  Y_R,          (36 - DELAY) * 32 * sizeof(float) );
    memcpy  ( Y_R,        SWAP,         DELAY        * 32 * sizeof(float) );

    for ( k = 0; k < 36; k++ ) {                                // apply global EQ to upper subbands
        for ( n = FIR_BANDS; n <= Max_Band; n++ ) {
            Y_L [k] [n] *= EQ_gain [n-FIR_BANDS];
            Y_R [k] [n] *= EQ_gain [n-FIR_BANDS];
        }
    }

    for ( i = 0; i < FIR_BANDS; i++ ) {                         // apply FIR to lower subbands for each channel
        for ( k = 0; k < EQ_TAP; k++ ) {                        // L: perform filter for lowest subbands
            tmp = 0.;
            for ( n = 0; n < EQ_TAP - k; n++ )
                tmp += FirSave_L  [i] [k+n]        * EQ_Filter [i] [n];
            for ( ; n < EQ_TAP; n++ )
                tmp += lowestSB_L [i] [k+n-EQ_TAP] * EQ_Filter [i] [n];
            Y_L [k] [i] = tmp;
        }
        for ( ; k < 36; k++ ) {                                 // L: perform filter for lowest subbands
            tmp = 0.;
            for ( n = 0; n < EQ_TAP; n++ )
                tmp += lowestSB_L [i] [k+n-EQ_TAP] * EQ_Filter [i] [n];
            Y_L [k] [i] = tmp;
        }
        for ( n = 0; n < EQ_TAP; n++ )
            FirSave_L [i] [n] = lowestSB_L [i] [36-EQ_TAP+n];

        for ( k = 0; k < EQ_TAP; k++ ) {                        // R: perform filter for lowest subbands
            tmp = 0.;
            for ( n = 0; n < EQ_TAP - k; n++ )
                tmp += FirSave_R  [i] [k+n]        * EQ_Filter [i] [n];
            for ( ; n < EQ_TAP; n++ )
                tmp += lowestSB_R [i] [k+n-EQ_TAP] * EQ_Filter [i] [n];
            Y_R [k] [i] = tmp;
        }
        for ( ; k < 36; k++ ) {                                 // R: perform filter for lowest subbands
            tmp = 0.;
            for ( n = 0; n < EQ_TAP; n++ )
                tmp += lowestSB_R [i] [k+n-EQ_TAP] * EQ_Filter [i] [n];
            Y_R [k] [i] = tmp;
        }
        for ( n = 0; n < EQ_TAP; n++ )
            FirSave_R [i] [n] = lowestSB_R [i] [36-EQ_TAP+n];
    }

    return;
}

int
MPC_decoder::DECODE ( MPC_SAMPLE_FORMAT *buffer )
{
    unsigned int  FrameBitCnt = 0;

    if ( DecodedFrames >= OverallFrames )
        return 0;                           // end of file -> abort decoding

    // read jump-info for validity check of frame
    FwdJumpInfo  = Bitstream_read (20);
    SeekTable [DecodedFrames] = 20 + FwdJumpInfo;       // ...

    ActDecodePos = (Zaehler << 5) + pos;

    // decode data and check for validity of frame
    FrameBitCnt = BitsRead ();
    switch ( StreamVersion ) {
    case 0x04:
    case 0x05:
    case 0x06:
        Lese_Bitstrom_SV6 ();
        break;
    case 0x07:
    case 0x17:
        Lese_Bitstrom_SV7 ();
        break;
    default:
        return 0;
    }
    FrameWasValid = BitsRead() - FrameBitCnt == FwdJumpInfo;

    // synthesize signal
    Requantisierung ( Max_Band );
    //if ( EQ_activated && PluginSettings.EQbyMPC )
    //    perform_EQ ();
#if   MPC_DECODE == TO_FLOAT

    Synthese_Filter_float ( buffer );

#elif MPC_DECODE == TO_PCM

#if 0
    if ( cfg_dither /*PluginSettings.DitherUsed*/ )
        Synthese_Filter_dithered ( (short*)buffer );
    else
        Synthese_Filter_opt ( (short*)buffer );
#endif
    Synthese_Filter_dithered ( buffer );

#else
#	error Define MPC_DECODE to either TO_FLOAT or TO_PCM
#endif
    // (PluginSettings.DitherUsed  ?  this->Synthese_Filter_dithered  :  this->Synthese_Filter_opt) ( (short*)buffer );

    DecodedFrames += 1;

    // cut off first SYNTH_DELAY zero-samples
    if ( DecodedFrames == 0 + 1 ) {
        //memmove ( buffer, buffer + SYNTH_DELAY * 4, (FRAMELEN - SYNTH_DELAY) * 4 );
        //return (FRAMELEN - 481) * 4;

        //memmove ( buffer, buffer + SYNTH_DELAY * 2, (FRAMELEN - SYNTH_DELAY) * 4*2 );
        memmove ( buffer, buffer + SYNTH_DELAY * 2, (FRAMELEN - SYNTH_DELAY) * 2 * sizeof (MPC_SAMPLE_FORMAT) );
        return (FRAMELEN - 481) * 2;
    }
    else if ( DecodedFrames == OverallFrames  &&  StreamVersion >= 6 )
    {        // reconstruct exact filelength
        int  mod_block   = Bitstream_read (11);
        int  FilterDecay;

        if (mod_block == 0) mod_block = 1152;                    // Encoder bugfix
        FilterDecay = (mod_block + SYNTH_DELAY) % FRAMELEN;

        // additional FilterDecay samples are needed for decay of synthesis filter
        if ( SYNTH_DELAY + mod_block >= FRAMELEN ) {

            // **********************************************************************
            // Rhoades 4/16/2002
            // Commented out are blocks of code which cause gapless playback to fail.
            // Temporary fix...
            // **********************************************************************

            if ( ! TrueGaplessPresent ) {
                RESET_Y ();
            } else {
                //if ( FRAMELEN != LastValidSamples ) {
                    Bitstream_read (20);
                    Lese_Bitstrom_SV7 ();
                    Requantisierung ( Max_Band );
                    //FilterDecay = LastValidSamples;
                //}
                //else {
                    //FilterDecay = 0;
                //}
            }
            //if ( EQ_activated && PluginSettings.EQbyMPC )
            //    perform_EQ ();
#if   MPC_DECODE == TO_FLOAT

            Synthese_Filter_float ( buffer + 2304 );

#elif MPC_DECODE == TO_PCM

#if 0
            if ( cfg_dither /*PluginSettings.DitherUsed*/ ) 
                Synthese_Filter_dithered ( (short*)buffer + 2304 );
            else 
                Synthese_Filter_opt ( (short*)buffer + 2304 );
#endif
            Synthese_Filter_dithered ( buffer + 2304 );

#endif
            // (PluginSettings.DitherUsed  ?  this->Synthese_Filter_dithered  :  this->Synthese_Filter_opt) ( (short*)buffer + 2304);
            // return (FRAMELEN + FilterDecay) * 4;
            return (FRAMELEN + FilterDecay) * 2;
        }
        else {                              // there are only FilterDecay samples needed for this frame
            // return FilterDecay * 4;
            return FilterDecay * 2;
        }
    }
    else {                  // full frame
        // return FRAMELEN * 4;
        return FRAMELEN * 2;
    }
}

void
MPC_decoder::Requantisierung ( const int Last_Band )
{
    int     Band;
    int     n;
    float   facL;
    float   facR;
    float   templ;
    float   tempr;
    float*  YL;
    float*  YR;
    int*    L;
    int*    R;

    // requantization and scaling of subband-samples
    for ( Band = 0; Band <= Last_Band; Band++ ) {   // setting pointers
        YL = Y_L [0] + Band;
        YR = Y_R [0] + Band;
        L  = Q [Band].L;
        R  = Q [Band].R;
        /************************** MS-coded **************************/
        if ( MS_Flag [Band] ) {
            if ( Res_L [Band] ) {
                if ( Res_R [Band] ) {    // M!=0, S!=0
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][0]];
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][0]];
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = *L++ * facL)+(tempr = *R++ * facR);
                        *YR   = templ - tempr;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][1]];
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][1]];
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = *L++ * facL)+(tempr = *R++ * facR);
                        *YR   = templ - tempr;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][2]];
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][2]];
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL   = (templ = *L++ * facL)+(tempr = *R++ * facR);
                        *YR   = templ - tempr;
                    }
                } else {    // M!=0, S==0
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][0]];
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = *L++ * facL;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][1]];
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = *L++ * facL;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][2]];
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = *L++ * facL;
                    }
                }
            } else {
                if (Res_R[Band])    // M==0, S!=0
                {
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][0]];
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = *(R++) * facR);
                    }
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][1]];
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = *(R++) * facR);
                    }
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][2]];
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = - (*YL = *(R++) * facR);
                    }
                } else {    // M==0, S==0
                    for ( n = 0; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = 0.f;
                    }
                }
            }
        }
        /************************** LR-coded **************************/
        else {
            if ( Res_L [Band] ) {
                if ( Res_R [Band] ) {    // L!=0, R!=0
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][0]];
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][0]];
                    for (n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = *L++ * facL;
                        *YR = *R++ * facR;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][1]];
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][1]];
                    for (; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = *L++ * facL;
                        *YR = *R++ * facR;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][2]];
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][2]];
                    for (; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = *L++ * facL;
                        *YR = *R++ * facR;
                    }
                } else {     // L!=0, R==0
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][0]];
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = *L++ * facL;
                        *YR = 0.f;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][1]];
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = *L++ * facL;
                        *YR = 0.f;
                    }
                    facL = Cc[Res_L[Band]] * SCF[(unsigned char)SCF_Index_L[Band][2]];
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = *L++ * facL;
                        *YR = 0.f;
                    }
                }
            }
            else {
                if ( Res_R [Band] ) {    // L==0, R!=0
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][0]];
                    for ( n = 0; n < 12; n++, YL += 32, YR += 32 ) {
                        *YL = 0.f;
                        *YR = *R++ * facR;
                    }
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][1]];
                    for ( ; n < 24; n++, YL += 32, YR += 32 ) {
                        *YL = 0.f;
                        *YR = *R++ * facR;
                    }
                    facR = Cc[Res_R[Band]] * SCF[(unsigned char)SCF_Index_R[Band][2]];
                    for ( ; n < 36; n++, YL += 32, YR += 32 ) {
                        *YL = 0.f;
                        *YR = *R++ * facR;
                    }
                } else {    // L==0, R==0
                    for ( n = 0; n < 36; n++, YL += 32, YR += 32 ) {
                        *YR = *YL = 0.f;
                    }
                }
            }
        }
    }
}

/****************************************** SV 6 ******************************************/
void
MPC_decoder::Lese_Bitstrom_SV6 ( void )
{
    int n,k;
    int Max_used_Band=0;
    HuffmanTyp *Table;
    const HuffmanTyp *x1;
    const HuffmanTyp *x2;
    int *L;
    int *R;
    int *ResL = Res_L;
    int *ResR = Res_R;

    /************************ HEADER **************************/
    ResL = Res_L;
    ResR = Res_R;
    for (n=0; n<=Max_Band; ++n, ++ResL, ++ResR)
    {
        if      (n<11)           Table = Region_A;
        else if (n>=11 && n<=22) Table = Region_B;
        else /*if (n>=23)*/      Table = Region_C;

        *ResL = Q_res[n][Huffman_Decode(Table)];
        if (MS_used)      MS_Flag[n] = Bitstream_read(1);
        *ResR = Q_res[n][Huffman_Decode(Table)];

        // only perform the following procedure up to the maximum non-zero subband
        if (*ResL || *ResR) Max_used_Band = n;
    }

    /************************* SCFI-Bundle *****************************/
    ResL = Res_L;
    ResR = Res_R;
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR) {
        if (*ResL) SCFI_Bundle_read(SCFI_Bundle, &SCFI_L[n], &DSCF_Flag_L[n]);
        if (*ResR) SCFI_Bundle_read(SCFI_Bundle, &SCFI_R[n], &DSCF_Flag_R[n]);
    }

    /***************************** SCFI ********************************/
    ResL = Res_L;
    ResR = Res_R;
    L    = SCF_Index_L[0];
    R    = SCF_Index_R[0];
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR, L+=3, R+=3)
    {
        if (*ResL)
        {
            /*********** DSCF ************/
            if (DSCF_Flag_L[n]==1)
            {
                L[2] = DSCF_Reference_L[n];
                switch (SCFI_L[n])
                {
                case 3:
                    L[0] = L[2] + Huffman_Decode_fast(DSCF_Entropie);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 1:
                    L[0] = L[2] + Huffman_Decode_fast(DSCF_Entropie);
                    L[1] = L[0] + Huffman_Decode_fast(DSCF_Entropie);
                    L[2] = L[1];
                    break;
                case 2:
                    L[0] = L[2] + Huffman_Decode_fast(DSCF_Entropie);
                    L[1] = L[0];
                    L[2] = L[1] + Huffman_Decode_fast(DSCF_Entropie);
                    break;
                case 0:
                    L[0] = L[2] + Huffman_Decode_fast(DSCF_Entropie);
                    L[1] = L[0] + Huffman_Decode_fast(DSCF_Entropie);
                    L[2] = L[1] + Huffman_Decode_fast(DSCF_Entropie);
                    break;
                default:
                    return;
                    break;
                }
            }
            /************ SCF ************/
            else
            {
                switch (SCFI_L[n])
                {
                case 3:
                    L[0] = Bitstream_read(6);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 1:
                    L[0] = Bitstream_read(6);
                    L[1] = Bitstream_read(6);
                    L[2] = L[1];
                    break;
                case 2:
                    L[0] = Bitstream_read(6);
                    L[1] = L[0];
                    L[2] = Bitstream_read(6);
                    break;
                case 0:
                    L[0] = Bitstream_read(6);
                    L[1] = Bitstream_read(6);
                    L[2] = Bitstream_read(6);
                    break;
                default:
                    return;
                    break;
                }
            }
            // update Reference for DSCF
            DSCF_Reference_L[n] = L[2];
        }
        if (*ResR)
        {
            R[2] = DSCF_Reference_R[n];
            /*********** DSCF ************/
            if (DSCF_Flag_R[n]==1)
            {
                switch (SCFI_R[n])
                {
                case 3:
                    R[0] = R[2] + Huffman_Decode_fast(DSCF_Entropie);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 1:
                    R[0] = R[2] + Huffman_Decode_fast(DSCF_Entropie);
                    R[1] = R[0] + Huffman_Decode_fast(DSCF_Entropie);
                    R[2] = R[1];
                    break;
                case 2:
                    R[0] = R[2] + Huffman_Decode_fast(DSCF_Entropie);
                    R[1] = R[0];
                    R[2] = R[1] + Huffman_Decode_fast(DSCF_Entropie);
                    break;
                case 0:
                    R[0] = R[2] + Huffman_Decode_fast(DSCF_Entropie);
                    R[1] = R[0] + Huffman_Decode_fast(DSCF_Entropie);
                    R[2] = R[1] + Huffman_Decode_fast(DSCF_Entropie);
                    break;
                default:
                    return;
                    break;
                }
            }
            /************ SCF ************/
            else
            {
                switch (SCFI_R[n])
                {
                case 3:
                    R[0] = Bitstream_read(6);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 1:
                    R[0] = Bitstream_read(6);
                    R[1] = Bitstream_read(6);
                    R[2] = R[1];
                    break;
                case 2:
                    R[0] = Bitstream_read(6);
                    R[1] = R[0];
                    R[2] = Bitstream_read(6);
                    break;
                case 0:
                    R[0] = Bitstream_read(6);
                    R[1] = Bitstream_read(6);
                    R[2] = Bitstream_read(6);
                    break;
                default:
                    return;
                    break;
                }
            }
            // update Reference for DSCF
            DSCF_Reference_R[n] = R[2];
        }
    }

    /**************************** Samples ****************************/
    ResL = Res_L;
    ResR = Res_R;
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR)
    {
        // setting pointers
        x1 = SampleHuff[*ResL];
        x2 = SampleHuff[*ResR];
        L = Q[n].L;
        R = Q[n].R;

        if (x1!=NULL || x2!=NULL)
            for (k=0; k<36; ++k)
            {
                if (x1 != NULL) *L++ = Huffman_Decode_fast (x1);
                if (x2 != NULL) *R++ = Huffman_Decode_fast (x2);
            }

        if (*ResL>7 || *ResR>7)
            for (k=0; k<36; ++k)
            {
                if (*ResL>7) *L++ = (int)Bitstream_read(Res_bit[*ResL]) - Dc[*ResL];
                if (*ResR>7) *R++ = (int)Bitstream_read(Res_bit[*ResR]) - Dc[*ResR];
            }
    }
}

/****************************************** SV 7 ******************************************/
void
MPC_decoder::Lese_Bitstrom_SV7 ( void )
{
    // these arrays hold decoding results for bundled quantizers (3- and 5-step)
    /*static*/ int idx30[] = { -1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1,-1, 0, 1};
    /*static*/ int idx31[] = { -1,-1,-1, 0, 0, 0, 1, 1, 1,-1,-1,-1, 0, 0, 0, 1, 1, 1,-1,-1,-1, 0, 0, 0, 1, 1, 1};
    /*static*/ int idx32[] = { -1,-1,-1,-1,-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1};
    /*static*/ int idx50[] = { -2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2};
    /*static*/ int idx51[] = { -2,-2,-2,-2,-2,-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2};

    int n,k;
    int Max_used_Band=0;
    const HuffmanTyp *Table;
    int idx;
    int *L   ,*R;
    int *ResL,*ResR;
    unsigned int tmp;

    /***************************** Header *****************************/
    ResL  = Res_L;
    ResR  = Res_R;

    // first subband
    *ResL = Bitstream_read(4);
    *ResR = Bitstream_read(4);
    if (MS_used && !(*ResL==0 && *ResR==0)) MS_Flag[0] = Bitstream_read(1);

    // consecutive subbands
    ++ResL; ++ResR; // increase pointers
    for (n=1; n<=Max_Band; ++n, ++ResL, ++ResR)
    {
        idx   = Huffman_Decode_fast(HuffHdr);
        *ResL = (idx!=4) ? *(ResL-1) + idx : Bitstream_read(4);

        idx   = Huffman_Decode_fast(HuffHdr);
        *ResR = (idx!=4) ? *(ResR-1) + idx : Bitstream_read(4);

        if (MS_used && !(*ResL==0 && *ResR==0)) MS_Flag[n] = Bitstream_read(1);

        // only perform following procedures up to the maximum non-zero subband
        if (*ResL!=0 || *ResR!=0) Max_used_Band = n;
    }
    /****************************** SCFI ******************************/
    L     = SCFI_L;
    R     = SCFI_R;
    ResL  = Res_L;
    ResR  = Res_R;
    for (n=0; n<=Max_used_Band; ++n, ++L, ++R, ++ResL, ++ResR) {
        if (*ResL) *L = Huffman_Decode_faster(HuffSCFI);
        if (*ResR) *R = Huffman_Decode_faster(HuffSCFI);
    }

    /**************************** SCF/DSCF ****************************/
    ResL  = Res_L;
    ResR  = Res_R;
    L     = SCF_Index_L[0];
    R     = SCF_Index_R[0];
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR, L+=3, R+=3) {
        if (*ResL)
        {
            L[2] = DSCF_Reference_L[n];
            switch (SCFI_L[n])
            {
                case 1:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[0] = (idx!=8) ? L[2] + idx : Bitstream_read(6);
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[1] = (idx!=8) ? L[0] + idx : Bitstream_read(6);
                    L[2] = L[1];
                    break;
                case 3:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[0] = (idx!=8) ? L[2] + idx : Bitstream_read(6);
                    L[1] = L[0];
                    L[2] = L[1];
                    break;
                case 2:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[0] = (idx!=8) ? L[2] + idx : Bitstream_read(6);
                    L[1] = L[0];
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[2] = (idx!=8) ? L[1] + idx : Bitstream_read(6);
                    break;
                case 0:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[0] = (idx!=8) ? L[2] + idx : Bitstream_read(6);
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[1] = (idx!=8) ? L[0] + idx : Bitstream_read(6);
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    L[2] = (idx!=8) ? L[1] + idx : Bitstream_read(6);
                    break;
                default:
                    return;
                    break;
            }
            // update Reference for DSCF
            DSCF_Reference_L[n] = L[2];
        }
        if (*ResR)
        {
            R[2] = DSCF_Reference_R[n];
            switch (SCFI_R[n])
            {
                case 1:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[0] = (idx!=8) ? R[2] + idx : Bitstream_read(6);
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[1] = (idx!=8) ? R[0] + idx : Bitstream_read(6);
                    R[2] = R[1];
                    break;
                case 3:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[0] = (idx!=8) ? R[2] + idx : Bitstream_read(6);
                    R[1] = R[0];
                    R[2] = R[1];
                    break;
                case 2:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[0] = (idx!=8) ? R[2] + idx : Bitstream_read(6);
                    R[1] = R[0];
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[2] = (idx!=8) ? R[1] + idx : Bitstream_read(6);
                    break;
                case 0:
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[0] = (idx!=8) ? R[2] + idx : Bitstream_read(6);
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[1] = (idx!=8) ? R[0] + idx : Bitstream_read(6);
                    idx  = Huffman_Decode_fast(HuffDSCF);
                    R[2] = (idx!=8) ? R[1] + idx : Bitstream_read(6);
                    break;
                default:
                    return;
                    break;
            }
            // update Reference for DSCF
            DSCF_Reference_R[n] = R[2];
        }
    }
    /***************************** Samples ****************************/
    ResL = Res_L;
    ResR = Res_R;
    L    = Q[0].L;
    R    = Q[0].R;
    for (n=0; n<=Max_used_Band; ++n, ++ResL, ++ResR, L+=36, R+=36)
    {
        /************** links **************/
        switch (*ResL)
        {
            case  -2: case  -3: case  -4: case  -5: case  -6: case  -7: case  -8: case  -9:
            case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17:
                L += 36;
                break;
            case -1:
                for (k=0; k<36; k++ ) {
                    tmp  = random_int ();
                    *L++ = ((tmp >> 24) & 0xFF) + ((tmp >> 16) & 0xFF) + ((tmp >>  8) & 0xFF) + ((tmp >>  0) & 0xFF) - 510;
                }
                break;
            case 0:
                L += 36;// increase pointer
                break;
            case 1:
                Table = HuffQ[Bitstream_read(1)][1];
                for (k=0; k<12; ++k)
                {
                    idx = Huffman_Decode_fast(Table);
                    *L++ = idx30[idx];
                    *L++ = idx31[idx];
                    *L++ = idx32[idx];
                }
                break;
            case 2:
                Table = HuffQ[Bitstream_read(1)][2];
                for (k=0; k<18; ++k)
                {
                    idx = Huffman_Decode_fast(Table);
                    *L++ = idx50[idx];
                    *L++ = idx51[idx];
                }
                break;
            case 3:
            case 4:
                Table = HuffQ[Bitstream_read(1)][*ResL];
                for (k=0; k<36; ++k)
                    *L++ = Huffman_Decode_faster(Table);
                break;
            case 5:
                Table = HuffQ[Bitstream_read(1)][*ResL];
                for (k=0; k<36; ++k)
                    *L++ = Huffman_Decode_fast(Table);
                break;
            case 6:
            case 7:
                Table = HuffQ[Bitstream_read(1)][*ResL];
                for (k=0; k<36; ++k)
                    *L++ = Huffman_Decode(Table);
                break;
            case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
                tmp = Dc[*ResL];
                for (k=0; k<36; ++k)
                    *L++ = (int)Bitstream_read(Res_bit[*ResL]) - tmp;
                break;
            default:
                return;
        }
        /************** rechts **************/
        switch (*ResR)
        {
            case  -2: case  -3: case  -4: case  -5: case  -6: case  -7: case  -8: case  -9:
            case -10: case -11: case -12: case -13: case -14: case -15: case -16: case -17:
                R += 36;
                break;
            case -1:
                for (k=0; k<36; k++ ) {
                    tmp  = random_int ();
                    *R++ = ((tmp >> 24) & 0xFF) + ((tmp >> 16) & 0xFF) + ((tmp >>  8) & 0xFF) + ((tmp >>  0) & 0xFF) - 510;
                }
                break;
            case 0:
                R += 36;// increase pointer
                break;
            case 1:
                Table = HuffQ[Bitstream_read(1)][1];
                for (k=0; k<12; ++k)
                {
                    idx = Huffman_Decode_fast(Table);
                    *R++ = idx30[idx];
                    *R++ = idx31[idx];
                    *R++ = idx32[idx];
                }
                break;
            case 2:
                Table = HuffQ[Bitstream_read(1)][2];
                for (k=0; k<18; ++k)
                {
                    idx = Huffman_Decode_fast(Table);
                    *R++ = idx50[idx];
                    *R++ = idx51[idx];
                }
                break;
            case 3:
            case 4:
                Table = HuffQ[Bitstream_read(1)][*ResR];
                for (k=0; k<36; ++k)
                    *R++ = Huffman_Decode_faster(Table);
                break;
            case 5:
                Table = HuffQ[Bitstream_read(1)][*ResR];
                for (k=0; k<36; ++k)
                    *R++ = Huffman_Decode_fast(Table);
                break;
            case 6:
            case 7:
                Table = HuffQ[Bitstream_read(1)][*ResR];
                for (k=0; k<36; ++k)
                    *R++ = Huffman_Decode(Table);
                break;
            case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 16: case 17:
                tmp = Dc[*ResR];
                for (k=0; k<36; ++k)
                    *R++ = (int)Bitstream_read(Res_bit[*ResR]) - tmp;
                break;
            default:
                return;
        }
    }
}

MPC_decoder::~MPC_decoder ()
{
  free ( SeekTable );
  //if ( m_reader ) delete m_reader;
	// ^^ breaks wa3
}

MPC_decoder::MPC_decoder(BPositionIO *file)
{
  m_reader = file;

  HuffQ[0][0] = 0;
  HuffQ[1][0] = 0;
  HuffQ[0][1] = HuffQ1[0];
  HuffQ[1][1] = HuffQ1[1];
  HuffQ[0][2] = HuffQ2[0];
  HuffQ[1][2] = HuffQ2[1];
  HuffQ[0][3] = HuffQ3[0];
  HuffQ[1][3] = HuffQ3[1];
  HuffQ[0][4] = HuffQ4[0];
  HuffQ[1][4] = HuffQ4[1];
  HuffQ[0][5] = HuffQ5[0];
  HuffQ[1][5] = HuffQ5[1];
  HuffQ[0][6] = HuffQ6[0];
  HuffQ[1][6] = HuffQ6[1];
  HuffQ[0][7] = HuffQ7[0];
  HuffQ[1][7] = HuffQ7[1];

  SampleHuff[0] = NULL;
  SampleHuff[1] = Entropie_1;
  SampleHuff[2] = Entropie_2;
  SampleHuff[3] = Entropie_3;
  SampleHuff[4] = Entropie_4;
  SampleHuff[5] = Entropie_5;
  SampleHuff[6] = Entropie_6;
  SampleHuff[7] = Entropie_7;
  SampleHuff[8] = NULL;
  SampleHuff[9] = NULL;
  SampleHuff[10] = NULL;
  SampleHuff[11] = NULL;
  SampleHuff[12] = NULL;
  SampleHuff[13] = NULL;
  SampleHuff[14] = NULL;
  SampleHuff[15] = NULL;
  SampleHuff[16] = NULL;
  SampleHuff[17] = NULL;

  SeekTable = NULL;
  EQ_activated = 0;
  MPCHeaderPos = 0;
  StreamVersion = 0;
  MS_used = 0;
  FwdJumpInfo = 0;
  ActDecodePos = 0;
  FrameWasValid = 0;
  OverallFrames = 0;
  DecodedFrames = 0;
  LastFrame = 0;
  LastBitsRead = 0;
  SectionBitrate = 0;
  LastValidSamples = 0;
  TrueGaplessPresent = 0;
  NumberOfConsecutiveBrokenFrames = 0;
  WordsRead = 0;
  Max_Band = 0;
  SampleRate = 0;
  clips = 0;
  __r1 = 1;
  __r2 = 1;

  dword = 0;
  pos = 0;
  Zaehler = 0;
  WordsRead = 0;
  Max_Band = 0;

  memset ( SAVE_L, 0, sizeof (SAVE_L) );
  memset ( SAVE_R, 0, sizeof (SAVE_R) );
  memset ( FirSave_L, 0, sizeof (FirSave_L) );
  memset ( FirSave_R, 0, sizeof (FirSave_R) );

  initialisiere_Quantisierungstabellen ();
  Huffman_SV6_Decoder ();
  Huffman_SV7_Decoder ();
}


void
MPC_decoder::SetStreamInfo(StreamInfo *si)
{
	fInfo = si;
		// this is used in MusePackDecoder::NegotiateOutputFormat()

  StreamVersion      = si->simple.StreamVersion;
  MS_used            = si->simple.MS;
  Max_Band           = si->simple.MaxBand;
  OverallFrames      = si->simple.Frames;
  MPCHeaderPos       = si->simple.HeaderPosition;
  LastValidSamples   = si->simple.LastFrameSamples;
  TrueGaplessPresent = si->simple.IsTrueGapless;
  SectionBitrate     = (int)si->simple.AverageBitrate;
  SampleRate         = (int)si->simple.SampleFreq;

  if ( SeekTable != NULL ) free ( SeekTable );
  SeekTable = (unsigned short *)calloc ( sizeof(unsigned short), OverallFrames+64 );
}

int MPC_decoder::FileInit ()
{
  // AB: setting position to the beginning of the data-bitstream
  switch ( StreamVersion ) {
  case 0x04: f_seek ( 4 + MPCHeaderPos, SEEK_SET); pos = 16; break;  // Geht auch über eine der Helperfunktionen
  case 0x05:
  case 0x06: f_seek ( 8 + MPCHeaderPos, SEEK_SET); pos =  0; break;
  case 0x07:
  case 0x17: f_seek ( 24 + MPCHeaderPos, SEEK_SET); pos =  8; break;
  default: return 0;
  }

  // AB: fill buffer and initialize decoder
  f_read ( Speicher, 4*MEMSIZE );
  dword = Speicher [Zaehler = 0];

  return 1;
}

//---------------------------------------------------------------
// will seek from the beginning of the file to the desired
// position in ms (given by seek_needed)
//---------------------------------------------------------------

void
MPC_decoder::Helper1 ( unsigned long bitpos )
{
    f_seek ( (bitpos>>5) * 4 + MPCHeaderPos, SEEK_SET );
    f_read ( Speicher, sizeof(int)*2 );
    dword = Speicher [ Zaehler = 0];
    pos   = bitpos & 31;
}

void
MPC_decoder::Helper2 ( unsigned long bitpos )
{
    f_seek ( (bitpos>>5) * 4 + MPCHeaderPos, SEEK_SET );
    f_read ( Speicher, sizeof(int) * MEMSIZE );
    dword = Speicher [ Zaehler = 0];
    pos   = bitpos & 31;
}

void
MPC_decoder::Helper3 ( unsigned long bitpos, long* buffoffs )
{
    pos      = bitpos & 31;
    bitpos >>= 5;
    if ( (unsigned long)(bitpos - *buffoffs) >= MEMSIZE-2 ) {
        *buffoffs = bitpos;
        f_seek ( bitpos * 4L + MPCHeaderPos, SEEK_SET );
        f_read ( Speicher, sizeof(int)*MEMSIZE );
    }
    dword = Speicher [ Zaehler = bitpos - *buffoffs ];
}

int
MPC_decoder::perform_jump ( int seek_needed )
{
    unsigned long  fpos;
    unsigned int   FrameBitCnt;
    unsigned int   RING;
    int            fwd;
    unsigned int   decframes;
    unsigned long  buffoffs = 0x80000000;

    switch ( StreamVersion ) {                                                  // setting position to the beginning of the data-bitstream
    case  0x04: fpos =  48; break;
    case  0x05:
    case  0x06: fpos =  64; break;
    case  0x07:
    case  0x17: fpos = 200; break;
    default   : return 0;                                                       // was gibt diese Funktion im Falle eines Fehlers zurück ???
    }

    fwd           = (int) ( seek_needed * (float)SampleRate * 0.001 / FRAMELEN + .5f ); // no of frames to skip
    fwd           = fwd < (int)OverallFrames  ?  fwd  :  (int)OverallFrames;    // prevent from desired position out of allowed range
    decframes     = DecodedFrames;
    DecodedFrames = 0;                                                          // reset number of decoded frames

    if ( fwd > 32 && decframes > 128 ) {                                        // only do proceed, if desired position is not in between the first 32 frames
        for ( ; (int)DecodedFrames + 32 < fwd; DecodedFrames++ ) {              // proceed until 32 frames before (!!) desired position (allows to scan the scalefactors)
            if ( SeekTable [DecodedFrames] == 0 ) {
#ifdef OTHER_SEEK
                Helper3 ( fpos, (long*)&buffoffs );
#else
                Helper1 ( fpos );
#endif
                fpos += SeekTable [DecodedFrames] = 20 + Bitstream_read (20);   // calculate desired pos (in Bits)
            } else {
                fpos += SeekTable [DecodedFrames];
            }
        }
    }
    Helper2 ( fpos );

    for ( ; (int)DecodedFrames < fwd; DecodedFrames++ ) {                       // read the last 32 frames before the desired position to scan the scalefactors (artifactless jumping)
        RING         = Zaehler;
        FwdJumpInfo  = Bitstream_read (20);                                     // read jump-info
        ActDecodePos = (Zaehler << 5) + pos;
        FrameBitCnt  = BitsRead ();                                             // scanning the scalefactors and check for validity of frame
        if (StreamVersion >= 7)  Lese_Bitstrom_SV7 ();
        else Lese_Bitstrom_SV6 ();
        if ( BitsRead() - FrameBitCnt != FwdJumpInfo ) {
//            Box ("Bug in perform_jump");
            return 0;
        }
        if ( (RING ^ Zaehler) & MEMSIZE2 )                                      // update buffer
            f_read ( Speicher + (RING & MEMSIZE2), 4 * MEMSIZE2 );
    }

    RESET_Synthesis ();                                                         // resetting synthesis filter to avoid "clicks"
    LastBitsRead = BitsRead ();
    LastFrame = DecodedFrames;

    return 1;
}

void MPC_decoder::UpdateBuffer ( unsigned int RING )
{
    if ( (RING ^ Zaehler) & MEMSIZE2 )
        f_read ( Speicher + (RING & MEMSIZE2), 4 * MEMSIZE2 );      // update buffer
}

#if 0
void
MPC_decoder::EQSet ( int on, char data[10], int preamp )
{
    /*static*/ const int  specline [10] = { 1, 4, 7, 14, 23, 70, 139, 279, 325, 372 };
    /*static*/ const int  sym           = (EQ_TAP - 1) / 2;
    float       set [512];
    float       x   [512];
    float       mid  [32];
    float       power[10];
    float       win;
    int         i;
    int         n;
    int         k;
    int         idx;

    /*
    The implemented EQ utilizes the given frequencies and interpolates the power level.
    All subbands except the lowest FIR_BANDS subbands will be attenuated by the calculated
    average gain. The first FIR_BANDS subbands will be processed by a EQ_TAP-order FIR-filter,
    because the frequency resolution of the EQ is much higher than the subbands bandwidth.
    */
    EQ_activated = 0;
    if ( on ) {

        // calculate desired attenuation
        for ( n = 0; n < 10; n++ ) {
            power[n]  = (31-(int)data[n]) * PluginSettings.EQdB/32.f;
            power[n] += (31-      preamp) * PluginSettings.EQdB/32.f;
        }

        // calculate desired attenuation for each bin
        set[0] = power[0];
        for ( k =  1; k <  4; k++ ) set [k]= (power[0]*(4  -k) + power[1]*(k-  1)) /  3.f;
        for ( k =  4; k <  7; k++ ) set [k]= (power[1]*(7  -k) + power[2]*(k-  4)) /  3.f;
        for ( k =  7; k < 14; k++ ) set [k]= (power[2]*(14 -k) + power[3]*(k-  7)) /  7.f;
        for ( k = 14; k < 23; k++ ) set [k]= (power[3]*(23 -k) + power[4]*(k- 14)) /  9.f;
        for ( k = 23; k < 70; k++ ) set [k]= (power[4]*(70 -k) + power[5]*(k- 23)) / 47.f;
        for ( k = 70; k <139; k++ ) set [k]= (power[5]*(139-k) + power[6]*(k- 70)) / 69.f;
        for ( k =139; k <279; k++ ) set [k]= (power[6]*(279-k) + power[7]*(k-139)) /140.f;
        for ( k =279; k <325; k++ ) set [k]= (power[7]*(325-k) + power[8]*(k-279)) / 46.f;
        for ( k =325; k <372; k++ ) set [k]= (power[8]*(372-k) + power[9]*(k-325)) / 47.f;
        for ( k =372; k <512; k++ ) set [k]=  power[9];

        // transform from level to power
        for ( k = 0; k < 512; k++ )
            set[k] = (float) pow ( 10., 0.1*set[k] );

        /************************** gain for upper subbands ****************************/
        // transform to attenuation (gain) per subband
        memset ( mid, 0, sizeof(mid) );
        for ( k = 16 * FIR_BANDS; k < 512; k++ )
            mid [k >> 4] += set [k];                                // summarize power
        for (n=FIR_BANDS   ; n<32 ; ++n)
            EQ_gain [n - FIR_BANDS] = (float) sqrt (mid[n] / 16.);  // average gain

        /************************** FIR for lowest subbands ****************************/
        for ( i = 0; i < FIR_BANDS; i++ ) {  // calculate impulse response of FIR via IDFT
            for ( n = 0; n < DELAY; n++ ) {
                x[n] = 0;
                for ( k = 0; k < 16; k++ ) {
                    idx   = i & 1  ?  (i<<4)+15-k  :  (i<<4)+k;   // frequency inversion of every second subband
                    x[n] += (float) ( sqrt (set[idx]) * cos (2*M_PI*n*k/32) );
                }
                x[n] /= 16.;
            }
            // calculate a symmetric EQ_TAP-tap FIR-filter
            for ( n = 0; n < EQ_TAP; n++ ) {
                win             = (float) sin ( (n+1) * M_PI / (EQ_TAP+1) );
                win            *= win;   // Portable, falls win = sin² () die Absicht sein sollte
                EQ_Filter[i][n] =  n <= sym  ?  x[sym-n]*win  :  x[n-sym]*win;
            }
        }
        EQ_activated = 1;
    }

    return;
}
#endif

//---------------------------------------------------------------
// calculates the factor that must be applied to match the
// chosen ReplayGain-mode
//---------------------------------------------------------------
#if 0
float
MPC_decoder::ProcessReplayGain ( int mode, StreamInfo *info )
{
    unsigned char modetab [7] = { 0,4,5,6,7,2,3 };
    char                  message    [800];
    float                 factor_gain;
    float                 factor_clip;
    float                 factor_preamp;
    int                   Gain     = 0;
    int                   Peak     = 0;
    int                   Headroom = 0;
    char*                 p        = message;

    mode = modetab [mode];

    if ( (mode & 1) == 0 )
        Gain = info->simple.GainTitle, Peak = info->simple.PeakTitle;
    else
        Gain = info->simple.GainAlbum, Peak = info->simple.PeakAlbum;

    /*
    if ( PluginSettings.DebugUsed ) {
        p += sprintf ( p, "UsedPeak: %u     UsedGain: %+5.2f\n", Peak, 0.01*Gain );
    }
    */

    if ( (mode & 2) == 0 )
        Gain = 0;

    if ( (mode & 4) == 0 )
        Peak = 0;

    if ( mode == 0 )
        Headroom = 0;
    else
        Headroom = cfg_headroom /*PluginSettings.ReplayGainHeadroom*/ - 14;   // K-2...26, 2 => -12, 26 => +12

    // calculate multipliers for ReplayGain and ClippingPrevention
    factor_preamp = (float) pow (10., -0.05 * Headroom);
    factor_gain   = (float) pow (10., 0.0005 * Gain);

    if ( cfg_clipprotect == FALSE ) {
        factor_clip = 1.f;
    } else {
        factor_clip   = (float) (32767./( Peak + 1 ) );

        if ( factor_preamp * factor_gain < factor_clip )
            factor_clip  = 1.f;
        else
            factor_clip /= factor_preamp * factor_gain;
    }

    return factor_preamp * factor_gain * factor_clip;
}
#endif
