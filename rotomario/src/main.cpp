#include <gba_video.h>
#include <gba_systemcalls.h>
#include <gba_interrupt.h>
#include <gba_sprites.h>
#include <gba_input.h>
#include <math.h>

static int blanks = 0;
static void vblank()
{
	blanks++;
}

OBJATTR sprites[128] ALIGN(32);

extern u8 tileset_img[];
extern u8 tileset_pal[];

#define MAP_BASE_BLOCK     28

int main()
{
	irqInit();
	SetMode((LCDC_BITS)(MODE_1 | BG0_ON | BG2_ON | OBJ_ENABLE));
	
	irqSet(IRQ_VBLANK, vblank);
	irqEnable(IRQ_VBLANK);
	
	float xpos = 0;
	float ypos = 0;
	
	float xdir = 0;
	float ydir = 0;
	int dir = 0;
	
	BG_COLORS[0] = RGB5(31, 0, 31);
	CpuFastSet(tileset_img, OBJ_BASE_ADR, COPY32 | (256 * 16) / 4);
	CpuFastSet(tileset_pal, OBJ_COLORS, COPY32 | (256 / 4));
	
	CpuFastSet(tileset_img, PATRAM4(0, 0), COPY32 | (256 * 16) / 4);
	CpuFastSet(tileset_pal, BG_COLORS, COPY32 | (256 / 4));
	
	// mock up a map
/*	((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK))[0] = 0;
	((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK))[1] = 1;
	((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK))[32] = 32;
	((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK))[33] = 33;
	((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK))[64] = 64;
	((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK))[65] = 65; */
	
	for (int y = 0; y < 32; ++y)
	{
		for (int x = 0; x < 32; ++x)
		{
			((vu16*)MAP_BASE_ADR(MAP_BASE_BLOCK + 1))[x + y * 32] = 5;
		}
	}
	for (int x = 0; x < 32; ++x) ((vu8*)MAP_BASE_ADR(MAP_BASE_BLOCK + 1))[x] = 16;
	for (int x = 0; x < 32; ++x) ((vu8*)MAP_BASE_ADR(MAP_BASE_BLOCK + 1))[31 * 32 + x] = 16;
	for (int y = 0; y < 32; ++y) ((vu8*)MAP_BASE_ADR(MAP_BASE_BLOCK + 1))[y * 32] = 16;
	for (int y = 0; y < 32; ++y) ((vu8*)MAP_BASE_ADR(MAP_BASE_BLOCK + 1))[y * 32 + 31] = 16;
	
	BGCTRL[0] = SCREEN_BASE(MAP_BASE_BLOCK + 0) | CHAR_BASE(0) | BG_SIZE_0 | BG_16_COLOR | BG_PRIORITY(1); // | CHAR_PALETTE(1);
	BGCTRL[2] = SCREEN_BASE(MAP_BASE_BLOCK + 1) | CHAR_BASE(0) | BG_SIZE_1 | BG_16_COLOR | BG_PRIORITY(0); // | CHAR_PALETTE(1);
	
	float rot = 0.0f;
	float last_rot = rot;
	float bg_scroll = 0;
	
	int frame = 0;
	while (1)
	{
		int sprite_frame = (int(floor(bg_scroll)) >> 3) & 1;
		if (fabs(xdir) < 1e-1) sprite_frame = 0;
//		if (ypos > 0) sprite_frame = 2;
		int tile_start = sprite_frame * 2;
		int pal = 0;
		
		for (int i = 0; i < 128; ++i)
		{
			sprites[i].attr0 = ATTR0_DISABLED;
		}
		
		int sprite = 0;
//		sprites[sprite].attr0 = OBJ_Y(int(ypos) + 80 - 8) | ATTR0_COLOR_16 | ATTR0_WIDE;
//		sprites[sprite].attr1 = OBJ_X(int(xpos) + 120) | OBJ_SIZE(Sprite_16x8) | (dir ? ATTR1_FLIP_X : 0);
		sprites[sprite].attr0 = OBJ_Y(int(0) + 80 - 16) | ATTR0_COLOR_16 | ATTR0_WIDE;
		sprites[sprite].attr1 = OBJ_X(int(0) + 120 - 8) | OBJ_SIZE(Sprite_16x8) | (dir ? ATTR1_FLIP_X : 0);
		sprites[sprite].attr2 = OBJ_CHAR(tile_start) | ATTR2_PALETTE(pal); // | OBJ_TRANSLUCENT;
		sprite++;

//		sprites[sprite].attr0 = OBJ_Y(int(ypos) + 80)  | ATTR0_COLOR_16 | ATTR0_SQUARE;
//		sprites[sprite].attr1 = OBJ_X(int(xpos) + 120) | OBJ_SIZE(Sprite_16x16) | (dir ? ATTR1_FLIP_X : 0);
		sprites[sprite].attr0 = OBJ_Y(int(0) + 80 - 8)  | ATTR0_COLOR_16 | ATTR0_SQUARE;
		sprites[sprite].attr1 = OBJ_X(int(0) + 120 - 8) | OBJ_SIZE(Sprite_16x16) | (dir ? ATTR1_FLIP_X : 0);
		sprites[sprite].attr2 = OBJ_CHAR(tile_start + 32)   | ATTR2_PALETTE(pal); // | OBJ_TRANSLUCENT;
		sprite++;

		float st = sin(last_rot);
		float ct = cos(last_rot);
		
		last_rot = last_rot + (rot - last_rot) * 0.1;
		
		int pa = int(0x100 * ct);
		int pb =-int(0x100 * st);
		int pc = int(0x100 * st);
		int pd = int(0x100 * ct);
		
		// don't setup any hw-regs until vblank
		VBlankIntrWait();
		
		// setup bg0 scroll
		REG_BG0HOFS = int(bg_scroll) & 0xFFFF;
		
		// setup bg2 transform
		REG_BG2PA = pa;
		REG_BG2PB = pb;
		REG_BG2PC = pc;
		REG_BG2PD = pd;
		REG_BG2X = (int(xpos) << 8) - (pa * 120 + pb * 80);
		REG_BG2Y = (int(ypos) << 8) - (pc * 120 + pd * 80);		
		
		++frame;
		CpuSet(sprites,  OAM, (128 * sizeof(OAM[0])) / 2);
		
		scanKeys();
		u32 held = keysHeld();
		u32 down = keysDown();
		u32 up = keysUp();
		
		float upx =  sin(rot);
		float upy = -cos(rot);
		float leftx =  upy;
		float lefty = -upx;
		
		if (KEY_UP    & down) { xdir = upx * 5; ydir = upy * 5; }
		if (KEY_L     & down) { rot += M_PI / 2; }
		if (KEY_R     & down) { rot -= M_PI / 2; }
/*		if (KEY_L     & up) { rot += M_PI / 4; }
		if (KEY_R     & up) { rot -= M_PI / 4; } */
		
		// external forces
#if 0
		xdir *= 0.95; // friction
		ydir *= 0.95; // friction
#else
		float a = leftx * xdir + lefty * ydir; // dot(dir, left)
		float b = upx   * xdir +   upy * ydir; // dot(dir, up)
		float new_xdir = upx * b + 0.85 * leftx * a;
		float new_ydir = upy * b + 0.85 * lefty * a;
		xdir = new_xdir;
		ydir = new_ydir;
#endif	
		if (fabs(xdir) < 1e-1) xdir = 0;
		if (fabs(ydir) < 1e-1) ydir = 0;
		
		if (KEY_RIGHT & held) { xdir -= leftx * 0.5f; ydir -= lefty * 0.5f; dir = 0; }
		if (KEY_LEFT  & held) { xdir += leftx * 0.5f; ydir += lefty * 0.5f; dir = 1; }
		
		xdir -= upx * 0.25;   // gravity
		ydir -= upy * 0.25;   // gravity
		
		float last_xpos = xpos;
		float last_ypos = ypos;
		
		// apply forces
		xpos += xdir;
		ypos += ydir;
		
		// resolve constraints
		if (xpos < 0)   xpos = 0;
		if (xpos > 255) xpos = 255;
		if (ypos < 0)   ypos = 0;
		if (ypos > 255) ypos = 255;
		
		float delta_xpos = last_xpos - xpos;
		float delta_ypos = last_ypos - ypos;
		
		bg_scroll += (delta_xpos * leftx + delta_ypos * lefty) * 0.5f;
	}
	return 0;
}
