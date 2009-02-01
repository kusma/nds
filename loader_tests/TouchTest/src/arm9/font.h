#ifndef FONT_H
#define FONT_H

#include <stddef.h>

struct glyph
{
	size_t width;
	size_t offset;
};

struct font
{
	size_t height;
	struct glyph glyphs[256];
	const unsigned int data[0x1000];
};

void drawChar(unsigned int *dst, int x, int colStride, struct font *font, int c);
void drawString(unsigned int *dst, int x, int w, int colStride, struct font *font, const char *str);

#endif // FONT_H
