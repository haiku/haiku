#ifndef _in_mpc_h_
#define _in_mpc_h_

#include <SupportDefs.h>
#include <sys/types.h>

#include "idtag.h"

class BPositionIO;

/************************ DEFINES *************************/
#define VERSION             "0.97f"

// error codes
#define ERROR_CODE_OK            0
#define ERROR_CODE_FILE         -1
#define ERROR_CODE_SV7BETA       1
#define ERROR_CODE_CBR           2
#define ERROR_CODE_IS            3
#define ERROR_CODE_BLOCKSIZE     4
#define ERROR_CODE_INVALIDSV     5

/************************ TYPEDEF *************************/
typedef struct {
  char*               Item;               // Name of item     (ASCII)
  char*               Value;              // Value of item    (UTF-8)
  unsigned int        Flags;              // Flags
  size_t              ItemSize;           // Length of name   (bytes)
  size_t              ValueSize;          // Length of value  (bytes)
  enum tag_rel        Reliability;        // Reliability of information
} TagItem;

class StreamInfo {
private:
  char *filename;

public: // ahem fixme
  typedef long off_t;

  typedef struct {
    double              SampleFreq;
    unsigned int        Channels;
    off_t               HeaderPosition;     // byte position of header
    unsigned int        StreamVersion;      // Streamversion of current file
    unsigned int        Bitrate;            // bitrate of current file (bps)
    double              AverageBitrate;     // Average bitrate in bits/sec
    unsigned int        Frames;             // number of frames contained
    int64               PCMSamples;
    unsigned int        MaxBand;            // maximum band-index used (0...31)
    unsigned int        IS;                 // Intensity Stereo (0: off, 1: on)
    unsigned int        MS;                 // Mid/Side Stereo (0: off, 1: on)
    unsigned int        BlockSize;          // only needed for SV4...SV6 -> not supported
    unsigned int        Profile;            // quality profile
    const char*         ProfileName;

    // ReplayGain related data
    short               GainTitle;
    short               GainAlbum;
    unsigned short      PeakAlbum;
    unsigned short      PeakTitle;

    // true gapless stuff
    unsigned int        IsTrueGapless;      // is true gapless used?
    unsigned int        LastFrameSamples;   // number of valid samples within last frame

    unsigned int        EncoderVersion;     // version of encoder used
    char                Encoder[256];

    off_t TagOffset;
    off_t TotalFileLength;
  } BasicData;

  BasicData           simple;
  size_t              tagitem_count;
  TagItem*            tagitems;
  enum tag_t          tagtype;

public:
  StreamInfo ();
  ~StreamInfo () { Clear (); }
  void SetFilename ( const char * );
  const char *GetFilename ();
  void Clear ();
  int ReadStreamInfo (BPositionIO *stream);

private:
  //fixme: add writetag here too
  int ReadHeaderSV6 ( BPositionIO *file );
  int ReadHeaderSV7 ( BPositionIO *file );
  int ReadHeaderSV8 ( BPositionIO *file );
  int ReadTags      ( BPositionIO *file );                         // reads all tags from file
  int ReadID3v1Tag  ( BPositionIO *file );                         // reads ID3v1 tag
  int ReadAPE1Tag   ( BPositionIO *file );                         // reads APE 1.0 tag
  int ReadAPE2Tag   ( BPositionIO *file );                         // reads APE 2.0 tag
};

typedef struct {
  int                 EQbyMPC;
  int                 EQdB;
  unsigned int        UpdateBitrate;      // 0 = average, >0 average over such a number of frames
  int                 MaxBrokenFrames;
  int                 SkipTrackOnError;
  int                 ReplayGainMode;
  int                 ReplayGainHeadroom;
  int                 DitherUsed;
} SettingsMPC;

/************************ GLOBALS *************************/
//extern SettingsMPC PluginSettings;

#endif
