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
#include "ColorQuantizer.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


static inline uint8
clip(float component)
{
	if (component > 255.0)
		return 255;

	return (uint8)(component + 0.5f);
}

struct BColorQuantizer::Node {
	bool			isLeaf;		// TRUE if node has no children
	uint32			pixelCount;	// Number of pixels represented by this leaf
	uint32			sumR;		// Sum of red components
	uint32			sumG;		// Sum of green components
	uint32			sumB;		// Sum of blue components
	uint32			sumA;		// Sum of alpha components
	Node*			child[8];	// Pointers to child nodes
	Node*			next;		// Pointer to next reducible node
};


BColorQuantizer::BColorQuantizer(uint32 maxColors, uint32 bitsPerColor)
	: fTree(NULL),
	  fLeafCount(0),
	  fMaxColors(maxColors),
	  fOutputMaxColors(maxColors),
	  fBitsPerColor(bitsPerColor)
{
	// override parameters if out of range
	if (fBitsPerColor > 8)
		fBitsPerColor = 8;

	if (fMaxColors < 16)
		fMaxColors = 16;

	for (int i = 0; i <= (int)fBitsPerColor; i++)
		fReducibleNodes[i] = NULL;
}


BColorQuantizer::~BColorQuantizer()
{
	if (fTree != NULL)
		_DeleteTree(&fTree);
}


bool
BColorQuantizer::ProcessImage(const uint8* const * rowPtrs, int width,
	int height)
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x += 3) {
			uint8 b = rowPtrs[y][x];
			uint8 g = rowPtrs[y][x + 1];
			uint8 r = rowPtrs[y][x + 2];
			_AddColor(&fTree, r, g, b, 0, fBitsPerColor, 0, &fLeafCount,
				fReducibleNodes);

			while (fLeafCount > fMaxColors)
				_ReduceTree(fBitsPerColor, &fLeafCount, fReducibleNodes);
		}
	}

	return true;
}


uint32
BColorQuantizer::GetColorCount() const
{
	return fLeafCount;
}


void
BColorQuantizer::GetColorTable(RGBA* table) const
{
	uint32 index = 0;
	if (fOutputMaxColors < 16) {
		uint32 sums[16];
		RGBA tmpPalette[16];
		_GetPaletteColors(fTree, tmpPalette, &index, sums);
		if (fLeafCount > fOutputMaxColors) {
			for (uint32 j = 0; j < fOutputMaxColors; j++) {
				uint32 a = (j * fLeafCount) / fOutputMaxColors;
				uint32 b = ((j + 1) * fLeafCount) / fOutputMaxColors;
				uint32 nr = 0;
				uint32 ng = 0;
				uint32 nb = 0;
				uint32 na = 0;
				uint32 ns = 0;
				for (uint32 k = a; k < b; k++){
					nr += tmpPalette[k].r * sums[k];
					ng += tmpPalette[k].g * sums[k];
					nb += tmpPalette[k].b * sums[k];
					na += tmpPalette[k].a * sums[k];
					ns += sums[k];
				}
				table[j].r = clip((float)nr / ns);
				table[j].g = clip((float)ng / ns);
				table[j].b = clip((float)nb / ns);
				table[j].a = clip((float)na / ns);
			}
		} else {
			memcpy(table, tmpPalette, fLeafCount * sizeof(RGBA));
		}
	} else {
		_GetPaletteColors(fTree, table, &index, NULL);
	}
}


// #pragma mark - private


void
BColorQuantizer::_AddColor(Node** _node, uint8 r, uint8 g, uint8 b, uint8 a,
	uint32 bitsPerColor, uint32 level, uint32* _leafCount,
	Node** reducibleNodes)
{
	static const uint8 kMask[8]
		= {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

	// If the node doesn't exist, create it.
	if (*_node == NULL)
		*_node = _CreateNode(level, bitsPerColor, _leafCount, reducibleNodes);

	// Update color information if it's a leaf node.
	if ((*_node)->isLeaf) {
		(*_node)->pixelCount++;
		(*_node)->sumR += r;
		(*_node)->sumG += g;
		(*_node)->sumB += b;
		(*_node)->sumA += a;
	} else {
		// Recurse a level deeper if the node is not a leaf.
		int shift = 7 - level;
		int index = (((r & kMask[level]) >> shift) << 2) |
					 (((g & kMask[level]) >> shift) << 1) |
					 (( b & kMask[level]) >> shift);
		_AddColor(&((*_node)->child[index]), r, g, b, a, bitsPerColor,
			level + 1, _leafCount, reducibleNodes);
	}
}


BColorQuantizer::Node*
BColorQuantizer::_CreateNode(uint32 level, uint32 bitsPerColor,
	uint32* _leafCount, Node** reducibleNodes)
{
	Node* node = (Node*)calloc(1, sizeof(Node));

	if (node == NULL)
		return NULL;

	node->isLeaf = (level == bitsPerColor) ? true : false;
	if (node->isLeaf)
		(*_leafCount)++;
	else {
		node->next = reducibleNodes[level];
		reducibleNodes[level] = node;
	}
	return node;
}


void
BColorQuantizer::_ReduceTree(uint32 bitsPerColor, uint32* _leafCount,
	Node** reducibleNodes)
{
	int i = bitsPerColor - 1;
	// Find the deepest level containing at least one reducible node.
	for (; i > 0 && reducibleNodes[i] == NULL; i--)
		;

	// Reduce the node most recently added to the list at level i.
	Node* node = reducibleNodes[i];
	reducibleNodes[i] = node->next;

	uint32 sumR = 0;
	uint32 sumG = 0;
	uint32 sumB = 0;
	uint32 sumA = 0;
	uint32 childCount = 0;

	for (i = 0; i < 8; i++) {
		if (node->child[i] != NULL) {
			sumR += node->child[i]->sumR;
			sumG += node->child[i]->sumG;
			sumB += node->child[i]->sumB;
			sumA += node->child[i]->sumA;
			node->pixelCount += node->child[i]->pixelCount;

			free(node->child[i]);
			node->child[i] = NULL;

			childCount++;
		}
	}

	node->isLeaf = true;
	node->sumR = sumR;
	node->sumG = sumG;
	node->sumB = sumB;
	node->sumA = sumA;

	*_leafCount -= (childCount - 1);
}


void
BColorQuantizer::_DeleteTree(Node** _node)
{
	for (int i = 0; i < 8; i++) {
		if ((*_node)->child[i] != NULL)
			_DeleteTree(&((*_node)->child[i]));
	}
	free(*_node);
	*_node = NULL;
}


void
BColorQuantizer::_GetPaletteColors(Node* node, RGBA* table, uint32* _index,
	uint32* sums) const
{
	if (node == NULL)
		return;

	if (node->isLeaf) {
		table[*_index].r = clip((float)node->sumR / node->pixelCount);
		table[*_index].g = clip((float)node->sumG / node->pixelCount);
		table[*_index].b = clip((float)node->sumB / node->pixelCount);
		table[*_index].a = clip((float)node->sumA / node->pixelCount);
		if (sums)
			sums[*_index] = node->pixelCount;
		(*_index)++;
	} else {
		for (int i = 0; i < 8; i++) {
			if (node->child[i] != NULL)
				_GetPaletteColors(node->child[i], table, _index, sums);
		}
	}
}

