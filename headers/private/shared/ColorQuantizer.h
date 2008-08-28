/* === C R E D I T S  &  D I S C L A I M E R S ==============
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * CQuantizer (c)  1996-1997 Jeff Prosise
 *
 * 31/08/2003 Davide Pizzolato - www.xdp.it
 * - fixed minor bug in ProcessImage when bpp<=8
 * - better color reduction to less than 16 colors
 *
 * COVERED CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS, WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, WITHOUT
 * LIMITATION, WARRANTIES THAT THE COVERED CODE IS FREE OF DEFECTS,
 * MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE OR NON-INFRINGING. THE ENTIRE
 * RISK AS TO THE QUALITY AND PERFORMANCE OF THE COVERED CODE IS WITH YOU.
 * SHOULD ANY COVERED CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE INITIAL
 * DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY NECESSARY
 * SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF WARRANTY CONSTITUTES AN
 * ESSENTIAL PART OF THIS LICENSE. NO USE OF ANY COVERED CODE IS AUTHORIZED
 * HEREUNDER EXCEPT UNDER THIS DISCLAIMER.
 *
 * Use at your own risk!
 * ==========================================================
 *
 * Modified for use with Haiku by David Powell & Stephan Aßmus.
 */
#ifndef COLOR_QUANTIZER_H
#define COLOR_QUANTIZER_H


#include <SupportDefs.h>


namespace BPrivate {

typedef struct _RGBA {
	uint8	b;
	uint8	g;
	uint8	r;
	uint8	a;
} RGBA;

class BColorQuantizer {
public:
							BColorQuantizer(uint32 maxColors,
								uint32 bitsPerColor);
	virtual					~BColorQuantizer();

			bool			ProcessImage(const uint8* const * rowPtrs, int width,
								int height);

			uint32			GetColorCount() const;
			void			GetColorTable(RGBA* table) const;

private:
			struct Node;

private:
			void			_AddColor(Node** _node, uint8 r, uint8 g, uint8 b,
								uint8 a, uint32 bitsPerColor, uint32 level,
								uint32* _leafCount, Node** reducibleNodes);
			Node*			_CreateNode(uint32 level, uint32 bitsPerColor,
								uint32* _leafCount, Node** reducibleNodes);
			void		    _ReduceTree(uint32 bitsPerColor, uint32* _leafCount,
								Node** reducibleNodes);
			void		    _DeleteTree(Node** _node);

			void		    _GetPaletteColors(Node* node, RGBA* table,
								uint32* pIndex, uint32* pSum) const;

private:
			Node*			fTree;
			uint32			fLeafCount;
			Node*			fReducibleNodes[9];
			uint32			fMaxColors;
			uint32			fOutputMaxColors;
			uint32			fBitsPerColor;
};

}	// namespace BPrivate

using BPrivate::BColorQuantizer;
using BPrivate::RGBA;

#endif // COLOR_QUANTIZER_H
