#include <nds.h>

#include <nds/arm9/console.h> 
#include <stdio.h>
//Include the font data here Doesn't have to be .h! Can be .c and what not.
#include "font.h"
#include "font.h"




struct glyph
{
	size_t width;
	size_t offset;
};

struct font
{
	size_t height;
	struct glyph glyphs[256];
	const unsigned int data[0x10000];
};

int glyphWidth(struct font *font, int c)
{
	struct glyph *g = &font->glyphs[c & 0xFF];
	return g->width;
}

#include "font_data.h"
#include "font_data2.h"

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


void drawString(unsigned int *dst, int x, int colStride, struct font *font, char *str)
{
	char c;
	while ('\0' != (c  = *str++))
	{
		drawChar(dst, x, colStride, font, c);
		x += glyphWidth(font, c);
	}
}

int main(void) {
//---------------------------------------------------------------------------------

	powerON(POWER_ALL_2D);
	
	// put the main screen on the bottom lcd
	lcdMainOnBottom();
	
	// Initialise the interrupt system
	irqInit();
	irqEnable(IRQ_VBLANK);
	
	vramSetMainBanks(
		VRAM_A_MAIN_BG_0x06000000, VRAM_B_LCD,
		VRAM_C_SUB_BG , VRAM_D_LCD
	);


	//set the video mode
	videoSetMode(   MODE_0_2D | DISPLAY_BG0_ACTIVE);
	videoSetModeSub(MODE_0_2D | DISPLAY_BG0_ACTIVE);
	
	SUB_BG0_CR = BG_MAP_BASE(31);
	BG_PALETTE_SUB[255] = RGB15(31,31,31);
	consoleInitDefault((u16*)SCREEN_BASE_BLOCK_SUB(31), (u16*)CHAR_BASE_BLOCK_SUB(0), 16);

	
	BG_PALETTE[0] = ~RGB5(0, 0, 0);
	BG_PALETTE[1] = ~RGB5(4, 4, 4);
	BG_PALETTE[2] = ~RGB5(12, 12, 12);
	BG_PALETTE[3] = ~RGB5(14, 14, 14);
	BG_PALETTE[4] = ~RGB5(18, 18, 18);
	BG_PALETTE[5] = ~RGB5(24, 24, 24);
	BG_PALETTE[6] = ~RGB5(31, 31, 31);
	
	BG0_CR = BG_MAP_BASE(31);
	for (int x = 0; x < 32; ++x)
		for (int y = 0; y < 32; ++y)
		{
			((u16*)SCREEN_BASE_BLOCK(31))[32 * x + y] = x + y * 32;
		}
/*	((u16*)SCREEN_BASE_BLOCK(31))[32 * 1 + 0] = 1;
	((u16*)SCREEN_BASE_BLOCK(31))[32 * 0 + 1] = 2;
	((u16*)SCREEN_BASE_BLOCK(31))[32 * 1 + 1] = 3;
	((u16*)SCREEN_BASE_BLOCK(31))[32 * 0 + 2] = 4;
	((u16*)SCREEN_BASE_BLOCK(31))[32 * 1 + 2] = 5; */
	
/*
	const unsigned int data[] = 
	{
		0x45555530,0x64000002,0x04654444,0x00064000,0x00036300,0x00000064,0x06400054,
		0x16300000,0x00006400,0x40026200,0x20000006,0x00640026,0x02630000,0x00000640,
		0x64000640,0x46200000,0x22264000,0x00015642,0x45666664,0x00000001,0x00000000,
		0x00000000,0x00000000,0x00000000,0x00000000,0x00000000
	};

	struct font font;
	font.height = 16;
	font.data = data;
	font.glyphs[int('B')].offset = 0;
	font.glyphs[int('B')].width  = 13;
*/
//	drawChar((unsigned int*)CHAR_BASE_BLOCK(0), 0, (2 * ((8 * 8) / 2)) / 4, &f, 'a');
//	drawChar((unsigned int*)CHAR_BASE_BLOCK(0), glyphWidth(&f, 'a'), (2 * ((8 * 8) / 2)) / 4, &f, 'b');
	drawString((unsigned int*)CHAR_BASE_BLOCK(0) + 4, 48, (32 * ((8 * 8) / 2)) / 4, &f, "mongohore.nds");
	drawString((unsigned int*)CHAR_BASE_BLOCK(0) + 4 + 16, 48, (32 * ((8 * 8) / 2)) / 4, &font_bmp, "913kb");
	drawString((unsigned int*)CHAR_BASE_BLOCK(0) + 4 + 32, 48, (32 * ((8 * 8) / 2)) / 4, &f, "colors.nds");


	while(1) {
		swiWaitForVBlank();
	}

	return 0;
}
