#include "font.h"

int glyphWidth(struct font *font, int c)
{
	struct glyph *g = &font->glyphs[c & 0xFF];
	return g->width;
}

void drawChar(unsigned int *dst, int x, int colStride, struct font *font, int c)
{
	struct glyph *g = &font->glyphs[c & 0xFF];
	const unsigned int *srcDataPtr = &font->data[g->offset];
	
	unsigned int srcData = *srcDataPtr++;
	int srcShift = 0;
	
	dst += colStride * (x >> 3);
	x = x & 7;
	
	for (size_t y = 0; y < font->height; ++y)
	{
		unsigned int *dstLine = dst;
		int dstShift = x * 4;
		int dstData = *dstLine;
		
		for (size_t x = 0; x < g->width; ++x)
		{
			// decode pixel
			int pixel = (srcData >> srcShift) & 0xF;
			
			// plot pixel
			dstData &= ~(0xF << dstShift);
			dstData |= pixel << dstShift;
			
			srcShift += 4;
			if (srcShift == 32)
			{
				// fetch next source word
				srcData = *srcDataPtr++;
				srcShift = 0;
			}
			
			dstShift += 4;
			if (dstShift == 32)
			{
				// move to the next tile
				*dstLine = dstData;
				dstLine += colStride;
				dstData = *dstLine;
				dstShift = 0;
			}
		}
		
		// write back last chunk of data
		if (dstShift > 0) *dstLine = dstData;
		
		// move to next line
		dst++;
	}
}

void drawString(unsigned int *dst, int x, int w, int colStride, struct font *font, const char *str)
{
	char c;
	while ('\0' != (c  = *str++) && w > 0)
	{
		int gw = glyphWidth(font, c);
		if (x + gw > w) break;
		drawChar(dst, x, colStride, font, c);
		x += gw;
	}
}
