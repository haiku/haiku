/*
 * JobData.h
 * Copyright 1999-2000 Y.Takagi. All Rights Reserved.
 */

#ifndef __JOBDATA_H
#define __JOBDATA_H

#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <Rect.h>

class BMessage;
class PrinterCap;

class JobData {
public:
	enum ORIENTATION {
		PORTRAIT,
		LANDSCAPE
	};

	enum PAPER {
		LETTER = 1,             //   1  Letter 8 1/2 x 11 in
		LETTERSMALL,            //   2  Letter Small 8 1/2 x 11 in
		TABLOID,                //   3  Tabloid 11 x 17 in
		LEDGER,                 //   4  Ledger 17 x 11 in
		LEGAL,                  //   5  Legal 8 1/2 x 14 in
		STATEMENT,              //   6  Statement 5 1/2 x 8 1/2 in
		EXECUTIVE,              //   7  Executive 7 1/4 x 10 1/2 in
		A3,                     //   8  A3 297 x 420 mm
		A4,                     //   9  A4 210 x 297 mm
		A4SMALL,                //  10  A4 Small 210 x 297 mm
		A5,                     //  11  A5 148 x 210 mm
		B4,                     //  12  B4 (JIS) 250 x 354
		B5,                     //  13  B5 (JIS) 182 x 257 mm
		FOLIO,                  //  14  Folio 8 1/2 x 13 in
		QUARTO,                 //  15  Quarto 215 x 275 mm
		P_10X14,                //  16  10x14 in
		P_11X17,                //  17  11x17 in
		NOTE,                   //  18  Note 8 1/2 x 11 in
		ENV_9,                  //  19  Envelope #9 3 7/8 x 8 7/8
		ENV_10,                 //  20  Envelope #10 4 1/8 x 9 1/2
		ENV_11,                 //  21  Envelope #11 4 1/2 x 10 3/8
		ENV_12,                 //  22  Envelope #12 4 \276 x 11
		ENV_14,                 //  23  Envelope #14 5 x 11 1/2
		CSHEET,                 //  24  C size sheet
		DSHEET,                 //  25  D size sheet
		ESHEET,                 //  26  E size sheet
		ENV_DL,                 //  27  Envelope DL 110 x 220mm
		ENV_C5,                 //  28  Envelope C5 162 x 229 mm
		ENV_C3,                 //  29  Envelope C3  324 x 458 mm
		ENV_C4,                 //  30  Envelope C4  229 x 324 mm
		ENV_C6,                 //  31  Envelope C6  114 x 162 mm
		ENV_C65,                //  32  Envelope C65 114 x 229 mm
		ENV_B4,                 //  33  Envelope B4  250 x 353 mm
		ENV_B5,                 //  34  Envelope B5  176 x 250 mm
		ENV_B6,                 //  35  Envelope B6  176 x 125 mm
		ENV_ITALY,              //  36  Envelope 110 x 230 mm
		ENV_MONARCH,            //  37  Envelope Monarch 3.875 x 7.5 in
		ENV_PERSONAL,           //  38  6 3/4 Envelope 3 5/8 x 6 1/2 in
		FANFOLD_US,             //  39  US Std Fanfold 14 7/8 x 11 in
		FANFOLD_STD_GERMAN,     //  40  German Std Fanfold 8 1/2 x 12 in
		FANFOLD_LGL_GERMAN,     //  41  German Legal Fanfold 8 1/2 x 13 in
		ISO_B4,                 //  42  B4 (ISO) 250 x 353 mm
		JAPANESE_POSTCARD,      //  43  Japanese Postcard 100 x 148 mm
		P_9X11,                 //  44  9 x 11 in
		P_10X11,                //  45  10 x 11 in
		P_15X11,                //  46  15 x 11 in
		ENV_INVITE,             //  47  Envelope Invite 220 x 220 mm
		RESERVED_48,            //  48  RESERVED--DO NOT USE
		RESERVED_49,            //  49  RESERVED--DO NOT USE
		LETTER_EXTRA,	        //  50  Letter Extra 9 \275 x 12 in
		LEGAL_EXTRA, 	        //  51  Legal Extra 9 \275 x 15 in
		TABLOID_EXTRA,	        //  52  Tabloid Extra 11.69 x 18 in
		A4_EXTRA,     	        //  53  A4 Extra 9.27 x 12.69 in
		LETTER_TRANSVERSE,      //  54  Letter Transverse 8 \275 x 11 in
		A4_TRANSVERSE,          //  55  A4 Transverse 210 x 297 mm
		LETTER_EXTRA_TRANSVERSE,//  56  Letter Extra Transverse 9\275 x 12 in
		A_PLUS,                 //  57  SuperA/SuperA/A4 227 x 356 mm
		B_PLUS,                 //  58  SuperB/SuperB/A3 305 x 487 mm
		LETTER_PLUS,            //  59  Letter Plus 8.5 x 12.69 in
		A4_PLUS,                //  60  A4 Plus 210 x 330 mm
		A5_TRANSVERSE,          //  61  A5 Transverse 148 x 210 mm
		B5_TRANSVERSE,          //  62  B5 (JIS) Transverse 182 x 257 mm
		A3_EXTRA,               //  63  A3 Extra 322 x 445 mm
		A5_EXTRA,               //  64  A5 Extra 174 x 235 mm
		B5_EXTRA,               //  65  B5 (ISO) Extra 201 x 276 mm
		A2,                     //  66  A2 420 x 594 mm
		A3_TRANSVERSE,          //  67  A3 Transverse 297 x 420 mm
		A3_EXTRA_TRANSVERSE,    //  68  A3 Extra Transverse 322 x 445 mm
		DBL_JAPANESE_POSTCARD,  //  69  Japanese Double Postcard 200 x 148 mm
		A6,                     //  70  A6 105 x 148 mm
		JENV_KAKU2,             //  71  Japanese Envelope Kaku #2
		JENV_KAKU3,             //  72  Japanese Envelope Kaku #3
		JENV_CHOU3,             //  73  Japanese Envelope Chou #3
		JENV_CHOU4,             //  74  Japanese Envelope Chou #4
		LETTER_ROTATED,         //  75  Letter Rotated 11 x 8 1/2 11 in
		A3_ROTATED,             //  76  A3 Rotated 420 x 297 mm
		A4_ROTATED,             //  77  A4 Rotated 297 x 210 mm
		A5_ROTATED,             //  78  A5 Rotated 210 x 148 mm
		B4_JIS_ROTATED,         //  79  B4 (JIS) Rotated 364 x 257 mm
		B5_JIS_ROTATED,         //  80  B5 (JIS) Rotated 257 x 182 mm
		JAPANESE_POSTCARD_ROTATED, //  81 Japanese Postcard Rotated 148 x 100 mm
		DBL_JAPANESE_POSTCARD_ROTATED, // 82 Double Japanese Postcard Rotated 148 x 200 mm
		A6_ROTATED,             //  83  A6 Rotated 148 x 105 mm
		JENV_KAKU2_ROTATED,     //  84  Japanese Envelope Kaku #2 Rotated
		JENV_KAKU3_ROTATED,     //  85  Japanese Envelope Kaku #3 Rotated
		JENV_CHOU3_ROTATED,     //  86  Japanese Envelope Chou #3 Rotated
		JENV_CHOU4_ROTATED,     //  87  Japanese Envelope Chou #4 Rotated
		B6_JIS,                 //  88  B6 (JIS) 128 x 182 mm
		B6_JIS_ROTATED,         //  89  B6 (JIS) Rotated 182 x 128 mm
		P_12X11,                //  90  12 x 11 in
		JENV_YOU4,              //  91  Japanese Envelope You #4
		JENV_YOU4_ROTATED,      //  92  Japanese Envelope You #4 Rotated
		P16K,                   //  93  PRC 16K 146 x 215 mm
		P32K,                   //  94  PRC 32K 97 x 151 mm
		P32KBIG,                //  95  PRC 32K(Big) 97 x 151 mm
		PENV_1,                 //  96  PRC Envelope #1 102 x 165 mm
		PENV_2,                 //  97  PRC Envelope #2 102 x 176 mm
		PENV_3,                 //  98  PRC Envelope #3 125 x 176 mm
		PENV_4,                 //  99  PRC Envelope #4 110 x 208 mm
		PENV_5,                 // 100  PRC Envelope #5 110 x 220 mm
		PENV_6,                 // 101  PRC Envelope #6 120 x 230 mm
		PENV_7,                 // 102  PRC Envelope #7 160 x 230 mm
		PENV_8,                 // 103  PRC Envelope #8 120 x 309 mm
		PENV_9,                 // 104  PRC Envelope #9 229 x 324 mm
		PENV_10,                // 105  PRC Envelope #10 324 x 458 mm
		P16K_ROTATED,           // 106  PRC 16K Rotated
		P32K_ROTATED,           // 107  PRC 32K Rotated
		P32KBIG_ROTATED,        // 108  PRC 32K(Big) Rotated
		PENV_1_ROTATED,         // 109  PRC Envelope #1 Rotated 165 x 102 mm
		PENV_2_ROTATED,         // 110  PRC Envelope #2 Rotated 176 x 102 mm
		PENV_3_ROTATED,         // 111  PRC Envelope #3 Rotated 176 x 125 mm
		PENV_4_ROTATED,         // 112  PRC Envelope #4 Rotated 208 x 110 mm
		PENV_5_ROTATED,         // 113  PRC Envelope #5 Rotated 220 x 110 mm
		PENV_6_ROTATED,         // 114  PRC Envelope #6 Rotated 230 x 120 mm
		PENV_7_ROTATED,         // 115  PRC Envelope #7 Rotated 230 x 160 mm
		PENV_8_ROTATED,         // 116  PRC Envelope #8 Rotated 309 x 120 mm
		PENV_9_ROTATED,         // 117  PRC Envelope #9 Rotated 324 x 229 mm
		PENV_10_ROTATED,        // 118  PRC Envelope #10 Rotated 458 x 324 mm
		USER_DEFINED = 256
	};

	enum PAPERSOURCE {
		AUTO,          // 7	o
		MANUAL,        // 4	o
		UPPER,         // 1	o
		MIDDLE,        // 3	o
		LOWER,         // 2	o
//		ONLYONE,       // 1	x
//		ENVELOPE,      // 5	o
//		ENVMANUAL,     // 6	x
//		TRACTOR,       // 8	x
//		SMALLFMT,      // 9	x
//		LARGEFMT,      // 10	x
//		LARGECAPACITY, // 11	x
//		CASSETTE,      // 14	x
//		FORMSOURCE,    // 15	x
		CASSETTE1 = 21,
		CASSETTE2,
		CASSETTE3,
		CASSETTE4,
		CASSETTE5,
		CASSETTE6,
		CASSETTE7,
		CASSETTE8,
		CASSETTE9,
		USER = 256     // device specific bins start here
	};

	enum PRINTSTYLE {
		SIMPLEX,
		DUPLEX,
		BOOKLET
	};
	
	enum BINDINGLOCATION {
		LONG_EDGE_LEFT,
		LONG_EDGE_RIGHT,
		SHORT_EDGE_TOP,
		SHORT_EDGE_BOTTOM,
		LONG_EDGE  = LONG_EDGE_LEFT,
		SHORT_EDGE = SHORT_EDGE_TOP
	};
	
	enum PAGEORDER {
		ACROSS_FROM_LEFT,
		DOWN_FROM_LEFT,
		ACROSS_FROM_RIGHT,
		DOWN_FROM_RIGHT,
		LEFT_TO_RIGHT = ACROSS_FROM_LEFT,
		RIGHT_TO_LEFT = ACROSS_FROM_RIGHT
	};

/*
	enum QUALITY {
		DRAFT  = -1,
		LOW    = -2,
		MEDIUM = -3,
		HIGH   = -4
	};

	enum COLOR {
		MONOCHROME = 1,
		COLOR
	};
*/

private:
	PAPER       __paper;
	int32       __xres;
	int32       __yres;
	ORIENTATION __orientation;
	float       __scaling;
	BRect       __paper_rect;
	BRect       __printable_rect;
	int32       __nup;
	int32       __first_page;
	int32       __last_page;
	color_space __surface_type;
	float       __gamma;
	PAPERSOURCE __paper_source;
	int32       __copies;
	bool        __collate;
	bool        __reverse;
	PRINTSTYLE  __print_style;
	BINDINGLOCATION __binding_location;
	PAGEORDER   __page_order;
	BMessage    *__msg;

public:
	JobData(BMessage *msg, const PrinterCap *cap);
	~JobData();

	JobData(const JobData &job_data);
	JobData &operator = (const JobData &job_data);

	void load(BMessage *msg, const PrinterCap *cap);
	void save(BMessage *msg = NULL);

	PAPER getPaper() const { return __paper; }
	void  setPaper(PAPER paper) { __paper = paper; }

	int32 getXres() const { return __xres; } 
	void  setXres(int32 xres) { __xres = xres; }

	int32 getYres() const { return __yres; }
	void  setYres(int32 yres) { __yres = yres; };

	ORIENTATION getOrientation() const { return __orientation; }
	void  setOrientation(ORIENTATION orientation) { __orientation = orientation; }

	float getScaling() const { return __scaling; }
	void  setScaling(float scaling) { __scaling = scaling; }

	const BRect &getPaperRect() const { return __paper_rect; }
	void  setPaperRect(const BRect &paper_rect) { __paper_rect = paper_rect; }

	const BRect &getPrintableRect() const { return __printable_rect; }
	void  setPrintableRect(const BRect &printable_rect) { __printable_rect = printable_rect; }

	int32 getNup() const { return __nup; }
	void  setNup(int32 nup) { __nup = nup; }

	bool getReverse() const { return __reverse; }
	void  setReverse(bool reverse) { __reverse = reverse; }

	int32 getFirstPage() const { return __first_page; }
	void  setFirstPage(int32 first_page) { __first_page = first_page; }

	int32 getLastPage() const { return __last_page; }
	void  setLastPage(int32 last_page) { __last_page = last_page; }

	color_space getSurfaceType() const { return __surface_type; }
	void setSurfaceType(color_space surface_type) { __surface_type = surface_type; }

	float getGamma() const { return __gamma; }
	void setGamma(float gamma) { __gamma = gamma; }

	PAPERSOURCE getPaperSource() const { return __paper_source; }
	void setPaperSource(PAPERSOURCE paper_source) { __paper_source = paper_source; };

	int32 getCopies() const { return __copies; }
	void  setCopies(int32 copies) { __copies = copies; }

	bool getCollate() const { return __collate; }
	void setCollate(bool collate) { __collate = collate; }

	PRINTSTYLE getPrintStyle() const { return __print_style; }
	void setPrintStyle(PRINTSTYLE print_style) { __print_style = print_style; }

	BINDINGLOCATION getBindingLocation() const { return __binding_location; }
	void setBindingLocation(BINDINGLOCATION binding_location) { __binding_location = binding_location; }

	PAGEORDER getPageOrder() const { return __page_order; }
	void setPageOrder(PAGEORDER page_order) { __page_order = page_order; }
/*
protected:
	JobData(const JobData &job_data);
	JobData &operator = (const JobData &job_data);
*/
};

#endif	/* __JOBDATA_H */
