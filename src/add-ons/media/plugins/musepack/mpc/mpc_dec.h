#ifndef _mpp_dec_h_
#define _mpp_dec_h_

#include <DataIO.h>

#ifndef _in_mpc_h_
class StreamInfo;
#endif

#define _inline inline

#define TO_FLOAT    1
#define TO_PCM      2

#ifndef MPC_DECODE
#define MPC_DECODE    TO_FLOAT    // decode to 32 bit floats ( -1.000...+1.000 )
//#define MPC_DECODE    TO_PCM      // decode to 16 bit PCM    ( -32768...+32767 )
#endif


#if   MPC_DECODE == TO_FLOAT
  #define MPC_SAMPLE_FORMAT   float
  #define MPC_SAMPLE_BPS      32
  #define MPC_SAMPLE_TYPE     "FLOAT"
#elif MPC_DECODE == TO_PCM
  #define MPC_SAMPLE_FORMAT   short
  #define MPC_SAMPLE_BPS      16
  #define MPC_SAMPLE_TYPE     "PCM"
#else
  #error MPC_DECODE incorrect or undefined.
#endif


#define EQ_TAP          13                      // length of FIR filter for EQ
#define DELAY           ((EQ_TAP + 1) / 2)      // delay of FIR
#define FIR_BANDS       4                       // number of subbands to be FIR filtered

#define MEMSIZE         16384                   // overall buffer size
#define MEMSIZE2        (MEMSIZE/2)             // size of one buffer
#define MEMMASK         (MEMSIZE-1)
#define V_MEM           2304
#define FRAMELEN        (36 * 32)               // samples per frame
#define SYNTH_DELAY     481


class MPC_decoder {
private:
  BPositionIO *m_reader;

public:
  MPC_decoder(BPositionIO *r);
  ~MPC_decoder();

  void SetStreamInfo ( StreamInfo *si );
  int FileInit ();
  void ScaleOutput ( double factor );
  float ProcessReplayGain ( int mode, StreamInfo *info );
  void perform_EQ ( void );

  int DECODE ( MPC_SAMPLE_FORMAT *buffer );

  void UpdateBuffer ( unsigned int RING );
  int perform_jump ( int seek_needed );

  void RESET_Synthesis ( void );
  void RESET_Globals ( void );

  unsigned int BitsRead ( void );

private:
  void RESET_Y ( void );
  void Lese_Bitstrom_SV6 ( void );
  void Lese_Bitstrom_SV7 ( void );

  void Requantisierung ( const int Last_Band );

public:
  typedef struct {
    int  L [36];
    int  R [36];
  } QuantTyp;

  typedef struct {
    unsigned int  Code;
    unsigned int  Length;
    int           Value;
  } HuffmanTyp;

public:
  unsigned int  Speicher [MEMSIZE];         // read-buffer
  unsigned int  dword;                      // actually decoded 32bit-word
  unsigned int  pos;                        // bit-position within dword
  unsigned int  Zaehler;                    // actual index within read-buffer

  unsigned int  FwdJumpInfo;
  unsigned int  ActDecodePos;
  unsigned int  FrameWasValid;

  unsigned int  DecodedFrames;
  unsigned int  LastFrame;
  unsigned int  LastBitsRead;
  unsigned int  OverallFrames;
  int           SectionBitrate;             // average bitrate for short section
  int           SampleRate;                 // Sample frequency
  int           NumberOfConsecutiveBrokenFrames;  // counter for consecutive broken frames
  StreamInfo    *fInfo;

private:
  unsigned int  StreamVersion;              // version of bitstream
  unsigned int  MS_used;                    // MS-coding used ?
  int           Max_Band;
  unsigned int  MPCHeaderPos;               // AB: needed to support ID3v2
  //unsigned int  OverallFrames;
  //unsigned int  DecodedFrames;
  unsigned int  LastValidSamples;
  unsigned int  TrueGaplessPresent;

  unsigned int  EQ_activated;

  unsigned int  WordsRead;                  // counts amount of decoded dwords
  unsigned short* SeekTable;

  /*static*/ unsigned int  __r1;
  /*static*/ unsigned int  __r2;

  unsigned long clips;

private:
  float         EQ_gain   [ 32 - FIR_BANDS ];
  float         EQ_Filter [ FIR_BANDS ][ EQ_TAP ];

  int           SCF_Index_L [32] [3];
  int           SCF_Index_R [32] [3];       // holds scalefactor-indices
  QuantTyp      Q [32];                     // holds quantized samples
  int           Res_L [32];
  int           Res_R [32];                 // holds the chosen quantizer for each subband
  int           DSCF_Flag_L [32];
  int           DSCF_Flag_R [32];           // differential SCF used?
  int           SCFI_L [32];
  int           SCFI_R [32];                // describes order of transmitted SCF
  int           DSCF_Reference_L [32];
  int           DSCF_Reference_R [32];      // holds last frames SCF
  int           MS_Flag[32];                // MS used?

  float         SAVE_L     [DELAY] [32];    // buffer for ...
  float         SAVE_R     [DELAY] [32];    // ... upper subbands
  float         FirSave_L  [FIR_BANDS] [EQ_TAP];// buffer for ...
  float         FirSave_R  [FIR_BANDS] [EQ_TAP];// ... lowest subbands

  HuffmanTyp    HuffHdr  [10];
  HuffmanTyp    HuffSCFI [ 4];
  HuffmanTyp    HuffDSCF [16];
  HuffmanTyp*   HuffQ [2] [8];

  HuffmanTyp    HuffQ1 [2] [3*3*3];
  HuffmanTyp    HuffQ2 [2] [5*5];
  HuffmanTyp    HuffQ3 [2] [ 7];
  HuffmanTyp    HuffQ4 [2] [ 9];
  HuffmanTyp    HuffQ5 [2] [15];
  HuffmanTyp    HuffQ6 [2] [31];
  HuffmanTyp    HuffQ7 [2] [63];

  const HuffmanTyp* SampleHuff [18];
  HuffmanTyp    SCFI_Bundle   [ 8];
  HuffmanTyp    DSCF_Entropie [13];
  HuffmanTyp    Region_A [16];
  HuffmanTyp    Region_B [ 8];
  HuffmanTyp    Region_C [ 4];

  HuffmanTyp    Entropie_1 [ 3];
  HuffmanTyp    Entropie_2 [ 5];
  HuffmanTyp    Entropie_3 [ 7];
  HuffmanTyp    Entropie_4 [ 9];
  HuffmanTyp    Entropie_5 [15];
  HuffmanTyp    Entropie_6 [31];
  HuffmanTyp    Entropie_7 [63];

  float         V_L [V_MEM + 960];
  float         V_R [V_MEM + 960];
  float         Y_L [36] [32];
  float         Y_R [36] [32];

  float         SCF  [256];                 // holds adapted scalefactors (for clipping prevention)
  unsigned int  Q_bit [32];                 // holds amount of bits to save chosen quantizer (SV6)
  unsigned int  Q_res [32] [16];            // holds conversion: index -> quantizer (SV6)

private: // functions
  void Reset_BitstreamDecode ( void );
  unsigned int Bitstream_read ( const unsigned int );
  int Huffman_Decode ( const HuffmanTyp* );         // works with maximum lengths up to 14
  int Huffman_Decode_fast ( const HuffmanTyp* );    // works with maximum lengths up to 10
  int Huffman_Decode_faster ( const HuffmanTyp* );  // works with maximum lengths up to  5
  void SCFI_Bundle_read ( const HuffmanTyp*, int*, int* );

  void Calculate_New_V ( const float* Sample, float* V );
  void Vectoring_float ( float* Data, const float* V );
  void Vectoring ( short* Data, const float* V );
  void Vectoring_dithered ( short* Data, const float* V, const float* dither );
  void GenerateDither_old ( float* p, size_t len );
  void GenerateDither ( float* p, size_t len );

  void Reset_V ( void );
  void Synthese_Filter_float ( float* dst );
  void Synthese_Filter_opt ( short* dst );
  void Synthese_Filter_dithered ( short* dst );
  unsigned int random_int ( void );

  void EQSet ( int on, char data[10], int preamp );

  void Helper1 ( unsigned long bitpos );
  void Helper2 ( unsigned long bitpos );
  void Helper3 ( unsigned long bitpos, long* buffoffs );

  void Huffman_SV6_Encoder ( void );
  void Huffman_SV6_Decoder ( void );
  void Huffman_SV7_Encoder ( void );
  void Huffman_SV7_Decoder ( void );

  void initialisiere_Quantisierungstabellen ( void );
  void Quantisierungsmodes ( void );        // conversion: index -> quantizer (bitstream reading)
  void Resort_HuffTables ( const unsigned int elements, HuffmanTyp* Table, const int offset );

private:
  _inline int f_read(void *ptr, size_t size) { return m_reader->Read(ptr, size); };
  _inline int f_seek(int offset, int origin) { return m_reader->Seek(offset, origin); };
};

#endif
