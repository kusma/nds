#include <nds.h>
#include <stdlib.h>

#include "texture1_headers_bin.h"
#include "texture1_tiles_bin.h"
#include "texture1_pal_bin.h"

#include "texture2_headers_bin.h"
#include "texture2_tiles_bin.h"
#include "texture2_pal_bin.h"


void *uploadCompressedTextureBankA(u32 offset, u32 w, u32 h, const u16 *src_tiles, const u16 *src_headers)
{
	u16 *dst_tiles = (u16*)((u8*)VRAM_A + offset);
	for (u32 i = 0; i < ((w * h) / 8) * 2; ++i)
	{
		dst_tiles[i] = src_tiles[i];
	}
	
	u16 *dst_headers = (u16*)((u8*)VRAM_B + (offset / 2));
	for (u32 i = 0; i < (w * h) / 8; ++i)
	{
		dst_headers[i] = src_headers[i];
	}
	return (void*)dst_tiles;
}

int main() {	
	
	float rotateX = 0.0;
	float rotateY = 0.0;

	powerON(POWER_ALL);

	//set mode 0, enable BG0 and set it to 3D
	videoSetMode(MODE_0_3D);

	//irqs are nice
	irqInit();
	irqEnable(IRQ_VBLANK);

	// initialize gl
	glInit();
	
	//enable textures
	glEnable(GL_TEXTURE_2D);
	
	// enable antialiasing
	glEnable(GL_ANTIALIAS);
	
	// setup the rear plane
	glClearColor(0,0,0,31); // BG must be opaque for AA to work
	glClearPolyID(63); // BG must have a unique polygon ID for AA to work
	glClearDepth(0x7FFF);

	//this should work the same as the normal gl call
	glViewport(0,0,255,191);
	
	//any floating point gl call is being converted to fixed prior to being implemented
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, 256.0 / 192.0, 0.1, 40);
	
	gluLookAt(	0.0, 0.0, 0.5,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 1.0, 0.0);		//up	
	
	// setup vram banks a and b
	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankB(VRAM_B_TEXTURE);
	
	// temp-alloc vram banks to LCD so we can upload data
	uint32 vramTemp = vramSetMainBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_LCD, VRAM_D_LCD);
	
	// upload texture1 tiles and headers
	void *tex1_data = uploadCompressedTextureBankA(
		0,        // offset
		128, 128, // size
		(const u16*)texture1_tiles_bin,
		(const u16*)texture1_headers_bin
	);
	
	// upload texture2 tiles and headers
	void *tex2_data = uploadCompressedTextureBankA(
		(128 * 128) / 4, // offset
		256, 256,        // size
		(const u16*)texture2_tiles_bin,
		(const u16*)texture2_headers_bin
	);
	
	// flush cache and restore vram settings
	DC_FlushAll();
	vramRestoreMainBanks(vramTemp);
	
	u16 *tex_pal = VRAM_E;
	vramSetBankE(VRAM_E_LCD);
	
	// upload texture1 palette
	void *tex1_pal = tex_pal;
	for (int i = 0; i < int(texture1_pal_bin_size / 2); ++i) *tex_pal++ = ((u16*)texture1_pal_bin)[i];
	
	// upload texture1 palette
	void *tex2_pal = tex_pal;
	for (int i = 0; i < int(texture2_pal_bin_size / 2); ++i) *tex_pal++ = ((u16*)texture2_pal_bin)[i];
	
	// flush cache
	DC_FlushAll();
	vramSetBankE(VRAM_E_TEX_PALETTE);
	
	while(1) {
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glRotateX(rotateX);
		glRotateY(rotateY);
		
		glMaterialf(GL_AMBIENT, RGB15(31,31,31));
		glMaterialf(GL_DIFFUSE, RGB15(16,16,16));
		glMaterialf(GL_SPECULAR, BIT(15) | RGB15(8,8,8));
		glMaterialf(GL_EMISSION, RGB15(16,16,16));
		
		//ds uses a table for shinyness..this generates a half-ass one
		glMaterialShinyness();
		
		//not a real gl function and will likely change
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK);
		
		scanKeys();
		u16 keys = keysHeld();
		if((keys & KEY_UP)) rotateX += 3;
		if((keys & KEY_DOWN)) rotateX -= 3;
		if((keys & KEY_LEFT)) rotateY += 3;
		if((keys & KEY_RIGHT)) rotateY -= 3;
		
		int texture = (keys & KEY_A) ? 1 : 0;
		unsigned int param = TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_T | GL_TEXTURE_WRAP_S;
		
		if (0 == texture)
		{
			unsigned int data_ptr = ((unsigned int)tex1_data >> 3) & 0xFFFF;
			GFX_TEX_FORMAT = param | (TEXTURE_SIZE_128 << 20) | (TEXTURE_SIZE_128 << 23) | data_ptr | (GL_COMPRESSED << 26);
			GFX_PAL_FORMAT = ((u8*)tex1_pal - (u8*)VRAM_E) >> 4;
		}
		else
		{
			unsigned int data_ptr = ((unsigned int)tex2_data >> 3) & 0xFFFF;
			GFX_TEX_FORMAT = param | (TEXTURE_SIZE_256 << 20) | (TEXTURE_SIZE_256 << 23) | data_ptr | (GL_COMPRESSED << 26);
			GFX_PAL_FORMAT = ((u8*)tex2_pal - (u8*)VRAM_E) >> 4;
		}
		
		//draw the obj
		glBegin(GL_QUAD);
			glNormal(NORMAL_PACK(0,inttov10(-1),0));
			GFX_TEX_COORD = (TEXTURE_PACK(0, inttot16(256)));
			glVertex3v16(floattov16(-0.5),	floattov16(-0.5), 0 );
			GFX_TEX_COORD = (TEXTURE_PACK(inttot16(256),inttot16(256)));
			glVertex3v16(floattov16(0.5),	floattov16(-0.5), 0 );
			GFX_TEX_COORD = (TEXTURE_PACK(inttot16(256), 0));
			glVertex3v16(floattov16(0.5),	floattov16(0.5), 0 );
			GFX_TEX_COORD = (TEXTURE_PACK(0,0));
			glVertex3v16(floattov16(-0.5),	floattov16(0.5), 0 );
		glEnd();
		glPopMatrix(1);
		
		glFlush(0);
		swiWaitForVBlank();
	}

	return 0;
}//end main 
